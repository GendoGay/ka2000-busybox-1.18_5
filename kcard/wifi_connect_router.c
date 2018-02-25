#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* busybox common include file */
#include "libbb.h"

/* kcard wsd common include file */
#include "common.h"

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_DEBUG 1

#define LOG_INFO(args...)	fprintf(stderr,"WIFI_CONN: " args)
#define LOG_ERR(args...)	fprintf(stderr,"WIFI_CONN: " args)
#define LOG_DEBUG(args...)   {if (_log_level >= LOG_LEVEL_DEBUG) fprintf(stderr,"WIFI_CONN: "args);}

static int _log_level = LOG_LEVEL_INFO;

//------------------------------------------------------------------------------
wifi_ssid_t iwlist_ap_list[100];
int iwlist_ap_count = 0;

static int check_essid(char *essid);
//------------------------------------------------------------------------------
static int search_ch(char *buf, char c, int from, int to)
{
    int i;
    for (i = from; i < to; i++)
    {
        if (buf[i] == c)
		{
			//printf(".%c", buf[i]);
            return i;
		}
    }
    return -1;
}
//------------------------------------------------------------------------------
static int search_str(char *buf, const char *token, int from, int to)
{
    int n = strlen(token);
    int p1 = from;
	int pos = from;
    int i;

    for (i = 0; i < n; i++)
    {
		//printf("search %d %d, %c, %d\n", from, to, token[i], n);
		pos = search_ch(buf, token[i], p1, to);
		//printf("p1 %d, p2 %d\n", p1, pos);
        if (pos == -1)
            return -1;
		if (i < n - 1 && buf[pos+1] != token[i+1])
			i = 0;
        from = pos + 1;
		p1 = pos + 1;
        if (from >= to)
            return -1;
    }
	//printf("found %s at %d\n", token, pos - n);
	return pos - n;
}
//------------------------------------------------------------------------------
static int search_str_end(char *buf, char *str, int from, int to)
{
    int i;
	int n = 0;
	char c;

    for (i = from; i < to; i++)
    {
		c = buf[i];
		if (c != '\"')
		{
			str[n] = c;
			n++;
		}
        if (c == '\0' || c =='\r' || c== '\n' )
		{
			str[n-1] = 0;
            return n;
		}
    }
    return n;
}
//------------------------------------------------------------------------------
static int get_next_token_value(char *buf, const char *token, char *value, int from, int to)
{
	int pos, len, n;
	n = strlen(token);
    pos = search_str(buf, token, from, to); /* strstr(largestring, smallstring); */
	if (pos == -1)
		return -1;

	len = search_str_end(buf, value, pos+n+2, to);
    //printf("[%s] = [%s]\n", token, value);
	return pos + len;
}


