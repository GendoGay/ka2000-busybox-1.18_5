#include "libbb.h"
#include "common.h"
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>


//#define DEBUG 1
extern int file_is_exist(char * path);
extern int get_config_apcount(int * apcount);
extern int get_config_apcount(int * apcount);
extern int get_config_ap(int index,wifi_ssid_t *ap);
extern int set_wsd_config(char *key,char * val) ;
extern int set_wsd_ap_config(char *ssid,char * key) ;
extern int reset_wsd_ap_config(void) ;
extern int picture_is_raw(char *filename);


/*
 * Given a URL encoded string, convert it to plain ascii.
 * Since decoding always makes strings smaller, the decode is done in-place.
 * Thus, callers should xstrdup() the argument if they do not want the
 * argument modified.  The return is the original pointer, allowing this
 * function to be easily used as arguments to other functions.
 *
 * string    The first string to decode.
 * option_d  1 if called for httpd -d
 *
 * Returns a pointer to the decoded string (same as input).
 */
static unsigned hex_to_bin(unsigned char c)
{
	unsigned v;

	v = c - '0';
	if (v <= 9)
		return v;
	/* c | 0x20: letters to lower case, non-letters
	 * to (potentially different) non-letters */
	v = (unsigned)(c | 0x20) - 'a';
	if (v <= 5)
		return v + 10;
	return ~0;
/* For testing:
void t(char c) { printf("'%c'(%u) %u\n", c, c, hex_to_bin(c)); }
int main() { t(0x10); t(0x20); t('0'); t('9'); t('A'); t('F'); t('a'); t('f');
t('0'-1); t('9'+1); t('A'-1); t('F'+1); t('a'-1); t('f'+1); return 0; }
*/
}
static char *decodeString(char *orig, int option_d)
{
	/* note that decoded string is always shorter than original */
	char *string = orig;
	char *ptr = string;
	char c;

	while ((c = *ptr++) != '\0') {
		unsigned v;

		if (option_d && c == '+') {
			*string++ = ' ';
			continue;
		}
		if (c != '%') {
			*string++ = c;
			continue;
		}
		v = hex_to_bin(ptr[0]);
		if (v > 15) {
 bad_hex:
			if (!option_d)
				return NULL;
			*string++ = '%';
			continue;
		}
		v = (v * 16) | hex_to_bin(ptr[1]);
		if (v > 255)
			goto bad_hex;
		if (!option_d && (v == '/' || v == '\0')) {
			/* caller takes it as indication of invalid
			 * (dangerous wrt exploits) chars */
			return orig + 1;
		}
		*string++ = v;
		ptr += 2;
	}
	*string = '\0';
	return orig;
}

extern char *strrep( char *str, char *old, char *new );
static int get_query_string(const char *token, char *dest)
{
	char* get_data = getenv("QUERY_STRING");
	char * tmp_buf;
	int data_len = 0;

	data_len = strlen(get_data);
	if (data_len <= 0) {
		fprintf(stderr,"%s: QUERY_STRING len %d\n",__FUNCTION__,data_len);
		return 0;
	}

	tmp_buf = malloc(data_len+1);
	if (tmp_buf == NULL) {
		fprintf(stderr,"%s: malloc %d failed\n",__FUNCTION__,data_len);
		return 0;
	}
	memset(tmp_buf,0x0,data_len+1);
	memcpy(tmp_buf,get_data,data_len);
	
	//get_data = decodeString(tmp_buf, 1);
	get_data = tmp_buf;
	if (get_data != NULL && strlen(get_data) > 2) {
		char* val = strstr(get_data, token);
		if (val != NULL){
			char * end_val = NULL;
			end_val = strstr(val+1,"&");
			if (end_val == NULL) {
				strcpy(dest, val+strlen(token));
				decodeString(dest, 1);
			} else {
				/* copy from "toekn" to "&" */
				strncpy(dest, val+strlen(token),end_val - (val+strlen(token)));
				decodeString(dest, 1);
			}
		} else {
			free(tmp_buf);
			return 0;
#ifdef TS_DEBUG
			printf("Can't find %s parameter!\n", token);
#endif
		}
	} else {
		free(tmp_buf);
		return 0;
	}

	//printf("%s\n", dest);
	free(tmp_buf);
	return 1;
}

typedef int (*cmd_func)(char * );
int 
test1(char * param) 
{
	printf("This is test1 : param %s\n",param);
	return 0;
}
int 
test2(char * param) 
{
	printf("This is test2 : param %s\n",param);
	return 0;
}

