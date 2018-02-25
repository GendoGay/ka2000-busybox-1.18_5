#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "libbb.h"
#include "buzzer.h"

//Tone value
#define TONE_C1 		262
#define TONE_D1 		294
#define TONE_E1 		330
#define TONE_F1 		349
#define TONE_G1 		392
#define TONE_A1 		440
#define TONE_B1 		494
#define TONE_C2 		523
#define TONE_REST 		0
#define TONE_MUTE		1


int buzzer_conf = -1;

/* get_buzzer_conf(), return value
0: Can't find config file
1: Buzzer is Mute
2: Buzzer it Normal
*/
int get_buzzer_conf()
{
	char line[90];
	FILE* fp = NULL;
	int times=0;

    if (buzzer_conf != -1)
        return buzzer_conf;

    fp = fopen("/tmp/wsd.conf", "r");
    if (fp == NULL)
    {
        /* first time do the copy */
        system("cp /mnt/mtd/config/wsd.conf /tmp/wsd.conf");
        fp = fopen("/tmp/wsd.conf", "r");
        if (fp == NULL)
        {
            printf("Can't find config file wsd.conf\n");
            buzzer_conf = 0;
            return 0;
        }
    }

    while (!feof(fp))
    {
        fgets(line, 90, fp);
        if (strstr(line,"Buzzer Mode") != NULL)
        {
            if(strstr(line,"Mute")!=NULL)
                buzzer_conf = 1;
            else
                buzzer_conf = 2;

            fclose(fp);
            return buzzer_conf;
        }
    }
    fclose(fp);

	return buzzer_conf;
}

//------------------------------------------------------------------------------
int play_tone_array(int tone[], int count, int repeat_play)
{
    int i;
    unsigned int snd = 0;
    int fd;

    fd = open("/dev/ka-pwm", O_RDWR);
    /* check pwm file exist */
    if (fd < 0)
    {
        printf("Fail to open PWM Device!\n");
        return -1;
    }

   // printf("play %d\n", count);
    /* send start play inst */
    ioctl(fd, 0);

    /* send tone inst */
    for (i = 0; i < count; i++)
    {
        // DDVVFFFF : (duration << 24) + (volumne << 16) + frequency;
        snd = (1 << 24) + (10 << 16) + tone[i];
        ioctl(fd, snd);
    }

    /* stop play inst - repeat or not */
    if (repeat_play == 0)
         ioctl(fd, (2 << 24) + (2 << 16) + 2);
    else
         ioctl(fd, (1 << 24) + (1 << 16) + 1);

    close(fd);
    return 0;
}
//------------------------------------------------------------------------------
int play_sound(int sound_type, int val)
{
    int tone[64];
    int i = 0;
    int repeat_play = 1;

    switch(sound_type)
    {
    case BUZZ_STOP:
        i = 0;
        repeat_play = 0;
        break;

    case BUZZ_ACT:
        tone[i] = TONE_C2; i++;
        tone[i] = TONE_MUTE; i++;
        tone[i] = TONE_E1; i++;
        tone[i] = TONE_G1; i++;
        tone[i] = TONE_C1; i++;
        tone[i] = 100; i++;
        break;

    case BUZZ_OK:   // ok, pass, success ....
        tone[i] = TONE_C1; i++;
        tone[i] = TONE_E1; i++;
        tone[i] = 77; i++;
        tone[i] = TONE_D1; i++;
        tone[i] = TONE_C1; i++;
        tone[i] = TONE_MUTE; i++;
        break;

    case BUZZ_CONNECT:
        //tone[i] = TONE_G1;   i++;
        tone[i] = 100;   i++;
        tone[i] = TONE_MUTE; i++;
        tone[i] = 100;   i++;
        tone[i] = TONE_G1;   i++;
        tone[i] = 100;   i++;
        tone[i] = TONE_MUTE; i++;
        break;

    case BUZZ_TRANSFER:
        tone[i] = TONE_C1; i++;
        tone[i] = TONE_E1; i++;
        tone[i] = TONE_G1; i++;
        tone[i] = TONE_C2; i++;
        tone[i] = TONE_MUTE; i++;
        tone[i] = TONE_MUTE; i++;
        break;

    case BUZZ_RECEIVE:
        tone[i] = TONE_C2; i++;
        tone[i] = TONE_G1; i++;
        tone[i] = TONE_E1; i++;
        tone[i] = TONE_C1; i++;
        break;

    case BUZZ_WAIT:
        printf("wait for action\n");
        tone[i] = 182;   i++;
        tone[i] = TONE_MUTE; i++;
        break;

    case BUZZ_DONE:
        tone[i] = TONE_E1; i++;
        tone[i] = TONE_A1; i++;
        break;

    case BUZZ_ERROR:
        //repeat_play = 0;   /* play only once */
        tone[i] = 6; i++;
        tone[i] = 222; i++;
        tone[i] = 9; i++;
        tone[i] = 333; i++;
        tone[i] = 9; i++;
        tone[i] = 444; i++;
        break;

    case BUZZ_SINGLE:
        tone[i] = val;       i++;
        tone[i] = TONE_MUTE;   i++;
        break;

    case BUZZ_VALUE:
        tone[i] = val;  i++;
        tone[i] = val;  i++;
        tone[i] = val;  i++;
        tone[i] = val;  i++;
        tone[i] = val;  i++;
        break;

    default:
        //repeat_play = 0;     /* play only once */
        tone[i] = sound_type;  i++;
        tone[i] = TONE_MUTE;   i++;
        break;
    }
    /* call play tone */
    play_tone_array(tone, i, repeat_play);

    /* return play success */
    return 0;
}

