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
#include <glob.h>

#include "libbb.h"

#include "common.h"
#include "buzzer.h"
//-----------------------------------------------------------------------
int is_dir_exist (char *dir)
{
    struct stat ostat;

    if(stat(dir, &ostat) == -1)
    {
        return 0;
    }

    return 1;
}
//-----------------------------------------------------------------------
int wifi_ftp_server_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_ftp_server_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    glob_t data;
    FILE *fp;

    play_sound (CONNECTION,1);

    if( !(is_dir_exist("/www/sd/DCIM/123_FTP/")))
    {
        system("mkdir -p /www/sd/DCIM/123_FTP/");
    }

    system("sync");
    system("sleep 10");

    play_sound(FINISH, 1);
    return 0;
}
