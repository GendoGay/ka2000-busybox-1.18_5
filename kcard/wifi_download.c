/*
 * KeyASIC KA2000 series software
 *
 * Copyright (C) 2013 KeyASIC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

/* busybox common include file */
#include "libbb.h"

/* kcard wsd common include file */
#include "common.h"

int wifi_download_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int download_file_size(char* file_name)
{
    struct stat buf;
    int i = stat ( file_name, &buf );

    printf("file_name %s\n",file_name);
    if (i !=0)
        printf("state error");

    return buf.st_size;
}
void aborted_download_handler(void)
{
    enable_kcard_call();
}

/*static void log(const char *str)
{
    FILE *fp;
    fp = fopen("/mnt/sd/log.txt", "w");
    if (fp) {
              fprintf(fp, "%s", str); 
              fclose(fp);
    }
}*/

static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;    
    
    return 0;    
}
// %20 -> 0x32 space char ...
static void decode_file_name(char *dst, char *src)
{
    int i, j;
    char c, c1, c2;
    if (dst == NULL || src == NULL)
        return;
    for (i = 0, j = 0; i < strlen(src); i++, j++)
    {
         c = src[i];
         if (c == '%')
         {
             c1 = src[i+1];
             c2 = src[i+2];
             i += 2;
             dst[j] = (char)((hex_to_int(c1) * 16) + hex_to_int(c2));
         }
         else
         {
            dst[j] = c;
         }              
    }
}

int wifi_download_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    char fullName[1024] = {0};
    char filename[256] = {0};
	char desti[1024] = {0};
    char* get = getenv("QUERY_STRING");
    int i = 0,j=0;
    int dir=0;
    
    disable_kcard_call();
    (void) signal(SIGPIPE, aborted_download_handler);
    memset(fullName, 0, 1024);
    memset(filename, 0, 256);

    if(get != NULL)
    {
        for(i=3; i<strlen(get); i++)
        {
            if(get[i]=='&')
            {
                j=0;
                dir=1;
                i+=4;
            }
            if(dir==0)
            {
                filename[j]=get[i];
            }
            else
            {
                fullName[j]=get[i];
            }
            j++;
        }
        strcat(fullName, "/");
        strcat(fullName, filename);
	    strcpy(desti, fullName);
        memset(fullName, 0 , 1024);
        decode_file_name(fullName, desti);
    }
    else
    {
        printf("No parameter!\n");
    }


    int fd;
    unsigned char *buf;
    int read_count;
    int len = 0;
    ssize_t full_write_num;
    int stdout = STDOUT_FILENO;
    int file_size = download_file_size(fullName);
    int offset = 0;
#define RDBUF_SIZE 8192
    buf = malloc(RDBUF_SIZE);
    memset(buf, 0, RDBUF_SIZE);

    /*send header at first */
    strcpy(buf,"Content-Type: application/force-download\n");
    sprintf(buf+strlen(buf), "Content-Length: %d\n", file_size);
    strcat(buf,"Content-Transfer-Encoding: binary\n");
    strcat(buf, "Accept-Ranges: bytes\n");
    strcat(buf, "Keep-Alive: 120\n");
    strcat(buf, "Pragma: no-cache\n");
    strcat(buf, "Cache-Control: no-cache, must-revalidate\n");
    sprintf(buf+strlen(buf), "Content-Disposition: attachment; filename=\"%s\"\n", filename);
    sprintf(buf+strlen(buf), "\n");
    full_write_num = full_write(stdout, buf, strlen(buf));

    fd = open(fullName, O_RDONLY);
    readahead(fd, offset, RDBUF_SIZE);

    while (file_size > 0)
    {
        read_count = read(fd, buf, RDBUF_SIZE);
        if (read_count > 0)
        {
            file_size -= read_count;
            readahead(fd, offset, RDBUF_SIZE);
            offset += RDBUF_SIZE;
            full_write_num = full_write(stdout, buf, read_count);
            if (read_count != full_write_num)
                break;
        }
    }


    free(buf);
    close(fd);
    enable_kcard_call();
    return 0;
}