//------------------------------------------------------------------------------
int buzzer_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int buzzer_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    unsigned char sound_type = 0;
    int opt;
    unsigned char t;
    int tone = 0;
    int mute = 0, force_play = 0;

    int map[] = {BUZZ_ERROR, BUZZ_OK, BUZZ_CONNECT,BUZZ_TRANSFER, BUZZ_RECEIVE, BUZZ_STOP, BUZZ_DONE, BUZZ_WAIT, BUZZ_ACT, BUZZ_ACT};

    /* initial option table */
    const struct option long_option[] =
    {
        { "act",   no_argument, NULL, BUZZ_ACT },
        { "ok",    no_argument, NULL, BUZZ_OK },
        { "error", no_argument, NULL, BUZZ_ERROR },

        { "stop",  no_argument, NULL, BUZZ_STOP },
        { "wait",  no_argument, NULL, BUZZ_WAIT },
		{ "done",  no_argument, NULL, BUZZ_DONE },

		{ "connect", no_argument, NULL, BUZZ_CONNECT },
		{ "trans",   no_argument, NULL, BUZZ_TRANSFER },
		{ "receive", no_argument, NULL, BUZZ_RECEIVE },

		{ "single", required_argument, NULL, BUZZ_SINGLE },
		{ "value",  required_argument, NULL, BUZZ_VALUE },
		/* if need more tune, add on here */

        { NULL, 0, NULL, 0}
    };
    /* check if no arg */
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s (--act --stop, --error, --wait, --connect, --trans, --done)\n",
                argv[0]);
        return 0;
    }

    mute = get_buzzer_conf();

    /* get only one argument */
    if ((opt = getopt_long(argc, argv, "l:t:f:v:sAOESWDCTRVG", long_option, NULL)) != -1)
    {
        switch (opt) {
        case BUZZ_STOP:
            force_play = 1;
        case BUZZ_ACT:
        case BUZZ_OK:
        case BUZZ_ERROR:
        case BUZZ_WAIT:
        case BUZZ_DONE:
        case BUZZ_CONNECT:
        case BUZZ_TRANSFER:
        case BUZZ_RECEIVE:
            sound_type = opt;
            tone = 0;
            break;
        case 'f' :
            force_play = 1;
        case 'l' :
        case 't' :
            t = atoi(optarg);
            //printf("optarg %d\n", t);
            if (t < 9)
                sound_type = map[t];
            else
                return 0;
            break;

        case 's' :
            force_play = 1;
            sound_type = BUZZ_STOP;
            break;

        case 'v':
            sound_type = atoi(optarg);
            break;

        case BUZZ_VALUE:
            sound_type = BUZZ_VALUE;
            tone = atoi(optarg);
            break;

        case BUZZ_SINGLE :
            sound_type = atoi(optarg);
            break;

        default: /* invalid arguments, print the usage */
            printf("invalid arg %c\n", opt);
        }
    }

    if (mute == 1 && force_play == 0)
        return 0;
    play_sound(sound_type, tone);

    return 0;
}
