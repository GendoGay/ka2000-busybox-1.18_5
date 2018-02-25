
#include "libbb.h"
//#include "libcoreutils/coreutils.h"

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

static int _log_level = LOG_LEVEL_INFO;

#define LOG_ERR(args...)	printf("INUP_CLEAN: " args)
#define LOG_INFO(args...)	printf("INUP_CLEAN: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) printf("INUP_CLEAN: "args);}


#define UPLOAD_MIRROR_BASE "/mnt/mtd/instant_uploaded"
static int file_is_exist(char * path)
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

static int file_is_exist_on_sd(char * path)
{
	return file_is_exist(path+strlen(UPLOAD_MIRROR_BASE));
}
static int delete_file(char * filename)
{
	char tmp[256] = {0};
	char tmp_dir[256] = {0};
	char * dirname_p = NULL;
	int fd;
	
	LOG_DEBUG("%s filename %s\n",__FUNCTION__,filename);

	if (filename == NULL) {
		LOG_ERR("%s: filename is empty\n",__FUNCTION__);
		return -1;
	}

	if (strlen(filename) >= (256 - 25)) {
		LOG_ERR("%s: filename %s too long\n",__FUNCTION__,filename);
		return -2;
	}

	if (file_is_exist(filename) == 1) {
		if (unlink(filename) < 0) {
			LOG_ERR("Delete Failed. %s\n",filename);
		} else {
			LOG_DEBUG("Delete %s\n",filename);
		}

		return 0;
	}
	return -1;
}

int insup_delete_list(const char *root_dir)
{
    DIR             *dip=NULL;
    struct dirent   *dit;
    char tmp[256]= {0};
    int   len = 0;
	int file_count = 0;

    dip = opendir(root_dir);
    if (dip  == NULL) {
        LOG_DEBUG("%s : opendir %s failed\n",__FUNCTION__,root_dir);
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
				if (file_is_exist_on_sd(tmp) == 0) {
					delete_file(tmp);
				} else {
					file_count++;
				}
			}

			/* Recursive List */
			if (dit->d_type == DT_DIR) {
				int ret;
				sprintf(tmp,"%s/%s", root_dir, dit->d_name);

				ret = insup_delete_list(tmp);
				if (ret == 0) {
					if (rmdir(tmp) < 0) {
						printf("%s: Rmdir %s failed\n",__FUNCTION__,tmp);
					}
				}else {
					file_count++;
				}
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
static void
usage(void) 
{
	printf("Usage: instant_upload_clean [OPTIONS]\n");
	printf("Options list --- \n");
	printf("	-h                      -- Display usage.\n");
	printf("	-d log_level\n");
}
int instant_upload_clean_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int instant_upload_clean_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	int ret;
	int i;

#define _DEBUG_ARGV
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

	insup_delete_list(UPLOAD_MIRROR_BASE);

    return 0;
}
