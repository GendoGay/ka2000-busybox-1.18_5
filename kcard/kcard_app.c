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
#include "kcard.h"

#define WSD_ACT_WEBCONF      0x01
#define WSD_ACT_UPLOAD       0x02
#define WSD_ACT_SERVER       0x03
#define WSD_ACT_SEND         0x04
//#define WSD_ACT_TOKEN  "PRT QTY"
//#define WSD_ACT_BEGIN_TOKEN "<IMG SRC"
//#define SELECTIONS_PRN_PATH "/mnt/sd/MISC/AUTPRINT.MRK"

#define KCARD_SIG_DPOF         6
#define KCARD_SIG_UPDATED      7
#define KCARD_SIG_SCAN         9
#define KCARD_SIG_DEL_IMG_0    1
#define KCARD_SIG_DEL_IMG_1    2
#define KCARD_SIG_DEL_IMG_2    3
#define KCARD_SIG_DEL_IMG_3    4
#define KCARD_SIG_NO_IMGS      8
#define KCARD_SIG_SLEEP       99
#define KCARD_SIG_WAKEUP     100


#define KCARD_CIMG1 "WSD00001.JPG"
#define KCARD_CIMG2 "WSD00002.JPG"
#define KCARD_CIMG3 "WSD00003.JPG"
#define KCARD_CIMG4 "WSD00004.JPG"
#define KCARD_WIFI_DIR "/mnt/sd/DCIM/199_WIFI/"
#define DCIM_DIR        "/mnt/sd/DCIM/"
#define KCARD_IMG_TOKEN "<IMG SRC"

char   config_control_file[]		= KCARD_CIMG1;
char   server_upload_control_file[] = KCARD_CIMG2;
char   receiver_control_file[]   	= KCARD_CIMG3;
char   sender_control_file[]     	= KCARD_CIMG4;
char   wifi_dir[] = KCARD_WIFI_DIR;
char   IMAGES_DIR[20] = DCIM_DIR;
char   WSD_ACT_BEGIN_TOKEN[]         = KCARD_IMG_TOKEN;

/*
char   *config_control_file		= "WSD0CONF.JPG";
char   *server_upload_control_file = "WSD1UPLD.JPG";
char   *receiver_control_file   	= "WSD2RECV.JPG";
char   *sender_control_file     	= "WSD3SEND.JPG";
char *wifi_dir = "/mnt/sd/DCIM/199_WIFI/";
*/

struct control_info
{
    int func;
    int exist;
    int triggered;
    char *source;
    char *target;
    char *name;
    char *info;
    char *act0;
    char *act1;
};

struct control_info control_image[4] =
{
    {.exist=0, .func=WSD_ACT_WEBCONF,.act0="w3", .act1="wifi_setup_app",        .name="WSD00001.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00001.JPG",           .target="/mnt/sd/DCIM/199_WIFI/WSD00001.JPG", .info="Web Config\n",     .triggered=0},
    {.exist=0, .func=WSD_ACT_UPLOAD, .act0="w2", .act1="wifi_ftp_upload",       .name="WSD00002.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00002.JPG",           .target="/mnt/sd/DCIM/199_WIFI/WSD00002.JPG", .info="FTP Upload\n",          .triggered=0},
    {.exist=0, .func=WSD_ACT_SERVER, .act0="p2p_server", .act1="wifi_ftp_server",   .name="WSD00003.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00003.JPG", .target="/mnt/sd/DCIM/199_WIFI/WSD00003.JPG", .info="P2P Server\n",  .triggered=0},
    {.exist=0, .func=WSD_ACT_SEND,   .act0="p2p_client", .act1="wifi_quick_send",       .name="WSD00004.JPG", .source="/home/sd/DCIM/199_WIFI/WSD00004.JPG",   .target="/mnt/sd/DCIM/199_WIFI/WSD00004.JPG", .info="P2P Send\n",    .triggered=0},
};


typedef enum{
  KCARD_RESCAN_IMAGES,
  KCARD_ENABLE_USERCALL,
  KCARD_DISABLE_USERCALL,
  KCARD_ENABLE_SLEEP,
  KCARD_DISABLE_SLEEP,
  KCARD_SET_PID,
  KCARD_GET_CMD,
  KCARD_WAKEUP,
  KCARD_GO_SLEEP,
  KCARD_DEL_PROGRAM_BIN,
  KCARD_SET_USERCALL_INTERVAL,
  KCARD_TRIGGER_FULL_RANGE,
  KCARD_STOP_CTRL_IMG_CHECK,
  }driver_contl_type;

