#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef BB_VER
/* For Busybox */
#include "libbb.h"
#else
/* For Boa */
#include "boa.h"
#include "apform.h"

#endif




//#define _HOST_BIG_ENDIAN_ 1

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1
#define LOG_INFO(args...)	fprintf(stderr,"THUMB: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) fprintf(stderr,"THUMB: "args);}

#define MARKER_SIZE			2
#define MARKER_LEN_SIZE		2

static int _log_level = LOG_LEVEL_INFO;
static int scan_size = 2048;
static int scan_block_size = 1024;

static const unsigned char EXIF_SOI[] = {0xFF, 0xD8};
static const unsigned char EXIF_EOI[] = {0xFF, 0xD9};
static const unsigned char EXIF_APP1[] = {0xFF, 0xE1};
static const unsigned char EXIF_APP0[] = {0xFF, 0xE0};
static const unsigned char EXIF_DQT[] = {0xFF, 0xDB};
static const unsigned char EXIF_SOS[]	= {0xFF, 0xDA};
static const unsigned char EXIF_SOF0[]	= {0xFF, 0xC0};
static const unsigned char EXIF_SOF2[]	= {0xFF, 0xC2};
static const unsigned char EXIF_DHT[]	= {0xFF, 0xC4};
static const unsigned char EXIF_RST[]	= {0xFF, 0xD0};
static const unsigned char EXIF_RST_MASK[]	= {0xFF, 0xF8};
static const unsigned char EXIF_COM[]	= {0xFF, 0xFE};
static const unsigned char EXIF_DRI[]	= {0xFF, 0xDD};
static const unsigned char EXIF_APP[]	= {0xFF, 0xE0};
static const unsigned char EXIF_APP_MASK[]	= {0xFF, 0xF0};

static const char ident_jfif[5] ={ 0x4A, 0x46, 0x49, 0x46, 0x00 };
static const char ident_jfxx[5] ={ 0x4A, 0x46, 0x58, 0x58, 0x00 };
static const char ident_exif[6] ={ 0x45, 0x78, 0x69, 0x66, 0x00, 0x00 };
static const char ident_xmp[] = "http://ns.adobe.com/xap/1.0/";

/* Tag set by little-endian */
static const char TAG_IMAGE_EXIF_POINTER[] = { 0x69, 0x87};
static const char TAG_IMAGE_WIDTH[]		= { 0x02, 0xA0};
static const char TAG_IMAGE_HEIGHT[]	= {	0x03, 0xA0};
static const char TAG_IMAGE_XRES[]	    = { 0x01, 0x1A};
static const char TAG_IMAGE_YRES[]		= { 0x01, 0x1B};
static const char TAG_IMAGE_EXP_TIME[]	= { 0x9A, 0x82};
static const char TAG_IMAGE_FNUMBER[]	= {	0x9D, 0x82};
static const char TAG_IMAGE_FOCAL_LEN[]	= {	0x0A, 0x92};
static const char TAG_IMAGE_EXP_BIAS[]		= {	0x04, 0x92};
static const char TAG_IMAGE_ISO[]		= {	0x27, 0x88};
static const char TAG_IMAGE_EXIF_VER[]   = { 0x00, 0x90};
static const char TAG_IMAGE_DATETIME_ORIGINAL[] = { 0x03, 0x90 };
static const char TAG_IMAGE_ORIENTATION[] = { 0x12, 0x01 };

/* IFD Structure */
struct ifd_element {
	uint16_t tag;
	uint16_t type;
	uint32_t count;
	uint32_t val;
}__attribute__((packed));

/* Exif descriptor */
struct exif_desc {
	char	filename[512];
	FILE *	file_fd;
	int		file_size;
	char *	tmp_buf;
	int		tmp_buf_len;
	int		tmp_buf_file_offset;
	uint8_t camera_is_little_endian;
	int		soi_file_offset;
	int		eoi_file_offset;
	int		app0_size;
	int		app0_extension;
	int		app0_file_offset;
	int		app1_size;
	int		app1_file_offset;
	int		app1_xmp_size;
	int		app1_xmp_file_offset;
	int		dqt_size;
	int		dqt_file_offset;
	int		dht_size;
	int		dht_file_offset;
	int		sos_size;
	int		sos_file_offset;
	int		tiff_file_offset;
	int		thumb_oft;
	int		thumb_len;
	uint32_t ifd1_oft;
	uint32_t ifd1_entry_count;
	uint32_t ifd0_oft;
	uint32_t ifd0_entry_count;
	uint32_t ifd_exif_oft;
	uint32_t ifd_exif_entry_count;

};

static int get_marker_length(struct exif_desc * desc,int offset, int * ifd_num);
static int comp_file_byte(struct exif_desc * desc,char *comp_str,int file_offset,int length );
static int get_file_byte(struct exif_desc * desc,char * ret_buf,int file_offset,int length);
int get_ifd0_attr(struct exif_desc *desc, uint16_t tag, struct ifd_element * ret_ifd)  ;
#ifdef _HOST_BIG_ENDIAN_
static uint16_t 
le16toh(uint16_t big) {
	char * buf = (char *)&big;
	char tmp;
	tmp = buf[0];
	buf[0] = buf[1];
	buf[1] = tmp;
	return big;

}
static uint16_t 
be16toh(uint16_t big) {
	return big;
}

static uint32_t 
le32toh(uint32_t big) {
	char * buf = (char *)&big;
	char tmp;
	tmp = buf[0];
	buf[0] = buf[3];
	buf[3] = tmp;

	tmp = buf[1];
	buf[1] = buf[2];
	buf[2] = tmp;
	return big;

}
static uint32_t 
be32toh(uint32_t big) {
	return big;
}
#else
static uint16_t 
le16toh(uint16_t big) {
	return big;
}
static uint16_t 
be16toh(uint16_t big) {
	char * buf = (char *)&big;
	char tmp;
	tmp = buf[0];
	buf[0] = buf[1];
	buf[1] = tmp;
	return big;
}
static uint32_t 
le32toh(uint32_t big) {
	return big;
}
static uint32_t 
be32toh(uint32_t big) {
	char * buf = (char *)&big;
	char tmp;
	tmp = buf[0];
	buf[0] = buf[3];
	buf[3] = tmp;

	tmp = buf[1];
	buf[1] = buf[2];
	buf[2] = tmp;
	return big;
}
#endif

/* camera endian */
static uint16_t 
ce16toh(int camera_is_little_endian,uint16_t big) {
	if (camera_is_little_endian) {
		return le16toh(big);
	}else {
		return be16toh(big);
	}
}
static uint32_t 
ce32toh(int camera_is_little_endian,uint32_t big) {
	if (camera_is_little_endian) {
		return le32toh(big);
	}else {
		return be32toh(big);
	}
}