int 
set_config_ssid(char * ssid)
{
	int ret;

	if (ssid == NULL)
		return -1;

#ifdef DEBUG
	printf("%s : SSID=[%s].\n",__FUNCTION__,ssid);
#endif

	ret = set_wsd_config("WIFISSID :",ssid);
	if (ret == 0){
		set_wsd_config("Config-State :","1");
	}
	return ret;
}
int 
add_config_ap_account(char * ssid,char *key)
{
	int ret;

	if (ssid == NULL || key == NULL)
		return -1;

#ifdef DEBUG
	printf("%s : SSID=[%s] Key=[%s].\n",__FUNCTION__,ssid,key);
#endif

	ret = set_wsd_ap_config(ssid,key);
	if (ret == 0){
		set_wsd_config("Config-State :","1");
	}

	return ret;
}
int 
reset_config_ap_account(void)
{
	return reset_wsd_ap_config();
}
int 
set_config_login_user(char * user)
{
	int ret;

	if (user == NULL)
		return -1;

#ifdef DEBUG
	printf("%s : USER=[%s].\n",__FUNCTION__,user);
#endif

	ret = set_wsd_config("Login-name :",user);
	if (ret == 0) {
		set_wsd_config("Config-State :","1");
		system("/usr/bin/gen_boa_passwd.sh");
		system("cp /etc/boa/ia.passwd /mnt/mtd/config/ia.passwd");
	}

	return ret;
}
int 
set_config_login_pass(char * pass)
{
	int ret = 0;
	if (pass == NULL)
		return -1;

#ifdef DEBUG
	printf("%s : PASS =[%s].\n",__FUNCTION__,pass);
#endif

	ret = set_wsd_config("Login-password :",pass);
	if (ret == 0) {
		set_wsd_config("Config-State :","1");
		system("/usr/bin/gen_boa_passwd.sh");
		system("cp /etc/boa/ia.passwd /mnt/mtd/config/ia.passwd");
	}

	return ret;
}

