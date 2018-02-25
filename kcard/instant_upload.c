		
#include "libbb.h"
#include "transcend.h"
//#include "libcoreutils/coreutils.h"
#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

#define MAX_FORK_PROC   1
#define mb()   __asm__ __volatile__  ( "" ::: "memory" ) 


static int _log_level = LOG_LEVEL_INFO;
static int _touch_file = 1;
static int child_stat; 
static int update_pic_flag = 0;
static int gen_cpt_flag = 0;
volatile static int fork_count = MAX_FORK_PROC;
static int auth_mode = 0;   //0->external , 1-> login by user, password

static char album[128] = "WiFiSD";
//static char * upload_done_pic = "/home/sd/DCIM/100_WIFI/WSD00003.JPG";
static char * upload_done_pic = NULL;

volatile static int total_jpg = 0;
volatile static int total_cpt = 0;
volatile static int total_fail= 0;
volatile static	int total_progress = 0;
#define LOG_INFO(args...)	printf("INUP: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) printf("INUP: "args);}

static int fd_progressd = -1;

static int progress_write_complete(char *file);
static int progress_write_total_complete(int total) ;
static int progress_write_fail(char *file);
static int progress_write_total_fail(int total);

static int progress_clean(void) 
{
	LOG_INFO("Clean progress data\n");
	unlink("/var/run/is/fail");
	unlink("/var/run/is/total_fail");

	unlink("/var/run/is/complete");
	unlink("/var/run/is/total_cpt");

	unlink("/var/run/is/total");
	total_cpt = 0;
	total_fail= 0;
	total_jpg= 0;
	total_progress  = 0;
	return 0;
}
static int progress_write_complete(char *file) 
{
	char tmp_buf[256] = {0};

	sprintf(tmp_buf,"%s\n",file);
	simple_append_file("/var/run/is/complete",tmp_buf);

	return 0;
}
static int progress_write_total_complete(int total) 
{
	char tmp_buf[256] = {0};

	sprintf(tmp_buf,"%d\n",total);
	simple_write_file("/var/run/is/total_cpt",tmp_buf);

	return 0;
}

static int progress_write_fail(char *file) 
{
	char tmp_buf[256] = {0};

	sprintf(tmp_buf,"%s\n",file);
	simple_append_file("/var/run/is/fail",tmp_buf);

	return 0;
}
static int progress_write_total_fail(int total) 
{
	char tmp_buf[256] = {0};

	sprintf(tmp_buf,"%d\n",total);
	simple_write_file("/var/run/is/total_fail",tmp_buf);

	return 0;
}
int file_is_exist(char * path)
{
	int fd;
	if (path == NULL) {
		return 0;
	}

	LOG_DEBUG("%s check %s \n",__FUNCTION__,path);
	fd = open(path, O_RDONLY);
	if (fd >= 0) {
		LOG_DEBUG("%s %s exist\n",__FUNCTION__,path);
		close(fd);
		return 1;
	}
	LOG_DEBUG("%s %s not exist\n",__FUNCTION__,path);
	return 0;
}
static int login(void) 
{
	char cmd[128] = {0};
	FILE * fd;
	char file[256];


	sprintf(cmd,"/usr/bin/gp_login.sh");
	if (system(cmd) != 0)
		return -3;


    fd = fopen("/etc/auth.inc","r");
    if (fd == NULL) {
		printf("Can't open %s\n","/etc/auth.inc");
		return -1;
	}
	while ((fgets(file,256,fd))){
		if (strncmp("Error=",file,strlen("Error="))==0){
			printf("Login Failed. %s\n",file);
			return -2;
		}
	}

    return 0;
}
static int set_external_progress(int code, char * msg)
{
	char tmp_buf[256] = {0};

	if (msg == NULL)
		return -1;

	sprintf(tmp_buf,"%d\n",code);
	simple_write_file("/var/run/is/status",tmp_buf);

	sprintf(tmp_buf,"%s\n",msg);
	simple_write_file("/var/run/is/message",tmp_buf);

	return 0;
}
static int clean_external_progress(void)
{
	char tmp_buf[256] = {0};

	LOG_DEBUG("%s: \n",__FUNCTION__);

	unlink("/var/run/is/status");

	unlink("/var/run/is/message");

	return 0;
}
static int get_config(void) 
{
	char cmd[128] = {0};
	FILE * fd;
	char file[256] = {0};
	char buf[256] = {0};
	FILE * filed;
	int ret = 0;



	sprintf(cmd,"/usr/bin/gp_config.sh");
	if (system(cmd) != 0) {
		printf("gp_config failed!\n");
		return -3;
	}


    fd = fopen("/etc/auth.inc","r");
    if (fd == NULL) {
		printf("Can't open %s\n","auth.inc");
		set_external_progress(300, "Can't open auth.inc\n");
		return -1;
	}
	fclose(fd);

	filed = popen("/usr/bin/gp_validate_token.sh","r");
	if (filed != NULL){
		fgets(buf,sizeof buf,filed);
		LOG_DEBUG("%s: gets %s\n",__FUNCTION__,buf);
		ret = pclose(filed);
		if (ret != 0) {
			set_external_progress(300, buf);
			return -5;
		}
		clean_external_progress();
	}else {
		set_external_progress(300, "Validate process failed.");
		return -5;
	}


    return 0;
}