static int 
get_soi(struct exif_desc * desc)
{
	register uint8_t *stream;
	int start_offset = -1; 
	int i = 0;
	int  max_search_size = scan_size;
	int  search_block_size = scan_block_size;
	int length;
	int cur_search_idx = 0;
	FILE * fd;


	if (max_search_size > desc->file_size) {
		max_search_size = desc->file_size;
	}

	fd = desc->file_fd;
	fseek(fd, 0, SEEK_SET);

	/* Allocate block size */
	stream = (uint8_t *)calloc(1, search_block_size);

	/* Use cur_search_idx to trace current block location */
	while (cur_search_idx < max_search_size) {
		/* Don't over maxmium */
		if ((cur_search_idx + scan_block_size) >= max_search_size) {
			search_block_size = (max_search_size - cur_search_idx);
		}
		memset(stream,0x0, search_block_size);
		length = fread(stream, 1, search_block_size, fd);

		for(i=0; i < length; i++) {
			/* Find SOI Marker */
			if (memcmp(stream+i, EXIF_SOI, sizeof(EXIF_SOI)) == 0) {
				/* Find APP1 Marker */
				if (memcmp(stream + i + sizeof(EXIF_SOI), EXIF_APP1, sizeof(EXIF_APP1)) == 0) {
					/* Find EXIF Identification string */
					if ((i+ sizeof(EXIF_SOI) + sizeof(EXIF_APP1) + MARKER_LEN_SIZE + sizeof(ident_exif)) < length && 
							(memcmp(stream+i+sizeof(EXIF_SOI) + sizeof(EXIF_APP1) + MARKER_LEN_SIZE, ident_exif, sizeof(ident_exif)) == 0)) {
						int app_len = 0;
						int last_app_len = 0;
						int app_num = -1;
						int next_offset = cur_search_idx + i + sizeof(EXIF_SOI); //get APP1 offset

						LOG_DEBUG("%s: %s Detect APP1 Offset %d, length [%02x][%02x]\n",__FUNCTION__,desc->filename,i+2,stream[i+4],stream[i+5]);

						while ((app_len = get_marker_length(desc, next_offset, &app_num)) > 0){
							LOG_DEBUG("%s: %s APP %d Offset %d [%08x], size %d\n",__FUNCTION__,desc->filename,app_num,
									next_offset, next_offset,app_len);
							/* Check DQT segment */

							last_app_len = app_len;
							next_offset += last_app_len + MARKER_SIZE; //get Next segment offset */
						}

						if (desc->dqt_file_offset > 0) {
							uint16_t tmp_size = 0;

							start_offset = i;			
							desc->app1_file_offset = start_offset + cur_search_idx + sizeof(EXIF_SOI);
							//memcpy(&desc->app1_size, (stream + i + sizeof(EXIF_SOI) + MARKER_SIZE), MARKER_LEN_SIZE);
							memcpy(&tmp_size, (stream + i + sizeof(EXIF_SOI) + MARKER_SIZE), MARKER_LEN_SIZE);

							/* Big endian to host endian */
							desc->app1_size = be16toh(tmp_size);

							/* No endian conversion until parsing tiff */
							LOG_DEBUG("%s: %s Find DQT Offset %d [%08x] - Set APP1 Offset %d [%08x], size %d[%04x]\n",__FUNCTION__, desc->filename,
									desc->dqt_file_offset,desc->dqt_file_offset,
									desc->app1_file_offset,desc->app1_file_offset,
									desc->app1_size, desc->app1_size);
							break;
						}

					} // exif
				} else if(memcmp(stream + i + sizeof(EXIF_SOI), EXIF_APP0 , sizeof(EXIF_APP0)) == 0) {  
					/* Find APP0 Marker for JFIF format */
					/* 2->SOI, 2->APP0, 2->Length, 5->Identifier = 2 + (2 +2 + 5) */
					/* Check jfif identification string */
					int app0_block_oft = i + sizeof(EXIF_SOI);
					int ident_block_oft = app0_block_oft + MARKER_SIZE + MARKER_LEN_SIZE;

					/* Check jfif identification string */
					if ((ident_block_oft + sizeof(ident_jfif)) < length && 
							memcmp((stream+ident_block_oft) ,ident_jfif  ,sizeof(ident_jfif)) == 0) {
						uint16_t tmp_size = 0;
						start_offset = i;			

						desc->app0_file_offset = cur_search_idx + app0_block_oft;
						//memcpy(&desc->app0_size, (stream+app0_block_oft + MARKER_SIZE),MARKER_LEN_SIZE);
						memcpy(&tmp_size, (stream+app0_block_oft + MARKER_SIZE),MARKER_LEN_SIZE);

						desc->app0_size = be16toh(tmp_size);
						LOG_DEBUG("%s: %s APP0 Offset %d, size %d [%04x]\n",__FUNCTION__,desc->filename,desc->app0_file_offset,
								desc->app0_size,desc->app0_size);
					} 
					/* Check jfif extension identification string */
					if ((ident_block_oft + sizeof(ident_jfxx)) < length && 
							memcmp((stream+ident_block_oft) ,ident_jfxx ,sizeof(ident_jfxx)) == 0) {
						uint16_t tmp_size = 0;

						start_offset = i;			

						desc->app0_file_offset = cur_search_idx + app0_block_oft;
						desc->app0_extension = 1;
						//memcpy(&desc->app0_size, (stream+app0_block_oft + MARKER_SIZE), MARKER_LEN_SIZE);
						memcpy(&tmp_size, (stream+app0_block_oft + MARKER_SIZE), MARKER_LEN_SIZE);

						desc->app0_size = be16toh(tmp_size);
						LOG_DEBUG("%s: %s APP0 Offset %d, size %d [%04x]\n",__FUNCTION__,desc->filename,desc->app0_file_offset,
								desc->app0_size,desc->app0_size);
					}
					if (desc->app0_size > 0) {
						int app_len = 0;
						int last_app_len = 0;
						int next_offset = desc->app0_file_offset;
						int app_num = -1;

						/* Get other segment info */
						while ((app_len = get_marker_length(desc,next_offset,&app_num)) > 0) {
							if (app_num == 1) {
								/* APP1 */
								if (comp_file_byte(desc, (char *)ident_exif ,next_offset + MARKER_SIZE + MARKER_LEN_SIZE, sizeof(ident_exif )) == 0) {
									desc->app1_file_offset = next_offset;
									desc->app1_size = app_len;
									LOG_DEBUG("%s: %s - Set APP1 Offset %d, size %d[%04x]\n",__FUNCTION__,desc->filename,desc->app1_file_offset,
										desc->app1_size, desc->app1_size);
								} else if (comp_file_byte(desc, (char *)ident_xmp,next_offset + MARKER_SIZE + MARKER_LEN_SIZE, sizeof(ident_xmp)) == 0) {
									/* XMP segment */
									desc->app1_xmp_file_offset = next_offset;
									desc->app1_xmp_size = app_len;
									LOG_DEBUG("%s: %s - Set APP1-XMP Offset %d, size %d[%04x]\n",__FUNCTION__,desc->filename,desc->app1_xmp_file_offset,
										desc->app1_xmp_size, desc->app1_xmp_size);
								}
							}
							last_app_len = app_len;
							next_offset += last_app_len + MARKER_SIZE;
						}
						/* DQT segment need to be existed for jpeg */
						if (desc->dqt_file_offset > 0){
							break;
						}
					}
				} else if (memcmp(stream+i+sizeof(EXIF_SOI), EXIF_DQT, sizeof(EXIF_DQT)) == 0 && stream[i+4] != 0xff) {
					LOG_DEBUG("%s: %s Find DQT - offset %d\n",__FUNCTION__,desc->filename,cur_search_idx+i+2);
					start_offset = i;			
					break;
				}
			}
		}

		if (i < length) {
			desc->soi_file_offset = start_offset + cur_search_idx;
			LOG_DEBUG("%s: %s SOI offset =%d %08x cond [%04x][%04x]\n",__FUNCTION__,desc->filename,desc->soi_file_offset,desc->soi_file_offset,*(stream+i+2),*(stream+i+3));
			free(stream);
			return 0;
		} 
		/* Jump to next block */
		cur_search_idx += length;
	}

	LOG_DEBUG("%s: %s Can't find SOI\n",__FUNCTION__,desc->filename);
	free(stream);
	return -1;
}