int 
set_config_wpa_enable(char * wpa)
{
	int ret1,ret2,ret3;

	if (wpa == NULL)
		return -1;


	if (strlen(wpa) < 8) {
		printf("%s : WPAKey=[%s] less than 8 characters.\n",__FUNCTION__,wpa);
		return -1;
	}
#ifdef DEBUG
	printf("%s : Key=[%s].\n",__FUNCTION__,wpa);
#endif


	ret1 = set_wsd_config("Host WPA2 Key :",wpa);
	ret2 = set_wsd_config("Host WPA2 Switch :","on");
	ret3 = set_wsd_config("Host WPA2 Key Backup :",wpa);
	if (ret1 || ret2 || ret3) {
		printf("%s: ret1 %d ret2 %d ret3 %d\n",ret1,ret2,ret3);
		return -1;
	}

	set_wsd_config("Config-State :","1");

	system("cp /mnt/mtd/config/wsd.conf /mnt/mtd/config/wsd_backup.conf && cp /mnt/mtd/config/wsd_backup.conf /etc/wsd_backup.conf");
	system("/usr/bin/gen_hostapd_config.sh");
	return 0;
}
int 
set_config_wpa_disable(void)
{
	int ret1,ret2,ret3;

#ifdef DEBUG
	printf("%s :\n",__FUNCTION__);
#endif

	ret1 = set_wsd_config("Host WPA2 Key :","");
	ret2 = set_wsd_config("Host WPA2 Switch :","");
	ret3 = set_wsd_config("Host WPA2 Key Backup :","");
	if (ret1 || ret2 || ret3) {
		printf("%s: ret1 %d ret2 %d ret3 %d\n",__FUNCTION__,ret1,ret2,ret3);
		return -1;
	}
	set_wsd_config("Config-State :","1");

	system("rm /mnt/mtd/config/hostapd.conf");
	return 0;
}
static int 
read_file(char * filename) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;


	fp = fopen(filename,"r");	
	if (fp == NULL) {
		printf("Can't open file %s \n",filename);
		return -1;
	}

	while (fgets(buf,sizeof(buf),fp) != NULL) {
		printf("%s",buf);
	}

	fclose(fp);

	return 0;
}
static int 
run_file(char * command) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;

	fp = popen(command,"r");
	if (fp != NULL) {
		while (fgets(buf,sizeof(buf),fp)!=NULL){
			printf("%s",buf);
		} 
		return pclose(fp);
	} else {
		printf("%s command failed!\n",command);
		return -1;
	}
}
static int 
get_upload_list(void) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;


	fp = fopen("/mnt/mtd/upload_list.txt","r");	
	if (fp == NULL){
#ifdef DEBUG
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/var/run/is/complete");
#endif
		return -1;
	}

	while (fgets(buf,sizeof(buf),fp) != NULL) {
		buf[strlen(buf)-1] = '\0';
		printf("file:%s\n",buf);
	}
	fclose(fp);

	return 0;
}
static int 
get_exif(char * pic,int log_level) 
{
	struct exif_desc *desc;
	int width;
	int height;
	int val;
	char buf[256] = {0};
	uint32_t filesize;

	if (log_level)
		fprintf(stderr,"%s: %s log %d\n",__FUNCTION__,pic,log_level);

	exif_set_log(log_level) ;
	exif_set_scan_size(64*1024);

	disable_kcard_call();
	desc = exif_desc_alloc(pic);
	if (desc == NULL) {
		fprintf(stderr,"%s: %s exif descriptor alloc failed\n",__FUNCTION__,pic);
		enable_kcard_call();
		return -1;
	}

	printf("file: %s<br>\n",pic);
	if (exif_get_image_width(desc,&width) < 0) {
		//printf("%s: %s Can't get image width\n",__FUNCTION__,pic);
		//enable_kcard_call();
		//exif_desc_free(desc);
		//return -1;
	}else if (exif_get_image_height(desc,&height) < 0) {
		//printf("%s: %s Can't get image width\n",__FUNCTION__,pic);
		//enable_kcard_call();
		//exif_desc_free(desc);
		//return -1;
	}else {
		printf("Size: %d x %d<br>\n",width,height);
	}

	memset(buf,0x0,256);
	if (exif_get_image_exptime(desc,buf,sizeof(buf)) >= 0) {
		printf("Exposure: %s<br>\n",buf);
	}
	memset(buf,0x0,256);
	if (exif_get_image_fnumber(desc,buf,sizeof(buf)) >= 0) {
		printf("F: %s<br>\n",buf);
	}
	/*
	memset(buf,0x0,256);
	if (exif_get_image_focal_length(desc,buf,sizeof(buf)) >= 0) {
		printf("Focal-length: %s<br>\n",buf);
	}
	*/

	memset(buf,0x0,256);
	if (exif_get_image_exposure_bias(desc, buf, sizeof(buf)) >= 0) {
		printf("EV: %s<br>\n",buf);
	}

	if (exif_get_image_iso(desc,&val) == 0) {
		printf("ISO: %d<br>\n",val);
	}

	/*
	memset(buf,0x0,256);
	if (exif_get_image_exif_version(desc, buf, sizeof(buf)) == 0) {
		printf("EXIF-Ver: %s<br>\n",buf);
	}
	*/
	memset(buf,0x0,256);
	if (exif_get_image_datetime_original(desc,buf,sizeof(buf)) >= 0) {
		printf("Date: %s<br>\n",buf);
	}

	if (exif_get_image_filesize(desc, &filesize) >= 0) {
		int mb = 0;
		int kb = 0;
		if (filesize > (1024*1024)) {
			mb = filesize / (1024*1024);
		}
		if (filesize > (1024)) {
			kb = ((filesize - (mb*1024*1024)) / 1024.0) * 10;
		}
		if (mb) {
			printf("Filesize: %d.%d MB<br>\n", mb, kb / 1000);
		}else {
			printf("Filesize: %d.%d KB<br>\n", kb / 10, kb - (kb / 10 * 10));
		}
	}

    if (exif_get_image_orientation(desc,&val) == 0) {
        printf("Orientation: %d<br>\n",val);
    }

	enable_kcard_call();
	exif_desc_free(desc);
	return 0;
}

static int 
get_raw_exif(char * pic,int log_level) 
{
	char buf[256] = {0};
	FILE * fd;
	int str_len = 0;
	struct stat mystat;
	int mb = 0;
	int kb = 0;
	int filesize = 0;  
	char * oft_ptr;

	sprintf(buf, "/usr/bin/decraw -i -v \"%s\"",pic);
	fd = popen(buf,"rb");
	if (fd == NULL)
		return -1;



	while (fgets(buf, sizeof(buf), fd) != NULL) {
		str_len = strlen(buf);
		if (str_len > 0 && buf[str_len-1] == '\n') {
			buf[str_len-1] = 0x0;
		}

		
		if (strncmp(buf,"ISO speed:", strlen("ISO speed:")) == 0) {
			printf("ISO:%s<br>\n",buf+strlen("ISO speed:"));
		} else if (strncmp(buf,"Aperture:", strlen("Aperture:")) == 0) {
			printf("F:%s<br>\n",buf+strlen("Aperture:"));
		} else if (strncmp(buf,"Timestamp:", strlen("Timestamp:")) == 0) {
			printf("Date:%s<br>\n",buf+strlen("Timestamp:"));
		} else if (strncmp(buf,"Image size:", strlen("Image size:")) == 0) {
			printf("Size:%s<br>\n",buf+strlen("Image size:"));
		} else if (strncmp(buf,"Shutter:", strlen("Shutter:")) == 0) {
			printf("Exposure:%s<br>\n",buf+strlen("Shutter:"));
		} else if (strncmp(buf,"Filename:", strlen("Filename:")) == 0) {
			printf("file:%s<br>\n",buf+strlen("Filename:"));
		} else  {
			/*
			if (buf[0] != '\n' && strlen(buf) > 0) {
				printf("%s<br>\n",buf);
			}
			*/
		}
	}

	/* Get file size */
	if (stat(pic,&mystat) < 0) {
		fprintf(stderr,"Stat %s failed\n",pic);
		return NULL;
	} else {
		filesize = mystat.st_size;
		if (filesize > (1024*1024)) {
			mb = filesize / (1024*1024);
		}
		if (filesize > (1024)) {
			kb = ((filesize - (mb*1024*1024)) / 1024.0) * 10;
		}
		if (mb) {
			printf("Filesize: %d.%d MB<br>\n", mb, kb / 1000);
		}else {
			printf("Filesize: %d.%d KB<br>\n", kb / 10, kb - (kb / 10 * 10));
		}
	}

	pclose(fd);
	return 0;
	
}
int 
cmd_get_exif(char * param)
{
	char file[256] = {0};
	char debug[32] = {0};
	int debug_level = 0;

	if (get_query_string("DEBUG=",debug)){
		debug_level = atoi(debug);
	}	
	if (get_query_string("PIC=",file)) {
		if (picture_is_raw(file)) { 
			return get_raw_exif(file,debug_level);
		}else {
			return get_exif(file,debug_level);
		}
	}else {
		printf("Can't find PIC= parameter\n");
	}
	return -1;
}
static int unzip(char *file, char *dir)
{
	char cmd_buf[256] = {0};
	sprintf(cmd_buf, "/usr/bin/unzip.sh %s %s", file, dir);
	return system(cmd_buf);
}

