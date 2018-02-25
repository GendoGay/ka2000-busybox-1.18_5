#include <stdlib.h>
#include <stdio.h>
#include "libbb.h"
#include <string.h>

#define DEBUG_MARKER 0
#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

static int _thumb_log_level = LOG_LEVEL_INFO;

#define LOG_INFO(args...)	fprintf(stderr,"THUMBV: " args)
#define LOG_DEBUG(args...)   {if (_thumb_log_level >= LOG_LEVEL_DEBUG) fprintf(stderr,"THUMBV: "args);}
#define LOG_DEBUG_IS_ENABLE() (_thumb_log_level >= LOG_LEVEL_DEBUG) 



#define MAX_THUMB_SIZE (256*1024) //Maximum thumbnail size
#define MAX_SEARCH_SIZE (3*1024*1024)
static char thumb_buffer[MAX_THUMB_SIZE] = {0};
static unsigned int current_thumb_index=0;
static int eoi_found = 0;
static int soi_found = 0;
typedef enum  			/* JPEG marker codes */
{
    M_SOF0  = 0xc0,
    M_SOF1  = 0xc1,
    M_SOF2  = 0xc2,
    M_SOF3  = 0xc3,

    M_SOF5  = 0xc5,
    M_SOF6  = 0xc6,
    M_SOF7  = 0xc7,

    M_JPG   = 0xc8,
    M_SOF9  = 0xc9,
    M_SOF10 = 0xca,
    M_SOF11 = 0xcb,

    M_SOF13 = 0xcd,
    M_SOF14 = 0xce,
    M_SOF15 = 0xcf,

    M_DHT   = 0xc4,

    M_DAC   = 0xcc,

    M_RST0  = 0xd0,
    M_RST1  = 0xd1,
    M_RST2  = 0xd2,
    M_RST3  = 0xd3,
    M_RST4  = 0xd4,
    M_RST5  = 0xd5,
    M_RST6  = 0xd6,
    M_RST7  = 0xd7,

    M_SOI   = 0xd8,
    M_EOI   = 0xd9,
    M_SOS   = 0xda,
    M_DQT   = 0xdb,
    M_DNL   = 0xdc,
    M_DRI   = 0xdd,
    M_DHP   = 0xde,
    M_EXP   = 0xdf,

    M_APP0  = 0xe0,
    M_APP1  = 0xe1,
    M_APP2  = 0xe2,
    M_APP3  = 0xe3,
    M_APP4  = 0xe4,
    M_APP5  = 0xe5,
    M_APP6  = 0xe6,
    M_APP7  = 0xe7,
    M_APP8  = 0xe8,
    M_APP9  = 0xe9,
    M_APP10 = 0xea,
    M_APP11 = 0xeb,
    M_APP12 = 0xec,
    M_APP13 = 0xed,
    M_APP14 = 0xee,
    M_APP15 = 0xef,

    M_JPG0  = 0xf0,
    M_JPG13 = 0xfd,
    M_COM   = 0xfe,

    M_TEM   = 0x01,

    M_ERROR = 0x100
} JPEG_MARKER;


unsigned char std_huffman_table[] =
{
0xff, M_DHT, 0x01, 0xA2,
0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x10,0x00,0x02,
0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7d,0x01,0x02,
0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,
0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,
0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,0x29,0x2a,
0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,
0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,
0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x92,
0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,
0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,0xe3,0xe4,
0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,
0x01,0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,
0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x11,0x00,0x02,
0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,0x77,0x00,0x01,
0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,
0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,
0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,
0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,
0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,
0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,
0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
};