static int create_album(void) 
{
	char cmd[128] = {0};
	int ret;

	sprintf(cmd,CREATE_SCRIPT " \"%s\"",album);
	if ((ret = system(cmd)) != 0) {
		printf("Create Album failed\n");
		return ret;
	}
    return 0;
}
static int setup_config(void) 
{
	char cmd[128] = {0};
	int ret;

	sprintf(cmd,SETUP_SCRIPT " \"%s\"",album);
	if ((ret = system(cmd)) != 0) {
		printf("Create Album failed\n");
		return ret;
	}
    return 0;
}
static int touch_file(char * filename)
{
	char tmp[256] = {0};
	char tmp_dir[256] = {0};
	char * dirname_p = NULL;
	int fd;
	
	LOG_DEBUG("%s filename %s\n",__FUNCTION__,filename);

	if (filename == NULL) {
		printf("Touch file failed. filename is empty\n");
		return -1;
	}

	if (strlen(filename) >= (256 - 25)) {
		printf("Touch file failed. filename %s too long\n",filename);
		return -2;
	}

	sprintf(tmp_dir,"%s",filename);
	dirname_p = dirname(tmp_dir);
	if (file_is_exist(dirname_p) == 0) {
		LOG_DEBUG("%s Create Dir %s\n",__FUNCTION__,dirname_p);
		if (bb_make_directory(dirname_p, 0755, FILEUTILS_RECUR)) {
			printf("create_full_dir failed. bb_make_directory %s failed\n",dirname_p);
			return -1;
		}

	}
	sprintf(tmp,"%s",filename);

	LOG_DEBUG("%s open filename %s\n",__FUNCTION__,tmp);
	fd = open(tmp, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		printf("Touch file failed. create %s failed\n",tmp);
		return -4;
	}
	close(fd);
	return 0;
}
#define UPLOAD_MIRROR_BASE "/mnt/mtd/instant_uploaded"
static int touch_mirror_file(char * filename)
{
	char tmp[256] = {0};
	char tmp_dir[256] = {0};
	char * dirname_p = NULL;
	int fd;
	
	LOG_DEBUG("%s filename %s\n",__FUNCTION__,filename);

	if (filename == NULL) {
		printf("Touch file failed. filename is empty\n");
		return -1;
	}

	if (strlen(filename) >= (256 - 25)) {
		printf("Touch file failed. filename %s too long\n",filename);
		return -2;
	}

	sprintf(tmp_dir,"%s%s",UPLOAD_MIRROR_BASE,filename);
	dirname_p = dirname(tmp_dir);
	if (file_is_exist(dirname_p) == 0) {
		LOG_DEBUG("%s Create Dir %s\n",__FUNCTION__,dirname_p);
		if (bb_make_directory(dirname_p, 0755, FILEUTILS_RECUR)) {
			printf("create_full_dir failed. bb_make_directory %s failed\n",dirname_p);
			return -1;
		}

	}
	sprintf(tmp,"%s%s",UPLOAD_MIRROR_BASE,filename);

	LOG_DEBUG("%s open filename %s\n",__FUNCTION__,tmp);
	fd = open(tmp, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		printf("Touch file failed. create %s failed\n",tmp);
		return -4;
	}
	close(fd);
	return 0;
}


