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


#define MSG_OK "Operation successful\r\n"
#define MSG_ERR "Error\r\n"

#define WIFI_LIST_FILE		"/mnt/mtd/config/is_wifi_list.json"
#define BASIC_INFO_FILE		"/etc/json/basic_dev_info.json"

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



#if 0
int get_cmd_string(FILE * fd,char * buf, int max_len)
{
	int idx =0;
	int ch;

	memset(buf,0x0,max_len);
	while (1) {
		ch = fgetc(fd);
		if (ch == EOF || ch == '\0') {
			break;
		}
#ifdef _DEBUG
		fprintf(stderr,"[%02d] = %02x [%c]\n",idx,ch,ch);
#endif
		buf[idx++] = ch;
	}
	return idx;
}
#endif



int cmd_server_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
#if !BB_MMU
int cmd_server_main(int argc, char **argv)
#else
int cmd_server_main(int argc UNUSED_PARAM, char **argv)
#endif
{
	unsigned abs_timeout;
	unsigned verbose_S;
	smallint opts;

	fprintf(stderr,"instant_setupd_main\n");
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

	fprintf(stderr,"get_socklsa\n");
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


	while (1) {
#if  0
#define MAX_CMD_LEN (8*1024)
		char *cmd_buf;
		char filename[256] = {0};
		int cmd_len;
#endif
		char * cmd;
		size_t len_on_stk = 8 * 1024;
		FILE * pp;
		char buf[1024] = {0};
		int ret; 

#if 0
		cmd_buf = xmalloc(MAX_CMD_LEN);
		if (cmd_buf == NULL) {
			fprintf(stderr,"Can't Alloc memory size %d\n",MAX_CMD_LEN);
			return -1;
		}
		sprintf(filename,"%s",WIFI_LIST_FILE);

		unlink(filename);
#endif

		cmd = xmalloc_fgets_str_len(stdin, "\n", &len_on_stk);
		if (!cmd)
			exit(0);

		fprintf(stderr,cmd);
		pp = popen(cmd,"r");
		if (pp != NULL) {
			while (fgets(buf,sizeof(buf),pp)!=NULL){
				fprintf(stderr,"%s",buf);
				io_write(buf);
			} 
			ret = pclose(pp);
			if (ret < 0) {
				fprintf(stderr,"%s return error %d\n",cmd,ret);
				sprintf(buf,"%s return error %d\n",cmd,ret);
				io_write(buf);
				return -1;
			}
			return 0;
		} else {
			fprintf(stderr,"Can't execute CMD %s\n",cmd);
			sprintf(buf,"Can't execute CMD %s\n",cmd);
			io_write(buf);
			return -1;
		}
		free(cmd);
		break;
	}
	return 0;
}