int print_marker(unsigned char m, int i, int len)
{
#if DEBUG_MARKER

    switch (m) {
    case M_SOI:
      printf("%5d, %02x, Start of Image\n", i, m);
      return 1;

    case M_SOF0:		/* Baseline */
      printf("%5d, %02x, sof0\n", i, m);
      return 1;

    case M_SOF1:		/* Extended sequential, Huffman */
      printf("%5d, %02x, sof1\n", i, m);
      return 1;

    case M_SOF2:		/* Progressive, Huffman */
      printf("%5d, %02x, sof2\n", i, m);
      return 1;

    case M_SOF9:		/* Extended sequential, arithmetic */
      printf("%5d, %02x, sof9\n", i, m);
      return 1;

    case M_SOF10:		/* Progressive, arithmetic */
      printf("%5d, %02x, sof10\n", i, m);
      return 1;

    /* Currently unsupported SOFn types */
    case M_SOF3:		/* Lossless, Huffman */
    case M_SOF5:		/* Differential sequential, Huffman */
    case M_SOF6:		/* Differential progressive, Huffman */
    case M_SOF7:		/* Differential lossless, Huffman */
    case M_JPG:			/* Reserved for JPEG extensions */
    case M_SOF11:		/* Lossless, arithmetic */
    case M_SOF13:		/* Differential sequential, arithmetic */
    case M_SOF14:		/* Differential progressive, arithmetic */
    case M_SOF15:		/* Differential lossless, arithmetic */
      printf("%5d, %02x, SOFS\n", i, m);
      return 1;

    case M_SOS:
      printf("%5d, %02x, sofs\n", i, m);
      return 1;
    case M_EOI:
      printf("%5d, %02x, End of Image\n", i, m);      return 1;
    case M_DAC:
      printf("%5d, %02x, DAC\n", i, m);      return 1;

    case M_DHT:
      printf("%5d, %02x, DHT\n", i, m);

      return 1;

    case M_DQT:
      printf("%5d, %02x, DQT\n", i, m);     return 1;

    case M_DRI:
      printf("%5d, %02x, DRI\n", i, m);     return 1;

    case M_APP0:
    case M_APP1:
    case M_APP2:
    case M_APP3:
    case M_APP4:
    case M_APP5:
    case M_APP6:
    case M_APP7:
    case M_APP8:
    case M_APP9:
    case M_APP10:
    case M_APP11:
    case M_APP12:
    case M_APP13:
    case M_APP14:
    case M_APP15:
      printf("%5d, %02x, APP\n", i, m);   return 1;

    case M_COM:
      printf("%5d, %02x, COM");     return 1;

    case M_RST0:		/* these are all parameterless */
    case M_RST1:
    case M_RST2:
    case M_RST3:
    case M_RST4:
    case M_RST5:
    case M_RST6:
    case M_RST7:
    case M_TEM:
      printf("%5d, %02x, RST\n", i, m);    return 1;

    case M_DNL:			/* Ignore DNL ... perhaps the wrong thing */
      printf("%5d, %02x, DNL\n", i, m);    return 1;
    default:
      //printf("OTHERS");      break;
      break;
    }
#endif
    return 0;
}

#if DEBUG_MARKER
int save_thumb(unsigned char *buf, int size, int m_dht)
{
    static int i;
    FILE *dst;
    char name[64];

    /* save to different image for multi file testing */
    sprintf(name, "t%d.jpg", i);
    i++;

    dst = fopen(name, "wb");      //stdout

    fseek(dst, 0, SEEK_SET);

    if (m_dht == 1)
    {
        fwrite(buf, 1, size, dst);
    }
    else
    {
        /* no huffman table, patch it with standard huffman table */
        fwrite(buf, 1, 2, dst);
        fwrite(std_huffman_table, 1, sizeof(std_huffman_table), dst);
        fwrite(buf+2, 1, size - 2, dst);
    }

    fclose(dst);

    return 0;
}
#else

void setHeader1(const char* h)
{
	printf("Content-Type: %s\n\n",h);
}
int save_thumb(char *buf, int size, int m_dht __attribute__((unused)))
{
	LOG_DEBUG("save_thumb size %d\n",size);
	setHeader1("image/jpeg");
    fwrite(buf, 1, size, stdout);
    return 0;
}
#endif

void dump_buffer(unsigned char *buf, int len)
{
    int j;

    for (j = 0; j < len; j++)
    {
        printf("0x%02x,", buf[j]);
        if ((j % 8) == 7)
            printf("\n");
    }
    printf("\n");
}