int 
cmd_unzip(char * param)
{
	char zip[256] = {0};
	char dir[256] = {0};

	if (!get_query_string("ZIP=",zip)){
		printf("Can't find PIC= parameter\n");
	}	
	if (!get_query_string("DIR=",dir)) {
		printf("Can't find DIR= parameter\n");
	}else {
		return unzip(zip, dir);
	}
	return -1;
}
int 
start_upload(void)
{
	char cmd_buf[256] = {0};

	sprintf(cmd_buf, "chmod a+x /mnt/mtd/iu/start.sh");
	system(cmd_buf);

	sprintf(cmd_buf, "/mnt/mtd/iu/start.sh");
	return system(cmd_buf);
}
int 
cmd_start_upload(char * param)
{
	return start_upload();
}
int 
cmd_get_upload_list(char * param)
{
	return get_upload_list();
}
int 
cmd_read_file(char * param)
{
	char filename[256] = {0};

	if (!get_query_string("FILE=", filename)){
		printf("Can't find FILE= parameter\n");
	}else {
		return read_file(filename);
	}
	return -1;
}
int 
cmd_run(char * param)
{
	char command[256] = {0};

	if (!get_query_string("EXEC=", command)){
		printf("Can't find RUN= parameter\n");
	}else {
		return run_file(command);
	}
	return -1;
}

static int gen_filelist(char *file, char *dir)
{
	char cmd_buf[256] = {0};
	sprintf(cmd_buf, "/usr/bin/gen_filelist -d %s -o %s", dir, file);
	return system(cmd_buf);
}


int 
cmd_gen_filelist(char * param)
{
	char outfile[256] = {0};
	char dir[256] = {0};

	if (!get_query_string("OUT=",outfile)){
		printf("Can't find OUT= parameter\n");
	}	
	if (!get_query_string("DIR=",dir)) {
		printf("Can't find DIR= parameter\n");
	}else {
		return gen_filelist(outfile, dir);
	}
	return -1;
}
int 
cmd_reset_upload_record(char * param)
{
	char cmd_buf[256] = {0};
	sprintf(cmd_buf, "/usr/bin/reset_upload_record.sh");
	system(cmd_buf);
	return 0;
}
int 
cmd_set_login_user(char * param)
{
	char user[256] = {0};
	if (get_query_string("USER=",user)) {
		return set_config_login_user(user);
	}else {
		printf("Can't find USER= parameter\n");
	}
	return -1;
}

