#include "libbb.h"

#include "kcard.h"

#include "tslib.h"

#include <sys/inotify.h>

//#include <linux/inotify.h>

#define KEYASIC_BURNIN_PATCH 1

#define WSD_ACT_WEBCONF      0x01

#define WSD_ACT_UPLOAD       0x02

#define WSD_ACT_SERVER       0x03

#define WSD_ACT_SEND         0x04

//#define WSD_ACT_TOKEN  "PRT QTY"

//#define WSD_ACT_BEGIN_TOKEN "<IMG SRC"

//#define SELECTIONS_PRN_PATH "/mnt/sd/MISC/AUTPRINT.MRK"



#define KCARD_SIG_DPOF         6

#define KCARD_SIG_UPDATED      7

#define KCARD_SIG_SCAN         9

#define KCARD_SIG_DEL_IMG_0    1

#define KCARD_SIG_DEL_IMG_1    2

#define KCARD_SIG_DEL_IMG_2    3

#define KCARD_SIG_DEL_IMG_3    4

#define KCARD_SIG_NO_IMGS      8

#define KCARD_SIG_SLEEP       99

#define KCARD_SIG_WAKEUP     100



#define MAX_OUT_BUF_LEN 	65536


#define KCARD_CIMG1 "WSD00001.JPG"

#define KCARD_CIMG2 "WSD00002.JPG"

#define KCARD_CIMG3 "WSD00003.JPG"

#define KCARD_CIMG4 "WSD00004.JPG"

#define KCARD_WIFI_DIR "/mnt/sd/DCIM/199_WIFI/"

#define DCIM_DIR        "/mnt/sd/DCIM/"

#define KCARD_IMG_TOKEN "<IMG SRC"



char   config_control_file[]		= KCARD_CIMG1;

char   server_upload_control_file[] = KCARD_CIMG2;

char   receiver_control_file[]   	= KCARD_CIMG3;

char   sender_control_file[]     	= KCARD_CIMG4;

char   wifi_dir[] = KCARD_WIFI_DIR;

char   IMAGES_DIR[20] = DCIM_DIR;

char   WSD_ACT_BEGIN_TOKEN[]         = KCARD_IMG_TOKEN;



/*

char   *config_control_file		= "WSD0CONF.JPG";

char   *server_upload_control_file = "WSD1UPLD.JPG";

char   *receiver_control_file   	= "WSD2RECV.JPG";

char   *sender_control_file     	= "WSD3SEND.JPG";

char *wifi_dir = "/mnt/sd/DCIM/100_WIFI/";

*/


#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

static int _log_level = LOG_LEVEL_INFO;
static int _log_diff_enable = 0;
static int _evt_log_enable = 0;
static int _debounce_time = 1;
static int _hide_wifi_folder = 1;
static int _skip_wifi_folder = 0;
#define LOG_INFO(args...)	printf("KCARD: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) printf("KCARD: "args);}
#define LOG_PATH_DEFAULT "/tmp/filediff"
#define LOG_EVT_PATH_DEFAULT "/tmp/log.kcard"
#define KCARD_MAIN_PID_FILE   "/var/run/kcard_app.pid"
#define KCARD_SUB_PID_FILE   "/var/run/kcard_app_sub.pid"

/* Use the file to record the event the fail of check,
 * Let bodyguard do check update-list again.*/
#define KCARD_CHECK_UPDATE_FILE   "/tmp/check_update"

static char * _log_path = LOG_PATH_DEFAULT;
static char _evt_log_path[255] = {0} ;

extern int file_is_exist(char * path);
int makehidden(char *file);
//Kobby-20120116-Add for support shoot and view

#define MAXFDCOUNT 64
#define WATCH_DIR 	"/mnt/sd/DCIM"
static int filefd[2];

static int lastfd = -1;
static int clifd[MAXFDCOUNT] = {-1};
static void send2app(char * output_buf);
static int alarm_lock = 0;
	

struct control_info

{

    int func;

    int exist;

    int triggered;

    char *source;

    char *target;

    char *name;

    char *info;

    char *act0;

    char *act1;

};

#define CTRL_PIC_NUM 1
#define CTRL_PIC1_ACT0 "factory_reset.sh"
#define CTRL_PIC1_ACT1 ""
//#define PIC3_ACT0 "ap_is.sh"
//#define PIC3_ACT1 ""

#ifdef TS_RESET
struct control_info control_image[] =
{
    {.exist=0, .func=0 ,.act0="", .act1="",        .name="", .source="", .target="", .info="Dummy\n",     .triggered=1},
	{.exist=0, .func=WSD_ACT_WEBCONF,.act0=CTRL_PIC1_ACT0 , .act1=CTRL_PIC1_ACT1,        .name="WSD00003.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00003.JPG",           .target="/mnt/sd/DCIM/199_WIFI/WSD00003.JPG", .info="Web Config\n",     .triggered=0},
};
#else
struct control_info control_image[] =

{

    {.exist=0, .func=0 ,.act0="", .act1="",        .name="", .source="", .target="", .info="Dummy\n",     .triggered=1},
#if TS_RESET
    {.exist=0, .func=WSD_ACT_WEBCONF,.act0="ap_server.sh", .act1="",        .name="WSD00001.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00001.JPG",           .target="/mnt/sd/DCIM/199_WIFI/WSD00001.JPG", .info="Web Config\n",     .triggered=0},

    {.exist=0, .func=WSD_ACT_UPLOAD, .act0="ap_client.sh", .act1="",       .name="WSD00002.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00002.JPG",           .target="/mnt/sd/DCIM/199_WIFI/WSD00002.JPG", .info="FTP Upload\n",          .triggered=0},
#else
    {.exist=0, .func=WSD_ACT_WEBCONF,.act0="w3", .act1="wifi_setup_app",        .name="WSD00001.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00001.JPG",           .target="/mnt/sd/DCIM/199_WIFI/WSD00001.JPG", .info="Web Config\n",     .triggered=0},