int file_is_exist_on_mirror(char * path)
{
	char tmp[512] = {0};

	if (strlen(path) >= (512 - strlen(UPLOAD_MIRROR_BASE))) {
		LOG_INFO("%s %s too long\n",__FUNCTION__,path);
		return 1;
	}

	sprintf(tmp,"%s%s",UPLOAD_MIRROR_BASE,path);

	return file_is_exist(tmp);
}
static int notify_progressd(char * buf)
{
	LOG_DEBUG("%s: %s\n",__FUNCTION__, buf);
	udp_send(fd_progressd, buf); 
}
static int upload_photo(char * filename,int create_record) 
{
	char cmd[512] = {0};
	char filename_buf[512] = {0};
	FILE * filed;
	int percent;

	LOG_DEBUG("[%d] Upload %s, Create mirror %d\n",getpid(),filename,create_record);

	if (file_is_exist_on_mirror(filename)) {
		LOG_INFO("[%d] %s already uploaded.\n",getpid(),filename);
		return 0;
	}

	sprintf(cmd,UPLOAD_SCRIPT " \"%s\" \"%s\"",album,filename);

	filed  = popen(cmd,"r");
	if (filed != NULL) {

		sprintf(filename_buf, "uploading:%d:%s", total_progress, filename);
		notify_progressd(filename_buf);
		while (!feof(filed)) {
			if (fgets(cmd,sizeof cmd,filed) != NULL) {
				percent = 0;
				if (sscanf(cmd, "%*s %d",&percent) == 1) {
					sprintf(filename_buf,"/var/run/is/UP/%s",filename);
					sprintf(cmd, "%d",percent);
					LOG_INFO("[%d] Upload %s Percent %d\n",getpid(),filename,percent);
					simple_write_file(filename_buf,cmd);
					if (percent == 100)
						break;
				}else {
					//LOG_INFO("Upload %s :can't get Percent str %s\n",filename,cmd);
					sleep(1);
				}
				memset(cmd, 0x0 , sizeof cmd);
			}
		}
		if (pclose(filed) != 0){
			LOG_INFO("[%d] Upload %s :%s return error\n",getpid(),filename,cmd);
		}
	} else {
		printf("[%d] Upload %s failed\n",getpid(),filename);
		return -1;
	}

	if (percent != 100) 
		LOG_INFO("Upload %s percent %d != 100%\n",filename,percent);

	if (create_record)
		touch_mirror_file(filename);

	LOG_DEBUG("[%d] Upload %s success\n",getpid(),filename);

	progress_write_complete(filename);
	 
	sprintf(filename_buf, "complete:%d:%s",total_progress,filename);

	notify_progressd(filename_buf);

    return 0;
}


static int count_pic(char * filelist)
{
	FILE * filed;
	char tmp[256] = {0};
	int ret; 

	snprintf(tmp,256,"grep . -c %s",filelist);
	filed = popen(tmp,"r");
	if (filed != NULL) {
		fgets(tmp,sizeof tmp,filed);
		LOG_DEBUG("%s: gets %s\n",__FUNCTION__,tmp);
		ret = atoi(tmp);
		pclose(filed);
	}else {
		LOG_INFO("%s: popen %s failed\n",__FUNCTION__,tmp);
		return -1;
	}

	LOG_DEBUG("%s: %s -> count %d\n",__FUNCTION__,filelist,ret);

	return ret;
}
extern int kcard_print_dir_file_list(const char *root_dir,FILE * fd);

void reapchild(int signo) { 
	int pid;
	//while( wait(&child_stat) <= 0 ) 
	while ((pid=waitpid(-1, &child_stat,WNOHANG)) > 0) {
		mb();
		fork_count++;
		LOG_DEBUG("Reap child %d return %d , fork_count remain %d\n",pid,WEXITSTATUS(child_stat),fork_count);
		if (WEXITSTATUS(child_stat)) {
			total_fail++;
			progress_write_total_fail(total_fail);
		}else {
			total_cpt++;
			progress_write_total_complete(total_cpt);
		}
	}
} 

static int install_signal(void)
{
	struct sigaction act; 
	act.sa_handler = reapchild; 
	act.sa_flags = SA_NOCLDSTOP; 

	sigaction( SIGCHLD, &act, NULL); 
	return 0;
}
static int uninstall_signal(void)
{
	struct sigaction act; 
	act.sa_handler = SIG_DFL; 
	act.sa_flags = SA_NOCLDSTOP; 

	sigaction( SIGCHLD, &act, NULL); 
	return 0;
}
static int progress_write_total(void) 
{
	char tmp_buf[256] = {0};
	sprintf(tmp_buf,"%d\n",total_jpg);
	simple_write_file("/var/run/is/total",tmp_buf);
	return 0;
}
static int progress_write_email(void) 
{
	char tmp_buf[256] = {0};
	FILE * filed;

	filed = popen("source /etc/auth.inc; echo \"$email\"","r");
	if (filed != NULL) {
		fgets(tmp_buf,sizeof tmp_buf,filed);
		LOG_DEBUG("%s: gets %s\n",__FUNCTION__,tmp_buf);
		pclose(filed);
	}else {
		LOG_INFO("%s: popen failed\n",__FUNCTION__);
		return -1;
	}

	simple_write_file("/var/run/is/email",tmp_buf);
	return 0;
}