int 
cmd_set_login_pass(char * param)
{
	char pass[256] = {0};
	if (get_query_string("PASS=",pass)) {
		return set_config_login_pass(pass);
	}else {
		printf("Can't find PASS= parameter\n");
	}
	return -1;
}
int 
cmd_add_ap_account(char * param)
{
	char ssid[256] = {0};
	char key[256] = {0};
	if (get_query_string("SSID=",ssid)
			&& get_query_string("KEY=",key) ) {

		return add_config_ap_account(ssid,key);
	}else {
		printf("Can't find SSID=, KEY= parameter\n");
	}
	return -1;
}
int 
cmd_get_ap_account(char * param)
{
	char ssid[256] = {0};
	char key[256] = {0};
	int apcount =0;
	int i=0;

	if (get_config_apcount(&apcount) != 0) {
		printf("Can't find AP Count\n");
		return -1;
	}
	printf("AP_Count=%d\n",apcount);
	while (i < apcount) {
		wifi_ssid_t ap;
		if (get_config_ap(i,&ap) == 0){
			printf("AP%d:SSID=%s\n",i,ap.ssid);
			printf("AP%d:KEY=%s\n",i,ap.key);
		}
		i++;
	}
	return 0;
}
int 
cmd_reset_ap_account(char * param)
{
	return reset_config_ap_account();
}
int 
cmd_reset_to_default(char * param)
{
	return system("/usr/bin/factory_reset.sh");
}
int 
cmd_set_wpa_enable(char * param)
{
	char key[256] = {0};
	if (get_query_string("KEY=",key)) {
		return set_config_wpa_enable(key);
	}else {
		printf("Can't find KEY= parameter\n");
	}
	return -1;
}
int 
cmd_set_wpa_disable(char * param)
{
	return set_config_wpa_disable();
}
int 
cmd_get_wireless_info(char * param)
{
	FILE * pp;
	char buf[255] = {0};

	pp = popen("/usr/bin/gen_iwconfig.pl","r");
	if (pp != NULL) {
		while (fgets(buf,sizeof(buf),pp)!=NULL){
			printf("%s",buf);
		} 
		pclose(pp);
		return 0;
	} else {
		return -1;
	}
}
int 
cmd_get_hostap_info(char * param)
{
	FILE * pp;
	char buf[255] = {0};


	memset(buf,0x0,255);
	if (read_wifiConfigFile_real("WIFISSID",buf) < 0)
		return -1;
	printf("WIFISSID=%s\n",buf);

	memset(buf,0x0,255);
	read_wifiConfigFile_real("Host WPA2 Switch",buf);
	printf("SECURITY=%s\n",buf);

	memset(buf,0x0,255);
	read_wifiConfigFile_real("Host WPA2 Key",buf);
	printf("KEY=%s\n",buf);

	memset(buf,0x0,255);
	read_wifiConfigFile_real("Channel",buf);
	printf("CHANNEL=%s\n",buf);

	memset(buf,0x0,255);
	read_wifiConfigFile_real("My IP Addr",buf);
	printf("HOSTIP=%s\n",buf);

	memset(buf,0x0,255);
	read_wifiConfigFile_real("Auto WIFI",buf);
	printf("AUTOWIFI=%s\n",buf);

	memset(buf,0x0,255);
	read_wifiConfigFile_real("Auto Mode",buf);
	printf("AUTOMODE=%s\n",buf);

	memset(buf,0x0,255);
	read_wifiConfigFile_real("Auto OFF",buf);
	printf("AUTOOFF=%s\n",buf);

	return 0;
}

int 
cmd_set_ssid(char * param)
{
	char ssid[255] = {0};
	if (get_query_string("SSID=",ssid)) {
		if (strlen(ssid) > 32) {
			fprintf(stderr,"%s:Can't set SSID over 32 characters.",__FUNCTION__);
			return -1;
		}
		return set_config_ssid(ssid);
	}else {
		printf("Can't find SSID= parameter\n");
	}
	return -1;
}
int 
cmd_scan_wifi(char * param)
{
	char buf[255] = {0};
	FILE * pp;

	//    system("iwlist mlan0 scan > /tmp/iw.lst");
	pp = popen("test -f /etc/iw.lst  && cat /etc/iw.lst || cat /tmp/iw.lst","r");
	if (pp != NULL) {
		while (fgets(buf,sizeof(buf),pp)!=NULL){
			printf("%s",buf);
		} 
		pclose(pp);
		return 0;
	} else {
		fprintf(stderr,"%s:Can't find iw.lst",__FUNCTION__);
		return -1;
	}
}
int 
cmd_shutdown_wifi(char * param)
{
	char buf[255] = {0};
	FILE * pp;

	//    system("iwlist mlan0 scan > /tmp/iw.lst");
	system("/usr/bin/wifi_shutdown.sh");
	return 0;
}
static int
set_auto_wifi_off(int seconds)
{
	int ret;
	char tmp_buf[128] = {0};

	/* Can't less than minimum.  */
	if (seconds < 60 && seconds != 0)   {
		printf("Seconds %d Can't less than 60 seconds\n",seconds);
		return -1;
	}

	sprintf(tmp_buf,"%d",seconds);

	ret = set_wsd_config("Auto OFF :",tmp_buf);
	return ret;
}
int 
cmd_set_auto_wifi_shutdown(char * param)
{
	char auto_off[255] = {0};
	int seconds = 0;
	if (get_query_string("OFF=",auto_off)) {
		seconds = atoi(auto_off);
		return set_auto_wifi_off(seconds);
	}

	printf("%s: Can't find OFF= parameter\n",__FUNCTION__);

	return -1;
}
static int
set_auto_wifi_mode(int mode)
{
	char tmp_buf[128] = {0};
	int ret;

	/* Can't less than minimum.  */
	switch (mode) {
		case 0: //direct share
			sprintf(tmp_buf,"DS");
			break;
		case 1: //internet
			sprintf(tmp_buf,"IN");
			break;
		default:
			printf("%s: mode %s not valid \n",__FUNCTION__,mode);
			return -1;
	}
	ret = set_wsd_config("Auto Mode :", tmp_buf);

	return ret;
}

