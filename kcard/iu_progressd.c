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
//#include <syslog.h>
#include <netinet/tcp.h>
#include "tslib.h"
#include "transcend.h"

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1
static int _log_level = LOG_LEVEL_DEBUG;

#define MSG_OK							"Operation successful\r\n"
#define MSG_ERR							"Error\r\n"
#define LOG_INFO(args...)				fprintf(stderr, "PROGRESS: " args)
#define LOG_DEBUG(args...)				{if (_log_level >= LOG_LEVEL_DEBUG) fprintf(stderr,"PROGRESS: "args);}
#define MAXFDCOUNT 64

static fd_set fdset;
static socklen_t sa_len;
static int clifd[MAXFDCOUNT] = {-1};
static int servfd = -1;
static int lastfd = -1;

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

#if 0
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
#endif
	return;

 timed_out:
	//cmdio_write_raw(STR(FTP_TIMEOUT)" Timeout\r\n");
/* TODO: do we need to abort (as opposed to usual shutdown) data transfer? */
	exit(1);
}

/* Simple commands */
static int get_uploading_list(const char *root_dir,int first)
{
    DIR             *dip=NULL;
    struct dirent   *dit;
    char tmp[256]= {0};
    int   len = 0;
	int file_count = 0;
	char buf[1024] = {0};

    dip = opendir(root_dir);
    if (dip  == NULL)
    {
        LOG_INFO("%s : opendir %s failed\n",__FUNCTION__,root_dir);
        return 0;
    }

    dit = readdir(dip);
    while (dit != NULL)
	{
		memset(tmp,0x0,256);
		if (dit->d_name != NULL && (strcmp(dit->d_name,".") != 0 && strcmp(dit->d_name,"..") != 0))
		{
			int dirlen;
			dirlen = strlen(dit->d_name);
			if (dit->d_type != DT_DIR) {
				sprintf(tmp,"%s/%s",root_dir, dit->d_name);
				if (first) {
					first = 0;
					len = sprintf(buf,	"{ \"fname\": \"%s\" }\n",tmp);
				} else  {
					len = sprintf(buf,	",{ \"fname\": \"%s\" }\n",tmp);
				}
				io_write(buf);
				file_count++;
			}

			/* Recursive List */
			if (dit->d_type == DT_DIR) {
				int ret;
				sprintf(tmp,"%s/%s", root_dir, dit->d_name);

				ret = get_uploading_list(tmp,first);
				file_count += ret;
			}
		}
		dit = readdir(dip);
	}

    if (closedir(dip) == -1)
    {
        perror("closedir");
    }
	LOG_DEBUG("Rootdir %s count %d\n",root_dir,file_count);
	return file_count;
}
int progress_uploading_write_start(void) 
{
	int len;
	char buf[1024] = {0};

	len = sprintf(buf,		"\"uploading\":  {");
	len = sprintf(buf + len,"\"file_list\": [ \n");
	io_write(buf);

	return 0;
}
int progress_uploading_write_end(int count) 
{
	int len;
	char buf[1024] = {0};

	/* array ending */
	len = sprintf(buf ,"],\n");

	/* total count */
	len += sprintf(buf + len,		 "\"count\": %d\n", count);

	/* complete final */
	len += sprintf(buf + len,"}\n");

	io_write(buf);

	return 0;
}
int progress_uploading_write(char *filename) 
{
	int len;
	char buf[1024] = {0};

	len = sprintf(buf,		"{\"fname\": \"%s\"},",filename);
	io_write(buf);

	return 0;
}

