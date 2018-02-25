
#ifndef __COMMON_H__
#define __COMMON_H__

FILE *open_wsd_config();
int check_next_config(int start_pos, const char find[], char value1[]);
int ReadConfigFile(char find[], char value1[]);
int config_wifi_adhoc(void);
int config_wifi_managed(void);
int checkStat(void);
int getKeyValue(char find[],char line[], char value[]);
int readConfigFile(char find[], char value1[]);
int getKeyValue1(char find[], char line[], char value[]);
void remove_newline(char* str);
int printLogin(void);

/*-----kcard_app.c */
void enable_kcard_call(void);
void disable_kcard_call(void);
void set_kcard_interval(int usercall_interval);
void enable_power_sleep(void);
void disable_power_sleep(void);

enum {
	NONE=0,
	INTERNET_EXPLORER=1,
	FIREFOX=2,
	SAFARI=3,
	CHROME=4,
	ANDROID=5,
}; // browser type

//---------------------------------------
#define MODE_ADHOC  2
#define MODE_MASTER 1

#define ENC_WEP    0
#define ENC_WPA    1
#define ENC_WPA2   2
#define ENC_OFF    3

#define CIPHER_NONE    0
#define CIPHER_WEP     1
#define CIPHER_TKIP    2
#define CIPHER_CCMP    3
//---------------------------------------
typedef struct
{
    int ap_mode;    /* 1: manage 2: Ad-hoc */
    int enc_group;   /* wep:0 wpa:1 wpa2:2 */
    int enc_pair;   /* wep:0 wpa:1 wpa2:2 */
    int enc_type;   /* wep:0 wpa:1 wpa2:2 */
	int enc_key;
    char ssid[128];
    char key[128];  /* wpa, wep - key */
} wifi_ssid_t;

#endif //__COMMON_H__
