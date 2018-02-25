#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>		/* getopt_long() */
#include "libbb.h"
#include "common.h"

int 
set_wsd_config(char *key,char * val) 
{
	FILE * fp;
	FILE * wfp;
	char buf[256]={0};
	char * tmp_p;
	int ret = -1;

	if (key == NULL || val == NULL)
		return -1;

	fp = fopen("/etc/wsd.conf","r");
	if (fp == NULL) {
		fprintf(stderr,"Can't open %s\n","/etc/wsd.cof");
		return -2;
	}
	wfp = fopen("/mnt/mtd/config/wsd.conf","w");
	if (wfp == NULL) {
		fprintf(stderr,"Can't open %s\n","/mnt/mtd/config/wsd.conf");
		return -3;
	}

	while (fgets(buf,sizeof(buf),fp)!=NULL){
		if ((tmp_p = strstr(buf,key)) != NULL) {
			//* Give a space for value after ":"
			sprintf(buf+strlen(key)," %s\n",val);
			ret = 0;
		}
		fprintf(wfp,"%s",buf);
	}
	fclose(fp);
	fclose(wfp);

	if (ret == 0){
		system("cp /mnt/mtd/config/wsd.conf /etc/wsd.conf");
		system("rm /tmp/wsd.conf > /dev/null");
	}
	return ret;
}
int 
set_wsd_ap_config(char *ssid,char * key) 
{
	FILE * fp;
	FILE * wfp;
	char buf[256]={0};
	char * tmp_p;
	int ret = -1;
	int ap_section = 0;
	long ap_offset;
	int current_ap  = 0;
	int start_write = 0;
	int ap1_valid = 0;
	char parse_ssid[255] = {0};

	if (key == NULL || ssid == NULL)
		return -1;

	fp = fopen("/etc/wsd.conf","r");
	if (fp == NULL) {
		fprintf(stderr,"Can't open %s\n","/etc/wsd.cof");
		return -1;
	}
	wfp = fopen("/mnt/mtd/config/wsd.conf","w");
	if (wfp == NULL) {
		fprintf(stderr,"Can't open %s\n","/mnt/mtd/config/wsd.conf");
		return -1;
	}
#ifdef DEBUG
	printf("%s : SSID=[%s] Key=[%s].\n",__FUNCTION__,ssid,key);
#endif

	while (fgets(buf,sizeof(buf),fp)!=NULL){
		char tmp[256] = {0};
		if (strstr(buf,"[AP]")!=NULL) {
			//AP Section start
			ap_section = 1;
			//ap_offset = ftell(fp);
			fprintf(wfp,"[AP]\n");
		}else if (ap_section == 1 && buf[0] == '[') {
			//AP Section end 
			if (current_ap < 1) {
				printf("%s: error current_ap %d<br>\n",__FUNCTION__, current_ap);
			}
			fprintf(wfp,"SSID : %s\n",ssid);
			fprintf(wfp,"Key : %s\n",key);
			ap_section = 0;
			current_ap = 0;
			ret = 0;
		} else if (ap_section == 1) {
			// Write totol count
			if (strncmp(buf,"AP_ACCOUNT :",strlen("AP_ACCOUNT :")) == 0) {
				printf("%s: buf=[%s] <br>\n",__FUNCTION__, buf);
			}

			if (sscanf(buf,"AP_ACCOUNT : %d",&current_ap) > 0) {
				//go to next line;
				printf("%s: current_ap %d<br>\n",__FUNCTION__, current_ap);
			} 
			if (current_ap == 1 && strncmp(buf,"SSID :",strlen("SSID :")) == 0) {
				printf("%s: buf=[%s] <br>\n",__FUNCTION__, buf);
			}

			if (current_ap == 1 && strncmp(buf,"SSID :",strlen("SSID :")) == 0) {
				if (sscanf(buf,"SSID :%s",parse_ssid) > 0) {
					printf("%s: pase_ssid %s\n",__FUNCTION__, parse_ssid);
					if (parse_ssid[1] != ' ' && parse_ssid[1] != '\n') {
						ap1_valid = 1;
						fprintf(wfp,"AP_ACCOUNT : %d\n",current_ap+1);
						fprintf(wfp,"%s",buf);
					} 
				}
				if (ap1_valid == 0)
					fprintf(wfp,"AP_ACCOUNT : %d\n",current_ap);

			} else if (current_ap == 1 && ap1_valid == 1 && strncmp(buf,"Key :",strlen("Key :")) == 0) {
				fprintf(wfp,"%s",buf);
				//current_ap++;
			} else if (current_ap > 1) {
				if (strncmp(buf,"AP_ACCOUNT :",strlen("AP_ACCOUNT :")) == 0) {
					fprintf(wfp,"AP_ACCOUNT : %d\n",current_ap+1);
				} else {
					fprintf(wfp,"%s",buf);
				}
			}
		}

		if (ap_section == 0)
			fprintf(wfp,"%s",buf);
	}
	fclose(fp);
	fclose(wfp);

	system("cp /mnt/mtd/config/wsd.conf /etc/wsd.conf");
	system("rm /tmp/wsd.conf > /dev/null");
	return ret;
}
int 
reset_wsd_ap_config(void) 
{
	FILE * fp;
	FILE * wfp;
	char buf[256]={0};
	char * tmp_p;
	int ret = -1;
	int ap_section = 0;
	long ap_offset;

	fp = fopen("/etc/wsd.conf","r");
	if (fp == NULL) {
		fprintf(stderr,"Can't open %s\n","/etc/wsd.cof");
		return -1;
	}
	wfp = fopen("/mnt/mtd/config/wsd.conf","w");
	if (wfp == NULL) {
		fprintf(stderr,"Can't open %s\n","/mnt/mtd/config/wsd.conf");
		return -1;
	}

	while (fgets(buf,sizeof(buf),fp)!=NULL){
		char tmp[256] = {0};
		if (strstr(buf,"[AP]")!=NULL) {
			//AP Section start
			ap_section = 1;
			//ap_offset = ftell(fp);
			fprintf(wfp,"[AP]\n");
		} else if (ap_section == 1 && buf[0] == '[') {
			//AP Section end 
			fprintf(wfp,"AP_ACCOUNT : 1\n");
			fprintf(wfp,"SSID : \n");
			fprintf(wfp,"Key : \n");
			ap_section = 0;
		}

		if (ap_section == 0)
			fprintf(wfp,"%s",buf);
	}
	fclose(fp);
	fclose(wfp);

	ret = 0;

	system("cp /mnt/mtd/config/wsd.conf /etc/wsd.conf");
	system("rm /tmp/wsd.conf > /dev/null");
	return ret;
}

//------------------------------------------------------------------------------
int wifi_set_config_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_set_config_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    char *get_key=NULL;
    char *get_val=NULL;
	char key_buf[256] ={0};
	int ret;
	int cmd;

	while ((cmd = getopt(argc, argv, "c:")) != -1) {
		switch (cmd) {
			case 'c':
				get_key= optarg;
				break;
		}
	}
    if (get_key == NULL) {
		fprintf(stderr,"%s: no option\n",__FUNCTION__);
		return -1;
	}
	for (;optind<argc;optind++) {
		get_val = argv[optind];
	}

	if ((optind+1) < argc) {
		fprintf(stderr,"%s: too many value\n",__FUNCTION__);
		return -1;
	}
    if (get_val == NULL) {
		fprintf(stderr,"%s: No config value\n",__FUNCTION__);
		return  -1;
	}

	/* Append : to the key */
	sprintf(key_buf,"%s :",get_key);
	ret = set_wsd_config(key_buf,get_val);
	if (ret != 0) {
		printf("Failed! Set %s=%s\n",key_buf,get_val); 
		return ret;
	}
	printf("Success! Set %s=%s\n",key_buf,get_val); 

    return ret;
}
//------------------------------------------------------------------------------