int 
cmd_set_auto_wifi_mode(char * param)
{
	char auto_mode[255] = {0};

	int seconds = 0;
	if (get_query_string("MODE=",auto_mode)) {
		if (strcmp("DS",auto_mode) == 0)
			return set_auto_wifi_mode(0);

		if (strcmp("IN",auto_mode) == 0)
			return set_auto_wifi_mode(1);

		printf("MODE %s not valid!\n",auto_mode);
		printf("Valid mode=(DS|IN)\n");
	} else {
		printf("Can't find MODE= parameter\n");
	}

	return -1;
}
static int
set_power_saving(int mode)
{
	char tmp_buf[128] = {0};
	int ret;

	/* Can't less than minimum.  */
	switch (mode) {
		case 0: //direct share
			sprintf(tmp_buf,"No");
			break;
		case 1: //internet
			sprintf(tmp_buf,"Yes");
			break;
		default:
			printf("%s: mode %s not valid \n",__FUNCTION__,mode);
			return -1;
	}
	ret = set_wsd_config("Power Saving :", tmp_buf);

	return ret;
}

int 
cmd_set_power_saving(char * param)
{
	char auto_mode[255] = {0};

	int seconds = 0;
	if (get_query_string("ENABLE=",auto_mode)) {
		if (strcmp("1",auto_mode) == 0)
			return set_power_saving(1);

		if (strcmp("0",auto_mode) == 0)
			return set_power_saving(0);

		printf("ENABLE %s not valid!\n",auto_mode);
		printf("Valid value=(1|0)\n");
	} else {
		printf("Can't find ENABLE= parameter\n");
	}

	return -1;
}
static int
get_upload_percent(char *file) 
{
	FILE *fp ;
	int percent;

	if (file == NULL)
		return 0;

	fp = fopen(file,"r");	
	if (fp == NULL)
		return 0;

	if (fscanf(fp,"%d",&percent) != 1) {
		fprintf(stderr,"%s:Can't get percent",__FUNCTION__);
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return percent;
}
static int 
get_uploading_list(const char *root_dir,int skip_head_len)
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
#ifdef DEBUG
        fprintf(stderr,"%s : opendir %s failed\n",__FUNCTION__,root_dir);
#endif
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
				//len = sprintf(buf,	"file:\"%s\"[%d] ",tmp+skip_head_len,10);
				printf(	"file:%s[%d] \n",tmp+skip_head_len,get_upload_percent(tmp));
				file_count++;
			}

			/* Recursive List */
			if (dit->d_type == DT_DIR) {
				int ret;
				sprintf(tmp,"%s/%s", root_dir, dit->d_name);

				ret = get_uploading_list(tmp,skip_head_len);
				file_count += ret;
			}
		}
		dit = readdir(dip);
	}

    if (closedir(dip) == -1)
    {
        perror("closedir");
    }
	//LOG_DEBUG("Rootdir %s count %d\n",root_dir,file_count);
	return file_count;
}
static int 
get_complete_list(void) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;


	fp = fopen("/var/run/is/complete","r");	
	if (fp == NULL){
#ifdef DEBUG
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/var/run/is/complete");
#endif
		return 0;
	}


	while (fgets(buf,sizeof(buf),fp) != NULL) {
		buf[strlen(buf)-1] = '\0';
		printf("file:%s[%d] \n",buf,100);
	}
	fclose(fp);

	return 0;
}


