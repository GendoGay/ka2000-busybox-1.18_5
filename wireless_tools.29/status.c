#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h> 

//#include <netinet/ether.h>

#include "iwlib.h"
#include "libbb.h"		// for wifi information
int status_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
#define VERSION_PATH "/mnt/mtd/version.txt"
#define STAT_PATH "/www/tem.txt"
#define CONFIG_PATH "/mnt/mtd/config/config.trek"



struct laninfo
{
	char ip[20];
	char mac[20];
	char netmask[20];
};

int getKeyValue(char find[],char line[], char value[])
{
	int len1 = strlen(find);
	int len2 = strlen(line);
	char newsub[100];
	int i,j,k;

	for (i = len1+3,j=0;i<len2;i++,j++)
	{
		newsub[j]=line[i];
	}
	for(k=strlen(newsub)-1;k>0;k--)
	{
		if(newsub[k]=='\n'||newsub[k]=='\r')
		{
			newsub[k]=0;
		}
	}

	strcpy(value,newsub);
	return 0;
}

int readVersionFile(char find[], char value1[])
{
	char* pdest;
	char line[100];

	FILE* fp;
	char value[100];

	fp = fopen(VERSION_PATH, "r");
	if (fp != NULL)
	{
		while (!feof(fp))
		{
			fgets(line, 100, fp);
			pdest = strstr(line, find);
			if (pdest != NULL)
			{
				getKeyValue(find, line, value);
				strcpy(value1, value);
			}
		}
		fclose(fp);
	}
	else
	{
		strcpy(value1,"");
	}

	return 0;
}


void get_laninfo(struct laninfo* laninfo, char* devname)
{
	// ethernet data structure  
	struct ifreq *ifr; 
	struct sockaddr_in *sin; 
	struct sockaddr *sa; 

	// ethernet config structure 
	struct ifconf ifcfg; 

	int fd; 
	int n; 
	int numreqs = 30; 
	fd = socket(AF_INET, SOCK_DGRAM, 0); 

	memset(&ifcfg, 0, sizeof(ifcfg)); 
	ifcfg.ifc_buf = NULL; 
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs; 
	ifcfg.ifc_buf = malloc(ifcfg.ifc_len); 

	if (ioctl(fd, SIOCGIFCONF, (char *)&ifcfg) < 0) 
	{ 
		perror("SIOCGIFCONF "); 
		//exit; 
	} 

	ifr = ifcfg.ifc_req; 
	for (n = 0; n < ifcfg.ifc_len; n+= sizeof(struct ifreq), ifr++) 
	{ 
		// find device
		if (strcmp(ifr->ifr_name, devname) != 0)
			continue;

		// ip address
		sin = (struct sockaddr_in *)&ifr->ifr_addr; 
		strcpy(laninfo->ip, inet_ntoa(sin->sin_addr));

		// mac address
		if ( (sin->sin_addr.s_addr) != INADDR_LOOPBACK) 
		{ 
			ioctl(fd, SIOCGIFHWADDR, (char *)ifr); 
			sa = &ifr->ifr_hwaddr; 
			//strcpy(laninfo->mac, (char*)ether_ntoa((struct ether_addr *)sa->sa_data));
			sprintf(laninfo->mac, "%02X:%02X:%02X:%02X:%02X:%02X",
					sa->sa_data[0] & 0xff,
					sa->sa_data[1] & 0xff,
					sa->sa_data[2] & 0xff,
					sa->sa_data[3] & 0xff,
					sa->sa_data[4] & 0xff,
					sa->sa_data[5] & 0xff
				   );

		} 

		// net mask 
		ioctl(fd, SIOCGIFNETMASK, (char *)ifr); 
		sin = (struct sockaddr_in *)&ifr->ifr_addr; 
		strcpy(laninfo->netmask, inet_ntoa(sin->sin_addr));
	} 
}