    {.exist=0, .func=WSD_ACT_UPLOAD, .act0="w2", .act1="wifi_ftp_upload",       .name="WSD00002.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00002.JPG",           .target="/mnt/sd/DCIM/199_WIFI/WSD00002.JPG", .info="FTP Upload\n",          .triggered=0},
#endif

#if TS_GPLUS 
    {.exist=0, .func=WSD_ACT_SERVER, .act0="ap_is.sh", .act1="",   .name="WSD00003.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00003.JPG", .target="/mnt/sd/DCIM/199_WIFI/WSD00003.JPG", .info="P2P Server\n",  .triggered=1},
#else
    {.exist=0, .func=WSD_ACT_SERVER, .act0="p2p_server", .act1="wifi_ftp_server",   .name="WSD00003.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00003.JPG", .target="/mnt/sd/DCIM/199_WIFI/WSD00003.JPG", .info="P2P Server\n",  .triggered=0},
#endif
#if TS_RESET
    {.exist=0, .func=WSD_ACT_SEND,   .act0="", .act1="factory_reset.sh",       .name="WSD00004.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00004.JPG",   .target="/mnt/sd/DCIM/199_WIFI/WSD00004.JPG", .info="P2P Send\n",    .triggered=0},
#else
    {.exist=0, .func=WSD_ACT_SEND,   .act0="p2p_client", .act1="wifi_quick_send",       .name="WSD00004.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00004.JPG",   .target="/mnt/sd/DCIM/199_WIFI/WSD00004.JPG", .info="P2P Send\n",    .triggered=0},
#endif

};
#endif





typedef enum{

  KCARD_RESCAN_IMAGES,

  KCARD_ENABLE_USERCALL,

  KCARD_DISABLE_USERCALL,

  KCARD_ENABLE_SLEEP,

  KCARD_DISABLE_SLEEP,

  KCARD_SET_PID,

  KCARD_GET_CMD,

  KCARD_WAKEUP,

  KCARD_GO_SLEEP,

  KCARD_DEL_PROGRAM_BIN,

  KCARD_SET_USERCALL_INTERVAL,

  KCARD_TRIGGER_FULL_RANGE,

  KCARD_STOP_CTRL_IMG_CHECK,

  }driver_contl_type;

int set_dir_hidden(char *fileName)
{
	LOG_INFO("Set Dir %s to Hidden\n",fileName);
	return makehidden(fileName);
}
static void kcard_print_signal(int signal, int write_log)
{
	char signal_buf[30];
	switch (signal) {
	case KCARD_SIG_DPOF:
		sprintf(signal_buf,"KCARD_SIG_DPOF\n");
		break;
	case KCARD_SIG_UPDATED:
		sprintf(signal_buf,"KCARD_SIG_UPDATED\n");
		break;
	case KCARD_SIG_SCAN:
		sprintf(signal_buf,"KCARD_SIG_SCAN\n");
		break;
	case KCARD_SIG_DEL_IMG_0:
		sprintf(signal_buf,"KCARD_SIG_DEL_IMG_0\n");
		break;
	case KCARD_SIG_DEL_IMG_1:
		sprintf(signal_buf,"KCARD_SIG_DEL_IMG_1\n");
		break;
	case KCARD_SIG_DEL_IMG_2:
		sprintf(signal_buf,"KCARD_SIG_DEL_IMG_2\n");
		break;
	case KCARD_SIG_DEL_IMG_3:
		sprintf(signal_buf,"KCARD_SIG_DEL_IMG_3\n");
		break;
	case KCARD_SIG_NO_IMGS:
		sprintf(signal_buf,"KCARD_SIG_NO_IMGS\n");
		break;
	case KCARD_SIG_SLEEP:
		sprintf(signal_buf,"KCARD_SIG_SLEEP\n");
		break;
	case KCARD_SIG_WAKEUP:
		sprintf(signal_buf,"KCARD_SIG_WAKEUP\n");
		break;
	default:
		sprintf(signal_buf,"%d Not defined\n",signal);
		break;
	}
	LOG_INFO("%s",signal_buf);
	if (write_log) {
		kcard_log(signal_buf);
	}
}
int kcard_log(char * str) {
	FILE * log_fd = NULL;
	if (_evt_log_enable) {
		log_fd = fopen(_evt_log_path,"a+");
		if (log_fd == NULL) {
			fprintf(stderr, "Can't open %s",_evt_log_path);
			/* Don't open again */
			_evt_log_enable = 0;
			return 1;
		}
		fwrite(str,strlen(str),1,log_fd);
		fclose(log_fd);
		return 0;
	}
	return 1;
}
/* Directory Monitor Code Start */
int kcard_print_dir_file_list(const char *root_dir,FILE * fd)
{
    DIR             *dip=NULL;
    struct dirent   *dit;
    char tmp[256]= {0};
    int   len = 0;

	/* Don't count in control foler */
	if (_skip_wifi_folder && strncmp(KCARD_WIFI_DIR ,root_dir,strlen(KCARD_WIFI_DIR)-1) == 0) {
		return 0;
	}

    dip = opendir(root_dir);
    if (dip  == NULL)
    {
        printf("%s: failed! get_file_list %s\n",__FUNCTION__,root_dir);
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

				/* Only JPG files */
				if ((dit->d_type != DT_DIR) && (dirlen >= 4) && 
					(strncmp(&dit->d_name[0],"WSD00",5) != 0) &&
					((strncasecmp(&dit->d_name[dirlen-3],"jpg",3) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-4],"jpeg",4) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-3],"png",3) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-3],"gif",3) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-3],"bmp",3) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-3],"mov",3) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-3],"mp4",3) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-3],"mpg",3) == 0)
					|| (strncasecmp(&dit->d_name[dirlen-3],"avi",3) == 0)
					)){
#if 1
            			fprintf(fd,"%s/%s\n",root_dir, dit->d_name);
#else
						memcpy(tmp,root_dir,strlen(root_dir));
						memcpy(tmp+strlen(tmp),"/",1);
						memcpy(tmp+strlen(tmp),dit->d_name,strlen(dit->d_name));
						fwrite(tmp,strlen(tmp),1,fd);
#endif
				}

				/* Recursive List */
				if (dit->d_type == DT_DIR) {
#if 1
            		sprintf(tmp,"%s/%s", root_dir, dit->d_name);
#else
					memcpy(tmp,root_dir,strlen(root_dir));
					memcpy(tmp+strlen(tmp),"/",1);
					memcpy(tmp+strlen(tmp),dit->d_name,strlen(dit->d_name));
					fwrite(tmp,strlen(tmp),1,fd);
#endif
					kcard_print_dir_file_list(tmp,fd);
	    		}
            }
	        dit = readdir(dip);
    }

    if (closedir(dip) == -1)
    {
        perror("closedir");
        return 0;
    }
	return 0;
}

void treegen(const char* directory)
{
    FILE * fd;

    LOG_DEBUG("Treegen start\n");

    fd = fopen("/tmp/flist","w");
    if (fd == NULL) {
		printf("Can't open /tmp/flist\n");
		return;
	}

    kcard_print_dir_file_list(directory,fd);

    LOG_DEBUG("Treegen End\n");

    fclose(fd);
}

