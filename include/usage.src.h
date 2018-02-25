/* vi: set sw=8 ts=8: */
/*
 * This file suffers from chronically incorrect tabification
 * of messages. Before editing this file:
 * 1. Switch you editor to 8-space tab mode.
 * 2. Do not use \t in messages, use real tab character.
 * 3. Start each source line with message as follows:
 *    |<7 spaces>"text with tabs"....
 * or
 *    |<5 spaces>"\ntext with tabs"....
 */
#ifndef BB_USAGE_H
#define BB_USAGE_H 1

#define NOUSAGE_STR "\b"

INSERT

#define busybox_notes_usage \
       "Hello world!\n"

/* KA path start -------------------------------------------------------- */

#define iwlist_trivial_usage \
       "[Usage].."
#define iwlist_full_usage \
       "Print hello string in the screen"

#define iwconfig_trivial_usage \
       "[Usage].."
#define iwconfig_full_usage \
       "Print hello string in the screen"

#define iwpriv_trivial_usage \
       "[Usage].."
#define iwpriv_full_usage \
       "Print hello string in the screen"

#define iwevent_trivial_usage \
       "[Usage].."
#define iwevent_full_usage \
       "Print hello string in the screen"

#define ifrename_trivial_usage \
       "[Usage].."
#define ifrename_full_usage \
       "Print hello string in the screen"

#define macaddr_trivial_usage \
       "[Usage].."
#define macaddr_full_usage \
       "Print hello string in the screen"

#define mlan2040coex_trivial_usage \
       ""
#define mlan2040coex_full_usage "\n\n" \
       "MURATA MLAN2040COEX Application for wireless."

#define mlanconfig_trivial_usage \
       ""
#define mlanconfig_full_usage "\n\n" \
    "Usage: " \
    "\n   mlanconfig -v  (version)" \
    "\n   mlanconfig <mlanX> <cmd> [...]" \
    "\n   where" \
    "\n   mlanX : wireless network interface" \
    "\n   cmd : hostcmd" \
    "\n     : mefcfg" \
    "\n     : customie" \
    "\n     : arpfilter" \
    "\n     : cfgdata" \
    "\n     : sdcmd52rw, sdcmd53rw" \
    "\n     : getscantable, setuserscan" \
    "\n     : addts, delts, qconfig, qstats, ts_status, qstatus" \
    "\n     : regrdwr, memrdwr" \
    "\n     : additional parameter for hostcmd" \
    "\n     :   <filename> <cmd>" \
    "\n 	  : additional parameters for mefcfg are:" \
    "\n     :   <filename>" \
    "\n     : additional parameters for customie are:" \
    "\n     :  <index> <mask> <IE buffer>" \
    "\n     : additional parameter for arpfilter" \
    "\n     :   <filename>" \
    "\n     : additional parameter for cfgdata" \
    "\n     :   <type> <filename>" \
    "\n     : additional parameter for sdcmd52rw" \
    "\n     :   <function> <reg addr.> <data>" \
    "\n     : additional parameter for sdcmd53rw" \
    "\n     :   <func> <addr> <mode> <blksiz> <blknum> <data1> ... ...<dataN> " \
    "\n     : additional parameter for addts" \
    "\n     :   <filename.conf> <section# of tspec> <timeout in ms>" \
    "\n     : additional parameter for delts" \
    "\n     :   <filename.conf> <section# of tspec>" \
    "\n     : additional parameter for qconfig" \
    "\n     :   <[set msdu <lifetime in TUs> [Queue Id: 0-3]]" \
    "\n     :    [get [Queue Id: 0-3]] [def [Queue Id: 0-3]]>" \
    "\n     : additional parameter for qstats" \
    "\n     :   <[get [Queue Id: 0-3]]>" \
    "\n     : additional parameter for regrdwr" \
    "\n     :   <type> <offset> [value]" \
    "\n     : additional parameter for memrdwr" \
    "\n     :   <address> [value]"

#define mlanevent_trivial_usage \
      "Usage : mlanevent.exe [-v] [-h]\n" \
      "     -v               : Print version information\n" \
      "     -h               : Print help information\n" \
      "\n"
#define mlanevent_full_usage \
       "Run mlanevent "

#define uaputl_trivial_usage \
      "\n"
#define uaputl_full_usage \
       "Run uaputl "
/*
#define ssmtp_trivial_usage \
      "\n"
#define ssmtp_full_usage \
       "Run ssmtp "
*/
#define perl_trivial_usage \
       "[Usage].."
#define perl_full_usage \
       "The Perl 5 language interpreter" \
       "perl [ -sTtuUWX ]      [ -hv ] [ -V[:configvar] ]" \
       "     [ -cw ] [ -d[t][:debugger] ] [ -D[number/list] ]" \
       "     [ -pna ] [ -Fpattern ] [ -l[octal] ] [ -0[octal/hexadecimal] ]" \
       "     [ -Idir ] [ -m[-]module ] [ -M[-]'module...' ] [ -f ]" \
       "     [ -C [nnuummbbeerr//lliisstt] ]      [ -S ]      [ -x[dir] ]" \
       "     [ -i[extension] ]" \
       "     [ [-e|-E] 'command' ] [ -- ] [ programfile ] [ argument ]..." \

#define hello_ka_trivial_usage \
       "hello_ka"
#define hello_ka_full_usage "\n\n" \
       "hello_ka application entry"

#define kcard_app_trivial_usage \
       "kcard_app"
#define kcard_app_full_usage "\n\n" \
       "kcard application entry"

#define kcard_cmd_trivial_usage \
       "kcard_cmd"
#define kcard_cmd_full_usage "\n\n" \
       "set/call kcard cmd function entry"

#define kcard_startup_trivial_usage \
       "kcard_startup"
#define kcard_startup_full_usage "\n\n" \
       "kcard_startup application entry"

#define buzzer_trivial_usage \
       "buzzer -t [TYPE]"
#define buzzer_full_usage "\n\n" \
       "play buzzer"

#define wifi_download_trivial_usage \
       "wifi_download"
#define wifi_download_full_usage "\n\n" \
       "wifi_download"

#define wifi_upload_trivial_usage \
       "wifi_upload"
#define wifi_upload_full_usage "\n\n" \
       "wifi_upload"

#define wifi_get_config_trivial_usage \
       "wifi_get_Config"
#define wifi_get_config_full_usage "\n\n" \
       "wifi_get_config"
#define wifi_quick_send_trivial_usage \
       "wifi_quick_send"
#define wifi_quick_send_full_usage "\n\n" \
       "wifi_quick_send"
#define wifi_ftp_server_trivial_usage \
       "wifi_ftp_server"
#define wifi_ftp_server_full_usage "\n\n" \
       "wifi_ftp_server"
#define wifi_connect_router_trivial_usage \
       "wifi_connect_router"
#define wifi_connect_router_full_usage "\n\n" \
       "wifi_connect_router"
#define wifi_ftp_upload_trivial_usage \
       "wifi_ftp_upload"
#define wifi_ftp_upload_full_usage "\n\n" \
       "wifi_ftp_upload"
#define thumbNail_trivial_usage \
       "thumbNail"
#define thumbNail_full_usage "\n\n" \
       "thumbNail"
#define thumbnail_video_trivial_usage \
       "thumbnail_video"
#define thumbnail_video_full_usage "\n\n" \
       "thumbNail_video"
#define wifi_filelist_trivial_usage \
       "wifi_filelist"
#define wifi_filelist_full_usage "\n\n" \
       "wifi_filelist"
/* KA patch end --------------------------------------------------------- */

#endif