int progress_get_uploading(int write_json) 
{
	char buf[256] = {0};
    int len;
	int count = 0;

	if (write_json) {
		len = sprintf(buf,		"\"uploading\": {\n");
		len = sprintf(buf + len,		"\"file_list\": [\n");
		io_write(buf);
	}

	count = get_uploading_list("/var/run/is/UP", 1);

	if (write_json) {
		len = sprintf(buf, " ],\n");
		io_write(buf);

		/* total count */
		len = sprintf(buf, "\"count\": %d }\n", count);
		io_write(buf);
	}

	return count;
}
int progress_get_email(void) 
{
	FILE *fp ;
	int len;
	int total;
	char buf[256] = {0};
	char buf2[256] = {0};

	fp = fopen("/var/run/is/email","r" );	
	if (fp == NULL){
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/var/run/is/email");
		return -1;
	}
	if (fgets(buf,sizeof(buf),fp) == NULL) {
		fprintf(stderr,"%s: Can't find\n",__FUNCTION__);
		return -2;
	}

	if (buf[strlen(buf)-1] == '\n'){
		buf[strlen(buf)-1] = '\0';
	}
	len = sprintf(buf2,		"\"email\":  \"%s\",\n",buf);
	io_write(buf2);
	return len;
}
int progress_get_external(void) 
{
	FILE *fp2 ;
	int len = 0;
	char buf[1024] = {0};

	int ret_code = 0;
	char msg[512] = {0};

	fp2 = fopen("/var/run/is/status","r");
	if (fp2) {
		fscanf(fp2,"%d",&ret_code);
		fclose(fp2);
	}else {
		return -1;
	}
	fp2 =fopen("/var/run/is/message","r");
	if (fp2) {
		fgets(msg,sizeof msg,fp2) ;
		fclose(fp2);
	}else {
		return -2;
	}
	if (msg[strlen(msg)-1] == '\n')
		msg[strlen(msg)-1] = '\0';

	len = sprintf(buf,		"{ \"progress_status\":  %d,\n",ret_code);
	len += sprintf(buf+len,	 "\"progress_message\":  \"%s\"\n",msg);
	len += sprintf(buf+len,	"}\n ");
	io_write(buf);
	io_write_end();

	return 0;
}
int progress_get_total(int write_json) 
{
	FILE *fp ;
	FILE *fp2 ;
	int len = 0;
	int total;
	int cpt;
	char buf[1024] = {0};

	fp = fopen("/var/run/is/total","r" );	
	if (fp == NULL){
		if (write_json) {
			len = sprintf(buf,		"{ \"progress_status\":  %d,\n",0);
			len += sprintf(buf+len,   "\"progress_message\":  \"No pictures.\"}");
			io_write(buf);
			io_write_end();	
		}
		return -1;
	}
	if (fscanf(fp,"%d",&total) <= 0 ) {
		if (write_json) {
			len = sprintf(buf,		"{ \"progress_status\":  %d,\n",200);
			len += sprintf(buf+len,	 "\"progress_message\":  \"Can't find total count\"}");
			io_write(buf);
			io_write_end();	
		}
		fclose(fp);
		return -2;
	}
	if (total == 0) {
		if (write_json) {
			len = sprintf(buf,		"{ \"progress_status\":  %d,\n",0);
			len += sprintf(buf+len,	 "\"progress_message\":  \"No files to upload\",\n");
		}
		fclose(fp);
		return -3;
	} 
	cpt = progress_get_complete(0);
	if (write_json) {
		if (cpt == total) {
			len = sprintf(buf,		"{ \"progress_status\":  %d,\n",1);
			len += sprintf(buf+len,	 "\"progress_message\":  \"finish\",\n");
			len += sprintf(buf+len,		"\"total\":  %d }\n",total);
			io_write(buf);
			total = 0;
		} else {
			len = sprintf(buf,		"{ \"progress_status\":  %d,\n",0);
			len += sprintf(buf+len,	 "\"progress_message\":  \"\",\n");
			len += sprintf(buf+len,		"\"total\":  %d,\n",total);
			io_write(buf);
		}
	}
	fclose(fp);
	return total;
}
int progress_get_total_progress(int progress) 
{
	int len = 0;
	int total;
	char buf[1024] = {0};

	len += sprintf(buf+len,		"\"progress\":  %d,\n",progress);
	io_write(buf);

	return total;
}
int progress_fail_write_start(void) 
{
	int len;
	char buf[1024] = {0};


	len = sprintf(buf,          "\"fail\":  {");
	len += sprintf(buf + len,   "\"file_list\": [ \n");
	io_write(buf);

	return 0;
}
int progress_fail_write_end(int count) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;


	/* array ending */
	len = sprintf(buf ,"],\n");

	/* total count */
	len += sprintf(buf + len,		 "\"count\": %d\n", count);

	/* fail final */
	len += sprintf(buf + len,"},:  %d",count);
	io_write(buf);

	return 0;
}
int progress_fail_write(char *filename) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	int total;
	int first =1;

	len = sprintf(buf,		"{\"fname\": \"%s\"},",filename);
	io_write(buf);

	return 0;
}

