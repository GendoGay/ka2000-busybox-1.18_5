#include "libbb.h"
#include <syslog.h>
#include <netinet/tcp.h>

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

static int _log_level = LOG_LEVEL_INFO;
#define LOG_INFO(args...)	fprintf(stderr, "TSLB: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) fprintf(stderr,"TSLB: "args);}

int udp_create_connection(char * ip,char *port)
{
	uint16_t server_port;
	len_and_sockaddr *servAddr;
	int clientfd;

	server_port = bb_lookup_port( port, "udp", 0);

	servAddr = xhost2sockaddr( ip, server_port);

	clientfd = xsocket(servAddr->u.sa.sa_family, SOCK_DGRAM, 0);
	if( clientfd < 0 ) { 
		LOG_INFO("TS UDP Client socket fail\n" );
		return -1;
	}
	//setsockopt_reuseaddr( clientfd );

	xconnect( clientfd, &servAddr->u.sa, servAddr->len );
	return clientfd;
}
int udp_close_connection(int fd)
{
	close(fd);
}
int udp_send(int fd,char * output_buf)
{
	int bytesent;

	LOG_DEBUG("%s [%s]\n",__FUNCTION__,output_buf);

	/* SIGPIPE get process crashed. */
	bytesent = send(fd, output_buf, strlen( output_buf ),MSG_NOSIGNAL);
	//bytesent = sendto(fd, output_buf, strlen( output_buf ),MSG_NOSIGNAL,(struct sockaddr *)to_server, sizeof(*to_server));
	LOG_DEBUG( "fd  %d byteSent = %d\n", fd, bytesent );
	if( bytesent < 0) {
		LOG_DEBUG("Send failed. FD %d return %d\n",fd, bytesent);
	}
	return bytesent;
}
int udp_recv(int fd,char * output_buf,int buflen,struct sockaddr * from,int * fromlen)
{
	int byteSent;

	LOG_DEBUG("%s \n",__FUNCTION__);

	/* SIGPIPE get process crashed. */
	byteSent = recvfrom(fd, output_buf, buflen,0, from, fromlen);
	LOG_DEBUG( "fd  %d byteSent = %d\n", fd, byteSent );
	if( byteSent < 0 ) 
	{
		LOG_INFO("Send failed. Close FD %d\n",fd);
	} else {
		LOG_DEBUG("%s: %s\n",__FUNCTION__,output_buf);
	}
	return byteSent;
}

//void TS_SetClientSocket()
int TS_create_cmd_tunnel(void)
{
	return udp_create_connection("127.0.0.1","55778");
}

void TS_close_cmd_tunnel(int fd)
{
	udp_close_connection(fd);
}
int TS_send_cmd(int fd,char * output_buf)
{
	int byteSent;

	LOG_DEBUG("%s [%s]\n",__FUNCTION__,output_buf);

	/* SIGPIPE get process crashed. */
	byteSent = send(fd, output_buf, strlen( output_buf ),MSG_NOSIGNAL);
	//byteSent = sendto(fd, output_buf, strlen( output_buf ),MSG_NOSIGNAL, (struct sockaddr *)cmd_serv_addr, sizeof * cmd_serv_addr);
	LOG_DEBUG( "fd  %d byteSent = %d\n", fd, byteSent );
	if( byteSent < 0 ) {
		LOG_INFO("Send fd %d failed. return %d\n",fd, byteSent);
		return -1;
	}
	return byteSent;
}



