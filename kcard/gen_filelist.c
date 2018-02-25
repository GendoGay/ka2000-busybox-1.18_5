#include "libbb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "common.h"
#ifdef unix
#include <sys/stat.h>
#else
#include <io.h>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <dir.h>
#endif
#endif
#include <time.h>

#ifdef unix
#define stricmp         strcasecmp
#define strnicmp        strncasecmp
#endif

#define COPYRIGHT  "<center><font size=6><b>Upload v2.6</b></font><br>&copy; 2000 <a href='http://www.kessels.com/'>Jeroen C. Kessels</a></center>\n<hr>"
#define NO   0
#define YES  1
#define MUST 2

/* Define DIRSEP, the character that will be used to separate directories. */
#ifdef unix
#define DIRSEP "/\\"
#else
#define DIRSEP "\\/"
#endif

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1


static int _log_level = LOG_LEVEL_INFO;

#define LOG_INFO(args...)	printf("GEN: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) printf("GEN: "args);}


extern void enable_kcard_call(void);
extern void disable_kcard_call(void);

extern int file_is_exist_on_mirror(char * path);
int filelist_gen(const char* directory,char * target_file, int striplen, int filter_uploaded)
{
	FILE * fd;
	char * temp_filename = "/tmp/iu_temp.list";
	FILE * temp_fd;
	char buf[256] = {0};

	LOG_DEBUG("%s\n",__FILE__);

	if (target_file == NULL)
		return -1;

	temp_fd = fopen(temp_filename,"w");
	if (temp_fd == NULL) {
		printf("%s: Can't open %s\n",__FUNCTION__,temp_fd);
		return -1;
	}
	kcard_print_dir_file_list(directory,temp_fd);
	fclose(temp_fd);
	LOG_DEBUG("%s End\n",__FILE__);


	if (filter_uploaded) {
		temp_fd = fopen(temp_filename,"r");
		if (temp_fd == NULL) {
			printf("%s: Can't open %s\n",__FUNCTION__,temp_filename);
			return -1;
		}
		fd = fopen(target_file,"w");
		if (fd == NULL) {
			printf("%s: Can't open %s\n",__FUNCTION__,target_file);
			return -1;
		}
		while ((fgets(buf, sizeof buf, temp_fd))){
			buf[strlen(buf) - 1] = '\0';
			LOG_DEBUG("%s: Check %s\n",__FUNCTION__,buf);
			if (file_is_exist_on_mirror(buf)) 
				continue;
			fprintf(fd,"%s\n",buf+striplen);
		}
		fclose(temp_fd);
		fclose(fd);

		/* Remove temp file */
		unlink(temp_filename);
	}else {
		sprintf(buf,"cp %s %s",temp_filename, target_file);
		system(buf);
	}
	return 0;
}
static void gen_usage(void)
{
	printf("%s: syntax gen_filelist -d [Directory] -o [Outputfile] -s StripLen -f\n",__FUNCTION__);
	printf("%s:      -f  -> filtered mirror\n",__FUNCTION__);
	printf("%s:      -s  -> strip characters from head of line\n",__FUNCTION__);
}
//------------------------------------------------------------------------------
int gen_filelist_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int gen_filelist_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	int ret = 0;
	int i=1;
	char * dir = NULL;
	char * output_file = NULL;
	int strip_len = 0;
	int flag_filter = 0;


	while (i < argc) {
		if (argv[i][0] == '-') {
			if ((i + 1) >= argc){
				gen_usage();
				return -1;
			}
			
			switch (argv[i][1]) {
			case 'd':
				i++;
				dir = argv[i];
				break;

			case 'o':
				i++;
				output_file = argv[i];
				break;

			case 's':
				i++;
				strip_len = atoi(argv[i]);
				break;

			case 'f':
				flag_filter = 1;
				break;
			default:
				printf("No matched option\n");
				gen_usage();
				return -1;
			}
		}
		i++;
	}

	if (dir == NULL) {
		printf("No directory\n");
		gen_usage();
		return -1;
	}

	if (output_file == NULL) {
		printf("No output file\n");
		gen_usage();
		return -1;
	}
	printf("Gen filelist from %s to %s striplen %d filter %d\n",dir, output_file, strip_len, flag_filter);
	ret = filelist_gen(dir, output_file, strip_len, flag_filter);

	return ret;
}

