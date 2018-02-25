
int TS_create_cmd_tunnel(void);
void TS_close_cmd_tunnel(int fd);
int TS_send_cmd(int fd,char * output_buf);

int udp_create_connection(char * ip,char *port);
int udp_close_connection(int fd);
int udp_send(int fd, char * output_buf);