static int 
get_eoi(struct exif_desc * desc)
{
	register uint8_t *stream;
	int start_offset = -1; 
	int i = 0;
	int  max_search_size =0;
	int  search_block_size = scan_block_size;
	int length;
	int cur_search_idx = 0;
	int find_start_oft = 0;
	FILE * fd;

	if (desc->eoi_file_offset > 0){
		LOG_DEBUG("%s: %s Take Off-the-shelf EOI offset =%d %08x\n",__FUNCTION__,desc->filename,desc->eoi_file_offset,desc->eoi_file_offset);
		return 0;
	}

	fd = desc->file_fd;

	max_search_size = (desc->file_size - desc->soi_file_offset);
	if (desc->sos_file_offset > 0) {
		fseek(fd, desc->sos_file_offset, SEEK_SET);
		find_start_oft = desc->sos_file_offset;
		LOG_DEBUG("%s: %s Find EOI from SOS file offset %d %08x\n",__FUNCTION__,desc->filename,desc->sos_file_offset,desc->sos_file_offset);
	} else {
		fseek(fd, desc->soi_file_offset, SEEK_SET);
		find_start_oft = desc->soi_file_offset;
		LOG_DEBUG("%s: %s Find EOI from SOI file offset %d %08x\n",__FUNCTION__,desc->filename,desc->soi_file_offset,desc->soi_file_offset);
	}

	if (max_search_size > scan_size) {
		max_search_size = scan_size;
	}

	while (cur_search_idx < max_search_size) {
		if ((cur_search_idx + scan_block_size) >= max_search_size){
			search_block_size = (max_search_size - cur_search_idx);
		}
		stream = (uint8_t *)calloc(1, search_block_size);
		length = fread(stream, 1, search_block_size, fd);

		for(i=0; i < length; i++) {
			if(memcmp(stream+i, EXIF_EOI, sizeof(EXIF_EOI)) == 0) {
				start_offset = i;			
				break;
			}
		}

		if (i < length) {
			desc->eoi_file_offset = start_offset+find_start_oft+cur_search_idx;
			LOG_DEBUG("%s: %s EOI offset =%d %08x\n",__FUNCTION__,desc->filename,desc->eoi_file_offset,desc->eoi_file_offset);
			return 0;
		} 
		cur_search_idx += length;
	}

	LOG_DEBUG("%s: %s Can't find EOI\n",__FUNCTION__,desc->filename);
	return -1;
}
static int 
get_tiff(struct exif_desc * desc)
{
	register uint8_t *stream;
	const unsigned char tiff_header_start_le[] = {0x49, 0x49, 0x2a,0x00};
	const unsigned char tiff_header_start_be[] = {0x4d, 0x4d, 0x00,0x2a};
	int max_search_size;
	FILE * fd;
	uint32_t ifd0_oft;
	int length;
	int tiff_file_offset = 0;
	int tiff_size = 2 + 2 + 4;
	int ret = 0;

	if (desc == NULL) {
		return -1;
	}
	if (desc->file_fd == NULL) {
		return -1;
	}

	fd = desc->file_fd;
	max_search_size = 1024;

	tiff_file_offset = desc->app1_file_offset + MARKER_SIZE + MARKER_LEN_SIZE + sizeof(ident_exif);
	fseek(fd, tiff_file_offset, SEEK_SET);
	stream = (uint8_t *)calloc(1, tiff_size);
	length = fread(stream, 1, tiff_size, fd);

	/* Try little endian first */
	if(memcmp(stream,tiff_header_start_le, sizeof(tiff_header_start_le)) == 0) {
		desc->camera_is_little_endian = 1;
	}else if(memcmp(stream,tiff_header_start_be, sizeof(tiff_header_start_be)) == 0) {
		desc->camera_is_little_endian = 0;
	}else {
		LOG_DEBUG("%s:%s can't find tiff_header \n",__FUNCTION__,desc->filename);
		free(stream);
		return -1;
	}

	desc->tiff_file_offset = tiff_file_offset;

	memcpy(&ifd0_oft ,stream + sizeof(tiff_header_start_le), 4);
	desc->ifd0_oft = ce32toh(desc->camera_is_little_endian ,ifd0_oft);

	/* Endian confirm, so perfrom endian conversion now for APP1 */
	//desc->app1_size = ce16toh(desc->camera_is_little_endian,desc->app1_size);

	LOG_DEBUG("%s:%s Tiff %s Endian offset = %d \n",__FUNCTION__,desc->filename,(desc->camera_is_little_endian)?"Little":"Big",desc->tiff_file_offset);
	LOG_DEBUG("%s:%s 0th IFD Offset = %u [%8x]\n",__FUNCTION__,desc->filename,desc->ifd0_oft, ifd0_oft);

	if (get_file_byte(desc, (char *)&desc->ifd0_entry_count, tiff_file_offset + desc->ifd0_oft, sizeof(desc->ifd0_entry_count)) < 0) {
		ret = -1;
	}else { 
		desc->ifd0_entry_count = ce16toh(desc->camera_is_little_endian,desc->ifd0_entry_count);
		LOG_DEBUG("%s:%s 0th IFD entry count = %d [%4x]\n",__FUNCTION__,desc->filename,desc->ifd0_entry_count, desc->ifd0_entry_count);
	}

	free(stream);

	return 0;
}