//------------------------------------------------------------------------------
//  kcard Kcard_app function
//------------------------------------------------------------------------------
//~~~~~~these function used by other files
//-------------------------------------------------------------------------------
void IO_control(driver_contl_type type, short* res_val)
{
    int fd,reg;

    switch(type)
    {
    case KCARD_RESCAN_IMAGES:
        reg =  0x13;
        break;
    case KCARD_ENABLE_USERCALL:
        reg =  0x11;
        break;
    case KCARD_DISABLE_USERCALL:
        reg =  0x10;
        break;
    case KCARD_ENABLE_SLEEP:
        reg =  0x21;
        break;
    case KCARD_DISABLE_SLEEP:
        reg =  0x20;
        break;
    case KCARD_SET_PID:
        reg =  0x16;
        break;
    case KCARD_GET_CMD:
        reg =  0x17;
        break;
    case KCARD_WAKEUP:
        reg =  0x100;
        break;
    case KCARD_GO_SLEEP:
        reg =  0x99;
        break;
    case KCARD_DEL_PROGRAM_BIN:
        reg =  0x14;
        break;
    case KCARD_SET_USERCALL_INTERVAL:
        reg =  0x12;
        break;
    case KCARD_TRIGGER_FULL_RANGE:
        reg = 0x23;
        break;
    case KCARD_STOP_CTRL_IMG_CHECK:
        reg =  0x22;
        break;
    default:
        printf("nothing");
        break;
    }
    fd = open ("/dev/ka-main", O_RDWR);
    if (fd<0)
    {
        printf ("err open /dev/ka-main\n");
        return ;
    }

    ioctl(fd,reg,res_val);
    close(fd);
}
//------------------------------------------------------------------------------
//~~~~~~these function used by other files ,just test kcard_app function for 18.3
//~~~~~~
void enable_kcard_call(void)
{
    IO_control(KCARD_ENABLE_USERCALL,NULL);
}
void disable_kcard_call(void)
{
    IO_control(KCARD_DISABLE_USERCALL,NULL);
}
void set_kcard_interval(int usercall_interval)
{
	int fd;

	fd = open ("/dev/ka-main", O_RDWR);
	if (fd<0)
	{
		printf ("err open /dev/ka-main\n");
		return ;
	}

	ioctl(fd,KCARD_SET_USERCALL_INTERVAL,usercall_interval);

	close(fd);
}
void enable_power_sleep(void)
{
    IO_control(KCARD_ENABLE_SLEEP,NULL);
}

void disable_power_sleep(void)
{
    IO_control(KCARD_DISABLE_SLEEP,NULL);
}
//~~~~~~

int check_control_image(void)
{
    int i;
    int cp = 0;
    for (i = 0; i < 4; i++)
    {
        if (control_image[i].exist == 0)
        {
            copy_file(control_image[i].source, control_image[i].target, FILEUTILS_FORCE);
            cp++;
        }
    }

    if (cp)
    {
        sync(); //force superblock are updated immediately

        system("umount /mnt/sd && mount -t vfat --shortname=winnt /dev/mmcblk0p1 /mnt/sd");
    }

    for (i = 0; i < 4; i++)
    {
        if (control_image[i].exist == 0)
        {
            return (control_image[i].func);
        }
    }
    return 0;
}

int check_dir_exist(char *fileName)
{
    DIR *dirp = opendir(fileName);

    if (dirp == NULL)
    {
        return 0;
    }
    closedir(dirp);
    return 1;
}
//------------------------------------------------------------------------------
int check_and_create_path(char *path)
{
    char cmd[1024];
    if (check_dir_exist(path) == 0)
    {
        sprintf(cmd, "mkdir -p %s", path);
        system(cmd);
    }
	return 1;
}
//------------------------------------------------------------------------------
int check_ctrl_folder_exist(void)
{

    int i;
    system("buzzer -f 2");
    //system("mkdir -p /mnt/sd/DCIM/199_WIFI");
    //system("mkdir -p /mnt/sd/MISC");
    //system("mkdir -p /mnt/sd/DCIM/101_WIFI");
    //system("cp /mnt/mtd/config/cimgconf /mnt/sd");
    system("factory_reset.sh");
    printf("dir %s not exist\n", wifi_dir);
    sync(); //force superblock are updated immediately

    if (check_dir_exist(wifi_dir) == 0)//floder not exist
        sleep(2);

    printf("copy\n");
    for (i = 0; i < 4; i++)
    {
        copy_file(control_image[i].source, control_image[i].target, FILEUTILS_FORCE);
        control_image[i].exist = 1;
    }

    system("sleep 0.2 && chmod 666 /mnt/sd/DCIM/199_WIFI/*");
    printf("sync\n");
    sync(); //force superblock are updated immediately
    printf("refresh\n");
    system("sleep 0.2 && umount /mnt/sd && sleep 0.2 && mount -t vfat --shortname=winnt /dev/mmcblk0p1 /mnt/sd");
    system("buzzer -f 5");
    printf("done\n");
    return 0;
}

