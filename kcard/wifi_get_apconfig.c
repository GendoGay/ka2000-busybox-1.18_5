#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* busybox common include file */
#include "libbb.h"

/* kcard wsd common include file */
#include "common.h"


int get_config_ap(int index,wifi_ssid_t *ap)
{
    char value[64];
    int ap_count = 0;
    int file_pos = 0;
    int i, ap_index;

    file_pos = check_next_config_real(0, "AP_ACCOUNT", value);
    if (file_pos == -1)
    {
        fprintf(stderr,"can't get AP_ACCOUNT\n");
        return -1;
    }

    if (value[0] != '\0')
    {
        ap_count = atoi(value);
    }

    if (ap_count == 0 || index > ap_count)
    {
        fprintf(stderr,"no preset AP\n");
        return -1;
    }

    for(i = 0; i < ap_count; i++)
    {
        file_pos = check_next_config_real(file_pos, "SSID", ap->ssid);
        file_pos = check_next_config_real(file_pos, "Key",  ap->key);

		if (i == index)
			break;
    }

    return 0;
}
int get_config_apcount(int * apcount)
{
    char value[64];
    int ap_count = 0;
    int file_pos = 0;
    int i, ap_index;

    file_pos = check_next_config_real(0, "AP_ACCOUNT", value);
    if (file_pos == -1)
    {
        fprintf(stderr,"can't get AP_ACCOUNT\n");
        return -1;
    }

    if (value[0] != '\0')
    {
        ap_count = atoi(value);
    }

	*apcount = ap_count;

    return 0;
}

//------------------------------------------------------------------------------
int wifi_get_apconfig_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_get_apconfig_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    char *get_parm=NULL;
    char temp_buffer[100];
    FILE *cmd_fp;
	int ap_count;
	int ret;
	wifi_ssid_t ap;
	int index;

    int cmd = getopt(argc, argv, "i:"); // option -c <parameter name>

    if (cmd == -1) {
		ret = get_config_apcount(&ap_count);
		if (ret != 0) {
			fprintf(stderr,"%s: can't find ap count\n",__FUNCTION__);
			return -1;
		} else {
			printf("%d\n",ap_count);
			return 0;
		}
	}

	if (cmd == 'i')
	{
		// read config to temp buffer and print out parameter
		get_parm = optarg;
		index = atoi(get_parm);
		ret = get_config_ap(index,&ap);
		if (ret == 0) {
			printf("SSID=%s\n",ap.ssid);
			printf("KEY=%s\n",ap.key);
		} else { 
			fprintf(stderr,"%s: can't find ap index %d\n",__FUNCTION__,index);
			return -1;
			return 0;
		}

	}
	return -1;

}
//------------------------------------------------------------------------------
