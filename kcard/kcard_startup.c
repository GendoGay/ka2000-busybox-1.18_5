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
#include <getopt.h>		/* getopt_long() */

#include "libbb.h"
//#include "kcard.h"
#include "common.h"

// these values are for ioctl of i2c driver
#define DISABLE_IRQ_HANDLING 0
#define ENABLE_IRQ_HANDLING 1
#define INIT_AGAIN 2


static void power_up_init_kcard(void);
static void disable_call_kcard(void);

static unsigned int usercall_interval = 10; // default value

static void power_up_init_kcard(void)
{
    printf ("kcard app, call interval %d\n", usercall_interval);
    set_kcard_interval(usercall_interval);
    enable_kcard_call();
}

static void disable_call_kcard(void)
{
    disable_kcard_call();
}

int kcard_startup_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int kcard_startup_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    printf("this is kcard startup app\n");

    power_up_init_kcard();
    // other initialization if necessary

    return 0;
}

