/* vi: set sw=4 ts=4: */
/*
 * Simple FTP daemon, based on vsftpd 2.0.7 (written by Chris Evans)
 *
 * Author: Adam Tkac <vonsch@gmail.com>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 *
 * Only subset of FTP protocol is implemented but vast majority of clients
 * should not have any problem.
 *
 * You have to run this daemon via inetd.
 */

#include "libbb.h"
#include <syslog.h>
#include <netinet/tcp.h>
#include "tslib.h"


#define MSG_OK "Operation successful\r\n"
#define MSG_ERR "Error\r\n"

#define WIFI_LIST_FILE		"/mnt/mtd/config/is_wifi_list.json"
#define BASIC_INFO_FILE		"/etc/json/basic_dev_info.json"
#define INSTANT_SETUP_PID_FILE   "/var/run/instant_setupd.pid"
#define INSTANT_SETUP_POST_SCRIPT "/usr/bin/is_post.sh"

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

#define LOG_INFO(args...)	fprintf(stderr,"INSETUP: " args)
#define LOG_ERR(args...)	fprintf(stderr,"INSETUP: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) fprintf(stderr,"INSETUP: "args);}

static int _log_level = LOG_LEVEL_INFO;


struct globals {
	int pasv_listen_fd;
#if !BB_MMU
	int root_fd;
#endif
	int local_file_fd;
	unsigned end_time;
	unsigned timeout;
	unsigned verbose;
	off_t local_file_pos;
	off_t restart_pos;
	len_and_sockaddr *local_addr;
	len_and_sockaddr *port_addr;
	char *ftp_cmd;
	char *ftp_arg;
#if ENABLE_FEATURE_FTP_WRITE
	char *rnfr_filename;
#endif
	/* We need these aligned to uint32_t */
	char msg_ok [(sizeof("NNN " MSG_OK ) + 3) & 0xfffc];
	char msg_err[(sizeof("NNN " MSG_ERR) + 3) & 0xfffc];
	
} FIX_ALIASING;
#define G (*(struct globals*)&bb_common_bufsiz1)
#define INIT_G() do { \
	/* Moved to main */ \
	/*strcpy(G.msg_ok  + 4, MSG_OK );*/ \
	/*strcpy(G.msg_err + 4, MSG_ERR);*/ \
} while (0)




static char *
escape_text(const char *prepend, const char *str, unsigned escapee)
{
	unsigned retlen, remainlen, chunklen;
	char *ret, *found;
	char append;

	append = (char)escapee;
	escapee >>= 8;

	remainlen = strlen(str);
	retlen = strlen(prepend);
	ret = xmalloc(retlen + remainlen * 2 + 1 + 1);
	strcpy(ret, prepend);

	for (;;) {
		found = strchrnul(str, escapee);
		chunklen = found - str + 1;

		/* Copy chunk up to and including escapee (or NUL) to ret */
		memcpy(ret + retlen, str, chunklen);
		retlen += chunklen;

		if (*found == '\0') {
			/* It wasn't escapee, it was NUL! */
			ret[retlen - 1] = append; /* replace NUL */
			ret[retlen] = '\0'; /* add NUL */
			break;
		}
		ret[retlen++] = escapee; /* duplicate escapee */
		str = found + 1;
	}
	return ret;
}

/* Returns strlen as a bonus */
static unsigned
replace_char(char *str, char from, char to)
{
	char *p = str;
	while (*p) {
		if (*p == from)
			*p = to;
		p++;
	}
	return p - str;
}

static void
verbose_log(const char *str)
{
	bb_error_msg("%.*s", (int)strcspn(str, "\r\n"), str);
}
static void
io_write(char * s)
{
	xwrite(STDOUT_FILENO, s, strlen(s));
}
static void
io_write_end(void)
{
	char buf[1] = {0};

	xwrite(STDOUT_FILENO,buf,1);
}

