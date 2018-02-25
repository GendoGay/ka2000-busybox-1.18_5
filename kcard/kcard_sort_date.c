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
#include "libbb.h"

int kcard_dir_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int kcard_dir_sort_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    glob_t data;
	int i;
	
    printf("dir %s\n",argv[1]);
    switch( glob(argv[1], 0, NULL, &data ) )
    {
        case 0:
		    error=0;
            break;
        case GLOB_NOSPACE:
            printf( "Out of memory\n" );
            break;
        case GLOB_ABORTED:
            printf( "Reading error\n" );
            break;
        case GLOB_NOMATCH:
		    error=0;
            printf( "No files found\n" );
            break;
        default:
            break;
    }
	
	for(i=0;i<data.gl_pathc;i++)
		{
			p=data.gl_pathv[i];	
			
			printf("dir %s\n",p);
		}

}
