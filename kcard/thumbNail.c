#include <string.h> 
#include <stdio.h> 
#include "libbb.h" 
#include <stdlib.h>


static const unsigned char thumbnail_start[] = {0xFF, 0xD8};
static const unsigned char app1data_start[] = {0xFF, 0xE1};

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

static int _log_level = LOG_LEVEL_INFO;

#define LOG_INFO(args...)	fprintf(stderr,"THUMB: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) fprintf(stderr,"THUMB: "args);}


static char thumbNail_pic_name[300] = "";

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
 void setHeaderLength(int len)
{
	printf("Content-Length: %d\r\n",len);
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
int
show_nothumb(void)
{
	FILE* pic;
	char buf[512]={0};
	int ret; 
	
	pic = fopen("/www/nothumb.jpg", "rb");
	if (pic == NULL) {
	   LOG_INFO( "No default ThumbNail %s\n","/www/nothumb.jpg" );
	   return -1;
	}
	while ((ret = fread(buf,1,512,pic)) > 0) {
		fwrite(buf, 1, ret, stdout);
	}

	fclose(pic);
	return 0;
}

int 
picture_is_raw(char *filename) 
{
	int len = strlen(filename);

	if (len <= 4)
		return 0;

		/* Canon */
	if (strcasecmp(filename+(len-4), ".cr2") == 0) {
		return 1;
	}
		/* Nikon */
	if (strcasecmp(filename+(len-4), ".nef") == 0 ||
		strcasecmp(filename+(len-4), ".nrw") == 0	) {
		return 1;
	}
		/* Olympus */
	if (strcasecmp(filename+(len-4), ".orf") == 0) {
		return 1;
	}
		/* Pentax */
	if (strcasecmp(filename+(len-4), ".pef") == 0) {
		return 1;
	}
		/* Leica */
	if (strcasecmp(filename+(len-4), ".rwl") == 0) {
		return 1;
	}
		/* Samsung */
	if (strcasecmp(filename+(len-4), ".srw") == 0) {
		return 1;
	}
		/* Panasonic */
	if (strcasecmp(filename+(len-4), ".raw") == 0 ||
		strcasecmp(filename+(len-4), ".rw2") == 0	) {
		return 1;
	}
		/* Sony */
	if (strcasecmp(filename+(len-4), ".arw") == 0 || 
		strcasecmp(filename+(len-4), ".sr2") == 0 ||
		strcasecmp(filename+(len-4), ".srf") == 0) {
		return 1;
	}
		/* Kodak */
	if (strcasecmp(filename+(len-4), ".dcr") == 0 || 
		strcasecmp(filename+(len-4), ".k25") == 0 ||
		strcasecmp(filename+(len-4), ".kdc") == 0) {
		return 1;
	}

	LOG_DEBUG("%s is not raw file\n",filename);
	return 0;
}
static int parse_thumb_size(char * filename) 
{
	char tmp_buf[256] = {0};
	char cmd[256] = {0};
	FILE * fd;
	int size = -1;
	sprintf(cmd, "/usr/bin/decraw -i -v \"%s\"",thumbNail_pic_name);
	LOG_DEBUG("Execute %s  \n",cmd);
	fd = popen(cmd ,"r");
	if (fd == NULL) {
		LOG_INFO("PIC %s, popen failed %d\n",thumbNail_pic_name);
		return -1;
	}
	while (fgets(tmp_buf, 256, fd)) {
		LOG_DEBUG("read %s\n",tmp_buf);
		if (strncmp(tmp_buf, "Thumb output-length: ", strlen("Thumb output-length: ")) == 0) {
			size = atoi(tmp_buf + strlen("Thumb output-length: "));
			LOG_DEBUG("thumb length = %s %d\n", tmp_buf + strlen("Thumb output-length: "),size);
			break;
		}

	}
	pclose(fd);
	return size;
}

#define MAX_THUMB_SIZE  (64*1024) //64k
int thumbNail_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int thumbNail_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	char Destination[300];
	char thumb_buf[MAX_THUMB_SIZE] = {0};
	struct exif_desc * desc;
	int thumb_size=MAX_THUMB_SIZE;
	char* get_data = getenv("QUERY_STRING");
	FILE * fd;
	
	disable_kcard_call();
	if (get_data != NULL)
	{
		char* val = strstr(get_data, "fn=");
		char * token_val = NULL;
		char * next_token = strstr(get_data, "&");
		//fprintf(stderr, "val=%s, next_token %p\n", val,next_token);
		if (val != NULL)
		{
			if (next_token == NULL) {
				strcpy(Destination, val+3);
			} else  {
				strncpy(Destination, val+3,next_token - (val+3));
			}
			//fprintf(stderr, "Destination = %s\n",Destination);
		}
		else {
			fprintf(stderr,"Get false parameter!\n");
			return -1;
		}

		if (next_token) {
			token_val = strstr(next_token, "DEBUG");
			if (token_val){
				_log_level = LOG_LEVEL_DEBUG;
			}
		}

		LOG_DEBUG("val %s Destination = %s, token_val %s\n",val,Destination,(token_val)?token_val:"NULL");
	}

	//strcat(thumbNail_pic_name, Destination);
	decode_file_name(thumbNail_pic_name, Destination);
	
	LOG_DEBUG("PIC %s, Log level %d\n",thumbNail_pic_name,_log_level);
    //fprintf(log, "thumbNail_pic_name=%s\n", thumbNail_pic_name);

	fd = fopen(thumbNail_pic_name, "r");
	if (fd == NULL){
		enable_kcard_call();
		fprintf(stdout,"Status: 404 Not Found\r\n\r\n"); 
		LOG_INFO("PIC %s, open failed!\n",thumbNail_pic_name);

		return 0;
	}
	fclose(fd);

	if (picture_is_raw(thumbNail_pic_name)) {
		char cmd[256] = {0};
		int ret = 0;
		int thumb_real_size = 0 ;

		thumb_real_size = parse_thumb_size(thumbNail_pic_name);
		if (thumb_real_size < 0) {
			LOG_INFO("PIC %s, get thumbsize failed %d\n",thumbNail_pic_name);
			enable_kcard_call();
			show_nothumb();
			return 0;
		}
		/* Raw format use decraw tool to extract thumbnail. */
		setHeaderLength(thumb_real_size);
		setHeader("image/jpeg");	
		sprintf(cmd, "/usr/bin/decraw -c -e \"%s\"",thumbNail_pic_name);
		LOG_DEBUG("Execute %s  \n",cmd);
		fd = popen(cmd ,"rb");
		if (fd == NULL) {
			LOG_INFO("PIC %s, popen failed %d\n",thumbNail_pic_name);
			enable_kcard_call();
			show_nothumb();
			return 0;
		}
		while ((ret = fread(thumb_buf, 1, thumb_size, fd)) > 0) {
			LOG_DEBUG("freadd size %d\n",ret);
			fwrite(thumb_buf, 1, ret, stdout);
			if (ret < thumb_size) {
				pclose(fd);
				enable_kcard_call();
				return 0;
			}
		}
		pclose(fd);
		enable_kcard_call();
		return 0;
		
	} else {
		exif_set_log(_log_level) ;
		exif_set_scan_size(64*1024);

		setHeader("image/jpeg");	
		desc = exif_desc_alloc(thumbNail_pic_name);
		if (desc == NULL) {
			LOG_INFO("%s: %s exif descriptor alloc failed\n",__FUNCTION__,thumbNail_pic_name);
			enable_kcard_call();
			show_nothumb();
			return 0;
		}
		if (exif_get_thumbnail(desc,thumb_buf,&thumb_size) < 0) {
			LOG_INFO("%s: %s Can't get thumbnail %d\n",__FUNCTION__,thumbNail_pic_name,thumb_size);
			enable_kcard_call();
			show_nothumb();
			return 0;
		}
		enable_kcard_call();
		exif_desc_free(desc);
		fwrite(thumb_buf, 1, thumb_size, stdout);
	}
	
	return 0;
}