int upload_list(char * filelist)
{
	FILE * fd;
	char file[256];

	total_jpg = count_pic(filelist);
	if (total_jpg  == 0) {
		LOG_INFO("No files to upload\n");
	}

	fd = fopen(filelist,"r");
	if (fd == NULL) {
		LOG_INFO("%s: Can't open %s\n",__FUNCTION__,filelist);
		return -1;
	}

	total_progress = total_cpt;

	progress_write_total();

	install_signal();

	while ((fgets(file,256,fd))){
		int len = strlen(file);
		int pid;
		file[len-1] = '\0';


		if (file_is_exist_on_mirror(file)) 
			continue;

		total_progress ++;

		mb();
		while (fork_count <= 0) {
			LOG_DEBUG("Without fork count %d , Sleep 1 second\n",fork_count);
			sleep(1);
		}

		LOG_INFO("Progress:%3d/%d - Uploading %s [%d]\n",total_progress,total_jpg,file,fork_count);

		if ((pid = fork()) == 0) {
			char filename_buf[512] = {0};

			/* Child do */
			uninstall_signal();
			pid = getpid();
			LOG_DEBUG("Child Pid %d forked.\n",pid);

			sprintf(filename_buf,"/var/run/is/UP/%s",file);
			touch_file(filename_buf);
			simple_write_file(filename_buf,"0");
			if (upload_photo(file,_touch_file) < 0){
				printf("Upload %s failed\n",file);
				unlink(filename_buf);
				progress_write_fail(file);

				sprintf(filename_buf, "fail:%d:%s", total_progress, file);
				notify_progressd(filename_buf);

				exit(-1);
			}
			unlink(filename_buf);

			exit(0);
		} else {
			/* Parent do */
			if (pid < 0) {
				LOG_INFO("Fork failed pid %d\n",pid);
			}else { 
				LOG_DEBUG("[%d] Fork pid %d\n",getpid(),pid);
				mb();
				fork_count--;
			}
		}
	}
	while (total_jpg > (total_fail+total_cpt)) {
		LOG_DEBUG("Wait for complete %d/%d\n",(total_fail+total_cpt),total_jpg);
		sleep(1);
	}
	LOG_INFO("Upload complete %d/%d\n",(total_fail+total_cpt),total_jpg);
	uninstall_signal();

	sprintf(file, "finish:%d", total_progress);
	notify_progressd(file);

	/* Clear uploading folder */
	system("rm -rf /var/run/is/UP");

	return 0;
}
static void
usage(void) 
{
	printf("Usage: instant_upload [OPTIONS]\n");
	printf("Options list --- \n");
	printf("	-h                      -- Display usage.\n");
	printf("	-k FORK_COUNT           -- Max fork process count.\n");
	printf("	-j                      -- Do not record uploaded photo.\n");
	printf("	-e                      -- Login by user/password.\n");
	printf("	-g DONE_PIC             -- Use DONE_PIC as final pic to upload.\n");
	printf("	-t                      -- Generate complete list only\n");
	printf("	-d log_level\n");

}

static 
int upload_state_init(char * list) 
{
	FILE * fd;
	char file[256] = {0};

	progress_clean();

	fd = fopen(list,"r");
	if (fd == NULL) {
		printf("%s: Can't open %s\n", __FUNCTION__,list);
		return -1;
	}

	while ((fgets(file,256,fd))) {
		int len = strlen(file);
		file[len-1] = '\0';

		if (file_is_exist_on_mirror(file)) {
			total_cpt++;
			progress_write_complete(file);
		} 
	}

	progress_write_total_complete(total_cpt);
	fclose(fd);
	LOG_DEBUG("%s: total_cpt %d\n",__FUNCTION__,total_cpt);

	return 0;
}
int gen_upload_complete(char * filelist) 
{
	FILE * fd;
	char file[256];

	if (upload_state_init(filelist) < 0) 
		return -1;

	total_jpg = count_pic(filelist);
	if (total_jpg  == 0) {
		LOG_INFO("No files to upload\n");
	}

	fd = fopen(filelist,"r");
	if (fd == NULL) {
		LOG_INFO("%s: Can't open %s\n",__FUNCTION__,filelist);
		return -1;
	}

	total_progress = total_cpt;

	progress_write_total();

	return 0;
}

