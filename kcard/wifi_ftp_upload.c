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
#include "buzzer.h"
//#include "kcard.h"
#if 1
//extern char *wifi_dir;
//extern char IMAGES_DIR[];
//extern char *Backup_autprintf;
//extern char *WSD_ACT_BEGIN_TOKEN;
//extern char *config_control_file;
//extern char *server_upload_control_file;
//extern char *receiver_control_file;
//extern char *sender_control_file;

typedef struct
{
    char ip[170];
    char user[35];
    char pwd[35];
} ftp;

ftp ftpconf;

void ftpsetting(void)
{
    read_wifiConfigFile("FTP Path", ftpconf.ip);
    read_wifiConfigFile("User Name", ftpconf.user);
    read_wifiConfigFile("Password", ftpconf.pwd);
}
int get_file_list(const char *dirname, struct dirent ***ret_namelist)
{
    int i, len;
    int count, used, allocated;
    DIR *dir;
    struct dirent *ent, *ent2;
    struct dirent **namelist = NULL;

    if ((dir = opendir(dirname)) == NULL)
    {
        printf("opendir %s fail\n", dirname);
        return -1;
    }
    /* get folder count */
    count = 0;
    while ((ent = readdir(dir)) != NULL)
    {
        count++;
    }
    closedir(dir);
    if (count == 0)
        return 0;

    used = 0;
    allocated = count;
    namelist = malloc(allocated * sizeof(struct dirent *));
    if (!namelist)
        goto error;
    dir = opendir(dirname);
    while ((ent = readdir(dir)) != NULL)
    {
        if (ent->d_name[0] == '.')
            continue;
        //printf("%s\n", ent->d_name);
        /* duplicate struct direct for this entry */
        len = offsetof(struct dirent, d_name) + strlen(ent->d_name) + 1;
        if ((ent2 = malloc(len)) == NULL)
            goto error;

        memcpy(ent2, ent, len);
        namelist[used++] = ent2;
    }
    closedir(dir);

    qsort(namelist, used, sizeof(struct dirent *),
          (int (*)(const void *, const void *)) alphasort);

    *ret_namelist = namelist;
    return used;

error:
    //closedir(dir);

    if (namelist)
    {
        for (i = 0; i < used; i++)
            free(namelist[i]);
        free(namelist);
    }
    return -1;
}
//-----------------------------------------------------------------------------------------------
int get_folder_list(const char *dirname, struct dirent ***ret_namelist)
{
    int i, len;
    int count, used, allocated;
    DIR *dir;
    struct dirent *ent, *ent2;
    struct dirent **namelist = NULL;
    char *name;

    if ((dir = opendir(dirname)) == NULL)
    {
        printf("opendir %s fail\n", dirname);
        return -1;
    }
    /* get folder count */
    count = 0;
    while ((ent = readdir(dir)) != NULL)
    {
        count++;
    }
    closedir(dir);
    if (count == 0)
        return 0;

    allocated = count;
    namelist = malloc(allocated * sizeof(struct dirent *));
    if (!namelist)
        goto error;
    used = 0;
    dir = opendir(dirname);
    while ((ent = readdir(dir)) != NULL)
    {
        //printf("%s\n", ent->d_name);
        name = ent->d_name;
        if (name[0] == '.')
            continue;
        /* duplicate struct direct for this entry */
        len = offsetof(struct dirent, d_name) + strlen(ent->d_name) + 1;
        if ((ent2 = malloc(len)) == NULL)
            goto error;

        memcpy(ent2, ent, len);
        namelist[used++] = ent2;
    }
    closedir(dir);
    qsort(namelist, used, sizeof(struct dirent *),
          (int (*)(const void *, const void *)) alphasort);
    *ret_namelist = namelist;
    return used;

error:
    //closedir(dir);

    if (namelist)
    {
        for (i = 0; i < used; i++)
            free(namelist[i]);
        free(namelist);
    }
    return -1;
}
//-----------------------------------------------------------------------------------------------
int ftp_folder(char *dir_path, char *remote_path,char *user, char *pwd, char *ip)
{
    struct dirent **file_list;
    char file_path[1024];
    char ftp_path[1024];
    char ftp_cmd[4096];
    int i, count;

    count = get_file_list(dir_path, &file_list);

    if (count > 0 && file_list != NULL)
    {
        for (i = 0; i < count; i++)
        {
            if (file_list[i])
            {

                sprintf(file_path, "%s/%s", dir_path, file_list[i]->d_name);
                sprintf(ftp_path, "/sd/DCIM/123_FTP/%s", file_list[i]->d_name);
                sprintf(ftp_cmd, "ftpput -v -u %s -p %s %s %s %s", user, pwd, ip, ftp_path, file_path);
                //sprintf(ftp_cmd, "ftpput -v  %s %s %s", ip, ftp_path, file_path);
                printf("%s\n", ftp_cmd);
                system(ftp_cmd);
                free(file_list[i]);
            }
        }

        free(file_list);
    }
    return 0;
}
//-----------------------------------------------------------------------------------------------
// get /mnt/sd/DCIM folder list
// skip control image folder, and upload folder
// upload these folder
int ftp_all_folders(char *local_path, char *remote_path, char *user, char *pwd, char *ip)
{
    struct dirent **folder_list;
    struct dirent **file_list;
    char dir_path[256];
    int i, n;
    char skip_p0[] = "199_WIFI";
    char skip_p1[] = "123_FTP";

    n = get_folder_list(local_path, &folder_list);
    if (n > 0 && folder_list != NULL)
    {
        for (i = 0; i < n; i++)
        {
            if (strcmp(skip_p0, folder_list[i]->d_name) == 0)
                continue;
            if (strcmp(skip_p1, folder_list[i]->d_name) == 0)
                continue;

            printf("ftp put folder %s\n", folder_list[i]->d_name);
            sprintf(dir_path, "%s/%s", local_path, folder_list[i]->d_name);
            ftp_folder(dir_path, remote_path, user, pwd, ip);
        }

        for (i = 0; i < n; i++)
        {
            free(folder_list[i]);
        }

        free(folder_list);
    }
    return 0;
}
#endif