static int 
get_ifd_exif_pointer(struct exif_desc * desc)
{
	struct ifd_element ret_ifd ; 

	if (desc == NULL) {
		return -1;
	}
	if (desc->file_fd == NULL) {
		return -1;
	}

	if (get_ifd0_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_EXIF_POINTER), &ret_ifd) < 0) {
		return -1;
	}

	if (desc->ifd_exif_oft > 0)
		return 0;

	desc->ifd_exif_oft = ret_ifd.val;

	if (get_file_byte(desc,(char *)&desc->ifd_exif_entry_count,desc->ifd_exif_oft + desc->tiff_file_offset,sizeof(desc->ifd_exif_entry_count))<0) {
		return -1;
	}

	desc->ifd_exif_entry_count = ce16toh(desc->camera_is_little_endian,desc->ifd_exif_entry_count);

	LOG_DEBUG("%s: %s ifd_exif offset %d entry count %d\n",__FUNCTION__,desc->filename,
			desc->ifd_exif_oft,desc->ifd_exif_entry_count);

	return 0;
}
static int 
get_ifd1(struct exif_desc * desc)
{
	uint32_t start_offset = 0; 
	uint16_t field_count =0;
	uint32_t next_ifd_oft = 0;
	register uint8_t *stream;
	int find_start_oft;
	int alloc_buffer = 0;

	if (desc == NULL) {
		return -1;
	}
	if (desc->file_fd == NULL) {
		return -1;
	}
	if (desc->tiff_file_offset == 0) {
		LOG_DEBUG("%s: No Tiff Info \n",__FUNCTION__);
		return -1;
	}

	if (desc->tmp_buf_file_offset == desc->tiff_file_offset) {
		stream = (uint8_t *)desc->tmp_buf;
		find_start_oft = 0;
	} else {
		stream = malloc(desc->app1_size);

		fseek(desc->file_fd, desc->tiff_file_offset, SEEK_SET);
		fread(stream, 1, desc->app1_size, desc->file_fd);
		find_start_oft = 0;
		alloc_buffer = 1;
	}
    LOG_DEBUG("%s: find_start_oft %d\n",__FUNCTION__,find_start_oft);

	/* Skip tiff header */
	start_offset = find_start_oft + 8;

	memcpy(&field_count,stream + start_offset , 2);
	//field_count = *(uint16_t *)(stream + start_offset);
	field_count = ce16toh(desc->camera_is_little_endian,field_count);

	/* Get IFD first entry offset */
	start_offset += 2;
    //LOG_DEBUG("%s: 0IFD entry offset %u\n",__FUNCTION__,start_offset);
	
	/* skip entry of 0th IFD */
	start_offset += (12 * field_count);

    //LOG_DEBUG("%s: Next IFD entry offset %u\n",__FUNCTION__,start_offset);

    LOG_DEBUG("%s: 0 IFD Field count= %u [%04x],NextIFD Offset %08x\n",__FUNCTION__,field_count,field_count,start_offset);

	memcpy(&next_ifd_oft,(stream + start_offset),4);
	next_ifd_oft = ce32toh(desc->camera_is_little_endian,next_ifd_oft);

	desc->ifd1_oft = next_ifd_oft;
	desc->ifd1_entry_count= field_count;
	LOG_DEBUG("%s: ifd1 offset = %u [%08x]\n",__FUNCTION__,next_ifd_oft,next_ifd_oft);

	if (alloc_buffer == 1) {
		free(stream);
	}

	if (desc->ifd1_oft == 0) {
		return -1;
	}
   return 0;
}
void dump_ifd_entry(char * ifd) 
{
	int i = 0;

	LOG_DEBUG("    Tag: %02x-%02x Type %02x-%02x Count %02x-%02x-%02x-%02x Val %02x-%02x-%02x-%02x\n",
			ifd[i++],ifd[i++], ifd[i++],ifd[i++], ifd[i++],ifd[i++], ifd[i++],ifd[i++], ifd[i++],ifd[i++], ifd[i++],ifd[i++]);

}
int
get_ifd_attr(struct exif_desc *desc,char *buf,int buf_len, uint16_t tag, struct ifd_element * ret_ifd) 
{
	int start_offset = 0;
	int i = 0;
	struct ifd_element ifd_node;
	uint16_t field_count =0;

	memcpy(&field_count,buf + start_offset , sizeof(field_count));
	field_count = ce16toh(desc->camera_is_little_endian, field_count);

	start_offset +=2;
	if ((start_offset+ (field_count * 12)) > buf_len) {
		LOG_DEBUG("%s: Buffer too small %d, field_count = %d \n",__FUNCTION__,buf_len,field_count);
		return -1;
	}

	for (i = 0; i < field_count; i++) {
		dump_ifd_entry(buf+start_offset);
		memcpy(&ifd_node,buf + start_offset ,12);
		LOG_DEBUG("%s: IFD raw entry %d, tag %02x%02x [%04x], type %04x, count %04x, val %04x  \n",__FUNCTION__,i,
				((char * )&ifd_node.tag)[0],((char * )&ifd_node.tag)[1], ifd_node.tag,ifd_node.type,ifd_node.count,ifd_node.val);

		ifd_node.tag = ce16toh(desc->camera_is_little_endian, ifd_node.tag);

		LOG_DEBUG("%s: entry %d Cmp tag %02x%02x <> node.tag %02x%02x \n",__FUNCTION__,i,
				((char *)&tag)[0], ((char *)&tag)[1], ((char * )&ifd_node.tag)[0],((char * )&ifd_node.tag)[1]);
		if (memcmp(&ifd_node.tag, &tag, 2) == 0) {
			memcpy(ret_ifd,buf + start_offset ,12);
			ret_ifd->tag = ce16toh(desc->camera_is_little_endian, ret_ifd->tag);
			ret_ifd->type = ce16toh(desc->camera_is_little_endian, ret_ifd->type);
			ret_ifd->count = ce16toh(desc->camera_is_little_endian, ret_ifd->count);

			LOG_DEBUG("%s: Got IFD entry %d, tag %04x [%d], type %04x, count %04x, val %04x  \n",__FUNCTION__,i,
				ifd_node.tag,ifd_node.tag,ifd_node.type,ifd_node.count,ifd_node.val);
			return 0;
		}
		start_offset += 12;
	}
	return -1;
}

int
get_ifd0_attr(struct exif_desc *desc, uint16_t tag, struct ifd_element * ret_ifd)  
{
	int ifd_size = 0;
	FILE * file_fd;
	int file_offset;
	char buf[2048] = {0};

	if (desc == NULL)
		return -1;
	if (ret_ifd == NULL)
		return -1;

	if (desc->ifd0_oft <= 0) {
		LOG_DEBUG("%s: ifd0 offset is not valid\n",__FUNCTION__);
		return -1;
	}

	file_fd = fopen(desc->filename, "r");
	if (file_fd == NULL) {
		LOG_DEBUG("%s: Can't open %s\n",__FUNCTION__,desc->filename);
		return -1;
	}

	ifd_size = desc->ifd0_entry_count * 12;
	if (ifd_size > 2048) {
		fclose(file_fd);
		LOG_DEBUG("%s: %s entry size %d over maximum 2048\n",__FUNCTION__,desc->filename,ifd_size);
		return -1;
	}
	file_offset = desc->tiff_file_offset + desc->ifd0_oft;
	if (file_offset + ifd_size > desc->file_size) {
		fclose(file_fd);
		LOG_DEBUG("%s: %s get buf offset %d over filesize %d",__FUNCTION__,desc->filename,file_offset + ifd_size,desc->file_size);
		return -1;
	}
	fseek(file_fd, file_offset, SEEK_SET);
	fread(buf, 1, ifd_size, file_fd);
	fclose(file_fd);

	LOG_DEBUG("%s: %s Tag %04x\n",__FUNCTION__,desc->filename,tag);
	return get_ifd_attr(desc,buf,2048,tag,ret_ifd); 
}
int
get_ifd1_attr(struct exif_desc *desc, uint16_t tag, struct ifd_element * ret_ifd)  
{
	int ifd_size = 0;
	FILE * file_fd;
	int file_offset;
	char buf[2048] = {0};

	if (desc == NULL)
		return -1;
	if (ret_ifd == NULL)
		return -1;

	if (desc->ifd0_oft <= 0) {
		LOG_DEBUG("%s: ifd0 offset is not valid\n",__FUNCTION__);
		return -1;
	}

	file_fd = fopen(desc->filename, "r");
	if (file_fd == NULL) {
		LOG_DEBUG("%s: Can't open %s\n",__FUNCTION__,desc->filename);
		return -1;
	}

	ifd_size = desc->ifd0_entry_count * 12;
	if (ifd_size > 2048) {
		fclose(file_fd);
		LOG_DEBUG("%s: %s entry size %d over maximum 2048\n",__FUNCTION__,desc->filename,ifd_size);
		return -1;
	}
	file_offset = desc->tiff_file_offset + desc->ifd0_oft;
	if (file_offset + ifd_size > desc->file_size) {
		fclose(file_fd);
		LOG_DEBUG("%s: %s get buf offset %d over filesize %d",__FUNCTION__,desc->filename,file_offset + ifd_size,desc->file_size);
		return -1;
	}
	fseek(file_fd, file_offset, SEEK_SET);
	fread(buf, 1, ifd_size, file_fd);
	fclose(file_fd);

	LOG_DEBUG("%s: %s Tag %04x\n",__FUNCTION__,desc->filename,tag);
	return get_ifd_attr(desc,buf,2048,tag,ret_ifd); 
}
int
get_ifd_exif_attr(struct exif_desc *desc, uint16_t tag, struct ifd_element * ret_ifd)  
{
	int ifd_size = 0;
	FILE * file_fd;
	int file_offset;
	char buf[2048] = {0};

	if (desc == NULL)
		return -1;

	if (ret_ifd == NULL)
		return -1;

	if (desc->ifd_exif_oft <= 0) {
		if (get_ifd_exif_pointer(desc) < 0) {
			LOG_DEBUG("%s: ifd0 offset is not valid\n",__FUNCTION__);
			return -1;
		}
	}

	file_fd = fopen(desc->filename, "r");
	if (file_fd == NULL) {
		LOG_DEBUG("%s: Can't open %s\n",__FUNCTION__,desc->filename);
		return -1;
	}

	ifd_size = desc->ifd_exif_entry_count * 12;
	if (ifd_size > 2048) {
		fclose(file_fd);
		LOG_DEBUG("%s: %s entry size %d over maximum 2048\n",__FUNCTION__,desc->filename,ifd_size);
		return -1;
	}
	file_offset = desc->tiff_file_offset + desc->ifd_exif_oft;
	if (file_offset + ifd_size > desc->file_size) {
		fclose(file_fd);
		LOG_DEBUG("%s: %s get buf offset %d over filesize %d",__FUNCTION__,desc->filename,file_offset + ifd_size,desc->file_size);
		return -1;
	}
	fseek(file_fd, file_offset, SEEK_SET);
	fread(buf, 1, ifd_size, file_fd);
	fclose(file_fd);

	LOG_DEBUG("%s: %s Tag %04x\n", __FUNCTION__, desc->filename,tag);
	return get_ifd_attr(desc, buf, 2048, tag, ret_ifd); 
}