int read_markers (unsigned char *buf, int size)
{
    int i, len;
    unsigned char m;
    int m_soi = 0;
    int m_eoi = 0;
    int mark[256] = {0};

    for (i = 0; i < size; i++)
    {
        if (buf[i] == 0xff)
        {
            m = buf[i+1];
            if (m == 0xff)
                continue;
            /* search for Start of image */
            if (m == M_SOI)
            {
                m_soi = i;
            }

            /* still wait for SOI */
            if (m_soi == 0)
                continue;

            mark[m] = i;
#if DEBUG_MARKER
            len = (buf[i+2] << 8) | buf[i+3];
            print_marker(m, i, len);

            if (m == M_DHT)
            {
                dump_buffer(buf + i, len + 2);
            }
#endif
            /* End of Image */
            if (m == M_EOI)
            {
                m_eoi = i;
                //printf("soi %d eoi %d, dqt %d\n", m_soi, m_eoi, mark[M_DQT]);
                save_thumb(buf + m_soi, m_eoi - m_soi + 2, mark[M_DHT]);
                return m_soi;
            }
        }
    }
//	printf("read_markers: not M_EOF found. M_SOI %d\n",m_soi);
}
int read_markers_new (unsigned char *buf, int size,FILE* src_fd)
{
    int i, len;
    unsigned char m;
    int m_soi = 0;
    int m_eoi = 0;
    int mark[256] = {0};
	unsigned int temp = 0;

	const unsigned char tiff_header_start_le[] = {0x49, 0x49, 0x2a,0x00};
	const unsigned char tiff_header_start_be[] = {0x4d, 0x4d, 0x00,0x2a};

    for (i = 0; i < size; i++) {
        if (buf[i] == 0xff) {
            m = buf[i+1];

            if (m == 0xff)
                continue;

            /* search for Start of image */
            if (m == M_SOI) {
				if (i+3 < size) {
					if (buf[i+2] == 0xff && (buf[i+3] == M_DQT || buf[i+3] == M_APP1)) {
						if (soi_found) {
							LOG_DEBUG("read_markers_new: Find double SOI\n");
							current_thumb_index = 0;

							//return -1;
						}
						m_soi = i;
						soi_found = 1;
						LOG_DEBUG("%s: Find SOI %u ,buf[%u]=%02x ,buf[%u]=%02x\n",__FUNCTION__,m_soi,i+2,buf[i+2],i+3,buf[i+3]);

						if (buf[i+3] == M_APP1) {
							int tiff_oft;
							int ifd1_oft; 
							int app1_size;
							int addr=0;
							int thumb_size=0;
							char * new_buf = NULL;

							tiff_oft = ImageFileSearchTiffHeader(buf,1024);
							if (tiff_oft <=0 ){
								LOG_DEBUG("%s: Can't find thumbnail length\n",__FUNCTION__);
								return -1;
							}

							app1_size = app1_data_size(buf);
							LOG_DEBUG("%s: APP1 Length = %d\n",app1_size);

							fseek(src_fd, 0, SEEK_SET);

							new_buf = (unsigned char*)calloc(1, app1_size);
							fread(new_buf, 1, app1_size, src_fd);

							ifd1_oft = ImageFileSearchFirstIFD(new_buf,app1_size,tiff_oft);
							if (ifd1_oft <= 0){
								LOG_DEBUG("%s: Can't find Fist IFD\n",__FUNCTION__);
								return -1;
							}
							ifd1_oft = ifd1_oft + tiff_oft;
							addr = ImageFileSearchthumbnailOffset(new_buf,app1_size,ifd1_oft);
							if (addr <= 0){
								LOG_DEBUG("%s: Can't find thumbnail offset\n",__FUNCTION__);
								return -1;
							}

							addr = addr + tiff_oft;
							thumb_size =ImageFileSearchthumbnailLength(new_buf,app1_size,ifd1_oft);
							if (thumb_size <= 0){
								LOG_DEBUG("%s: Can't find thumbnail length\n",__FUNCTION__);
								return -1;
							}

							save_thumb(new_buf+addr,thumb_size, mark[M_DHT]);
							free(new_buf);
							eoi_found = 1;
							return 1;
						}

					}else {
						LOG_DEBUG("%s: Find Vague SOI index %d ,Clear SOI buf[%u]=%02x-%02x-%02x-%02x\n",__FUNCTION__,i,i+2,buf[i+2],buf[i+3],buf[i+4],buf[i+5]);
						soi_found = 0;
						m_soi = 0;
					}
				}
			}

            /* still wait for SOI */
            if (m_soi == 0 && soi_found == 0)
                continue;

            mark[m] = i;
#if DEBUG_MARKER
            len = (buf[i+2] << 8) | buf[i+3];
            print_marker(m, i, len);

            if (m == M_DHT)
            {
                dump_buffer(buf + i, len + 2);
            }
#endif
            /* End of Image */
            if (m == M_EOI)
            {
				eoi_found = 1;
                m_eoi = i;

				temp += (m_eoi - m_soi + 2);
				if (temp > MAX_THUMB_SIZE) {
					LOG_INFO("%s: Copy to Buffer: data length %d too long! MAX %d\n",__FUNCTION__,temp,MAX_THUMB_SIZE);
					return -1;
				}
				LOG_INFO("%s: EOI Found,Copy to Buffer: Addr %p length %d\n",__FUNCTION__,buf+ m_soi,m_eoi - m_soi + 2);
				memcpy(thumb_buffer+current_thumb_index, buf+ m_soi, m_eoi - m_soi + 2);
				current_thumb_index +=  m_eoi - m_soi + 2;

                //printf("soi %d eoi %d, dqt %d\n", m_soi, m_eoi, mark[M_DQT]);
                //save_thumb(buf + m_soi, m_eoi - m_soi + 2, mark[M_DHT]);
                save_thumb(thumb_buffer, current_thumb_index, mark[M_DHT]);
                return m_eoi;
            }
        }
    }
	if (soi_found == 1) {
		m_eoi = size - 1;
		//save_thumb(buf + m_soi, m_eoi - m_soi + 2, mark[M_DHT]);
		temp = current_thumb_index;
		temp += (m_eoi - m_soi + 2);
		if (temp > MAX_THUMB_SIZE) {
			LOG_INFO("%s: Copy to Buffer: data length %d too long! MAX %d\n",__FUNCTION__,temp,MAX_THUMB_SIZE);
			return -1;
		}

		LOG_DEBUG("%s: Copy partial to Buffer: addr %p length %d\n",__FUNCTION__,buf+ m_soi,m_eoi - m_soi + 2);
		memcpy(thumb_buffer+current_thumb_index, buf+ m_soi, m_eoi - m_soi + 2);
		current_thumb_index +=  m_eoi - m_soi + 2;
	} 
	return m_soi;
//	printf("read_markers: not M_EOF found. M_SOI %d\n",m_soi);
}