static int get_wifiinfo(int	skfd, char*	ifname, struct wireless_info* info)
{
	struct iwreq wrq;

	memset((char *) info, 0, sizeof(struct wireless_info));

	/* Get basic information */
	if(iw_get_basic_config(skfd, ifname, &(info->b)) < 0)
	{
		/* If no wireless name : no wireless extensions */
		/* But let's check if the interface exists at all */
		struct ifreq ifr;

		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		if(ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
			return(-ENODEV);
		else
			return(-ENOTSUP);
	}

	/* Get ranges */
	if(iw_get_range_info(skfd, ifname, &(info->range)) >= 0)
		info->has_range = 1;

	/* Get AP address */
	if(iw_get_ext(skfd, ifname, SIOCGIWAP, &wrq) >= 0)
	{
		info->has_ap_addr = 1;
		memcpy(&(info->ap_addr), &(wrq.u.ap_addr), sizeof (sockaddr));
	}

	/* Get bit rate */
	if(iw_get_ext(skfd, ifname, SIOCGIWRATE, &wrq) >= 0)
	{
		info->has_bitrate = 1;
		memcpy(&(info->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
	}

	/* Get Power Management settings */
	wrq.u.power.flags = 0;
	if(iw_get_ext(skfd, ifname, SIOCGIWPOWER, &wrq) >= 0)
	{
		info->has_power = 1;
		memcpy(&(info->power), &(wrq.u.power), sizeof(iwparam));
	}

	/* Get stats */
	if(iw_get_stats(skfd, ifname, &(info->stats),
				&info->range, info->has_range) >= 0)
	{
		info->has_stats = 1;
	}

#ifndef WE_ESSENTIAL
	/* Get NickName */
	wrq.u.essid.pointer = (caddr_t) info->nickname;
	wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;
	if(iw_get_ext(skfd, ifname, SIOCGIWNICKN, &wrq) >= 0)
		if(wrq.u.data.length > 1)
			info->has_nickname = 1;

	if((info->has_range) && (info->range.we_version_compiled > 9))
	{
		/* Get Transmit Power */
		if(iw_get_ext(skfd, ifname, SIOCGIWTXPOW, &wrq) >= 0)
		{
			info->has_txpower = 1;
			memcpy(&(info->txpower), &(wrq.u.txpower), sizeof(iwparam));
		}
	}

	/* Get sensitivity */
	if(iw_get_ext(skfd, ifname, SIOCGIWSENS, &wrq) >= 0)
	{
		info->has_sens = 1;
		memcpy(&(info->sens), &(wrq.u.sens), sizeof(iwparam));
	}

	if((info->has_range) && (info->range.we_version_compiled > 10))
	{
		/* Get retry limit/lifetime */
		if(iw_get_ext(skfd, ifname, SIOCGIWRETRY, &wrq) >= 0)
		{
			info->has_retry = 1;
			memcpy(&(info->retry), &(wrq.u.retry), sizeof(iwparam));
		}
	}

	/* Get RTS threshold */
	if(iw_get_ext(skfd, ifname, SIOCGIWRTS, &wrq) >= 0)
	{
		info->has_rts = 1;
		memcpy(&(info->rts), &(wrq.u.rts), sizeof(iwparam));
	}

	/* Get fragmentation threshold */
	if(iw_get_ext(skfd, ifname, SIOCGIWFRAG, &wrq) >= 0)
	{
		info->has_frag = 1;
		memcpy(&(info->frag), &(wrq.u.frag), sizeof(iwparam));
	}
#endif	/* WE_ESSENTIAL */

	return(0);
}


void remove_newline113(char* str)
{
	int i;
	int len = strlen(str);

	for (i=0; i<len; i++)
	{
		if (str[i] == '\n'|| str[i] == '\r')
		{
			str[i] = 0;
		}
	}
}

int getKeyValue113(char find[], char line[], char value[])
{
	remove_newline113(line);

	int len1 = strlen(find);
	int len2 = strlen(line);
	char newsub[100] = {0, };
	int i,j/*,k*/;

	// length check.
	if (len2 - len1 <= 3) // 3 means strlen(" : ");
	{
		strcpy(value, newsub);
		return 0;
	}

	for (i=len1+3, j=0; i<len2; i++,j++) // 3 means strlen(" : ");
	{
		newsub[j]=line[i];
	}


	strcpy(value, newsub);
	return 0;
}

 int readConfigFile113(char find[], char value1[])
{
	char* pdest = NULL;
	char line[100];

	FILE* fp = NULL;
	char value[100];// = {0, };

	fp = fopen(CONFIG_PATH, "r");
	if (fp != NULL)
	{
		while (!feof(fp))
		{
			fgets(line, 100, fp);
			pdest = strstr(line, find);
			if (pdest != NULL)
			{
				getKeyValue113(find, line, value);
				strcpy(value1, value);
			}
		}
		fclose(fp);
	}
	else
	{
		strcpy(value1, "");
	}

	return 0;
}

int checkStat113()
{
	char* pdest = NULL;
	char line[100];

	FILE* fp = NULL;
	char value[100];// = {0, };

	fp = fopen(STAT_PATH, "r");
	if (fp != NULL)
	{
		while (!feof(fp))
		{
			fgets(line, 100, fp);
			pdest = strstr(line, "ip");
			if (pdest != NULL)
			{
				getKeyValue113("ip", line, value);
			
			}
		}
		fclose(fp);
	}
	if (strcmp(value, getenv("REMOTE_ADDR")) == 0)
	{
		return 0;
	}else if ( strcmp(value, "NONE") == 0){
		return 1;
	}else{
		return 2;
	}

	
}




int printLogin113(){

	printf("Content-Type:text/html; charset=utf-8\n\n");
	printf("<html><head><title>FLU-CARD CONFIG</title>\n");
	printf("<link href='../css/css_main.css' rel='stylesheet' type='text/css'>\n");	
	printf("</head>\n");
	printf("<body style='background: #FFFFFF' > \n");
	printf("<form id='loginForm' action='../cgi-bin/logpage'  method='post' enctype='multipart/form-data' ><table>\n");
	printf("<tr><td align='left' height='30px'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<font face='Arial' color='#736F6E' size='4' style='font-weight: normal;margin-right:140px'><B>Login Name :</B></font> &nbsp;&nbsp;&nbsp;<input id='logname' name='logname' type='text'></td></tr>\n");
	printf("<tr><td align='left' height='30px'>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<font face='Arial' color='#736F6E' size='4' style='font-weight: normal;margin-right:140px'><B>Password :</B></font>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input id='logword' name='logword' type='password'></td></tr>\n");
	printf("<tr><td align='right' height='30px'><input name='log' type='submit' value='Log in'></td></tr>\n");
	printf("</form>\n");
	printf("</body>\n");
	printf("</html>\n");
	
	return 0;
	
}

int status_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	struct laninfo eth0;

	get_laninfo(&eth0, "mlan0");
	
	int s = checkStat113();

	char lenable[200] = {0, };

	readConfigFile113("Login-enable", lenable);

	int skfd;		// generic raw socket desc

	// Create a channel to the NET kernel.
	if((skfd = iw_sockets_open()) < 0)
	{
		perror("socket");
		exit(-1);
	}

	char buffer[128];
	
	char ifname[20] = "mlan0";

	struct wireless_info info;

	get_wifiinfo(skfd, ifname, &info);


	// Close the Socket
	iw_sockets_close(skfd);

	if(s == 0 || strcmp(lenable, "No") == 0 || strcmp(lenable, "") == 0)
	{
	
	printf("Content-Type:text/html; charset=utf-8\n\n");
	printf("<html><head><title>FLU-CARD CONFIG</title>\n");
	printf("<link href='../css/css_main.css' rel='stylesheet' type='text/css'>\n");	
	printf("</head>\n");
	printf("<body style='background: #FFFFFF' > \n");

	printf("  <table id='autoWidth' style='width: 100%%; '>\n");
	printf("    <tbody>\n");
	printf("      <tr>\n");
	printf("        <td class='h1' colspan='3' id='t_title'><font face='Arial' color='#736F6E' size='5'  ><b>Status</b></font></td>\n");
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='blue' colspan='3'>\n");
	printf("      </tr>\n");
	printf("\n");
	printf("      <tr>\n");
	printf("        <td class='h1' colspan='3' id='t_lan'><font face='Arial' color='#736F6E' size='4'  ><b>LAN</b></font></td>\n");
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td width='25%%' class='Item' id='t_mac_addr'><font face='Arial' color='#736F6E' size='4' ><i>MAC Address:</i></font></td>\n");
	printf("        <td colspan='2'><div id='lanMac'><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", eth0.mac);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item' id='t_ip_addr'><font face='Arial' color='#736F6E' size='4'  ><i>IP Address:</i></font></td>\n");
	printf("        <td colspan='2'><div id='lanIP'><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", eth0.ip);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item' id='t_sub_mask'><font face='Arial' color='#736F6E' size='4'  ><i>Subnet Mask:</i></font></td>\n");
	printf("        <td colspan='2'><div id='lanMask'><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", eth0.netmask);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='blue' colspan='3'>\n");
	printf("      </tr>\n");

	printf("      <tr>\n");
	printf("        <td class='h1' colspan='3' id='t_lan'><font face='Arial' color='#736F6E' size='4'  ><b>Wireless</b></font></td>\n");
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td width='25%%' class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>Name (SSID):</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", info.b.essid);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>Channel:</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%d</font></div></td>\n", iw_freq_to_channel(info.b.freq, &info.range));
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>Mode:</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", info.b.name);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>MAC Address:</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", iw_sawap_ntop(&info.ap_addr, buffer));
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='blue' colspan='3'>\n");
	printf("      </tr>\n");

	
	iw_sockets_close(skfd);

	char pname[200];
	char fversion[200];
	char bdate[200];
	char revision[200];

	readVersionFile("Product Name", pname);
	readVersionFile("Firmware Version", fversion);
	readVersionFile("Build Date", bdate);
	readVersionFile("Revision", revision);

	printf("      <tr>\n");
	printf("        <td class='h1' colspan='3' id='t_lan'><font face='Arial' color='#736F6E' size='4'  ><b>Firmware</b></font></td>\n");
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td width='25%%' class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>Product Name:</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", pname);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>Firmware Version:</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", fversion);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>Build Date:</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", bdate);
	printf("      </tr>\n");
	printf("      <tr>\n");
	printf("        <td class='Item'><font face='Arial' color='#736F6E' size='4'  ><i>Revision:</i></font></td>\n");
	printf("        <td colspan='2'><div><font face='Arial' color='#736F6E' size='4'  >%s</font></div></td>\n", revision);
	printf("      </tr>\n");

	printf("    </tbody>\n");
	printf("  </table>\n");
	
	printf("</body> \n");
	printf("</html>");

	}else if(s == 1){
		printLogin113();
	}else{
	printf( "Content-Type:text/html; charset=utf-8\n\n");
	printf( "<html><head><title>FLU-CARD CONFIG</title>\n");
	printf( "	<body style='background: #FFFFFF'>\n");
	printf( "	<TR>\n");
	printf( "		<TD colspan='2' height='30px'>&nbsp;<h2><font face='Arial' color='#736F6E' size='6' style='font-weight: normal;margin-right:140px'><b>Sorry!</b></font></h2></TD>\n");
	printf( "	</TR>\n");
	printf( "	<TR>	<TD width='10%%'><font face='Arial' color='#736F6E' size='4' style='font-weight: normal;margin-right:140px'>Another user is currently logging in. </font></TD>\n");
	printf( "	</TR>\n");
	printf( "	<TR>	<TD width='10%%'><h2><font face='Arial' color='#736F6E' size='4' style='font-weight: normal;margin-right:140px'>Please try again later.</font><h2></TD>\n");
	
	printf( "	</TR>\n");
	printf( "	</body></head></html>\n");
	}
	

	return 0;
}