void treecheck(char* directory,char * output_buf)
{
    FILE * mypipe;
	char buff[256];
	int len = 0;
    int skip_line = 3;
    int count =0;
    FILE * fd = NULL;
    FILE * log_fd = NULL;
    FILE * diff_fd = NULL;
	int add_flag = 0;
	int del_flag = 0;
	int try_count = 0;
	int ret = 0;
	int tsrun_flag = 0;


    LOG_INFO("treecheck\n");
    ret = system("/usr/bin/refresh_sd");
    while (try_count <= 5 && ret != 0) {
	    LOG_INFO("%s: Try %d Refresh SD ret %d\n",__FUNCTION__, try_count, ret);
	    try_count++;
	    sleep(2);
	    ret = system("/usr/bin/refresh_sd");
    }

    if (ret != 0) {
	LOG_INFO("%s: Refresh SD failed! Give up\n",__FUNCTION__);
	simple_write_file(KCARD_CHECK_UPDATE_FILE,"refresh_sd failed\n");
	return ;
    }

    LOG_DEBUG("find start\n");

    fd = fopen("/tmp/newflist","w");
    if (fd == NULL) {
		printf("Can't open /tmp/newflist to write\n");
		return;
	}
	
    kcard_print_dir_file_list(directory,fd);

    fclose(fd);

    LOG_DEBUG("diff start\n");
    mypipe = popen("diff /tmp/flist /tmp/newflist" , "r");
    if( mypipe == NULL || mypipe == (void *)-1){
        perror("something wrong");
		return;
    }

    LOG_DEBUG("Get diff\n");
	if (_log_diff_enable) {
		log_fd = fopen(_log_path,"w");
		if (!log_fd) {
			LOG_DEBUG("Diff Log failed. Can't open %s\n",_log_path);
		}
	}

	

    while( fgets(buff,sizeof(buff),mypipe) != NULL ){
		int str_length=0;
		count ++;
		LOG_DEBUG("buff = [%s]",buff);
		if (count <=3)
			continue;

		if (buff[0] != '+' && buff[0] != '-')
			continue;

		if (strncmp(buff,"+/",2) == 0) {
			buff[0] = '>';
			add_flag++;
		}

		if (strncmp(buff,"-/",2) == 0){
			buff[0] = '<';
			del_flag++;
		}


/*
		if (len > 0)
			len += sprintf(output_buf+len,"|");
*/

		/* Discard newline character at end of line. */
		str_length = strlen(buff);

		if (diff_fd) {
			fwrite(buff,str_length,1,diff_fd);
		} else {
			diff_fd = fopen("/tmp/latest_files.txt","w");
			if (!diff_fd) {
				LOG_DEBUG("write Diff %s failed. Can't open %s\n","/tmp/latest_files.txt");
			} else {
				fwrite(buff,str_length,1,diff_fd);
			}
		}

		buff[str_length-1]=0x0;
		len += str_length;

		send2app(buff);

		if (log_fd){
			LOG_DEBUG("Write buff = [%s] to log\n",buff);
			fwrite(buff,str_length,1,log_fd);
		}

		if (strncmp(buff+1,"/mnt/sd/DCIM/tsrun.jpg",17) == 0)
			tsrun_flag = 1;
/*
		len += sprintf(output_buf+len,"%s",buff);
		if (len >= (MAX_OUT_BUF_LEN-256)){
			printf("Buffer is going to full, Len %d\n",len);
			break;
		}
*/
	}

	if (log_fd) {
		LOG_DEBUG("Close FD\n");
		log_fd = NULL;
		fclose(log_fd);
	}

	LOG_DEBUG("Close command pipe\n");
	if (diff_fd) {
		fclose(diff_fd);
	}
	pclose(mypipe);

	if (len == 0)
		LOG_INFO("No files changes\n");

	if (tsrun_flag) {
		LOG_INFO("Run /mnt/sd/DCIM/tsrun.sh\n");
		kcard_log("Execute /mnt/sd/DCIM/tsrun.sh\n");
		system("/mnt/sd/DCIM/tsrun.sh &");
	}
	LOG_DEBUG("Replace Filelist\n");
	system("cp -f /tmp/flist /tmp/oldflist");
	system("mv /tmp/newflist /tmp/flist");

	/* Clean instant-uploaded list */
	if (add_flag) 
		system("/usr/bin/notify_iu.sh");

	if (del_flag)
		system("instant_upload_clean");

	LOG_INFO("treecheck done\n");
}
static void send2app(char * output_buf)
{
	int index;
	char recvbuf[32] = "";

	LOG_DEBUG("Send2app [%s]\n",output_buf);
	for( index = lastfd; ( lastfd != -1 ) && ( index >= 0 ); --index ) {
		if( clifd[index] != -1 ) {
			int byteSent;

			/* SIGPIPE get process crashed. */
			byteSent = send( clifd[index], output_buf, strlen( output_buf )+1,MSG_NOSIGNAL); //add extra 1 byte for null character

			LOG_DEBUG( "clifd[%d] = %d, byteSent = %d\n", index, clifd[index], byteSent );
			if( byteSent < 0 ) 
			{
				LOG_INFO("Send failed. Close FD %d\n",clifd[index]);

				close( clifd[index] );
				clifd[index] = -1;
				if (index == lastfd) {
					while (index > 0 && clifd[index - 1] < 0) {
						index--;
					}

					if (index > 0)
						lastfd = index - 1;
					else
						lastfd = -1;
				}
			}
		} //if( clifd[index] != -1 ) 
	} //for index
} 