int find_jpeg_thumb_file(char *imageName)
{
    int n = strlen(imageName);
    char new_name[1024];
    FILE *src;

    strcpy(new_name, imageName);
    new_name[n-3] = 'J';
    new_name[n-2] = 'P';
    new_name[n-1] = 'G';
    //printf("new_name %s\n", new_name);

    src = fopen(new_name, "rb");
    if (!src)
    {
        //printf("Cannot find selected file\n");
        return -1;
    }
    fseek(src, 0, SEEK_END);

    int file_size = ftell(src);

    /* seek to file start point */
    fseek(src, 0, SEEK_SET);

    /* allocate buffer for read and parsing */
    unsigned char *buffer_source = (unsigned char*)calloc(1, file_size);

    /* read and fill buffer */
    fread(buffer_source, 1, file_size, src);

    /* trace marker and extract jpeg */
    save_thumb(buffer_source, file_size, 1);

    /* at the end close the source file */
    fclose(src);
    return 0;

}

int extract_jpeg_frame(char *imageName)
{
    int search_size = 256 * 1024;
    unsigned char* buffer_source = NULL;
    FILE* src;
	int ret = 0;
	int final = 0;
	unsigned int total_size =0;
	unsigned int max_size = MAX_SEARCH_SIZE;
	struct exif_desc * desc;
	int thumb_size  = MAX_THUMB_SIZE;


    if (find_jpeg_thumb_file(imageName) == 0) {
		LOG_INFO("thumbnail_video: %s Find off-the-shelf thumbnail\n",imageName);
        return 0;
	}

    src = fopen(imageName, "rb");
    if (!src) {
        LOG_INFO("%s: Cannot find selected file\n",imageName);
        return -1;
    }

	exif_set_log(_thumb_log_level) ;
	exif_set_scan_size(max_size);
	exif_set_scan_block_size(MAX_THUMB_SIZE);

	desc = exif_desc_alloc(imageName);
	if (desc == NULL) {
		LOG_INFO("%s: %s exif descriptor alloc failed\n",__FUNCTION__,imageName);
		enable_kcard_call();
		setHeader1("image/jpeg");
		show_nothumb();
		return 0;
	}
	if (exif_get_jpeg(desc,thumb_buffer,&thumb_size) < 0){
		LOG_INFO("%s: %s Can't get thumbnail %d\n",__FUNCTION__,imageName,thumb_size);
		enable_kcard_call();
		setHeader1("image/jpeg");
		show_nothumb();
		return 0;
	}

	exif_desc_free(desc);
	save_thumb(thumb_buffer, thumb_size, NULL); 

	return 0;
}



extern void decode_file_name(char *dst, char *src);

int thumbnail_video_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int thumbnail_video_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    char imageName[300] = "P1070802.MOV";
    char Destination[300];
    char* get_data = getenv("QUERY_STRING");

    disable_kcard_call();
	//FILE *log = fopen("log.txt", "at");
	//fprintf(log, "\n------\nget_data=%s\n", get_data);
    if (get_data != NULL) {
		char * val = NULL;
		char * token_val = NULL;
		char * next_token = NULL;

        val = strstr(get_data, "fn=");
		next_token = strstr(get_data, "&");
        if (val != NULL) {
			if (next_token == NULL) {
				strcpy(imageName, val+3);
			} else {
				strncpy(imageName, val+3,next_token - (val+3));
			}
        } else {
            LOG_INFO("Can't find filename parameter!\n");
			return -1;
        }
#if 1
		if (next_token) {
			token_val = strstr(next_token,"DEBUG");
			if (token_val != NULL) {
				_thumb_log_level = LOG_LEVEL_DEBUG;
			}
		}
#endif

    } else {
        LOG_INFO("No parameter!\n");
    }
	
    //fprintf(log, "\n------\imageName=%s\n", imageName);
    decode_file_name(Destination, imageName);

    LOG_DEBUG("imageName %s, Log = %d\n",Destination,_thumb_log_level);

    extract_jpeg_frame(Destination);

    //fclose(log);
    enable_kcard_call();

    return 0;
}



