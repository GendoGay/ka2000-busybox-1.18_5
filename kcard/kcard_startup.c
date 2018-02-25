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
    printf ("kcard_startup: Set call interval %d\n", usercall_interval);
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
    //printf("this is kcard startup app\n");

    power_up_init_kcard();
    // other initialization if necessary

    return 0;
}