#if 0
void vircheck(char* directory,char * output_buf)
{
/*
    char type;
    char pattern[10]="";
    char thePath[PATH_MAX], quar[PATH_MAX], pathBack[PATH_MAX];
    struct stat buf;
*/
    struct dirent *dirp;    
    DIR * dp;    
    static int filename0_count =0;
    static int filename1_count =0;
    static char filename0[128][128];
    static char filename1[128][128];
    static char (*cmp_filename)[128] = filename0;
    static char (*new_filename)[128] = filename1;
    static char (*tmp_filename)[128];
    static int cmp_counter = 0;
    static int new_counter = 0;
    int i = 0;
    int found = 0;
    int len = 0;
    

    system("umount /mnt/sd && mount -t vfat --shortname=winnt /dev/mmcblk0p1 /mnt/sd");

    if ((dp = opendir(directory)) == NULL){
         printf("Directory open error");
	return ;
    }

    printf("filename0 = %p, filename1 = %p\n",filename0,filename1);
    printf("cmp_filename = %p, new_filename = %p\n",cmp_filename,new_filename);
    printf("cmp_filename[0] = %p, new_filename[0] = %p\n",cmp_filename[0],new_filename[0]);
    printf("cmp_filename[1] = %p, new_filename[1] = %p\n",cmp_filename[1],new_filename[1]);
    printf("cmp_counter = %p, new_counter = %p\n",&cmp_counter,&new_counter);
    printf("0.0.0 &new_filename[%d][0] = %p\n",new_counter,&new_filename[new_counter][0]);
        
    new_counter = 0;
    while ((dirp = readdir(dp)) != NULL) 
    {
	found = 0;
        if ( !strcmp(dirp->d_name,".") || !strcmp(dirp->d_name, "..") )
            continue;
            
/*
	printf("0 new_counter = %d,cmp_ounter = %d\n",new_counter, cmp_counter);
        printf("name %-30s\n", dirp->d_name);
	printf("0.1 new_counter = %d,cmp_ounter = %d\n",new_counter, cmp_counter);
	printf("new_counter %d cmp_counter %d\n",new_counter,cmp_counter);
	printf("0.2 new_counter = %d,cmp_ounter = %d\n",new_counter, cmp_counter);
	printf("0.2.1 &new_filename[%d][0] = %p\n",new_counter,&new_filename[new_counter][0]);
*/
	//strncpy(new_filename[new_counter],dirp->d_name,strlen(dirp->d_name));
	sprintf(&new_filename[new_counter][0],"%s\n",dirp->d_name);
/*
	printf("0.3 new_counter = %d,cmp_ounter = %d\n",new_counter, cmp_counter);
	printf("new_filename[%d] %s\n",new_counter,new_filename[new_counter]);
	printf("0.4 new_counter = %d,cmp_ounter = %d\n",new_counter, cmp_counter);
*/
	new_counter++;

//	printf("1 new_counter = %d,cmp_ounter = %d\n",new_counter, cmp_counter);
	for (i=0;i < cmp_counter;i++){
		printf("1.1 compare i=%d ,cmp_counter %d, cmp_name %s\n",i,cmp_counter,cmp_filename[i]);
		if (strncmp(dirp->d_name,&cmp_filename[i][0],strlen(dirp->d_name)) == 0) {
			printf("1.2 found compare i=%d ,cmp_counter %d, cmp_name %s\n",i,cmp_counter,cmp_filename[i]);
			found = 1;
			break;
		}
	}
	printf("2 found %d\n",found);
	if (found)
		continue;
		
	printf("3 len = %d\n",len );
        printf("%-30s\n", dirp->d_name);
        len += sprintf(output_buf+len, "%s/%s\n", directory, dirp->d_name);    
	printf("4 new_counter = %d,cmp_ounter = %d, len %d\n",new_counter, cmp_counter,len);
   }
   printf("While End\n");
   tmp_filename = cmp_filename;
   cmp_filename = new_filename;
   new_filename = tmp_filename;

   cmp_counter = new_counter;
   printf("End new_counter = %d\n",new_counter);
   closedir(dp);

}
#endif
void sigalrm_fn(int sig)
{
		char sendbuf[32] = "Update list";
		int byteSent;

		LOG_DEBUG("alarm! pipefd %d\n",filefd[1]);
		byteSent = write( filefd[1], sendbuf, strlen( sendbuf ) );
		if (byteSent <=0){
			LOG_INFO("Send Pipe Failed %d\n",byteSent);
		}
		LOG_DEBUG("ByteSent %d write pipefd %d \n",byteSent,filefd[1]);


		alarm_lock = 0;
		
		return;

}

int alarm_main(void)

{
	LOG_DEBUG("alarm_main\n");

	treegen(WATCH_DIR);

	signal(SIGALRM, sigalrm_fn);

	return 0;
}
/* Directory Monitor Code End */



//------------------------------------------------------------------------------

//  kcard Kcard_app function

//------------------------------------------------------------------------------

//~~~~~~these function used by other files

//-------------------------------------------------------------------------------

void IO_control(driver_contl_type type, short* res_val)

{

    int fd,reg;



    switch(type)

    {

    case KCARD_RESCAN_IMAGES:

        reg =  0x13;

        break;

    case KCARD_ENABLE_USERCALL:

        reg =  0x11;

        break;

    case KCARD_DISABLE_USERCALL:

        reg =  0x10;

        break;

    case KCARD_ENABLE_SLEEP:

        reg =  0x21;

        break;

    case KCARD_DISABLE_SLEEP:

        reg =  0x20;

        break;

    case KCARD_SET_PID:

        reg =  0x16;

        break;

    case KCARD_GET_CMD:

        reg =  0x17;

        break;

    case KCARD_WAKEUP:

        reg =  0x100;

        break;

    case KCARD_GO_SLEEP:

        reg =  0x99;

        break;

    case KCARD_DEL_PROGRAM_BIN:

        reg =  0x14;

        break;

    case KCARD_SET_USERCALL_INTERVAL:

        reg =  0x12;

        break;

    case KCARD_TRIGGER_FULL_RANGE:

        reg = 0x23;

        break;

    case KCARD_STOP_CTRL_IMG_CHECK:

        reg =  0x22;

        break;

    default:

        printf("nothing");

        break;

    }

    fd = open ("/dev/ka-main", O_RDWR);

    if (fd<0)

    {

        printf ("err open /dev/ka-main\n");

        return ;

    }



    ioctl(fd,reg,res_val);

    close(fd);

}

//------------------------------------------------------------------------------

//~~~~~~these function used by other files ,just test kcard_app function for 18.3

//~~~~~~

void enable_kcard_call(void)

{

	LOG_DEBUG("%s: \n",__FUNCTION__);
    IO_control(KCARD_ENABLE_USERCALL,NULL);

}

void disable_kcard_call(void)

{

	LOG_DEBUG("%s: \n",__FUNCTION__);
    IO_control(KCARD_DISABLE_USERCALL,NULL);

}

void set_kcard_interval(int usercall_interval)

{

	int fd;



	fd = open ("/dev/ka-main", O_RDWR);

	if (fd<0)

	{

		printf ("err open /dev/ka-main\n");

		return ;

	}



	ioctl(fd,KCARD_SET_USERCALL_INTERVAL,usercall_interval);



	close(fd);

}

void enable_power_sleep(void)

{

    IO_control(KCARD_ENABLE_SLEEP,NULL);

}



void disable_power_sleep(void)

{

    IO_control(KCARD_DISABLE_SLEEP,NULL);

}

//~~~~~~



int check_control_image(void)