//-----------------------------------------------------------------------
static int is_file_exist (char *path)
{
    FILE* fp = fopen(path, "r");
    if (fp) {
        // file exists
        fclose(fp);
        return 1;
    } else {
        // file doesn't exist
        return 0;
    }
}
//-----------------------------------------------------------------------
int kcard_app_act(int cmd)
{
    int img_ctrl_cmd = 0;

    IO_control(KCARD_DISABLE_USERCALL,NULL);
    system("refresh_sd");
    sleep(3);
    img_ctrl_cmd = cmd;         
    if (img_ctrl_cmd == KCARD_SIG_UPDATED)
    {                                   
        IO_control(KCARD_ENABLE_USERCALL,NULL);        
        return 0;
    }
    else if (img_ctrl_cmd == KCARD_SIG_SCAN)
    {
        check_ctrl_folder_exist();
        sleep(3);
        IO_control(KCARD_ENABLE_USERCALL,NULL);
        return 0;
    }
    else if (img_ctrl_cmd == KCARD_SIG_NO_IMGS)
    {
        if (check_ctrl_folder_exist() == 0)
        {
            sleep(3);
            IO_control(KCARD_ENABLE_USERCALL,NULL);
            return 0;
        }
        img_ctrl_cmd = check_control_image();
        //return 0;
    }
    img_ctrl_cmd++;
    if(img_ctrl_cmd==KCARD_SIG_DEL_IMG_0||
            img_ctrl_cmd==KCARD_SIG_DEL_IMG_1||
            img_ctrl_cmd==KCARD_SIG_DEL_IMG_2||
            img_ctrl_cmd==KCARD_SIG_DEL_IMG_3)
    {
        img_ctrl_cmd--;
        
        printf("kcard_app_act_cmd %d\n",img_ctrl_cmd);       
        int i = img_ctrl_cmd;		
        if (!is_file_exist(control_image[i].target))
        {
            copy_file(control_image[i].source, control_image[i].target, FILEUTILS_FORCE);
            system("sync");  
			
        }
        if( img_ctrl_cmd != -1 )
        {
            printf (control_image[img_ctrl_cmd].act1);
            if(system(control_image[img_ctrl_cmd].act0)==-1)
                printf("failed to call act0\n");

            if(img_ctrl_cmd > 0)
            {
                if(system(control_image[img_ctrl_cmd].act1)==-1)
                    printf("failed to call act1\n");
            }
            control_image[img_ctrl_cmd].triggered = 0;
        }
    }
    sleep(3);
    IO_control(KCARD_ENABLE_USERCALL,NULL);
    return 0;
}
void null(void)
{
    printf("hello");
}
void kcard_program(int sig)
{
    short cmd = 0;
    int skip = 0;

    /* set NULL sig handler avoid */
    (void) signal(SIGUSR1, null);
    IO_control(KCARD_GET_CMD,&cmd);
    printf("get_Kcard_app cmd %d\n", cmd);

    if(cmd==KCARD_SIG_SLEEP || cmd==KCARD_SIG_WAKEUP)
    {
        printf("Re-setup the signal handler");
    }
    else if(cmd >= KCARD_SIG_DEL_IMG_0&& cmd <= KCARD_SIG_DEL_IMG_3)
    {
    cmd--;
        if (control_image[cmd].triggered)
            skip = 1;
        control_image[cmd].triggered = 1;

        if (!skip)
        {
            kcard_app_act(cmd);
            if(cmd == KCARD_SIG_NO_IMGS)
                IO_control(KCARD_RESCAN_IMAGES, NULL);
        }
        else
        {
            system ("refresh_sd");
            sleep(3);
        }
    }
    else
    {
        kcard_app_act(cmd);
        if(cmd == KCARD_SIG_NO_IMGS)
            IO_control(KCARD_RESCAN_IMAGES, NULL);
    }

    (void) signal(SIGUSR1, kcard_program);

}
//-------------------------------------------------------------------------
int kcard_app_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int kcard_app_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    pid_t  pid = getpid();

    /* set sig handler */
    (void) signal(SIGUSR1, kcard_program);
    printf("set pid %d\n", (int)pid);
    IO_control(KCARD_SET_PID,&pid);
    //IO_control(KCARD_STOP_CTRL_IMG_CHECK, NULL);
    //IO_control(KCARD_TRIGGER_FULL_RANGE, NULL);

    /* start the main loop */
    while (1)
    {
        //printf("waitting for sig");
        pause();
    }
}