//------------------------------------------------------------------------------
static int get_all_essid(char *buf, int size)
{
	int pos = 0;
	int i = 0;
	char str[64];
	int from = 0, to = size;

	i = iwlist_ap_count;
	LOG_INFO("Append list from index %d, get_all_essid %d -> %d\n", i, from, to);

	while (from < to) {
		/* start from Cell */
		pos = get_next_token_value(buf, "Cell", str, from, size);
		if (pos == -1)
			return i;

		to = get_next_token_value(buf, "Cell", str, pos + 5, size);
		if (to == -1)
			to = size;

		from = pos;

		/* get ssid */
		pos = get_next_token_value(buf, "ESSID", iwlist_ap_list[i].ssid, from, to);
		if (pos == -1) 
			return i;


		if (check_essid(iwlist_ap_list[i].ssid) != -1) {
			LOG_DEBUG("%s: Skip duplicated AP %s\n", __FUNCTION__, iwlist_ap_list[i].ssid);
			from = from + 100;
			continue;
		}
			
		LOG_INFO("ESSID  [%s], cell %d -> %d\n", iwlist_ap_list[i].ssid, from, to);

		from = from + 100;

		/* ap mode 1 or ad-hoc mode 2 */
		pos = get_next_token_value(buf, "Mode", str, from, to);
		if (pos != -1)
		{
			if (strcmp(str, "Master") == 0)
			{
				iwlist_ap_list[i].ap_mode = MODE_MASTER;
			}
			else
			{
				iwlist_ap_list[i].ap_mode = MODE_ADHOC;
			}
		}

		/* check if with security code */
		pos = get_next_token_value(buf, "Encryption key", str, from, to);
		if (pos != -1)
		{
			if (strcmp(str, "on") == 0)
			{
				iwlist_ap_list[i].enc_key = 1;
				iwlist_ap_list[i].enc_type = ENC_WEP;

				//int enc_type;  // wep:0 wpa:1 wpa2:2
				pos = search_str(buf, "WPA Version", from, to);
				if (pos != -1) {
					LOG_INFO("WPA Version\n");
					iwlist_ap_list[i].enc_type = ENC_WPA;
				}
				else
				{
					pos = search_str(buf, "WPA2 Version", from, to);
					if (pos != -1) {
						LOG_INFO("WPA2 Version\n");
						iwlist_ap_list[i].enc_type = ENC_WPA2;
					}
					else
					{
						iwlist_ap_list[i].enc_type = ENC_WEP;
					}
				}

				if (iwlist_ap_list[i].enc_type != ENC_WEP) {
					pos = get_next_token_value(buf, "Group Cipher :", str, from, to);
					if (pos != -1) {
						if (strncmp(str, "TKIP", 4) == 0) {
							LOG_INFO("Cipher: TKIP\n");
							iwlist_ap_list[i].enc_group = CIPHER_TKIP;
						} else if (strncmp(str,"CCMP", 4) == 0) {
							LOG_INFO("Cipher: CCMP\n");
							iwlist_ap_list[i].enc_group = CIPHER_CCMP;
						} else {
							LOG_INFO("Unknwo Group Cipher %s\n",str);
						}
					} else { 
						LOG_INFO("Can't find Group Cipher in encryption mode\n");
					}
					pos = get_next_token_value(buf, "Pairwise Ciphers", str, from, to);
					if (pos != -1) {
						if (strstr(str, "CCMP")) {
							/* CCMP have higher
							 * priority over TKIP */
							LOG_INFO("PairCipher: CCMP\n");
							iwlist_ap_list[i].enc_pair = CIPHER_CCMP;
						} else {
							if (strstr(str, "TKIP")) {
								LOG_INFO("PairCipher: TKIP\n");
								iwlist_ap_list[i].enc_pair = CIPHER_TKIP;
							} else {
								LOG_INFO("Unknown Pairwise Cipher %s\n",str);
							}
						} 	
					} else { 
						LOG_INFO("Can't find Pairwise Cipher in encryption mode\n");
					}
				}
			} else {
				iwlist_ap_list[i].enc_type = ENC_OFF;
			}
		}


		from = from + 100;
		i++;
	}

	return i;
}
//------------------------------------------------------------------------------
static int check_essid(char *essid)
{
	int i = 0;

	for (i = 0; i < iwlist_ap_count; i++)
	{
		//printf("%s, %s, %d\n", iwlist_ap_list[i].ssid, essid, strcmp(iwlist_ap_list[i].ssid, essid));
		if (strcmp(iwlist_ap_list[i].ssid, essid) == 0) {
			LOG_DEBUG("found %s at %d\n", iwlist_ap_list[i].ssid, i);
			return i;
		}
	}

	return -1;
}
//------------------------------------------------------------------------------
static int read_iwlist(const char *file_name)
{
    FILE *fp;
    char *buffer;
    int size;

    fp = fopen(file_name, "r");
    if (fp == NULL) {
        LOG_ERR("error: open %s!\n", file_name);
        return -1;
    }

    /* get file size */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    LOG_INFO("size %d\n", size);

    /* allocate space */
    buffer = (char *)malloc(size);
    if (buffer == NULL) {
        LOG_ERR("buffer alloc fail\n");
        fclose(fp);
        return -1;
    }
    memset(buffer, 0, size);

    /* read whole file */
	fseek(fp, 0, SEEK_SET);
    fread(buffer, size, 1, fp);
    fclose(fp);

	iwlist_ap_count = get_all_essid(buffer, size);

	if (buffer != NULL)
		free(buffer);

    return iwlist_ap_count;
}
static void print_wpa_conf_header(void)
{
    FILE *fp;
    if (!(fp = fopen ("/etc/wpa.conf", "w"))) {
        printf ("fopen /etc/wpa.conf fail!\n");
        return;
    }
    fprintf(fp, "ap_scan=2\n");
//    fprintf(fp, "ctrl_interface=DIR=/var/run/wpa_supplicant \n");
    fclose(fp);
}
//------------------------------------------------------------------------------
static void print_wpa_conf( wifi_ssid_t *cur_ap)
{
	FILE *fp;

	if (!(fp = fopen ("/etc/wpa.conf", "a"))) {
		printf ("fopen /etc/wpa.conf fail!\n");
		return;
	}

	fprintf (fp, "network={\n");
	fprintf (fp, "	ssid=\"%s\"\n", cur_ap->ssid);
	fprintf (fp, "	key_mgmt=WPA-PSK\n");

	if (cur_ap->enc_type == ENC_WPA2) {
		fprintf (fp, "	proto=WPA2\n");
	} else if (cur_ap->enc_type == ENC_WPA) {
		fprintf (fp, "	proto=WPA\n");
	}

	if (cur_ap->enc_group == CIPHER_CCMP) {
		fprintf (fp, "	group=CCMP\n");
	} else if (cur_ap->enc_group == CIPHER_TKIP) {
		fprintf (fp, "	group=TKIP\n");
	}

	if (cur_ap->enc_pair == CIPHER_CCMP) {
		fprintf (fp, "	pairwise=CCMP\n");
	} else if (cur_ap->enc_pair == CIPHER_TKIP) {
		fprintf (fp, "	pairwise=TKIP\n");
	}

	fprintf (fp, "	psk=\"%s\"\n", cur_ap->key); // ASCII
	fprintf (fp, "}\n");
	fclose (fp);

	//system("cat /etc/wpa.conf");
}

