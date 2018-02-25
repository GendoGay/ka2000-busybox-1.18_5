#include "libbb.h"
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
/*
 * Given a URL encoded string, convert it to plain ascii.
 * Since decoding always makes strings smaller, the decode is done in-place.
 * Thus, callers should xstrdup() the argument if they do not want the
 * argument modified.  The return is the original pointer, allowing this
 * function to be easily used as arguments to other functions.
 *
 * string    The first string to decode.
 * option_d  1 if called for httpd -d
 *
 * Returns a pointer to the decoded string (same as input).
 */
static unsigned hex_to_bin(unsigned char c)
{
	unsigned v;

	v = c - '0';
	if (v <= 9)
		return v;
	/* c | 0x20: letters to lower case, non-letters
	 * to (potentially different) non-letters */
	v = (unsigned)(c | 0x20) - 'a';
	if (v <= 5)
		return v + 10;
	return ~0;
/* For testing:
void t(char c) { printf("'%c'(%u) %u\n", c, c, hex_to_bin(c)); }
int main() { t(0x10); t(0x20); t('0'); t('9'); t('A'); t('F'); t('a'); t('f');
t('0'-1); t('9'+1); t('A'-1); t('F'+1); t('a'-1); t('f'+1); return 0; }
*/
}
static char *decodeString(char *orig, int option_d)
{
	/* note that decoded string is always shorter than original */
	char *string = orig;
	char *ptr = string;
	char c;

	while ((c = *ptr++) != '\0') {
		unsigned v;

		if (option_d && c == '+') {
			*string++ = ' ';
			continue;
		}
		if (c != '%') {
			*string++ = c;
			continue;
		}
		v = hex_to_bin(ptr[0]);
		if (v > 15) {
 bad_hex:
			if (!option_d)
				return NULL;
			*string++ = '%';
			continue;
		}
		v = (v * 16) | hex_to_bin(ptr[1]);
		if (v > 255)
			goto bad_hex;
		if (!option_d && (v == '/' || v == '\0')) {
			/* caller takes it as indication of invalid
			 * (dangerous wrt exploits) chars */
			return orig + 1;
		}
		*string++ = v;
		ptr += 2;
	}
	*string = '\0';
	return orig;
}

char *strrep( char *str, char *old, char *new )
{

	char *reStr;
	int i, count = 0;
	size_t newlen = strlen( new );
	size_t oldlen = strlen( old );

	for( i = 0; str[i] != '\0'; ++i ) {

		if( strstr( &str[i], old ) == &str[i] ) {

			++count;
			i = i + oldlen - 1;

		}

	}
	/* 1 for Null character */
	reStr = malloc( i + count * ( newlen - oldlen ) + 1); 
	if (reStr == NULL) 
		return NULL;

	i = 0;
	while( *str ) {

		if( strstr( str, old ) == str ) {

			strcpy( &reStr[i], new );
			i = i + newlen;
			str = str + oldlen;

		}
		else 

			reStr[i++] = *str++;

	}
	reStr[i] = '\0';
	return reStr;
}

static int get_query_string(const char *token, char *dest)
{
	char* get_data = getenv("QUERY_STRING");
	char * tmp_buf;
	int data_len = 0;

	data_len = strlen(get_data);
	if (data_len <= 0) {
		fprintf(stderr,"%s: QUERY_STRING len %d\n",__FUNCTION__,data_len);
		return 0;
	}

	tmp_buf = malloc(data_len+1);
	if (tmp_buf == NULL) {
		fprintf(stderr,"%s: malloc %d failed\n",__FUNCTION__,data_len);
		return 0;
	}
	memset(tmp_buf,0x0,data_len+1);
	memcpy(tmp_buf,get_data,data_len);
	
	//get_data = decodeString(tmp_buf, 1);
	get_data = tmp_buf;
	if (get_data != NULL && strlen(get_data) > 2) {
		char* val = strstr(get_data, token);
		if (val != NULL){
			char * end_val = NULL;
			end_val = strstr(val+1,"&");
			if (end_val == NULL) {
				strcpy(dest, val+strlen(token));
				decodeString(dest, 1);
			} else {
				/* copy from "toekn" to "&" */
				strncpy(dest, val+strlen(token),end_val - (val+strlen(token)));
				decodeString(dest, 1);
			}
		} else {
			free(tmp_buf);
			return 0;
#ifdef TS_DEBUG
			printf("Can't find %s parameter!\n", token);
#endif
		}
	} else {
		free(tmp_buf);
		return 0;
	}

	//printf("%s\n", dest);
	free(tmp_buf);
	return 1;
}