static void
timeout_handler(int sig UNUSED_PARAM)
{
	off_t pos;
	int sv_errno = errno;

	if ((int)(monotonic_sec() - G.end_time) >= 0)
		goto timed_out;

	if (!G.local_file_fd)
		goto timed_out;

	pos = xlseek(G.local_file_fd, 0, SEEK_CUR);
	if (pos == G.local_file_pos)
		goto timed_out;
	G.local_file_pos = pos;

	alarm(G.timeout);
	errno = sv_errno;
	return;

 timed_out:
	//cmdio_write_raw(STR(FTP_TIMEOUT)" Timeout\r\n");
/* TODO: do we need to abort (as opposed to usual shutdown) data transfer? */
	exit(1);
}

/* Simple commands */


int reply_basic_info(void)
{
	FILE *fp ;
	char buf[1024] = {0};

	LOG_DEBUG("%s\n",__FUNCTION__);

	fp = fopen(BASIC_INFO_FILE,"r" );	
	if (fp == NULL){
		LOG_ERR("%s , Can't get basic device info\n",__FUNCTION__);
		reply_result("Can't get basic device info!",400);
		return -1;
	}
	while (fgets(buf,sizeof(buf),fp) != NULL) {
		io_write(buf);	
	}
	fclose(fp);

	io_write_end();	
	return 0;
}
int reply_result(char * message,int status_code)
{
	FILE *fp ;
	char buf[1024] = {0};
	int len =0;


	len = sprintf(buf,"{ \"status_code\":  %d,",status_code);
	len += sprintf(buf+len,"\"error_message\":  \"%s\"",message);
	len += sprintf(buf+len,"}");

	LOG_INFO("%s buf=[%s]\n",__FUNCTION__,buf);
	io_write(buf);

	io_write_end();	
	return 0;
}

int get_cmd_string(FILE * fd,char * buf, int max_len)
{
	int idx =0;
	int ch;
	int err=0;

	memset(buf,0x0,max_len);
	while (1) {
		ch = fgetc(fd);
		if (ch == EOF || ch == '\0') {
			break;
		}
		LOG_DEBUG("[%02d] = %02x [%c]\n",idx,ch,ch);
		if (err == 0){
			buf[idx++] = ch;
		}else {
			idx++;
		}
		if (err == 0 && idx >= max_len) {
			err=1;
		}
	}
	if (err == 1) {
		LOG_INFO("CMD length %d too long!\n",idx);
		return -1;
	}
	return idx;
}
static void
usage(void) 
{
	printf("Usage: instant_upload [OPTIONS] [COMMAND]\n");
	printf("Options list --- \n");
	printf("	-d log_level\n");

}