{

    int i;

    int cp = 0;

    for (i = 1; i <= CTRL_PIC_NUM; i++)

    {

        if (control_image[i].exist == 0)

        {
			LOG_INFO("%s: Copy %s to %s\n",__FUNCTION__,control_image[i].source,control_image[i].target);
            copy_file(control_image[i].source, control_image[i].target, FILEUTILS_FORCE);

            cp++;

        }

    }



    if (cp)

    {

        sync(); //force superblock are updated immediately

		LOG_INFO("%s: check_control_image umount & mount\n",__FUNCTION__);

        //system("umount /mnt/sd && mount -t vfat --shortname=winnt /dev/mmcblk0p1 /mnt/sd");
    	system("/usr/bin/refresh_sd");

    }



    for (i = 1; i <= CTRL_PIC_NUM ; i++)

    {

        if (control_image[i].exist == 0)

        {

//			printf("%s: return %d\n",__FUNCTION__,control_image[i].func);
            return (control_image[i].func);

        }

    }

    return 0;

}



int check_dir_exist(char *fileName)

{

    DIR *dirp = opendir(fileName);



    if (dirp == NULL)

    {

        return 0;

    }

    closedir(dirp);

    return 1;

}

//------------------------------------------------------------------------------

int check_and_create_path(char *path)

{

    char cmd[1024];

    if (check_dir_exist(path) == 0)

    {

        sprintf(cmd, "mkdir -p %s", path);

        system(cmd);

    }

	return 1;

}

int check_dev(void)
{
	FILE *fp;
	char line[100];
	char *token;

	system("cp /proc/self/mounts /etc/");
	fp=fopen("/etc/mounts","r");

	if (fp == NULL)
		return -1;

	/* seek to pos */
	fseek(fp, 0, SEEK_SET);

	/* scan line one by one */
	while(!feof(fp)) {
		/* get one line */
		if(fgets(line, 100, fp)==NULL)
			break;

		/* find if with token */
		token = strstr(line, "mmcblk0p1");
		if(token != NULL) {
			printf("mmcblk0p1 mounted\n");
			fclose(fp);
			return 1;
		}
	}
	printf("mount sd failed\n");
	fclose(fp);		
	return 0;
}
//------------------------------------------------------------------------------

int check_ctrl_folder_exist(void)
{



	int try_copy_count =0;

	int i,timeout=0;
	int checkdev=0;
	system("buzzer -f 2");

	checkdev=check_dev();

	while(checkdev==0 ||checkdev==-1) {
		timeout++;
		sleep(1);
		printf("re_mount sd\n");
		system("mount -t vfat --shortname=winnt /dev/mmcblk0p1 /mnt/sd");
		checkdev=check_dev();
		if(timeout==15) {
			printf("stop remount sd");
			break;
		}
	}
    //system("buzzer -f 2");

    //system("mkdir -p /mnt/sd/DCIM/100_WIFI");

    //system("mkdir -p /mnt/sd/MISC");

    //system("mkdir -p /mnt/sd/DCIM/101_WIFI");

    //system("cp /mnt/mtd/config/cimgconf /mnt/sd");

    LOG_INFO("%s: Recovery dir %s\n",__FUNCTION__, wifi_dir);

#if TS_RESET == 0
    system("factory_reset.sh");
#else

    system("control_create.sh");
#endif

    sync(); //force superblock are updated immediately



    if (check_dir_exist(wifi_dir) == 0)//floder not exist
        sleep(2);



    //printf("copy\n");

    for (i = 1; i <= CTRL_PIC_NUM ; i++) {
		LOG_INFO("%s: Recovery %d \n",__FUNCTION__,i);
		LOG_DEBUG("%s: %d Copy %s to %s\n",__FUNCTION__,i,control_image[i].source,control_image[i].target);
        copy_file(control_image[i].source, control_image[i].target, FILEUTILS_FORCE);
        control_image[i].exist = 1;
    }
	if (_hide_wifi_folder)
		set_dir_hidden(wifi_dir);

    system("sleep 0.2 && chmod 666 /mnt/sd/DCIM/199_WIFI/*");
    sync(); //force superblock are updated immediately

//    printf("sync \n");

//    printf("refresh\n");

//    printf("check_ctrl_folder_exist umount & mount\n");
    LOG_INFO("check_ctrl_folder_exist umount & mount\n");

    system("sleep 0.2 && umount /mnt/sd && sleep 0.2 && mount -t vfat --shortname=winnt /dev/mmcblk0p1 /mnt/sd");

    //system("buzzer -f 5");

//    printf("done\n");

	/* Clean instant-uploaded list */
	system("instant_upload_clean");

    return 0;

}
static int is_file_exist (char *path)
{
    FILE* fp = fopen(path, "r");
    if (fp) {
        // file exists
        fclose(fp);
        return 1;
    } else {
        // file doesn't exist
        return 0;
    }
}

int kcard_app_act(int cmd)

{

    int img_ctrl_cmd = 0;

	LOG_INFO("kcard_app_act - ");
	kcard_print_signal(cmd, 0);

    //IO_control(KCARD_DISABLE_USERCALL,NULL);
	disable_kcard_call();

    //system("refresh_sd");

    img_ctrl_cmd = cmd;



    if (img_ctrl_cmd == KCARD_SIG_UPDATED)

    {
		int update_cmd;

        //IO_control(KCARD_ENABLE_USERCALL,NULL);
		enable_kcard_call();

#if 0
		update_cmd = check_control_image_real();
		if( update_cmd == KCARD_SIG_DEL_IMG_0||
            update_cmd == KCARD_SIG_DEL_IMG_1||
            update_cmd == KCARD_SIG_DEL_IMG_2||
            update_cmd == KCARD_SIG_DEL_IMG_3) {

			if (control_image[update_cmd].triggered == 0) {
				LOG_INFO("%s update_cmd %d\n",__FUNCTION__,update_cmd);
				if( update_cmd != -1 ) {
					LOG_INFO ("Run ACT0 %s\n",control_image[update_cmd].act0);
					if(system(control_image[update_cmd].act0)==-1)
						printf("failed to call act0\n");

					if(update_cmd > 0) {
						LOG_INFO("Run ACT1 %s\n",control_image[update_cmd].act1);
						if(system(control_image[update_cmd].act1)==-1)
							printf("failed to call act1\n");
					}
					control_image[update_cmd].triggered = 1;
				}
			}
		}
#endif
        return 0;

    }

    else if (img_ctrl_cmd == KCARD_SIG_SCAN)

    {
//timc
#ifdef KEYASIC_BURNIN_PATCH
		enable_kcard_call();
        return 0;
#else
        check_ctrl_folder_exist();

        sleep(3);

        //IO_control(KCARD_ENABLE_USERCALL,NULL);
		enable_kcard_call();

        return 0;
#endif
    }

    else if (img_ctrl_cmd == KCARD_SIG_NO_IMGS)

    {
//timc
#ifdef KEYASIC_BURNIN_PATCH
		enable_kcard_call();
		return 0;
#else
        if (check_ctrl_folder_exist() == 0)

        {

            sleep(3);

           //IO_control(KCARD_ENABLE_USERCALL,NULL);
			enable_kcard_call();

            return 0;

        }

        //img_ctrl_cmd = check_control_image();
        img_ctrl_cmd = 0;

        //return 0;
#endif
    }

//img_ctrl_cmd++;

    if(img_ctrl_cmd==KCARD_SIG_DEL_IMG_0||

            img_ctrl_cmd==KCARD_SIG_DEL_IMG_1||

            img_ctrl_cmd==KCARD_SIG_DEL_IMG_2||

            img_ctrl_cmd==KCARD_SIG_DEL_IMG_3)

    {

//        img_ctrl_cmd--;

        LOG_INFO("kcard_app_act_cmd %d\n",img_ctrl_cmd);
        int i = img_ctrl_cmd;		

        if (!is_file_exist(control_image[i].target)) {
			LOG_INFO("%s: Copy %s to %s\n",__FUNCTION__,control_image[i].source,control_image[i].target);
            copy_file(control_image[i].source, control_image[i].target, FILEUTILS_FORCE);
            system("sync");  
        }

        if( img_ctrl_cmd != -1) {

            LOG_INFO ("Run ACT0 %s\n",control_image[img_ctrl_cmd].act0);


            if(system(control_image[img_ctrl_cmd].act0)==-1)

                printf("failed to call act0\n");



            if(img_ctrl_cmd > 0)

            {

            	LOG_INFO("Run ACT1 %s\n",control_image[img_ctrl_cmd].act1);
                if(system(control_image[img_ctrl_cmd].act1)==-1)

                    printf("failed to call act1\n");

            }

            control_image[img_ctrl_cmd].triggered = 1;

        }

    }

    sleep(3);
 
    //IO_control(KCARD_ENABLE_USERCALL,NULL);
	enable_kcard_call();

    return 0;

}

