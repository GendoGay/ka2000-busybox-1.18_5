#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <errno.h>
#include "libbb.h"
#include "common.h"

/*
 * struct statvfs {
 *  unsigned long  f_bsize;    / file system block size
 *  unsigned long  f_frsize;   / fragment size
 *  fsblkcnt_t     f_blocks;   / size of fs in f_frsize units
 *  fsblkcnt_t     f_bfree;    / # free blocks
 *  fsblkcnt_t     f_bavail;   / # free blocks for non-root
 *  fsfilcnt_t     f_files;    / # inodes
 *  fsfilcnt_t     f_ffree;    / # free inodes
 *  fsfilcnt_t     f_favail;   / # free inodes for non-root
 *  unsigned long  f_fsid;     / file system ID
 *  unsigned long  f_flag;     / mount flags
 *  unsigned long  f_namemax;  / maximum filename length
 * };
 */

void echo_stat(struct statvfs *buff)
{
	fprintf (stdout, "Block Size  :%lu\n",buff->f_bsize );
	fprintf (stdout, "Fragm Size  :%lu\n",buff->f_frsize );
	fprintf (stdout, "Blocks      :%lu\n",buff->f_blocks );
	fprintf (stdout, "Free blocks :%lu\n",buff->f_bfree );
	fprintf (stdout, "Non root fb :%lu\n",buff->f_bavail );
	fprintf (stdout, "Inodes      :%lu\n",buff->f_files );
	fprintf (stdout, "Free Inodes :%lu\n",buff->f_ffree );
	fprintf (stdout, "Non root fi :%lu\n",buff->f_favail );
	fprintf (stdout, "FS Id       :%lu\n",buff->f_fsid );
	fprintf (stdout, "Mount flags :%lu\n",buff->f_flag );
	fprintf (stdout, "Max fn leng :%lu\n",buff->f_namemax );
}
static void fs_info_usage(void)
{
	printf("Syntax: fs_info [PATH]\n");
}

int fs_info_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int fs_info_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{

	struct statvfs * buff;
	//extern int errno;

	if (argc != 2) 
		return -1;

	if ( !(buff = (struct statvfs *)
				malloc(sizeof(struct statvfs)))) {
		perror ("Failed to allocate memory to buffer.");
		return -1;
	}
	if (statvfs(argv[1], buff) < 0) {
		perror("statvfs() has failed.");
		free(buff);
		return -1;
	} else {
		fprintf(stderr,"statvfs() completed successfully. \n");
		echo_stat(buff);
	}
	free(buff);

	exit(EXIT_SUCCESS);
}


