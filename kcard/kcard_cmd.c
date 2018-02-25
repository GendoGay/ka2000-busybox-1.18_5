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
#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>		
#include "libbb.h"

#ifndef AUTOCONF_TIMESTAMP
#define AUTOCONF_TIMESTAMP "2011-11-24 15:38:52 CST"
#endif
void enable_power_sleep(void);
void disable_power_sleep(void);

int cmd(int argc, char *argv[]) {
    char option;
	const char *short_option = "hvcs:";
    const struct option long_option[] = {
        { "help", no_argument, NULL, 'h' },
        { "version", no_argument, NULL, 'v' },
        { "conf", required_argument, NULL, 'c' },
		{ "sleep", required_argument, NULL, 's' },
        { NULL, 0, NULL, 0}
    };
    while((option = getopt_long(argc, argv, short_option, long_option, NULL)) != (char)-1) {
        //printf("option is %c\t\t\n", option);
        switch(option) {
        case 'h':
            printf("kcard -s 0 : disable sleep mode\n");
			printf("kcard -s 1 : enable sleep mode\n");
			printf("kcard -v : check busybox version\n");
            break;
        case 'v':
            printf("%s\n", AUTOCONF_TIMESTAMP);
            break;
        case 'c':
            system("cat /mnt/mtd/config/wsd.conf");
            break;
		case 's':
		//	printf("sleep=\"%s\"\n",optarg); 
			if (optarg[0] == '0')		
			{				
				printf("sleep disable\n");
				disable_power_sleep();
			}
			else
			{
				printf("sleep enable\n");
				enable_power_sleep();
			}
        }

    }
    return 0;

}

int kcard_cmd_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int kcard_cmd_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
   // printf("kcard call cmd and setting\n");

    cmd(argc, argv);

    return 0;
}