int progress_get_fail(int write_json) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;

	fp = fopen("/var/run/is/total_fail","r" );	
	if (fp == NULL){
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/var/run/is/total_fail");
		return -1;
	}
	if (fscanf(fp,"%d",&total) <= 0 ) {
		fprintf(stderr,"%s: Can't get total_fail\n",__FUNCTION__);
		fclose(fp);
		return -2;
	}
	fclose(fp);

	if (write_json) {
		len = sprintf(buf,		"\"fail\":  {");
		len += sprintf(buf+len,	"\"count\": %d, ",total);
		io_write(buf);
	}


	fp = fopen("/var/run/is/fail","r");	
	if (fp == NULL){
		if (write_json) {
			len = sprintf(buf,	"]}, \n");
			io_write(buf);
		}
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/var/run/is/fail");
		return len ;
	}

	if (write_json) {
		len = sprintf(buf,	"\"file_list\": [ \n");
		io_write(buf);
	}

	while (fgets(buf,sizeof(buf),fp) != NULL) {
		/* Strip newline */
		buf[strlen(buf)-1] = '\0';
		if (!first) {
			if (write_json) {
				len = sprintf(buf2,	",{\"fname\": \"%s\"}\n ",buf);
				io_write(buf2);	
			}
		} else {
			if (write_json) {
				len = sprintf(buf2,	"{\"fname\": \"%s\"}\n ",buf);
				io_write(buf2);	
			}
			first = 0;
		} 
	}
	fclose(fp);

	/* End of Filelist */
	if (write_json) {
		len = sprintf(buf,	"]}, \n");
		io_write(buf);
	}

	return total;
}

int progress_complete_write_start(void) 
{
	int len;
	char buf[1024] = {0};

	len = sprintf(buf,		"\"complete\":  {");
	len = sprintf(buf + len,"\"file_list\": [ \n");
	io_write(buf);

	return 0;
}
int progress_complete_write_end(int count) 
{
	int len;
	char buf[1024] = {0};

	/* array ending */
	len = sprintf(buf ,"],\n");

	/* total count */
	len += sprintf(buf + len,		 "\"count\": %d\n", count);

	/* complete final */
	len += sprintf(buf + len,"}\n",count);

	io_write(buf);

	return 0;
}
int progress_complete_write(char *filename) 
{
	int len;
	char buf[1024] = {0};

	len = sprintf(buf,		"{\"fname\": \"%s\"},",filename);
	io_write(buf);

	return 0;
}
int progress_get_complete(int write_json) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;

	fp = fopen("/var/run/is/total_cpt","r" );	
	if (fp == NULL){
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/var/run/is/total_cpt");
		return -1;
	}
	if (fscanf(fp,"%d",&total) <= 0 ) {
		fprintf(stderr,"%s: Can't get total_cpt\n",__FUNCTION__);
		return -2;
	}
	fclose(fp);

	if (write_json) {
		len = sprintf(buf,		"\"complete\":  {");
		len += sprintf(buf+len,	"\"count\": %d, ",total);
		io_write(buf);
	}

	fp = fopen("/var/run/is/complete","r");	
	if (fp == NULL){
		if (write_json) {
			len = sprintf(buf,	"]}, \n");
			io_write(buf);
		}
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/var/run/is/complete");
		return -1;
	}

	if (write_json) {
		len = sprintf(buf,	"\"file_list\": [ \n");
		io_write(buf);
	}

	while (fgets(buf,sizeof(buf),fp) != NULL) {
		/* Strip newline */
		buf[strlen(buf)-1] = '\0';
		if (!first) {
			if (write_json) {
				len = sprintf(buf2,	",{\"fname\": \"%s\"}\n ",buf);
				io_write(buf2);	
			}
		} else {
			if (write_json) {
				len = sprintf(buf2,	"{\"fname\": \"%s\"}\n ",buf);
				io_write(buf2);	
			}
			first = 0;
		} 
	}
	fclose(fp);

	/* End of Filelist */
	if (write_json) {
		len = sprintf(buf,	"]}, \n");
		io_write(buf);
	}

	return total;
}