static int 
get_file_byte(struct exif_desc * desc,char * ret_buf,int file_offset,int length)
{
	FILE * file_fd;
	int ret = 0;

	if (desc == NULL) {
		return -1;
	}
	if (ret_buf == NULL) {
		return -1;
	}
	file_fd = fopen(desc->filename, "r");
	if (file_fd == NULL) {
		LOG_DEBUG("%s: Can't open %s\n",__FUNCTION__,desc->filename);
		return -1;
	}
	if ((file_offset + length) >= desc->file_size) {
		LOG_DEBUG("%s:%s Can't get file byte, offset %d length %d bigger than filesize %d\n",
				__FUNCTION__,desc->filename,file_offset,length,desc->file_size);
		fclose(file_fd);
		return -1;
	}

	fseek(file_fd, file_offset, SEEK_SET);

	memset(ret_buf, 0x0, length);
	if (fread(ret_buf, 1, length, file_fd) < length) {
		LOG_DEBUG("%s:%s Can't get file byte, read size smaller than requested size %d.\n",
				__FUNCTION__,desc->filename,length);
		ret = -1;
	}
	fclose(file_fd);
	return ret;
}
static int 
comp_file_byte(struct exif_desc * desc,char *comp_str,int file_offset,int length )
{
	register uint8_t *stream;
	FILE * file_fd;
	int ret;

	if (desc == NULL) {
		return -1;
	}
	file_fd = fopen(desc->filename, "r");
	if (file_fd == NULL) {
		LOG_DEBUG("%s: Can't open %s\n",__FUNCTION__,desc->filename);
		return -1;
	}
	if ((file_offset+length) >= desc->file_size) {
		LOG_DEBUG("%s: %s Can't compare , offset %d length %d bigger than filesize %d\n",
				__FUNCTION__,desc->filename,file_offset,length,desc->file_size);
		fclose(file_fd);
		return -1;
	}

	fseek(file_fd, file_offset, SEEK_SET);

	stream = malloc(length);
	memset(stream, 0x0 , length);
	fread(stream, 1, length, file_fd);

	fclose(file_fd);

	ret = memcmp(stream,comp_str,length);

	LOG_DEBUG("%s: compare offset %d length %d, result %d\n",__FUNCTION__,file_offset,length,ret);
	free(stream);

	return ret;

}