static int do_upload(void)
{
	/* prepare upload list in advance */
	if (!file_is_exist("/mnt/mtd/upload_list.txt")) {
		LOG_INFO("Can't find %s\n","/mnt/mtd/upload_list.txt");
		return -1;
	}

	/* generate configuration */
	if (setup_config() != 0) {
		fprintf(stderr,"Can't create album\n");
		return -1;
	}
	//progress_write_email();
    
	if (create_album() == 6) {
		fprintf(stderr,"Can't create album\n");
		return -1;
	}

	system("/usr/bin/refresh_sd");

	update_pic_flag = 0;
	/*
	if (filelist_gen("/mnt/sd/DCIM","/tmp/upload_list.txt",1) < 0) {
		fprintf(stderr,"Can't generate upload list\n");
		return -1;
	}
	*/
	upload_list("/mnt/mtd/upload_list.txt");
	if (upload_done_pic != NULL) {
		if (upload_photo(upload_done_pic ,0) < 0) {
			printf("Upload %s failed\n",upload_done_pic);
		}
	}

	system("/bin/sync");

	return 0;
}
static void signal_usr1(int sig)
{
	LOG_INFO("Refresh Upload List\n");
	update_pic_flag  = 1;
}
int instant_upload_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int instant_upload_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	char tmp_buf[128] = {0x0};
	int i;
	pid_t  pid = getpid();

	(void) signal(SIGUSR1, signal_usr1);

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
			case 'k':
				if ((i+1) >= argc){
					usage();
					return -1;
				}
				fork_count = atoi(argv[i+1]);
				i++;
				break;
			case 'a':
				if ((i+1) >= argc){
					usage();
					return -1;
				}
				snprintf(album,128,"%s",argv[i+1]);
				i++;
				break;
			case 'g':
				if ((i+1) >= argc){
					usage();
					return -1;
				}
				upload_done_pic = argv[i+1];
				i++;
				break;
			case 'j':
				_touch_file  = 0;
				break;
			case 'e':
				auth_mode = 1;
				break;
			case 't':
				gen_cpt_flag = 1;
				break;
			case 'h':
				usage();
				return 0;
			default:
				usage ();
				return -1;
			}
		}
		i++;
	}

	LOG_INFO("Log Level = %d\n",_log_level);
	LOG_INFO("Record Upload = %s\n",(_touch_file)?"Enable":"Disable");
	LOG_INFO("Max Fork Process Count = %d\n",fork_count);
	LOG_INFO("Album name = %s\n",album);
	LOG_INFO("Auth Mode = %s\n",auth_mode?"Login":"External");
	LOG_INFO("Upload done file = %s\n",(upload_done_pic)?upload_done_pic:"Disable");
	LOG_INFO("Gen complete list = %d\n",gen_cpt_flag);

	if (gen_cpt_flag) {
		return gen_upload_complete("/mnt/mtd/upload_list.txt");
	}
	/* Set PID file */
	LOG_INFO("set pid %d\n", (int)pid);
	sprintf(tmp_buf,"%d", (int)pid);
	simple_write_file(INSTANT_UPLOAD_PID_FILE, tmp_buf);

	printf("%s: udp_create_connection\n",__FUNCTION__);
	sprintf(tmp_buf,"%d", PROGRESSD_PORT);
	
	fd_progressd = udp_create_connection("127.0.0.1",tmp_buf);
	if (fd_progressd < 0){
		LOG_INFO("Create progressd connection failed.%d\n", fd_progressd);
	}
	LOG_INFO("Create progressd connection fd %d\n", fd_progressd);

	upload_state_init("/mnt/mtd/upload_list.txt");

	do_upload();

#if 0
	while (1) {
		//LOG_INFO("%s: waitting for sig\n",__FUNCTION__);
		//pause();
		sleep(1);
		if (update_pic_flag) {
			do_upload();
		}
	}
#endif

	unlink(INSTANT_UPLOAD_PID_FILE);
	return 0;
}