/* 0 -> failed, 1-> success , 2-> uploading */
int reply_progress(char * filename, int progress, int state)
{
	char buf[256] = {0};
	int total = 0;

		LOG_INFO("%s: file %s state %d\n",__FUNCTION__,filename,state);

	total = progress_get_external();
	if (total) {
		total = progress_get_total(1);
		if (total >= 0) {
			progress_get_total_progress(progress);
			if (total > 0) {
				if (state == 1) {
					progress_complete_write_start();
					progress_complete_write(filename);
					progress_complete_write_end(1);
				} else if (state == 0){
					progress_fail_write_start();
					progress_fail_write(filename);
					progress_fail_write_end(1);
				} else if (state == 2){
					progress_uploading_write_start();
					progress_uploading_write(filename);
					progress_uploading_write_end(1);
				}
			}
			sprintf(buf,	"}\n ");
			io_write(buf);
			io_write_end();
			return 0;
		}
	}
	return -1;
}

int reply_progress_all(void)
{
	char buf[256] = {0};
	int total = 0;

	total = progress_get_external();
	if (total) {
		total = progress_get_total(1);
		if (total >= 0) {
			if (total > 0) {
				progress_get_complete(1);
				progress_get_fail(1);
				progress_get_uploading(1);
			}

			sprintf(buf,	"}\n ");
			io_write(buf);
			io_write_end();
			return total;
		}
	}
	return -1;
}


static int set_server_socket() {
	uint16_t local_port;
	len_and_sockaddr *lsa;
	unsigned backlog = 20;
	char tmp_buf[128] = {0};

#ifdef _DEBUG_LOG		
	fprintf(stderr, "********************\n" );
	fprintf(stderr, "TS UDP Server Start*\n" );
	fprintf(stderr, "********************\n" );
#endif

	for( int tempindex = 0; tempindex < MAXFDCOUNT; ++tempindex )
		clifd[tempindex] = -1;

	sprintf(tmp_buf,"%d",PROGRESSD_PORT);
	local_port = bb_lookup_port( tmp_buf, "udp", 0);
	lsa = xhost2sockaddr( "127.0.0.1", local_port);

	servfd = xsocket(lsa->u.sa.sa_family, SOCK_DGRAM, 0);
	if( servfd < 0 ) {
		fprintf(stderr, "TS UDP Server create socket fail\n" );
		return -1;
	}
	fprintf(stderr,"Server FD %d\n",servfd);

	setsockopt_reuseaddr( servfd );
	sa_len = lsa->len; /* I presume sockaddr len stays the same */

	xbind( servfd, &lsa->u.sa, sa_len );
	//xlisten( servfd, backlog );
	close_on_exec_on( servfd );

	//set_tcp_keepalive(servfd);

	return servfd;

}
static 
int event_parsing(char * buf,int len) 
{
	int progress = 0;
	int filename_ptr = NULL;

	if (len < 6) {
		LOG_INFO("%s:buf len %d is too small\n", __FUNCTION__, len);
		return -1;
	}

	/* String is supposed to be "complete:$filename" */
	if (strncmp(buf,"complete:",strlen("complete:")) == 0) {
		sscanf(buf+strlen("complete:"),"%d:", &progress);
		filename_ptr = strstr(buf+strlen("complete:"),":");
		if (filename_ptr == NULL) {
			LOG_INFO("%s: string not valid. %s\n", __FUNCTION__, buf);
			return -1;
		}

		reply_progress(filename_ptr+1, progress, 1);

	} else if (strncmp(buf,"fail:",strlen("fail:")) == 0) {
		sscanf(buf+strlen("fail:"),"%d:", &progress);
		filename_ptr = strstr(buf+strlen("fail:"),":");
		if (filename_ptr == NULL) {
			LOG_INFO("%s: string not valid. %s\n", __FUNCTION__, buf);
			return -1;
		}

		reply_progress(filename_ptr+1, progress, 0);

	} else if (strncmp(buf,"uploading:",strlen("uploading:")) == 0) {
		sscanf(buf+strlen("uploading:"),"%d:", &progress);
		filename_ptr = strstr(buf+strlen("uploading:"),":");
		if (filename_ptr == NULL) {
			LOG_INFO("%s: string not valid. %s\n", __FUNCTION__, buf);
			return -1;
		}
		reply_progress(filename_ptr+1, progress, 2);
	} else if (strncmp(buf,"finish:",strlen("finish:")) == 0) {
		LOG_INFO("%s: %s\n", __FUNCTION__, buf);
		return 1;
	}

	return 0;
}
void loop_check(void) {

	while( 1 ) {
		int state;
		int index;
		int maxfd;

		FD_ZERO( &fdset );
		FD_SET( servfd, &fdset );

		maxfd = servfd + 1; 
		LOG_DEBUG("Start select \n");
		state = select(maxfd, &fdset, NULL, NULL, NULL );
		if( state < 0) {
			LOG_INFO( "TS TCP Server select fail\n" );
			return;
		}
		LOG_DEBUG("Got select fd state %d\n",state);

		/* New connection is comming */
		if (FD_ISSET(servfd, &fdset)) {
			int newfd = -1;
			int fdIndex;
			len_and_sockaddr newAddr;
			int socklen = 0;
			int byterecv = 0;
			char recvbuf[1024] = {0};

			byterecv = udp_recv(servfd, recvbuf, sizeof(recvbuf), &newAddr, &socklen);
			if (byterecv > 0) {
				LOG_INFO("TCP FD %d got data len %d!\n",servfd, byterecv);
				/* Finish */
				if (event_parsing(recvbuf, byterecv) == 1) {
					break;
				}
			}
		}
	} // while (1)
}
static void signal_usr1(int sig)
{
    /* set NULL sig handler avoid */
    (void) signal(SIGUSR1, NULL);

	reply_progress_all();

    (void) signal(SIGUSR1, signal_usr1);
}