//------------------------------------------------------------------------------
static void print_wep_conf(wifi_ssid_t * cur_ap)
{
    FILE *fp;

    if (!(fp = fopen ("/etc/wpa.conf", "a"))) {
        printf ("fopen /etc/wpa.conf fail!\n");
        return;
    }
                   
    fprintf (fp, "network={\n");
    fprintf (fp, "	ssid=\"%s\"\n", cur_ap->ssid);
    //if (cur_ap->ap_mode != MODE_ADHOC) // Ad-hoc
   //     fprintf (fp, "	mode=1\n");
    fprintf (fp, "	key_mgmt=NONE\n");
    if (strlen(cur_ap->key) == 5 || strlen(cur_ap->key) == 13 || strlen(cur_ap->key) == 29) // ASCII
        fprintf (fp, "	wep_key0=\"%s\"\n", cur_ap->key);
    else // hexadecimal digits
        fprintf (fp, "	wep_key0=%s\n", cur_ap->key);
    fprintf (fp, "  wep_tx_keyidx=0\n");
    fprintf (fp, "}\n");
    fclose (fp);

	//system("cat /etc/wep.conf");
}
static void print_open_conf(wifi_ssid_t * cur_ap) 
{
    FILE *fp;

    if (!(fp = fopen ("/etc/wpa.conf", "a"))) {
        printf ("fopen /etc/wpa.conf fail!\n");
        return;
    }

    fprintf (fp, "network={\n");
    fprintf (fp, "	ssid=\"%s\"\n", cur_ap->ssid);
    fprintf (fp, "	key_mgmt=NONE\n");
    fprintf (fp, "}\n");
    fclose (fp);
}
//------------------------------------------------------------------------------
static void write_ap_config(wifi_ssid_t * cur_ap)
{
    if (cur_ap->enc_type == ENC_WEP)  // wep
    {
		//printf("write_ap_config wep\n");
        print_wep_conf(cur_ap);
    } else if (cur_ap->enc_type == ENC_OFF) {
        print_open_conf(cur_ap);
    }
    else //wpa wpa2
    {
		//printf("write_ap_config wpa\n");
        print_wpa_conf(cur_ap);
    }
}
//------------------------------------------------------------------------------
static void connect_ap(wifi_ssid_t *ap)
{
	char cmd[64];

	memset(cmd, 0, 64);
	sprintf(cmd, "wpa_supplicant -i mlan0 -B -c /etc/wpa.conf");
	system(cmd);

#if 0
	if (ap->enc_key == 1)
	{
		sprintf(cmd, "iwconfig mlan0 key %s", ap->key);
		system(cmd);
		//printf(cmd);
		sprintf(cmd, "iwconfig mlan0 essid \"%s\"", ap->ssid);
		system(cmd);
		//printf(cmd);
		if (ap->enc_type == ENC_WEP) 
			sprintf(cmd, "wpa_supplicant -i mlan0 -B -c /etc/wpa.conf");
		else
			sprintf(cmd, "wpa_supplicant -i mlan0 -B -c /etc/wpa.conf");
		system(cmd);
		//printf(cmd);
	}
	else
	{
		sprintf(cmd, "iwconfig mlan0 essid \"%s\"", ap->ssid);
		system(cmd);
	}
#endif
}
//------------------------------------------------------------------------------
/* iwlist mlan0 scanning essid [SSID] */
/*
[AP]
AP_ACCOUNT : 1
SSID : AirStation
Key : 12345678
*/
static int scan_preconfig_ap(wifi_ssid_t *ap)
{
    char value[64];
    int ap_count = 0;
    int file_pos = 0;
    int i, ap_index;
    int match_flag = 0;

    file_pos = check_next_config(0, "AP_ACCOUNT", value);
    if (file_pos == -1) {
        printf("can't get AP_ACCOUNT\n");
        return -1;
    }

    if (value[0] != '\0') {
        ap_count = atoi(value);
    }

    if (ap_count == 0) {
        printf("no preset AP\n");
        return -1;
    }

    system("iwlist mlan0 scan > /tmp/iw.lst");
    read_iwlist("/tmp/iw.lst");
    sleep(1);
    system("iwlist mlan0 scan > /tmp/iw.lst");
    read_iwlist("/tmp/iw.lst");
    sleep(2);
    system("iwlist mlan0 scan > /tmp/iw.lst");
    read_iwlist("/tmp/iw.lst");
    sleep(3);
    system("iwlist mlan0 scan > /tmp/iw.lst");
    read_iwlist("/tmp/iw.lst");

	if (ap_count > 0) {
		print_wpa_conf_header();
	}

    for(i = 0; i < ap_count; i++) {
		file_pos = check_next_config(file_pos, "SSID", ap->ssid);
		file_pos = check_next_config(file_pos, "Key",  ap->key);

		ap_index = check_essid(ap->ssid);
		if (ap_index != -1) {
			ap->ap_mode = iwlist_ap_list[ap_index].ap_mode;
			ap->enc_type = iwlist_ap_list[ap_index].enc_type;
			ap->enc_key = iwlist_ap_list[ap_index].enc_key;
			ap->enc_group = iwlist_ap_list[ap_index].enc_group;
			ap->enc_pair = iwlist_ap_list[ap_index].enc_pair;

			printf("AP:ssid=%s, key=%s, mode=%d, enc_type=%d group %d pair %d\n", 
					ap->ssid, ap->key, ap->ap_mode, ap->enc_type,
					ap->enc_group,ap->enc_pair);

			/* output to wep.conf or wpa.conf */
			write_ap_config(ap);

			match_flag = 1;
		}
	}
	if (match_flag) {
		connect_ap(ap);
		return 1;
	}

    return 0;
}
//------------------------------------------------------------------------------
static int auto_connect_other_ap(wifi_ssid_t *ap)
{
	int i = 0;

	for (i = 0; i < iwlist_ap_count; i++) {
		if (iwlist_ap_list[i].enc_key == 0) {
		    memcpy(ap, &iwlist_ap_list[i], sizeof(wifi_ssid_t));

			printf("auto connect %s at %d\n", iwlist_ap_list[i].ssid, i);
			connect_ap(&iwlist_ap_list[i]);
			return i;
		}
	}

	return -1;
}
//------------------------------------------------------------------------------
int ap_connector(void) 
{
	wifi_ssid_t ap;
	int try_count = 0;

	/* clean up */
	memset(&ap, 0, sizeof(wifi_ssid_t));
	memset(iwlist_ap_list, 0, sizeof(iwlist_ap_list));

	try_count++;
	while (scan_preconfig_ap(&ap) == 0) {
		sleep(3);

		if (try_count >= 3) 
			return -1;

		try_count++;
	}

	//auto_connect_other_ap(&ap);
	//return ap.enc_type;
	return 0;
}
//------------------------------------------------------------------------------
int wifi_connect_router_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wifi_connect_router_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    return ap_connector();
}
//------------------------------------------------------------------------------