int instant_setupd_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
#if !BB_MMU
int instant_setupd_main(int argc, char **argv)
#else
int instant_setupd_main(int argc UNUSED_PARAM, char **argv)
#endif
{
	unsigned abs_timeout;
	unsigned verbose_S;
	smallint opts;
	int cmd_fd;
	int pid;
	int i;
	int ret;
	char tmp_buf[128] ={0};
	char * process = NULL;

	LOG_INFO("--- instant_setupd_main\n");
	INIT_G();

	abs_timeout = 1 * 60 * 60;
	verbose_S = 0;
	G.timeout = 2 * 60;
#if 0
	opt_complementary = "t+:T+:vv:SS";
#if BB_MMU
	opts = getopt32(argv,   "vS" IF_FEATURE_FTP_WRITE("w") "t:T:", &G.timeout, &abs_timeout, &G.verbose, &verbose_S);
#else
	opts = getopt32(argv, "l1vS" IF_FEATURE_FTP_WRITE("w") "t:T:", &G.timeout, &abs_timeout, &G.verbose, &verbose_S);
	if (opts & (OPT_l|OPT_1)) {
		/* Our secret backdoor to ls */
/* TODO: pass -n? It prevents user/group resolution, which may not work in chroot anyway */
/* TODO: pass -A? It shows dot files */
/* TODO: pass --group-directories-first? would be nice, but ls doesn't do that yet */
		//ftp_xchdir(argv[2]);
		argv[2] = (char*)"--";
		/* memset(&G, 0, sizeof(G)); - ls_main does it */
		return ls_main(argc, argv);
	}
#endif
#endif
//#define _DEBUG_ARGV
#ifdef _DEBUG_ARGV
	printf("argc %d, argv[0] = %s\n",argc,argv[0]);
#endif
	i = 1;
	while (i < argc) {
#ifdef _DEBUG_ARGV
		printf("(argv[%d][0] = %c \n",i,argv[i][0]);
		printf("(argv[%d][1] = %c length %d\n",i,argv[i][1],strlen(argv[i]));
#endif
		if (argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'd':
				if ((i+1) >= argc){
					usage();
					return -1;
				}
				_log_level = atoi(argv[i+1]);
				i++;
				break;
			default:
				usage ();
				return -1;
			}
		}else {
			process = argv[i];
			break;
		}
		i++;
	}

	LOG_INFO("Log Level = %d\n",_log_level);
	if (process != NULL) {
		LOG_INFO("Post Command = %s\n",process);
	}

	if (G.verbose < verbose_S)
		G.verbose = verbose_S;
	if (abs_timeout | G.timeout) {
		if (abs_timeout == 0)
			abs_timeout = INT_MAX;
		G.end_time = monotonic_sec() + abs_timeout;
		if (G.timeout > abs_timeout)
			G.timeout = abs_timeout;
	}
	strcpy(G.msg_ok  + 4, MSG_OK );
	strcpy(G.msg_err + 4, MSG_ERR);

#if 0
	G.local_addr = get_sock_lsa(STDIN_FILENO);
	if (!G.local_addr) {
		/* This is confusing:
		 * bb_error_msg_and_die("stdin is not a socket");
		 * Better: */
		bb_show_usage();
		/* Help text says that ftpd must be used as inetd service,
		 * which is by far the most usual cause of get_sock_lsa
		 * failure */
	}

	if (!(opts & OPT_v))
		logmode = LOGMODE_NONE;
	if (opts & OPT_S) {
		/* LOG_NDELAY is needed since we may chroot later */
		openlog(applet_name, LOG_PID | LOG_NDELAY, LOG_DAEMON);
		logmode |= LOGMODE_SYSLOG;
	}
	if (logmode)
		applet_name = xasprintf("%s[%u]", applet_name, (int)getpid());