static int 
get_marker_length(struct exif_desc * desc,int offset, int * ifd_num)
{
	uint16_t ifd_length = 0;
	register uint8_t *stream;
	int max_search_size = scan_size;
	int  search_block_size = scan_block_size;
	int cur_search_idx = 0;
	int i = 0;
	FILE * file_fd;
	int length;

	if (desc == NULL) {
		return -1;
	}
	file_fd = fopen(desc->filename, "r");
	if (file_fd == NULL) {
		LOG_DEBUG("%s: Can't open %s\n",__FUNCTION__,desc->filename);
		return -1;
	}

	if (max_search_size > desc->file_size) {
		max_search_size = desc->file_size;
	}

	stream = malloc(search_block_size);

	fseek(file_fd, offset, SEEK_SET);
	cur_search_idx = offset;

	memset(stream,0x0, search_block_size);
	length = fread(stream, 1, search_block_size, file_fd);

	if (memcmp(stream+i, EXIF_SOS, sizeof(EXIF_SOS)) == 0) {
		/* SOS */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		desc->sos_size = ifd_length;
		desc->sos_file_offset = offset;
		LOG_DEBUG("%s: Got SOS length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (memcmp(stream+i, EXIF_SOF0, sizeof(EXIF_SOF0)) == 0) {
		/* SOF0 */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got SOF0 length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (memcmp(stream+i, EXIF_SOF2, sizeof(EXIF_SOF2)) == 0) {
		/* SOF2 */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got SOF2 length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (memcmp(stream+i, EXIF_DHT, sizeof(EXIF_DHT)) == 0) {
		/* DHT */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got DHT length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (memcmp(stream+i, EXIF_DRI, sizeof(EXIF_DRI)) == 0) {
		/* DRI */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got DRI length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (((*(uint16_t*)stream+i) & *(uint16_t*)EXIF_RST_MASK) == *(uint16_t*)EXIF_RST) {
		/* RSTn */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got RST length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (memcmp(stream+i, EXIF_COM, sizeof(EXIF_COM)) == 0) {
		/* COM */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got COM length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (memcmp(stream+i, EXIF_DQT, sizeof(EXIF_DQT)) == 0) {
		/* DQT */
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		desc->dqt_size = ifd_length;
		desc->dqt_file_offset = offset;
		LOG_DEBUG("%s: Got DQT length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (((*(uint16_t*)stream+i) & *(uint16_t*)EXIF_APP_MASK) == *(uint16_t*)EXIF_APP) {
		/* APPn*/
		*ifd_num = (stream[i+1]&0x0f);
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got APP %d length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				*ifd_num,ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);

	} else if (memcmp(stream+i,EXIF_EOI,sizeof(EXIF_EOI)) == 0) {
		/* EOI */
		*ifd_num = -1;
		desc->eoi_file_offset = offset;
		LOG_DEBUG("%s: Got EOI FileOft %d , find start %d\n",__FUNCTION__,cur_search_idx+i,offset);

	} else if (stream[i] == 0xff && stream[i+1] != 0xff && stream[i+1] != 0x0) {
		*ifd_num = -1;
		memcpy(&ifd_length,stream + i + MARKER_SIZE, MARKER_LEN_SIZE);
		ifd_length = be16toh(ifd_length);
		LOG_DEBUG("%s: Got [%02x][%02x] length %d [%02x][%02x], FileOft %d , find start %d\n",__FUNCTION__,
				stream[i],stream[i+1], ifd_length,stream[i+2],stream[i+3],cur_search_idx+i,offset);
	}

	free(stream);
	fclose(file_fd);

	if (ifd_length == 0) {
		LOG_DEBUG("%s: Offset %d Not a APP header [%02x][%02x]\n",__FUNCTION__,offset ,stream[i+0],stream[i+1]);
		return -1;
	}else {
		return ifd_length;
	}
}

static int
get_thumb_offset(struct exif_desc * desc)
{
	int start_offset = 0; 
	int thumbnail_offset = 0;
	uint16_t type = 0;
	uint16_t entry_count = 0;
	uint16_t short_val = 0;
	uint16_t tag = 0;
	struct ifd_element * ifd_node;
	int i = 0;
	int alloc_buffer = 0;
	register uint8_t *stream ;
	int find_start_oft;
	const unsigned char thumbnail_oft_start[] = {0x01, 0x02 };
	uint16_t thumbnail_oft_host = 0;

	if (desc == NULL) {
		return -1;
	}
	if (desc->file_fd == NULL) {
		return -1;
	}
	if (desc->tiff_file_offset == 0) {
		LOG_DEBUG("%s: No Tiff Info \n",__FUNCTION__);
		return -1;
	}


	if (desc->tmp_buf_file_offset == desc->tiff_file_offset) {
		stream = (uint8_t *)desc->tmp_buf;
		find_start_oft = desc->ifd1_oft;;
	} else {
		stream = malloc(desc->app1_size);

		fseek(desc->file_fd, desc->tiff_file_offset, SEEK_SET);
		fread(stream, 1, desc->app1_size, desc->file_fd);

		find_start_oft = desc->ifd1_oft;
		alloc_buffer = 1;
	}

	start_offset = find_start_oft;
	entry_count = ce16toh(desc->camera_is_little_endian, *(uint16_t*)(stream + start_offset));
	start_offset += 2;

	thumbnail_oft_host = le16toh(*(uint16_t *)thumbnail_oft_start);

	LOG_DEBUG("%s: IFD1 entry count = %d [%04x]\n",__FUNCTION__,entry_count,entry_count);
	ifd_node = (struct ifd_element *)(stream + start_offset);
	for (i=0; i< entry_count; i++) {
		LOG_DEBUG("%s: IFD1 entry %d ,TAG = [%04x]\n",__FUNCTION__, i, ifd_node->tag);
		tag = ce16toh(desc->camera_is_little_endian, ifd_node->tag);
		if (memcmp(&tag, &thumbnail_oft_host, sizeof(thumbnail_oft_host)) == 0) {
			break;
		}

		start_offset += sizeof(struct ifd_element);
		ifd_node = (struct ifd_element *)(stream + start_offset);
	}
	if (i == entry_count) {
		LOG_DEBUG("%s:%s can't find thumbnail Offset\n",__FUNCTION__,desc->filename);
		if (alloc_buffer == 1) {
			free(stream);
		}
		return -1;
	}

	type = ce16toh(desc->camera_is_little_endian,ifd_node->type);
	if (type == 3) { /* short type */
		memcpy(&short_val ,&ifd_node->val, 2);
		short_val = ce16toh(desc->camera_is_little_endian, short_val);
		thumbnail_offset = short_val;
	}else if (type == 4) {
		thumbnail_offset = ce32toh(desc->camera_is_little_endian, ifd_node->val);
	}else {
		thumbnail_offset = ifd_node->val;
	}

	LOG_DEBUG("%s:thumbnail offset type = %d [%02x] value = %d [%04x]\n",__FUNCTION__,type,type,
			thumbnail_offset,thumbnail_offset);

	if (alloc_buffer == 1) {
		free(stream);
	}

	return thumbnail_offset;
}

static int  
get_thumb_length(struct exif_desc * desc)
{
	int start_offset = 0; 
	int thumbnail_length = 0;
	uint16_t type = 0;
	uint16_t entry_count = 0;
	uint16_t short_val = 0;
	uint16_t tag = 0;
	struct ifd_element * ifd_node;
	int i = 0;
	int alloc_buffer = 0;
	register uint8_t *stream ;
	int find_start_oft;
	const unsigned char thumbnail_oft_start[] = {0x02, 0x02 };
	uint16_t thumbnail_oft_host = 0;

	if (desc == NULL) {
		return -1;
	}
	if (desc->file_fd == NULL) {
		return -1;
	}
	if (desc->tiff_file_offset == 0) {
		LOG_DEBUG("%s: No Tiff Info \n",__FUNCTION__);
		return -1;
	}

	if (desc->tmp_buf_file_offset == desc->tiff_file_offset) {
		stream = (uint8_t *)desc->tmp_buf;
		find_start_oft = desc->ifd1_oft;;
	} else {
		stream = malloc(desc->app1_size);

		fseek(desc->file_fd, desc->tiff_file_offset, SEEK_SET);
		fread(stream, 1, desc->app1_size, desc->file_fd);
		
		find_start_oft = desc->ifd1_oft;
		alloc_buffer = 1;
	}

	start_offset = find_start_oft;
	entry_count = ce16toh(desc->camera_is_little_endian,*(uint16_t*)(stream + start_offset));
	start_offset += 2;

	thumbnail_oft_host = le16toh(*(uint16_t *)thumbnail_oft_start);

	LOG_DEBUG("%s: IFD1 entry count = %d [%04x]\n", __FUNCTION__, entry_count, entry_count);
	ifd_node = (struct ifd_element *)(stream + start_offset);
	for (i=0; i< entry_count; i++) {
		LOG_DEBUG("%s: IFD1 entry %d ,TAG = [%04x]\n",__FUNCTION__,i,ifd_node->tag);

		tag = ce16toh(desc->camera_is_little_endian, ifd_node->tag);
		if (memcmp(&tag, &thumbnail_oft_host, sizeof(thumbnail_oft_host)) == 0) {
			break;
		}

		start_offset += sizeof(struct ifd_element);
		ifd_node = (struct ifd_element *)(stream + start_offset);
	}
	if (i == entry_count) {
		LOG_DEBUG("%s:%s can't find thumbnail length\n",__FUNCTION__,desc->filename);
		if (alloc_buffer == 1) {
			free(stream);
		}
		return -1;
	}

	type = ce16toh(desc->camera_is_little_endian, ifd_node->type);
	if (type == 3) { /* short type */
		memcpy(&short_val, &ifd_node->val, 2);
		short_val = ce16toh(desc->camera_is_little_endian, short_val);
		thumbnail_length = short_val;
	}else if (type == 4) {
		thumbnail_length = ce32toh(desc->camera_is_little_endian, ifd_node->val);
	}else {
		thumbnail_length = ifd_node->val;
	}

	LOG_DEBUG("%s:thumbnail length type = %d [%02x] ,value = %d [%04x]\n",__FUNCTION__,type,type,
			thumbnail_length,thumbnail_length);

	if (alloc_buffer == 1) {
		free(stream);
	}

	return thumbnail_length;
}


static int 
get_app1_size(struct exif_desc * desc)
{
	register uint8_t *stream;
	int length ;
	int start_offset = 0; 
	int src_len = 0; 
	FILE * fd;

	if (desc->file_fd == NULL) {
		return -1;
	}
	fd = desc->file_fd;

	fseek(fd, desc->app1_file_offset, SEEK_SET);

	stream = (unsigned char*)calloc(1, scan_size);
	length = fread(stream, 1, scan_size, fd);

	start_offset = 0;
	if(memcmp(stream, EXIF_APP1, sizeof(EXIF_APP1)) == 0) {
		src_len |= stream[start_offset+2] << 8;
		src_len |= stream[start_offset+3];
		desc->app1_size = src_len;

		LOG_DEBUG("%s: %s app1 file offset %d size=%d [%04x]\n",__FUNCTION__,
				desc->filename,desc->app1_file_offset,desc->app1_size,desc->app1_size);
		free(stream);
		return 0;
	}else {
		LOG_DEBUG("%s: %s app1 file offset %d but can't find APP1\n",__FUNCTION__,
				desc->filename,desc->app1_file_offset);
		free(stream);
		return -1;
	}
}
struct exif_desc *
exif_desc_alloc(char * filename) 
{
	struct exif_desc * exif;
	struct stat stat_buf;

	if (filename == NULL) {
		return NULL;
	}

	if (stat(filename, &stat_buf) < 0){
		LOG_INFO("Stat %s failed\n",filename);
		return NULL;
	}
	if (!S_ISREG(stat_buf.st_mode)) {
		LOG_INFO("%s is not regular file\n",filename);
		return NULL;
	}

	exif = malloc(sizeof(struct exif_desc));
	if (exif != NULL) {
		FILE * fd;
		struct stat mystat;

		memset(exif, 0x0, sizeof(struct exif_desc));

		fd = fopen(filename,"r");
		if (fd == NULL) {
			LOG_INFO("Open %s failed\n",filename);
			free(exif);
			return NULL;
		}
		exif->file_fd = fd;
		strncpy(exif->filename, filename, 511);

		/* Get file size */
		if (stat(filename,&mystat) < 0) {
			LOG_INFO("Stat %s failed\n",filename);
			free(exif);
			return NULL;
		}
		exif->file_size = mystat.st_size;

		/* Get SOI First */
		if (get_soi(exif) < 0) {
			LOG_INFO("%s: Can't find SOI\n",filename);
			free(exif);
			return NULL;
		}

		if (exif->app1_size > 0) {
			if (get_tiff(exif) < 0) {
				LOG_INFO("%s: APP1 no TIFF\n",filename);
				free(exif);
				return NULL;
			}
			/* Copy from Tiff header */
            exif->tmp_buf_len = MARKER_SIZE + exif->app1_size - (exif->tiff_file_offset - exif->app1_file_offset);
			exif->tmp_buf = malloc(exif->tmp_buf_len);
			if (exif->tmp_buf == NULL) {
				LOG_INFO("%s: %s Alloc tmp_buf failed\n",__FUNCTION__,filename);
				free(exif);
				return NULL;
			}
			
			exif->tmp_buf_file_offset = exif->tiff_file_offset;
			fseek(fd, exif->tiff_file_offset, SEEK_SET);

			/* Sanity Check */
			if (fread(exif->tmp_buf, 1, exif->tmp_buf_len, exif->file_fd) != exif->tmp_buf_len) {
				LOG_INFO("%s: %s APP1 Szie %d [%0x] don't match read size\n",__FUNCTION__,
						filename,exif->tmp_buf_len,exif->tmp_buf_len);
				free(exif->tmp_buf);
				free(exif);
				return NULL;
			}

			/* Get Frist IFD  */
			if (get_ifd1(exif) < 0) {
				LOG_INFO("%s: %s APP1 no First IFD\n",__FUNCTION__,filename);
			}
		} else {
			LOG_DEBUG("%s: %s NO APP1\n",__FUNCTION__,filename);
		}
	}

	return exif;
}

void
exif_desc_free(struct exif_desc * exif) 
{
	if (exif != NULL) {
		if (exif->tmp_buf)
			free(exif->tmp_buf);

		if (exif->file_fd)
			fclose(exif->file_fd);

		free(exif);
	}
}
int 
exif_get_thumbnail(struct exif_desc * desc,char * buf,int *buf_len) 
{
	int offset;
	int length;
	char * src_buf = NULL;
	
	if (desc == NULL) {
		return -1;
	}
	if (buf == NULL) {
		return -1;
	}

	if (desc->ifd1_oft == 0) {
		LOG_INFO("%s: NO First IFD\n",__FUNCTION__);
		return -1;
	}
	offset = get_thumb_offset(desc);
	if (offset < 0) {
		return -1;
	}

	length = get_thumb_length(desc);
	if (length < 0) {
		return -1;
	}
	if (*buf_len < length){
		LOG_INFO("%s: Buf length %d is less than thumb length %d\n",__FUNCTION__,*buf_len,length);
		return -1;
	}
	if (desc->tmp_buf && (desc->tmp_buf_file_offset == desc->tiff_file_offset) 
			&& (length <= desc->tmp_buf_len)) {
		src_buf = desc->tmp_buf;
		memcpy(buf,src_buf + offset, length);
	} else {
		fseek(desc->file_fd, desc->tiff_file_offset + offset, SEEK_SET);
		if (fread(buf, 1, length, desc->file_fd) != (length)){
			LOG_INFO("%s: Read length is less than thumb length %d\n",__FUNCTION__,length);
			return -1;
		}
	}

	*buf_len = length;
	return 0;
}
int 
exif_get_jpeg(struct exif_desc * desc,char * buf,int * buf_len) 
{
	int thumb_offset;
	int length;
	char * src_buf = NULL;
	int size = 0;
	
	if (desc == NULL){
		return -1;
	}
	if (buf == NULL){
		return -1;
	}

	if (desc->ifd1_oft > 0) {
		thumb_offset = get_thumb_offset(desc);
		if (thumb_offset > 0) {
			length = get_thumb_length(desc);
			if (length < 0) {
				return -1;
			}
			if (*buf_len < length) {
				LOG_INFO("%s: Buf length %d is less than thumb length %d\n",__FUNCTION__,*buf_len,length);
				return -1;
			}

			if (desc->tmp_buf && (desc->tmp_buf_file_offset == desc->tiff_file_offset) 
					&& (length <= desc->tmp_buf_len)){
				src_buf = desc->tmp_buf;
				memcpy(buf,desc->tmp_buf + thumb_offset, length);
			} else {
				fseek(desc->file_fd, desc->tiff_file_offset + thumb_offset, SEEK_SET);
				if (fread(buf, 1, length, desc->file_fd) != (length)){
					LOG_INFO("%s: Read length is less than thumb length %d\n",__FUNCTION__,length);
					return -1;
				}
			}
			*buf_len = length;
			return 0;
		}
	} 

	if (get_eoi(desc) < 0) {
		LOG_INFO("%s: Can't get EOI\n",__FUNCTION__);
		return -1;
	}

	size = (desc->eoi_file_offset - desc->soi_file_offset) + 2;
	if (size > *buf_len) {
		LOG_INFO("%s: Buf length %d is less than jpeg length %d\n",__FUNCTION__,*buf_len,size);
		return -1;
	}

	fseek(desc->file_fd, desc->soi_file_offset, SEEK_SET);
	LOG_INFO("%s: Copy length %d SOI %d, EOI %d\n",__FUNCTION__,size,
			desc->soi_file_offset,desc->eoi_file_offset);
	fread(buf, 1, size, desc->file_fd);
	*buf_len = size;
	return 0;
}

int
exif_set_log(int log) 
{
	_log_level = log;
	return 0;
}
int
exif_set_scan_size(int size) 
{
	scan_size = size;
	return 0;
}
int
exif_set_scan_block_size(int size) 
{
	if (size <= scan_size)
		scan_block_size = size;
	else
		scan_block_size = scan_size;
	return 0;
}
int
exif_get_image_width(struct exif_desc * desc,int *width)
{
	struct ifd_element ret_ifd; 
	uint16_t short_val = 0;
	uint32_t long_val = 0;
	
	if (width == NULL)
		return -1;

	if (get_ifd_exif_attr(desc,le16toh(*(uint16_t *)TAG_IMAGE_WIDTH), &ret_ifd) < 0) {
		return -1;
	}

	if (ret_ifd.type ==3) { /* short type */
		memcpy(&short_val,&ret_ifd.val,2);
		*width = ce16toh(desc->camera_is_little_endian,short_val);
	}else if (ret_ifd.type == 4) {
		memcpy(&long_val,&ret_ifd.val,4);
		*width = ce32toh(desc->camera_is_little_endian,long_val);
	} else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}
	LOG_DEBUG("%s: %s type %d width %d \n",__FUNCTION__,desc->filename,ret_ifd.type,*width);
	return 0;
}
int
exif_get_image_height(struct exif_desc * desc,int *height)
{
	struct ifd_element ret_ifd; 
	uint16_t short_val = 0;
	uint32_t long_val = 0;
	
	if (height == NULL) {
		return -1;
	}

	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_HEIGHT), &ret_ifd) < 0) {
		return -1;
	}

	if (ret_ifd.type == 3) { /* short type */
		memcpy(&short_val, &ret_ifd.val, 2);
		*height = ce16toh(desc->camera_is_little_endian, short_val);
	}else if (ret_ifd.type == 4) {
		memcpy(&long_val, &ret_ifd.val, 4);
		*height = ce32toh(desc->camera_is_little_endian, long_val);
	} else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}

	LOG_DEBUG("%s: %s type %d height %d \n",__FUNCTION__,desc->filename,ret_ifd.type,*height);
	return 0;
}

int
exif_get_image_exptime(struct exif_desc * desc, char * buf, int buf_len)
{
	struct ifd_element ret_ifd; 
	char double_long[8];
	uint32_t denominator;
	uint32_t numerator;
	int		oft = 0;
	
	if (buf == NULL) {
		return -1;
	}

	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_EXP_TIME), &ret_ifd) < 0){
		return -1;
	}

	oft = ce32toh(desc->camera_is_little_endian, ret_ifd.val);
	oft += desc->tiff_file_offset;

	if (ret_ifd.type == 5) {
		if (get_file_byte(desc, double_long ,oft, sizeof(double_long)) < 0) {
			return -1;
		}
		memcpy(&numerator,&double_long[0],4);
		memcpy(&denominator,&double_long[4],4);
		denominator = ce32toh(desc->camera_is_little_endian, denominator);
		numerator = ce32toh(desc->camera_is_little_endian, numerator);
	}else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}

	LOG_DEBUG("%s: %s type %d => %d/%d \n",__FUNCTION__,desc->filename,ret_ifd.type,numerator,denominator);
	snprintf(buf,buf_len,"%d/%d", numerator, denominator);
	return 0;
}
int
exif_get_image_fnumber(struct exif_desc * desc,char * buf, int buf_len)
{
	struct ifd_element ret_ifd; 
	char double_long[8];
	uint32_t denominator;
	uint32_t numerator;
	int		oft = 0;
	
	if (buf== NULL)
		return -1;

	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_FNUMBER), &ret_ifd) < 0) {
		return -1;
	}

	oft = ce32toh(desc->camera_is_little_endian, ret_ifd.val);
	oft += desc->tiff_file_offset;

	if (ret_ifd.type == 5) {
		if (get_file_byte(desc, double_long ,oft, sizeof(double_long)) < 0) {
			return -1;
		}
		memcpy(&numerator, double_long, 4);
		memcpy(&denominator, &double_long[4], 4);
		denominator = ce32toh(desc->camera_is_little_endian, denominator);
		numerator = ce32toh(desc->camera_is_little_endian, numerator);
	}else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}

	LOG_DEBUG("%s: %s type %d => %d/%d \n",__FUNCTION__,desc->filename,ret_ifd.type,numerator,denominator);
	snprintf(buf,buf_len,"%d/%d", numerator, denominator);
	return 0;
}
int
exif_get_image_focal_length(struct exif_desc * desc, char * buf, int buf_len)
{
	struct ifd_element ret_ifd; 
	char double_long[8];
	uint32_t denominator;
	uint32_t numerator;
	int		oft = 0;
	
	if (buf == NULL) {
		return -1;
	}
	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_FOCAL_LEN), &ret_ifd) < 0) {
		return -1;
	}

	oft = ce32toh(desc->camera_is_little_endian, ret_ifd.val);
	oft += desc->tiff_file_offset;

	if (ret_ifd.type == 5) {
		if (get_file_byte(desc, double_long, oft, sizeof(double_long)) < 0) {
			return -1;
		}
		memcpy(&numerator,double_long, 4);
		memcpy(&denominator,&double_long[4], 4);
		denominator = ce32toh(desc->camera_is_little_endian, denominator);
		numerator = ce32toh(desc->camera_is_little_endian, numerator);
	}else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}

	LOG_DEBUG("%s: %s type %d => %d/%d \n",__FUNCTION__,desc->filename,
			ret_ifd.type,numerator,denominator);
	snprintf(buf,buf_len,"%u/%u", numerator, denominator);
	return 0;
}
int
exif_get_image_exposure_bias(struct exif_desc * desc, char * buf, int buf_len)
{
	struct ifd_element ret_ifd; 
	char double_long[8];
	int32_t denominator;
	int32_t numerator;
	int		oft = 0;
	
	if (buf == NULL) {
		return -1;
	}
	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_EXP_BIAS), &ret_ifd) < 0) {
		return -1;
	}

	oft = ce32toh(desc->camera_is_little_endian, ret_ifd.val);
	oft += desc->tiff_file_offset;

	if (ret_ifd.type == 10) {
		if (get_file_byte(desc, double_long, oft, sizeof(double_long)) < 0) {
			return -1;
		}
		memcpy(&numerator,double_long, 4);
		memcpy(&denominator,&double_long[4], 4);
		denominator = ce32toh(desc->camera_is_little_endian, denominator);
		numerator = ce32toh(desc->camera_is_little_endian, numerator);
	}else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}

	LOG_DEBUG("%s: %s type %d => %d/%d \n",__FUNCTION__,desc->filename,
			ret_ifd.type,numerator,denominator);
	snprintf(buf,buf_len,"%d/%d", numerator, denominator);
	return 0;
}
int
exif_get_image_iso(struct exif_desc * desc,int *iso)
{
	struct ifd_element ret_ifd; 
	uint16_t short_val = 0;

	if (iso == NULL) {
		return -1;
	}

	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_ISO),&ret_ifd) < 0) {
		return -1;
	}

	if (ret_ifd.type == 3) { /* short type */
		memcpy(&short_val, &ret_ifd.val, 2);
		*iso = ce16toh(desc->camera_is_little_endian, short_val);
	} else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}

	LOG_DEBUG("%s: %s type %d iso %d \n",__FUNCTION__,desc->filename,ret_ifd.type,*iso);
	return 0;
}