void null(void)

{

    printf("hello");

}



//Kobby-20120116-Add_Begin for support shoot and view

/*

int clientfd;

void TS_SetClientSocket()

{



	uint16_t server_port;

	len_and_sockaddr *servAddr;



	printf( "TS TCP Client set Client Socket Begin\n" );

	

	server_port = bb_lookup_port( "5566", "tcp", 0);

	servAddr = xhost2sockaddr( "127.0.0.1", server_port);

	

	clientfd = xsocket(servAddr->u.sa.sa_family, SOCK_STREAM, 0);

	

	if( clientfd < 0 ) {



		printf( "TS TCP Client socket fail\n" );

		return;



	}

	

	setsockopt_reuseaddr( clientfd );

	

	xconnect( clientfd, &servAddr->u.sa, servAddr->len );

	

	printf( "TS TCP Client set Client Socket End\n" );



}

*/

//#define _DEBUG_LOG

int getFreeIndex( int clifd[] )
{
	int index;
	int ret = -1;

	for( index = 0; index < MAXFDCOUNT; ++index ) {
		if( clifd[index] == -1 ) {
			ret = index;
			break;
		}
	}

#ifdef _DEBUG_LOG	
	printf( "Free ret = %d \n", ret );
#endif

	return ret;
}

void set_tcp_keepalive(int fd)
{
	int optval,optlen;
   /* Set the option active */
   optval = 1;
   optlen = sizeof(optval);
   if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
      perror("setsockopt()");
   }
   LOG_DEBUG("SO_KEEPALIVE set on socket\n");
}

void TS_SetServerSocket() {
	int servfd = -1;
	fd_set fdset;
	uint16_t local_port;
	len_and_sockaddr *lsa;
	socklen_t sa_len;
	unsigned backlog = 20;

#ifdef _DEBUG_LOG		
	printf( "********************\n" );
	printf( "TS TCP Server Start*\n" );
	printf( "********************\n" );
#endif

	for( int tempindex = 0; tempindex < MAXFDCOUNT; ++tempindex )
		clifd[tempindex] = -1;

	local_port = bb_lookup_port( "5566", "tcp", 0);
	lsa = xhost2sockaddr( "0.0.0.0", local_port);//modify by willie 20120410

	servfd = xsocket(lsa->u.sa.sa_family, SOCK_STREAM, 0);
	if( servfd < 0 ) {
#ifdef _DEBUG_LOG	
		printf( "TS TCP Server create socket fail\n" );
#endif
		return;
	}
	LOG_DEBUG("Server FD %d\n",servfd);

	setsockopt_reuseaddr( servfd );
	sa_len = lsa->len; /* I presume sockaddr len stays the same */

	xbind( servfd, &lsa->u.sa, sa_len );
	xlisten( servfd, backlog );
	close_on_exec_on( servfd );

	set_tcp_keepalive(servfd);

	while( 1 ) {
		int state;
		int index;
		int maxfd;

		FD_ZERO( &fdset );
		FD_SET( filefd[0], &fdset );
		FD_SET( servfd, &fdset );

		maxfd = (servfd > filefd[0]) ? servfd + 1 : filefd[0] + 1;
		for( index = 0; (lastfd != -1) && (index <= lastfd); index++) {
			if (clifd[index] != -1) {
				FD_SET(clifd[index], &fdset);
				if (clifd[index] > maxfd)
					maxfd = clifd[index];
			}
		}
		LOG_DEBUG("Start select \n");
		state = select(maxfd, &fdset, NULL, NULL, NULL );
		if( state < 0) {
			LOG_INFO( "TS TCP Server select fail\n" );
			return;
		}
		LOG_DEBUG("Got select fd state %d\n",state);

		if (FD_ISSET(servfd, &fdset)) {
			int newfd = -1;
			int fdIndex;
			len_and_sockaddr newAddr;

			newAddr.len = sa_len;
			newfd = accept(servfd, &newAddr.u.sa, &newAddr.len);

			fdIndex = getFreeIndex( clifd );
			if (fdIndex != -1) {
				clifd[fdIndex] = newfd;
				LOG_DEBUG("Alloc slot %d for fd %d\n",fdIndex,newfd);
				if( ( lastfd == -1 ) || ( fdIndex > lastfd ) ) {
					lastfd = fdIndex;
					LOG_DEBUG("lastfd = %d\n",lastfd);
				}
				LOG_INFO( "new connection index %d newfd = %d %s:%d lastfd_index %d\n", 
						fdIndex, newfd, 
						inet_ntoa(newAddr.u.sin.sin_addr),ntohs(newAddr.u.sin.sin_port),
						lastfd);
			}else {
				LOG_INFO("No freeIndex for new connection newfd = %d %s:%d lastfd_index %d\n", 
						newfd, 
						inet_ntoa(newAddr.u.sin.sin_addr),ntohs(newAddr.u.sin.sin_port),
						lastfd);

			}
		} // ISSET(serverfd)

		/* Try to detect TCP connection */
		for( index = 0; (lastfd != -1) && (index <= lastfd); index++) {
			if (clifd[index] != -1) {
				if (FD_ISSET( clifd[index], &fdset)) {
					int byteRecv = 0;
					char recvbuf[32] = "";

					byteRecv = read(clifd[index], recvbuf, sizeof( recvbuf ) );
					if (byteRecv == 0) {
						LOG_INFO("index %d FD %d Disconnect\n",index,clifd[index]);
						clifd[index] = -1;
						if (index == lastfd) {
							if (index == 0)
								lastfd = -1;
							else
								lastfd--;
						}
					}
					if (byteRecv > 0) {
						LOG_INFO("TCP FD %d is supposed to get no data!\n",clifd[index]);
					}
				}
			}
		}


		if( FD_ISSET( filefd[0], &fdset ) ) {
			int byteRecv = 0;
			char recvbuf[32] = "";
			char outbuf[MAX_OUT_BUF_LEN] = "";

			byteRecv = read( filefd[0], recvbuf, sizeof( recvbuf ) );
			LOG_INFO("Get Pipe FD %d Read Len %d [%s]\n",filefd[0],byteRecv,recvbuf);	
			if( strcmp( recvbuf, "Update list" ) == 0 ) {
				int index;
				LOG_DEBUG( "Update list\n" );	
				/* Format will cause update signal , so check DCIM folder to recreate DCIM folder 
				 * After remove all control picture, the NO_IMG is not happend right away */
				if (!file_is_exist(WATCH_DIR)) {
					system("control_create.sh");
					sync(); //force superblock are updated immediately
				}

				treecheck(WATCH_DIR,outbuf);

#ifdef _DEBUG_LOG	
				printf("outbuf=%s\n",outbuf);
#endif
			}
		} //ISSET(fd) PIPE IN
	} // while (1)
}