int tslist_main( int argc, char **argv ) MAIN_EXTERNALLY_VISIBLE;
int tslist_main( int argc UNUSED_PARAM, char **argv UNUSED_PARAM )
{
	DIR *d;
	struct dirent *dir;
	char dest[1024] = {0};
	char *listPath = strrep( getenv( "QUERY_STRING" ), "%20", " " );
	int fileCount = 0;
	struct stat buf;
	char filePath[256] = {0};
	int hide = 0;
	int try_count = 0;

	printf( "Content-Type: text/html\r\n\r\n" );
	printf( "TS list1\n" );

#ifdef TS_DEBUG
	printf( "listPath = %s\n", listPath );
#endif

	if( listPath == NULL ) {
		printf( "Fail" );
		return -1;
	}
	listPath = decodeString(listPath, 1);

	/* CMD_DEL&FILE=xxxxx */
	if (get_query_string("ACTION=",dest) && 
		(strncmp(dest,"CMD_DEL",strlen("CMD_DEL")) == 0)) {

		if (get_query_string("FILE=",dest)) {
			if (unlink(dest)) {
				printf("Fail: Can't delete %s\n",dest);
				return -1;
			} else {
				printf("Success: Delete %s\n",dest);
			}
		}else {
			printf("Fail: Can't find %s\n",dest);
			return -1;
		}
		return 0;
		
	} else if (get_query_string("HIDEDIR=",dest) && 
			get_query_string("PATH=",filePath)) {
		hide = 1;
		listPath = filePath;
	} else if (get_query_string("PATH=",filePath)) {
		listPath = filePath;
	}


		system("/usr/bin/refresh_sd");
		if (hide) 
			printf( "HideDir = %s\n", dest);

		printf( "List Files = %s\n", listPath );

		disable_kcard_call();

		d = opendir( listPath );
		while (d == NULL && try_count++ <= 5) {
			sleep(1);
			d = opendir( listPath );
		}

		if( d ) {

			while( ( dir = readdir( d ) ) != NULL ) {

				if( strcmp( dir->d_name, "." ) == 0 )
					continue;
				if( strcmp( dir->d_name, ".." ) == 0 )
					continue;

				/* Hide folder */
				if (hide == 1 && strlen(dest) > 0) {
					if (strcmp(dir->d_name, dest) == 0 )
						continue;
				}

#if 0
				/* stat take too much process time */
				sprintf(filePath, "%s/%s", listPath, dir->d_name);
				if (stat(filePath, &buf) < 0) {
					perror("Can't stat:");
					fprintf(stderr,"Can't stat %s"
							" errno %d\n",filePath, errno);
				} else {
					//fprintf(stderr,"stat %s success\n",filePath);
				}
				printf( "FileName%d=%s&FileSize%d=%d&", fileCount, dir->d_name, fileCount, buf.st_size );
				if (S_ISDIR( buf.st_mode ))
					printf( "FileType%d=Directory&", fileCount );
				else
					printf( "FileType%d=File&", fileCount );

#endif
				printf( "FileName%d=%s&", fileCount, dir->d_name);
				if (dir->d_type == DT_DIR)
					printf( "FileType%d=Directory&", fileCount );
				else
					printf( "FileType%d=File&", fileCount );

				++fileCount;

			}
			closedir(d);
		} else {

			fprintf(stderr,"%s: Opendir failed!: '%s'\n",__FUNCTION__,listPath);

			/* For backward compatibility , return fake DCIM folder */
			if (strcmp(listPath,"/www/sd/DCIM") == 0) {
				//printf( "FileName%d=%s&", fileCount, dir->d_name);
				//printf( "FileType%d=Directory&", fileCount );
				//++fileCount;
			}
		}
		enable_kcard_call();
		printf( "FileCount=%d", fileCount );

	return 0;
}
