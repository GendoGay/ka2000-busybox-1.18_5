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

struct file_info
{
char *name;
unsigned long size;
char date[30];
};


int dirwalk(char *dir, void (*fcn)(char *))
{
	char name[256];
	struct dirent *dp;
	DIR *dfd;
	
if ((dfd = opendir(dir)) == NULL) 
{
		//fprintf(stderr, "dirwalk: can't open %s\n", dir);  
		return -1;
	}

while ((dp = readdir(dfd)) != NULL)
	{
	if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
		continue; 
		
	if (strlen(dir)+strlen(dp->d_name)+2 > sizeof(name))
		{}//fprintf(stderr, "dirwalk: name %s / %s too long\n",dir, dp->d_name);

		else
		{
		  sprintf(name, "%s/%s", dir, dp->d_name);
	         (*fcn)(name);
		}
	}
closedir(dfd);
	return 0;
}

int fdata(struct file_info *file_i)
{
  struct stat stbuf;
  struct tm *accesstime;
  char newdate[30];
  
  if (stat(file_i->name, &stbuf) == -1) {
	 // fprintf(stderr, "fsize: can't access %s\n\n", file_i->name);
      return -1;
  }

 
 // printf("%8ld %s", stbuf.st_size, file_i->name);
  
 accesstime=gmtime(&(stbuf.st_mtime));

 file_i->size=stbuf.st_size;

 sprintf(file_i->date,"%s",asctime(accesstime));

 file_i->date[24]='\0';

 if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
      dirwalk(file_i->name, fdata);

 return 0;
}

time_t get_filetime(struct file_info *file_i)
{
    struct stat stbuf;

    if (stat(file_i->name, &stbuf) == -1)
    {
        // fprintf(stderr, "fsize: can't access %s\n\n", file_i->name);
        return 0;
	}

    return stbuf.st_mtime;
}

//http://192.168.56.101/cgi-bin/wifi_filelist?fn=/mnt/sd/DCIM/&tm=1335317802
static unsigned long newest_time = 0;
int get_new_filelist(char *root_dir, unsigned long get_time)
{
    DIR             *dip=NULL;
    struct dirent   *dit;
    char file_path[256];
    struct tm *ptm;
    struct file_info file;
	unsigned long utime = 0;                
	printf("<file open_dir=\"%s\"/>\n", root_dir);
    dip = opendir(root_dir);

    if (dip  == NULL)
    {
        perror("No DCIM Directory\n");
        return 0;
    }

    dit = readdir(dip);
    while (dit != NULL)
    {
            if(dit->d_name != NULL && strcmp(dit->d_name,".")!=0 && strcmp(dit->d_name,"..")!=0)
            {
            sprintf(file_path, "%s/%s",root_dir, dit->d_name);
            file.name=file_path ;
            utime = get_filetime(&file);
			if (utime > newest_time)
				newest_time = utime;
			if (utime > get_time)       
				printf("<file name=\"%s\" size=\"%d\" time=\"%d\" type=\"%d\"/>\n", dit->d_name,file.size,utime,dit->d_type == DT_DIR);
            }
         dit = readdir(dip);
    }

    if (closedir(dip) == -1)
    {
        perror("closedir");
        return 0;
    }
	return 1;
}

int filelist(char *root_dir)
{
    DIR             *dip=NULL;
    struct dirent   *dit;
    char file_path[256];
    struct tm *ptm;
    struct file_info file;
	
    dip = opendir(root_dir);
  
    if (dip  == NULL)
{
        perror("No DCIM Directory\n");
        return 0;
    }

    dit = readdir(dip);
    while (dit != NULL)
    {
            if(dit->d_name != NULL && strcmp(dit->d_name,".")!=0 && strcmp(dit->d_name,"..")!=0)
        {
                sprintf(file_path, "%s/%s",root_dir, dit->d_name); 
		  file.name=file_path ;
		  fdata(&file);
                printf("<file name=\"%s\" size=\"%d\" date=\"%s\" type=\"%d\"/>\n", dit->d_name,file.size,&file.date[4],dit->d_type == DT_DIR);
            }
         dit = readdir(dip);
        }

    if (closedir(dip) == -1)
        {
        perror("closedir");
        return 0;
        }
	return 1;
    }

//%E6%96%B0%E5%A2%9E%E8%B3%87%E6%96%99%E5%A4%BE  -> 新增資料夾
//-----------------------------------------------------------------------------------------------
char *get_token_value(char *str, char *token)
{
	int i, j, k, len1, len2;
	int start = -1, end = 0;
    static char value[64];

	len1 = strlen(str);
	len2 = strlen(token);

	memset(value, 0, 64);
	for (i = 0; i <= len1; i++)
	{
	    if (strncmp(str + i, token, len2) == 0)
	    {
	        start = i;
	        for (j = i + len2, k = 0; j <= len1; j++, k++)
	        {
	            if (str[j] == 0 || str[j] == '&')
    {
	                value[k] = 0;
	                end = j - 1;
	                return value;
		}

	            value[k] = str[j];
    }
}
}
	return NULL;
}

  
extern void decode_file_name(char *dst, char *src);
int wifi_filelist_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_filelist_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    char dest[300]={0};

    char* get = getenv("QUERY_STRING");

    disable_kcard_call();

    printf("Content-type: text/xml\n\n");
    printf("<filelist>\n");
    if (get != NULL)
    {
        char* value = get_token_value(get, "fn=");
        if (value != NULL)
        {
            decode_file_name(dest, value);
        }
        else
        {
            printf("False parameter!\n");
        }
    }
    else
    {
        printf("No parameter!\n");///mnt/sd/DCIM/100_check
    }

	char *v = get_token_value(get, "tm=");
	unsigned long tm = 0xffffffff;
	if (v != NULL)
		sscanf(v, "%d", &tm);
    //printf("tm %s\n", v);	
	if (tm != 0xffffffff)
	{
		get_new_filelist(dest, tm);
		printf("<latest time=\"%d\"></latest>\n", newest_time);
}
	else
{
		filelist(dest);
	}
	
	printf("</filelist>\n");
     
	enable_kcard_call();	
    return(0);
}


/*
Ideeen:

- Multiple filemasks.
- Maximum directory size.
- Maximum and Minimum file size.
- Result-macros as parameters to OkUrl.

*/