//Kobby-20120116-Add_End for support shoot and view

void kcard_usr2(int sig)
{
	char sendbuf[32] = "Update list";
	int byteSent;

	/* set NULL sig handler avoid */

	(void) signal(SIGUSR2, null);

	LOG_DEBUG("usr2 ! pipefd %d\n",filefd[1]);
	byteSent = write( filefd[1], sendbuf, strlen( sendbuf ) );
	if (byteSent <=0){
		LOG_INFO("usr2 Send Pipe Failed %d\n",byteSent);
	}
	LOG_DEBUG("usr2 ByteSent %d write pipefd %d \n",byteSent,filefd[1]);

	(void) signal(SIGUSR2, kcard_usr2);
}


void kcard_program(int sig)

{

    short cmd = 0;

    int skip = 0;



    /* set NULL sig handler avoid */

    (void) signal(SIGUSR1, null);

    IO_control(KCARD_GET_CMD,&cmd);

    LOG_INFO("get_Kcard_app cmd %d - ", cmd);
	kcard_print_signal(cmd, 1);



//*Kobby-20120116-Add_Begin for support shoot and view

	if( cmd == KCARD_SIG_UPDATED ) {
		if (alarm_lock == 0){
				alarm_lock = 1;
				LOG_DEBUG("Send alarm %d\n",_debounce_time);
				alarm(_debounce_time);
		}else {
			LOG_INFO("No alarm lock\n");
		}
	}

//Kobby-20120116-Add_End for support shoot and view*/



    if(cmd==KCARD_SIG_SLEEP || cmd==KCARD_SIG_WAKEUP)

    {

        printf("Re-setup the signal handler");

    }

    else if(cmd >= KCARD_SIG_DEL_IMG_0&& cmd <= KCARD_SIG_DEL_IMG_3)

    {

//    cmd--;

        if (control_image[cmd].triggered)

            skip = 1;

        control_image[cmd].triggered = 1;



        if (!skip)

        {

            kcard_app_act(cmd);

            if(cmd == KCARD_SIG_NO_IMGS)

                IO_control(KCARD_RESCAN_IMAGES, NULL);

        }

        else

        {

            system ("refresh_sd");

            sleep(3);

        }

    }

    else

    {

        kcard_app_act(cmd);

        if(cmd == KCARD_SIG_NO_IMGS)

            IO_control(KCARD_RESCAN_IMAGES, NULL);

    }

    

    (void) signal(SIGUSR1, kcard_program);



}
int check_sd_mount_rw(void)
{
	FILE * fd = NULL;
	char buff[512]  = {0};
	int ret=0; 

	fd = fopen("/proc/mounts","r");
    if (fd == NULL) {
		LOG_INFO("%s: can't open /proc/mounts\n",__FUNCTION__);
        return 0;
    }

    while( fgets(buff,sizeof(buff),fd) != NULL ){
		if (strncmp(buff,"/dev/mmcblk0p1",strlen("/dev/mmcblk0p1")) == 0) {
			if (strstr(buff,"rw") != NULL){
				ret =1;
				break;
			}else  {
				LOG_INFO("%s: SD Mount but not read/write\n",__FUNCTION__);
				ret = 0;
				break;
			}
		}
	}
	fclose(fd);

    return ret;

}
int check_control_image_real(void)

{

    int i;
    int cp = 0;
	int cmd = 0;
	int try_copy_count =0;

    for (i = 1; i <= CTRL_PIC_NUM; i++) {

		if (file_is_exist(control_image[i].target) == 0)//floder not exist
        {
			LOG_INFO("%s: Copy %s to %s\n",__FUNCTION__,control_image[i].source,control_image[i].target);
			copy_file(control_image[i].source, control_image[i].target, FILEUTILS_FORCE);

			control_image[i].exist == 1;
            cp++;

			if (control_image[i].triggered ==0 && cmd == 0)
				cmd = control_image[i].func;
        }
    }

    if (cp) {
        sync(); //force superblock are updated immediately
		LOG_INFO("%s: umount & mount\n",__FUNCTION__);
    	system("/usr/bin/refresh_sd");
    }

    return cmd;

}
int bootup_check_image(void)
{
	int cmd;


    if (check_dir_exist(wifi_dir) == 0)//floder not exist
		cmd = 0;
	else
		cmd = check_control_image_real();

    if(cmd ==KCARD_SIG_DEL_IMG_0||

            cmd == KCARD_SIG_DEL_IMG_1||

            cmd == KCARD_SIG_DEL_IMG_2||

            cmd == KCARD_SIG_DEL_IMG_3)
	{

		LOG_INFO("%s cmd %d\n",__FUNCTION__,cmd);
		if( cmd != -1 && control_image[cmd].triggered == 0) {
//			cmd--;
			LOG_INFO ("Run ACT0 %s\n",control_image[cmd].act0);
			if(system(control_image[cmd].act0)==-1)
				printf("failed to call act0\n");

			if(cmd > 0) {
				LOG_INFO("Run ACT1 %s\n",control_image[cmd].act1);
				if(system(control_image[cmd].act1)==-1)
					printf("failed to call act1\n");
			}
			control_image[cmd].triggered = 1;

		}
		return cmd;
	}
	return 0;
}
int bootup_check_process(void)
{
	char * find = "GPlus-Enable";
	char value[256] = {0};
	char cmd[256] = {0};
	int ret;
	int cmd_fd;

	/* Check Gplus instant upload */
	cmd_fd = TS_create_cmd_tunnel();
	if (cmd_fd > 0) {
		ret = read_wifiConfigFile(find, value);
		if (ret > 0) {
			if (strncmp(value,"YES",strlen("YES")) == 0) {
				sprintf(cmd,"%s\n","/usr/bin/ap_iu.sh");
				TS_send_cmd(cmd_fd,cmd);
			}
		}

		TS_close_cmd_tunnel(cmd_fd);
		return 0;
	}
	LOG_INFO("%s failed. Create cmd tunnel failed. failed.\n",__FUNCTION__);
	return -1;
}