int iu_progressd_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
#if !BB_MMU
int iu_progressd_main(int argc, char **argv)
#else
int iu_progressd_main(int argc UNUSED_PARAM, char **argv)
#endif
{
	unsigned abs_timeout;
	unsigned verbose_S;
	smallint opts;
	int cmd_fd;
	int pid;
	char tmp_buf[256];


#if 0
	abs_timeout = 1 * 60 * 60;
	verbose_S = 0;
	G.timeout = 2 * 60;
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

#endif
	fprintf(stderr,"get_socklsa\n");



	//umask(077); - admin can set umask before starting us

	/* Signals. We'll always take -EPIPE rather than a rude signal, thanks */
	signal(SIGPIPE, SIG_IGN);

	pid = getpid();
	LOG_INFO("pid %d\n", (int)pid);
    sprintf(tmp_buf,"%d", (int)pid);
	simple_write_file(PROGRESSD_PID_FILE, tmp_buf);

	/* Set up options on the command socket (do we need these all? why?) */
	setsockopt(STDIN_FILENO, IPPROTO_TCP, TCP_NODELAY, &const_int_1, sizeof(const_int_1));
	setsockopt(STDIN_FILENO, SOL_SOCKET, SO_KEEPALIVE, &const_int_1, sizeof(const_int_1));
	/* Telnet protocol over command link may send "urgent" data,
	 * we prefer it to be received in the "normal" data stream: */
	setsockopt(STDIN_FILENO, SOL_SOCKET, SO_OOBINLINE, &const_int_1, sizeof(const_int_1));

	/* Setup progressd socket */
	set_server_socket();

	cmd_fd = TS_create_cmd_tunnel();
	if (cmd_fd < 0) {
		fprintf(stderr,"Can't create CMD tunnel , fd %d\n",cmd_fd);
		return -1;
	}

	signal(SIGALRM, timeout_handler);
	signal(SIGUSR1, signal_usr1);


#if 0
	cmd_buf = xmalloc(MAX_CMD_LEN);
	if (cmd_buf == NULL) {
		fprintf(stderr,"Can't Alloc memory size %d\n",MAX_CMD_LEN);
		TS_close_cmd_tunnel(cmd_fd);
		return -1;
	}
	cmd_len = get_cmd_string(stdin,cmd_buf,MAX_CMD_LEN);
	if (cmd_len <= 0) {
		fprintf(stderr,"%s:get_cmd_string failed %d\n",cmd_len);
		return -1;
	}
	free(cmd_buf);
#endif

	if (reply_progress_all() > 0) {
		loop_check();
	}
	TS_close_cmd_tunnel(cmd_fd);
	unlink(PROGRESSD_PID_FILE);
	return 0;
}
