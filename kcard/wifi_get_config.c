#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>		/* getopt_long() */
#include "libbb.h"
#include "common.h"
#define parameter_file "/config_value"

FILE *wsd_conf_fp = NULL;

//------------------------------------------------------------------------------
int Get_wificonf_Val(char find[], char line[], char value[])
{
    int len1 = strlen(find);
    int len2 = strlen(line);
    char newsub[100];
    int i, j, k;
    for(i = len1 + 3, j = 0; i < len2; i++, j++)
    {
        newsub[j] = line[i];
    }
    for(k = strlen(newsub) - 1; k > 0; k--)
    {
        if(newsub[k] == '\n' || newsub[k] == '\r')
        {
            newsub[k] = 0;
        }
    }
    strcpy(value, newsub);
    return 0;
}
//------------------------------------------------------------------------------
FILE *open_wsd_config()
{
    if (wsd_conf_fp == NULL)
        wsd_conf_fp = fopen("/tmp/wsd.conf", "r");

    if (wsd_conf_fp == NULL)
    {
        system("cp /mnt/mtd/config/wsd.conf /tmp/wsd.conf && sync");
        wsd_conf_fp = fopen("/tmp/wsd.conf", "r");    
    }
    
    if (wsd_conf_fp == NULL)
        printf("Can not read wsd.conf\n");

    return wsd_conf_fp;
}
//------------------------------------------------------------------------------
int check_next_config(int start_pos, const char find[], char value1[])
{
    char *token;
    char line[100];
    char value[100];
    int find_pos = -1;
    FILE *fp = open_wsd_config();

	if (fp == NULL)
        return -1;

    /* seek to pos */
    fseek(fp, start_pos, SEEK_SET);

    /* scan line one by one */
    while(!feof(fp))
    {
        /* get one line */
        if(fgets(line, 100, fp)==NULL)
          break;

        /* find if with token */
        token = strstr(line, find);

        if(token != NULL)
        {
            /* get file current position */
            find_pos = ftell(fp);

            Get_wificonf_Val(find, line, value);
            strcpy(value1, value);
            //printf("Finding:\"%s\": Got: \"%s\"\n", find, value1);
            break;
        }
    }

    return find_pos;
}
//------------------------------------------------------------------------------
int read_wifiConfigFile(char find[], char value1[])
{
    FILE *fp;
    int find_pos = -1;

	//system("cp /mnt/sd/wsd.conf /tmp/wsd.conf");
	//system("cp /mnt/mtd/wsd.conf /tmp/wsd.conf"); // move to /etc/init.d/rcS

    fp = open_wsd_config(); //fopen("/tmp/wsd.conf", "r");
    if(fp != NULL)
    {
        find_pos = check_next_config(0, find, value1);
        //fclose(fp);
    }
    else
    {
        printf("Can not read wsd.conf\n");
    }
    return find_pos;
}

//------------------------------------------------------------------------------
int wifi_get_config_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_get_config_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    char *get_parm=NULL;
    char temp_buffer[100];
    FILE *cmd_fp;
    int cmd = getopt(argc, argv, "c:"); // option -c <parameter name>
    if (cmd==-1)
	{
		printf("no option\n");
		return 0;
	}

	if (cmd == 'c')
	{
		// read config to temp buffer and print out parameter
		get_parm = optarg;
		read_wifiConfigFile(get_parm, temp_buffer);
		cmd_fp = fopen(parameter_file, "w");
		if (cmd_fp != NULL)
		{
			/* Use fwrite for safely store of special character */
			fwrite(temp_buffer,strlen(temp_buffer),1,cmd_fp);
			fclose(cmd_fp);
		}
	}

    return 0;
}
//------------------------------------------------------------------------------
