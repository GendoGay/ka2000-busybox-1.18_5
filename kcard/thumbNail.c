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
#include <string.h>
#include <stdio.h>
#include "libbb.h"
#include <stdlib.h>


static const unsigned char thumbnail_start[] = {0xFF, 0xD8};
static const unsigned char app1data_start[] = {0xFF, 0xE1};



int ImageFileSearchthumbnail(register unsigned char *stream ,int file_size)
{
int start_offset = 0; 
int i;
	for(i=2; i < file_size; i++)
	{
		if(memcmp(stream+i, thumbnail_start, sizeof(thumbnail_start)) == 0)
		{
			start_offset = i;			
			break;
		}
	}
   //printf("thumbnail atart addr =%d",start_offset);
   return start_offset;
}

int app1_data_size(register unsigned char *stream )
{
int start_offset = 0; 
int i;
int src_len = 0; 
int apptal_size = 0;
	for(i=0; i < 1024; i++)
	{
		if(memcmp(stream+i, app1data_start, sizeof(app1data_start)) == 0)
		{
			start_offset = i;

			src_len |= stream[start_offset+2] << 8;
			src_len |= stream[start_offset+3];
			apptal_size = start_offset + src_len;
			
			break;
		}
	}
	//printf("total size=%d",total_size);
	return apptal_size;
}

unsigned short 	ImageFileGetWord ( register unsigned char *stream )
{

	register unsigned short w;//good for running
	register unsigned char *b = stream;
	w = (b[0] << 8) | b[1];
	//printf("file title= %x\n", w );
	return w & 0xffff;
}
 void setHeader(char* h)
{
	printf("Content-Type: %s\n\n",h);
}   

int hex_to_int(char c)
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
void decode_file_name(char *dst, char *src)
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

int thumbNail_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int thumbNail_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	FILE* pic_src;
	char Destination[300];
	char picName[300] = "";
	
	FILE *fp = NULL; 
	
	int addr=0;
	int app1_size;
	unsigned char* bufdata = NULL;
	unsigned char* buf = NULL;
    //FILE *log;
	 
	char* get_data = getenv("QUERY_STRING");
	
	//log = fopen("log.txt", "at");
	//fprintf(log, "\n------\nget_data=%s\n", get_data);
	
	disable_kcard_call();
	if (get_data != NULL)
	{
		char* val = strstr(get_data, "fn=");
		//fprintf(log, "val=%s\n", val);
		if (val != NULL)
		{
			strcpy(Destination, val+3);
			//fprintf(log, "Destination=%s\n", Destination);
		}
		else
			printf("Get false parameter!\n");
	}

	//strcat(picName, Destination);
	decode_file_name(picName, Destination);
	
    //fprintf(log, "picName=%s\n", picName);
   
	pic_src = fopen(picName, "rb");
    //fclose(log);
	if(pic_src)
	{

		setHeader("image/jpeg");	
		fseek(pic_src, 0, SEEK_SET);
		bufdata = (unsigned char*)calloc(1, 1024);
		fread(bufdata, 1, 1024, pic_src);
		if(ImageFileGetWord(bufdata)!=0xFFD8)
	  	{
        		printf("not jpeg file");
        		free(bufdata);
        		return -1;
    	}	
		app1_size=app1_data_size(bufdata);
		free(bufdata);
		fseek(pic_src, 0, SEEK_SET);
		buf = (unsigned char*)calloc(1, app1_size);
		fread(buf, 1, app1_size, pic_src);
		addr=ImageFileSearchthumbnail(buf,app1_size);
		fwrite(buf+addr, 1, app1_size-addr+3, stdout);
		free(buf);
		
         	enable_kcard_call();
	return 0;
	}
	else
      {
		printf("It cannot find file\n");
		return -1;
	}

	
}