#endif



	//umask(077); - admin can set umask before starting us

	/* Signals. We'll always take -EPIPE rather than a rude signal, thanks */
	signal(SIGPIPE, SIG_IGN);

	/* Set up options on the command socket (do we need these all? why?) */
	setsockopt(STDIN_FILENO, IPPROTO_TCP, TCP_NODELAY, &const_int_1, sizeof(const_int_1));
	setsockopt(STDIN_FILENO, SOL_SOCKET, SO_KEEPALIVE, &const_int_1, sizeof(const_int_1));
	/* Telnet protocol over command link may send "urgent" data,
	 * we prefer it to be received in the "normal" data stream: */
	setsockopt(STDIN_FILENO, SOL_SOCKET, SO_OOBINLINE, &const_int_1, sizeof(const_int_1));

	signal(SIGALRM, timeout_handler);


	/* RFC-959 Section 5.1
	 * The following commands and options MUST be supported by every
	 * server-FTP and user-FTP, except in cases where the underlying
	 * file system or operating system does not allow or support
	 * a particular command.
	 * Type: ASCII Non-print, IMAGE, LOCAL 8
	 * Mode: Stream
	 * Structure: File, Record*
	 * (Record structure is REQUIRED only for hosts whose file
	 *  systems support record structure).
	 * Commands:
	 * USER, PASS, ACCT, [bbox: ACCT not supported]
	 * PORT, PASV,
	 * TYPE, MODE, STRU,
	 * RETR, STOR, APPE,
	 * RNFR, RNTO, DELE,
	 * CWD,  CDUP, RMD,  MKD,  PWD,
	 * LIST, NLST,
	 * SYST, STAT,
	 * HELP, NOOP, QUIT.
	 */
	/* ACCOUNT (ACCT)
	 * "The argument field is a Telnet string identifying the user's account.
	 * The command is not necessarily related to the USER command, as some
	 * sites may require an account for login and others only for specific
	 * access, such as storing files. In the latter case the command may
	 * arrive at any time.
	 * There are reply codes to differentiate these cases for the automation:
	 * when account information is required for login, the response to
	 * a successful PASSword command is reply code 332. On the other hand,
	 * if account information is NOT required for login, the reply to
	 * a successful PASSword command is 230; and if the account information
	 * is needed for a command issued later in the dialogue, the server
	 * should return a 332 or 532 reply depending on whether it stores
	 * (pending receipt of the ACCounT command) or discards the command,
	 * respectively."
	 */

	cmd_fd = TS_create_cmd_tunnel();
	if (cmd_fd < 0) {
		LOG_ERR("Can't create CMD tunnel , fd %d\n",cmd_fd);
		return -1;
	}

	pid = getpid();
	LOG_INFO("%s: set pid %d\n",__FUNCTION__, (int)pid);
    sprintf(tmp_buf,"%d", (int)pid);
	simple_write_file(INSTANT_SETUP_PID_FILE, tmp_buf);

	reply_basic_info();
	while (1) {

#define MAX_CMD_LEN (8*1024)
		char *cmd_buf;
		char filename[256] = {0};
		int cmd_len;
		
		cmd_buf = xmalloc(MAX_CMD_LEN);
		if (cmd_buf == NULL) {
			LOG_ERR("Can't Alloc memory size %d\n",MAX_CMD_LEN);
			TS_close_cmd_tunnel(cmd_fd);
			unlink(INSTANT_SETUP_PID_FILE);
			return -1;
		}
		sprintf(filename,"%s",WIFI_LIST_FILE);

		unlink(filename);
		cmd_len = get_cmd_string(stdin,cmd_buf,MAX_CMD_LEN);
		if (cmd_len > 0) {
			LOG_DEBUG("%s\n",cmd_buf);

			if (set_wsd_config("GPlus-Enable :","YES") != 0){
				LOG_ERR("Failed to Set Config GPlus-Enable to YES\n");
				reply_result("Failed to Set Config GPlus-Enable = YES",500);
				return -1;
			} else {
				simple_write_file(filename,  cmd_buf);
#define CHECK_JSON 1
#ifdef CHECK_JSON
				/* Check access_token */
				sprintf(tmp_buf,"/bin/jshon -e access_token < %s > /dev/null", filename);
				ret = system(tmp_buf);
				if (ret != 0){
					LOG_ERR("Failed to get access_token from json\n");
					reply_result("Failed to get access token from json",200);
					return -2;
				}

				/* Check Wifi configs */
				sprintf(tmp_buf,"/bin/jshon -e wifiConfigs < %s  > /dev/null", filename);
				ret = system(tmp_buf);
				if (ret != 0){
					LOG_ERR("Failed to get wifiConfigs from json\n");
					reply_result("Failed to get wifiConfigs from json",200);
					return -3;
				}
#endif
				reply_result("Success!",0);

				sprintf(cmd_buf,"cp %s /etc/json/",filename);
				system(cmd_buf);
			}

			snprintf(filename,256,"%s\n",INSTANT_SETUP_POST_SCRIPT);
			LOG_INFO("Execute %s\n",filename);
			TS_send_cmd(cmd_fd,filename);

			sleep(2);
			if (process != NULL) {
				/* \n is indication of end string */
				snprintf(filename,256,"%s\n",process);
				LOG_INFO("Execute %s\n",filename);
				TS_send_cmd(cmd_fd,filename);
			}
		} else if (cmd_len < 0) {
			reply_result("Get json string failed!",400);
		}
		free(cmd_buf);
		break;
	}
	TS_close_cmd_tunnel(cmd_fd);
	unlink(INSTANT_SETUP_PID_FILE);
	return 0;
}