int wifi_ftp_upload_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_ftp_upload_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    
    //char remote_path[256];
    char ftp_cmd[4096];
    play_sound (CONNECTION,1);
    ftpsetting();

    //sprintf(remote_path, "/home/%s", ftpconf.user);
   // ftp_all_folders("/mnt/sd/DCIM", ".", ftpconf.user, ftpconf.pwd,ftpconf.ip);
    //ftp_all_folders("/mnt/sd/DCIM", " ", ftpconf.user, ftpconf.pwd,ftpconf.ip);
    
      sprintf(ftp_cmd, "ncftpput -R -z -u %s -p %s  %s . /mnt/sd/DCIM/*", ftpconf.user, ftpconf.pwd, ftpconf.ip);
                //sprintf(ftp_cmd, "ftpput -v  %s %s %s", ip, ftp_path, file_path);
    printf("%s\n", ftp_cmd);

    system(ftp_cmd);
				
    printf("FTP uplaod success\n");
	
    play_sound (FINISH,1);

    sleep(5);
    play_sound (STOP_BUZZER,1);

		
    return 0;
}

int wifi_quick_send_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_quick_send_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    play_sound (CONNECTION,1);
    ftpsetting();

    ftp_all_folders("/mnt/sd/DCIM", "/mnt/sd/DCIM/123_FTP", "guest", "guest", "192.168.1.1");
	//ftp_all_folders("/mnt/sd/DCIM", "/sd/DCIM/123_FTP/", ftpconf.user,ftpconf.pwd, "192.168.1.1");

    printf("Quick Send done\n");
    play_sound (FINISH,1);
	sleep(5);
    play_sound (STOP_BUZZER,1);
	
    return 0;
}