static int
get_file_info(char *file) 
{
	struct stat sb;

	if (file == NULL)
		return -1;

	if (stat(file,&sb) < 0) {
#ifdef DEBUG
		printf("%s : file %s stat failed.\n",__FUNCTION__,file);
#endif
		return -1;
	}
//	printf("sz=%lld\n",mystat.st_size);
//	printf("file=%s\n",file);
	printf("File type:                ");

	switch (sb.st_mode & S_IFMT) {
	case S_IFBLK:  printf("block device\n");
		       break;
		       case
			       S_IFCHR:
			       printf("character device\n");
		       break;
		       case
			       S_IFDIR:
			       printf("directory\n");
		       break;
		       case
			       S_IFIFO:
			       printf("FIFO/pipe\n");
		       break;
		       case
			       S_IFLNK:
			       printf("symlink\n");
		       break;
		       case
			       S_IFREG:
			       printf("regular file\n");
		       break;
		       case
			       S_IFSOCK:
			       printf("socket\n");
		       break;
	default:
		       printf("unknown?\n");
		       break;
	}

	printf("I-node number:            %ld\n", (long) sb.st_ino);

	printf("Mode:                     %lo (octal)\n",
			(unsigned long) sb.st_mode);

	printf("Link count:               %ld\n", (long) sb.st_nlink);
	printf("Ownership:                UID=%ld   GID=%ld\n",
			(long) sb.st_uid, (long) sb.st_gid);

	printf("Preferred I/O block size: %ld bytes\n",
			(long) sb.st_blksize);
	printf("File size:                %lld bytes\n",
			(long long) sb.st_size);
	printf("Blocks allocated:         %lld\n",
			(long long) sb.st_blocks);

	printf("Last status change:       %s", ctime(&sb.st_ctime));
	printf("Last file access:         %s", ctime(&sb.st_atime));
	printf("Last file modification:   %s", ctime(&sb.st_mtime));

	return 0;
}

