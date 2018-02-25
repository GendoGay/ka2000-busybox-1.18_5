#include "libbb.h"
#include "buzzer.h"
int wifi_setup_app_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;

int wifi_setup_app_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
    char essid[64];
    char ipAddr[32];
    char cmd[100];
    play_sound (CONNECTION);

    read_wifiConfigFile("FLUCARD SSID", essid);
    read_wifiConfigFile("My IP Addr", ipAddr);

    printf("essid is %s\n", essid);
    printf("My IP Addr is %s\n", ipAddr);
    memset(cmd, 0, 100);
    sprintf(cmd,"ifconfig mlan0 %s netmask 255.255.255.0 up", ipAddr);
    printf("cmd = %s\n", cmd);
    system(cmd);
    system("iwconfig mlan0 mode ad-hoc");
    memset(cmd, 0, 100);
    sprintf(cmd,"iwconfig mlan0 essid %s", essid);
    printf("cmd = %s\n",cmd);
    system(cmd);
    system("udhcpd /etc/udhcpd.conf");
    system("httpd -h /www");

    return 0;
}
