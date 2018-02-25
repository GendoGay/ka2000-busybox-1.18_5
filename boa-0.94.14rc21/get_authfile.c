#include <stdio.h>
#include <string.h>
//#include "base64.h"
#include "md5.h"
#include "libbb.h"
#define AUTH_LEN 1024
 int base64decode(void *dst,char *src,int maxlen);
 void base64encode(unsigned char *from, char *to, int len);
void get_authfile(char *user, char *pass, char *auth);

int get_authfile_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int get_authfile_main(int argc, char **argv)
{
	char auth[AUTH_LEN] = {0, };
	if (argc != 3) {
		printf("Syntax: %s USER PASSWORD\n",argv[0]);
		return -1;
	}

	get_authfile(argv[1], argv[2], auth);
	printf("%s\n",auth);
	return 0;
}
void get_authfile(char *user, char *pass, char *auth)
{
	struct MD5Context mc;
	unsigned char final[16];
	char encoded_passwd[0x40];
	MD5Init(&mc);
	MD5Update(&mc, (unsigned char *)pass, strlen(pass));
	MD5Final(final, &mc);
	strcpy(encoded_passwd,"$1$");
	base64encode(final, encoded_passwd+3, 16);
	snprintf(auth, AUTH_LEN, "%s:%s\n", user, encoded_passwd);
}