static int 
cmd_get_file_info(char * param)
{
	char filename[512] = {0};

	if (get_query_string("FILE=",filename)) {
		return get_file_info(filename);
	}

	return -1;
}
static int 
cmd_get_iu_status(char * param)
{
	char buf[512] = {0};
	char buf2[512] = {0};
	FILE * pp;

	get_complete_list();
	get_uploading_list("/var/run/is/UP",strlen("/var/run/is/UP"));

	return 0;

}
static int get_sd_info(void) 
{
	FILE * pp;
	char buf[255] = {0};
	char cmd[255] = {0};
	int ret = -1;


	sprintf(cmd, "/usr/bin/fs_info /mnt/sd");
	pp = popen(cmd,"r");
	if (pp != NULL) {

		while (fgets(buf,sizeof(buf),pp) != NULL) 
			printf("%s",buf);

		pclose(pp);
		return 0;
	}
	return -1;
}
static int 
cmd_get_sd_info(char * param)
{
	return get_sd_info();

}
static int 
get_latest_list(void) 
{
	FILE *fp ;
	int len;
	char buf[1024] = {0};
	char buf2[1024] = {0};
	int total;
	int first =1;


	fp = fopen("/tmp/latest_files.txt","r");	
	if (fp == NULL){
#ifdef DEBUG
		fprintf(stderr,"%s: Can't open %s \n",__FUNCTION__,"/tmp/latest_files.txt");
#endif
		return -1;
	}


	while (fgets(buf,sizeof(buf),fp) != NULL) {
		if (buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';
		printf("file:%s\n",buf);
	}
	fclose(fp);

	return 0;
}
static int 
cmd_get_latest_pic(char * param)
{
	char buf[512] = {0};
	char buf2[512] = {0};
	FILE * pp;

	return get_latest_list();

}

static int get_netinfo(char *key, char * output) 
{
	FILE * pp;
	char buf[255] = {0};
	char cmd[255] = {0};
	char * val_p = NULL;
	int ret = -1;

	if (key == NULL || output == NULL)
		return -1;

	if (!file_is_exist("/var/run/net.info")) {
#ifdef DEBUG
		printf("%s : file %s not exist.\n",__FUNCTION__,"/var/run/net.info");
#endif
		return -1;
	}

	sprintf(cmd, "cat /var/run/net.info | grep %s=",key);
	pp = popen(cmd,"r");
	if (pp != NULL) {
		fgets(buf,sizeof buf, pp);
		val_p = strstr(buf,"=");	
		if (val_p != NULL){
			if (val_p[strlen(val_p)-1] == '\n')
				val_p[strlen(val_p)-1] = '\0';

			val_p++;
			strcpy(output,val_p);
			ret = 0;
		} else {
#ifdef DEBUG
			printf("%s : can't find '=' ! %s\n",__FUNCTION__,buf);
#endif
		}
		pclose(pp);
	}else {
#ifdef DEBUG
		printf("%s : popen failed! %s\n",__FUNCTION__,cmd);
#endif
	}
	return ret;
}
int 
disable_kcall(char * param) 
{
	char outbuf[255];
#ifdef DEBUG
	printf("This is %s : param %s\n",__FUNCTION__,param);
#endif

	disable_kcard_call();
	return 0;
}
int 
enable_kcall(char * param) 
{
	char outbuf[255];
#ifdef DEBUG
	printf("This is %s : param %s\n",__FUNCTION__,param);
#endif

	enable_kcard_call();
	return 0;
}
int 
switch_to_share(char * param) 
{
	char outbuf[255];
#ifdef DEBUG
	printf("This is %s : param %s\n",__FUNCTION__,param);
#endif
	if (get_netinfo("mode",outbuf) == 0){
#ifdef DEBUG
		printf("outbuf = %s\n",outbuf);
#endif
		if (strncmp(outbuf,"client",strlen("client")) == 0) {
			system("ap_server.sh &");
			return 0;
		}
		// Already in share mode 
		return 0;
	}
	return -1;
}
int 
switch_to_personal(char * param) 
{
	char outbuf[255];
#ifdef DEBUG
	printf("This is %s : param %s\n",__FUNCTION__,param);
#endif
	if (get_netinfo("mode",outbuf) == 0){
#ifdef DEBUG
		printf("outbuf = %s\n",outbuf);
#endif
		if (strncmp(outbuf,"server",strlen("server")) == 0) {
			system("ap_client.sh &");
			return 0;
		}
		// Already in personal mode 
		return 0;
	}
	return -1;
}
struct tscmd_t {
	const char * cmd_string;
	cmd_func func_p;
};
struct tscmd_t tscmd_array[] = {
	{ "TEST1", test1 },
	{ "TEST2", test2 },
	{ "GET_WIRELESS_INFO", cmd_get_wireless_info },
	{ "GET_HOSTAP_INFO", cmd_get_hostap_info},
	{ "SET_SSID", cmd_set_ssid},
	{ "SET_LOGIN_USER", cmd_set_login_user},
	{ "SET_LOGIN_PASS", cmd_set_login_pass},
	{ "GET_AP_ACCOUNT", cmd_get_ap_account},
	{ "ADD_AP_ACCOUNT", cmd_add_ap_account},
	{ "RESET_AP_ACCOUNT", cmd_reset_ap_account},
	{ "RESET_TO_DEFAULT", cmd_reset_to_default},
	{ "SET_WPA_ENABLE", cmd_set_wpa_enable},
	{ "SET_WPA_DISABLE", cmd_set_wpa_disable},
	{ "SHUTDOWN_WIFI", cmd_shutdown_wifi},
	{ "SET_AUTO_SHUTDOWN", cmd_set_auto_wifi_shutdown},
	{ "SET_AUTO_WIFI_MODE", cmd_set_auto_wifi_mode},
	{ "SET_POWER_SAVING", cmd_set_power_saving},
	{ "SCAN_WIFI", cmd_scan_wifi},
	{ "GET_IU_STATUS", cmd_get_iu_status},
	{ "GET_SD_INFO", cmd_get_sd_info},
	{ "GET_FILE_INFO", cmd_get_file_info},
	{ "GET_LATEST_PIC", cmd_get_latest_pic},
	{ "GET_EXIF", cmd_get_exif},
	{ "UNZIP", cmd_unzip},
	{ "GET_UPLOAD_LIST", cmd_get_upload_list},
	{ "READ_FILE", cmd_read_file},
	{ "RUN", cmd_run},
	{ "START_UPLOAD", cmd_start_upload},
	{ "GEN_FILELIST", cmd_gen_filelist},
	{ "RESET_UPLOAD_RECORD", cmd_reset_upload_record},
	{ "ENABLE_KCARD", enable_kcall},
	{ "DISABLE_KCARD", disable_kcall},
	{ "SET_SHAREMODE", switch_to_share},
	{ "SET_PERSONALMODE", switch_to_personal},
};

int tscmd_main( int argc, char **argv ) MAIN_EXTERNALLY_VISIBLE;
int tscmd_main( int argc UNUSED_PARAM, char **argv UNUSED_PARAM )
{
	char dest[1024] = {0};
	int count = 0;
	int i,ret = 0;


	fflush(stdout);
	printf( "Content-Type: text/html\n\n" );
	fflush(stdout);

	count = sizeof(tscmd_array)/sizeof(struct tscmd_t);

	/* CMD_DEL&FILE=xxxxx */
//	fprintf(stderr, "tscmd_main\n" );
	if (get_query_string("CMD=",dest)) {
		for (i = 0; i < count; i++) {
			if (strncmp(dest,tscmd_array[i].cmd_string,strlen(tscmd_array[i].cmd_string)) == 0) {
				ret = tscmd_array[i].func_p(dest);
				break;
			}
		}
		if (i == count ) {
			printf("Fail: Cmd %s not found.\n\r\n",dest);
			fprintf(stderr, "tscmd: command not found , %s \n" ,dest);
		} else {
			if (ret == 0) {
				printf("Success: Cmd %s\n\r\n",dest);
			}else {
				printf("Fail: Cmd %s ret = %d\n\r\n",dest,ret);
				fprintf(stderr, "tscmd: Failed , %s \n" ,dest);
			}
		}
	}else {
		printf( "Wrong Command Syntax! %s\n\r\n", dest);
		fprintf(stderr, "tscmd: Wrong command , %s \n" ,dest);
	}
	return 0;
}