int
exif_get_image_exif_version(struct exif_desc * desc,char * buf, int buf_len)
{
	struct ifd_element ret_ifd; 

	if (buf == NULL) {
		return -1;
	}

	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_EXIF_VER), &ret_ifd) < 0) {
		return -1;
	}

	if (ret_ifd.type == 7) {
		memcpy(buf, &ret_ifd.val, 4);
	} else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}
	LOG_DEBUG("%s: %s type %d => %s \n",__FUNCTION__,desc->filename,ret_ifd.type,buf);
	return 0;
}
int
exif_get_image_datetime_original(struct exif_desc * desc,char * buf, int buf_len)
{
	struct ifd_element ret_ifd; 
	int		oft = 0;
	int count = 0;

	if (buf == NULL) {
		return -1;
	}

	if (get_ifd_exif_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_DATETIME_ORIGINAL), &ret_ifd) < 0) {
		return -1;
	}

	oft = ce32toh(desc->camera_is_little_endian, ret_ifd.val);
	oft += desc->tiff_file_offset;

	count = ret_ifd.count;
	if (count >= buf_len){
		LOG_DEBUG("%s: %s Buffer length %d is smaller than %d\n",__FUNCTION__,desc->filename,buf_len,count);
		return -1;
	}

	memset(buf,0x0,buf_len);
	if (ret_ifd.type == 2) {
		if (get_file_byte(desc, buf, oft, count) < 0) {
			return -1;
		}
	}else {
		LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
		return -1;
	}

	LOG_DEBUG("%s: %s type %d => %s \n",__FUNCTION__,desc->filename,ret_ifd.type,buf);
	return 0;
}
int
exif_get_image_filesize(struct exif_desc * desc,uint32_t * size)
{
	if (size== NULL) {
		return -1;
	}

	if (desc->file_fd == NULL) {
		return -1;
	}
	*size = desc->file_size;
	return 0;
}

int
exif_get_image_orientation(struct exif_desc * desc,int *orientation)
{
    struct ifd_element ret_ifd; 
    uint16_t short_val = 0;

    if (orientation == NULL) {
        return -1;
    }

    if (get_ifd0_attr(desc, le16toh(*(uint16_t *)TAG_IMAGE_ORIENTATION), &ret_ifd) < 0) {
        return -1;          
    }

    if (ret_ifd.type == 3) { /* short type */
        memcpy(&short_val, &ret_ifd.val, 2);
        *orientation = ce16toh(desc->camera_is_little_endian, short_val);
    } else {
        LOG_DEBUG("%s: %s Can't process type %d\n",__FUNCTION__,desc->filename,ret_ifd.type);
        return -1;
    }

    LOG_DEBUG("%s: %s type %d orientation %d \n",__FUNCTION__,desc->filename,ret_ifd.type,*orientation);
    return 0;
}