//-------------------------------------------------------------------------
void simple_write_file(char *filename, char * buf)
{
	FILE * fd = NULL;
	fd = fopen(filename,"w");
	if (fd == NULL){
		LOG_DEBUG("simple_write_file failed. Open %s failed.\n",filename);
		return;
	}
	fwrite(buf,strlen(buf),1,fd);
	fsync(fd);
	fclose(fd);
}
void simple_append_file(char *filename, char * buf)
{
	FILE * fd = NULL;
	fd = fopen(filename,"a");
	if (fd == NULL){
		LOG_DEBUG("simple_append_file failed. Open %s failed.\n",filename);
		return;
	}
	fwrite(buf,strlen(buf),1,fd);
	fsync(fd);
	fclose(fd);
}
static void
usage(void) 
{
	printf("Usage: kcard_app [OPTIONS]\n");
	printf("Options list --- \n");
	printf("	-h                      -- Display usage.\n");
	printf("	-d log_level\n");
	printf("	-l [log_path]           -- default is /tmp/filediff \n");
	printf("	-e [log_event_path]     -- default is /tmp/log.kcard\n");
	printf("	-t debounce_time        -- default is 2s\n");
	printf("	--nohidden              -- Hide WIFI folder.\n");
	printf("	--skipctrl              -- Skip detect WIFI folder.\n");
}

int kcard_app_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;

int kcard_app_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)

{

	char tmp_buf[128] = {0x0};
    pid_t  pid = getpid();



    /* set sig handler */

    (void) signal(SIGUSR1, kcard_program);

    //LOG_INFO("set pid %d\n", (int)pid);
    sprintf(tmp_buf,"%d", (int)pid);

	simple_write_file(KCARD_MAIN_PID_FILE, tmp_buf);

    LOG_INFO("Notify driver about pid %d\n", (int)pid);
    IO_control(KCARD_SET_PID,&pid);

    //IO_control(KCARD_STOP_CTRL_IMG_CHECK, NULL);

    //IO_control(KCARD_TRIGGER_FULL_RANGE, NULL);



	//Kobby-20120116-Add_Begin for support shoot and view

	int tspid;

	int ret;
	int i;

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
			case 't':
				if ((i+1) >= argc){
					usage();
					return -1;
				}
				_debounce_time = atoi(argv[i+1]);
				i++;
				break;
			case 'l':
				_log_diff_enable = 1;
				if ((i+1) < argc && (argv[i+1][0] != '-')){
					_log_path = argv[i+1];
				}else {
					_log_path = LOG_PATH_DEFAULT;
				}
				break;
			case 'e':
				_evt_log_enable = 1;
				if ((i+1) < argc && (argv[i+1][0] != '-')){
					strncpy(_evt_log_path, argv[i+1], 255);
				}else {
					strncpy(_evt_log_path, LOG_PATH_DEFAULT, strlen(LOG_PATH_DEFAULT));
				}
				break;

			case 'h':
				usage();
				return 0;
			case '-':
				if (strcmp(&argv[i][1],"-nohidden") == 0){
					_hide_wifi_folder = 0;
				}else if (strcmp(&argv[i][1],"-skipctrl")== 0) {
					_skip_wifi_folder = 1;
				}else {
					usage(); 
					return -1;
				}
				break;
			default:
				usage ();
				return -1;
			}
		}
		i++;
	}

	LOG_INFO("Log = %d, Diff %s, Path %s\n",_log_level,_log_diff_enable?"Enable":"Disable",_log_path);
	LOG_INFO("Debounce = %d, WiFi Hide %s, Skip %s\n",_debounce_time, _hide_wifi_folder?"Enable":"Disable",_skip_wifi_folder?"Enable":"Disable");
	sprintf(tmp_buf, "Log = %d, Diff %s, Path %s\n",_log_level,_log_diff_enable?"Enable":"Disable",_log_path);
	kcard_log(tmp_buf);

	sprintf(tmp_buf, "Debounce = %d, WiFi Hide %s, Skip %s\n",_debounce_time, _hide_wifi_folder?"Enable":"Disable",_skip_wifi_folder?"Enable":"Disable");
	kcard_log(tmp_buf);
	

	ret = pipe( filefd );	//create pipe

	tspid = fork();			/* fork to execute external program or scripts */

	

#ifdef _DEBUG_LOG

	printf("ret pipe = %d\n", ret);

	printf("tspid fork = %d\n", tspid);

	printf( " kcard_app_main filefd[0] = %d\n",  filefd[0] );

	printf( " kcard_app_main filefd[1] = %d\n",  filefd[1] );

#endif



	if( tspid == -1 ) 

		printf( "TS fork fail\n" );

	else if( tspid == 0 ) //child process

	{
		//bootup_check_image();
		LOG_DEBUG( "TS_SetServerSocket()\n" );

		//close( filefd[1] );

		TS_SetServerSocket();

		return 0;
	}

	else //parent process

	{


    	sprintf(tmp_buf,"%d", (int)tspid);
		simple_write_file(KCARD_SUB_PID_FILE, tmp_buf);
		close( filefd[0] );

//		sleep( 3 );

//		TS_SetClientSocket();

		bootup_check_process();

		alarm_main();

		/* Setup USR2 signal */
		(void) signal(SIGUSR2, kcard_usr2);
	}

	//Kobby-20120116-Add_End for support shoot and view



    /* start the main loop */

    while (1)
    {
        LOG_INFO("waitting for sig\n");
        pause();
    }

}


