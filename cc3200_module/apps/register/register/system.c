#include "system.h"
#include "userconfig.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
// #include "httpclientapp.h"
#ifdef TARGET_IS_CC3200
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gprcm.h"
#include "prcm.h"
#include "simplelink.h"
#include "network_if.h"
#include "uart_if.h"
#include "cc3200_system.h"
#include "rom_map.h"
// #include "bootmgr.h"
#endif

#include "HttpCore.h"
#include "network_handler.h"
#include "server_api.h"
#include "server_mqtt.h"
#include "jsmn.h"

#include "common.h"
#include "p2p_config.h"
#include "p2p_main.h"
#include "server_mqtt.h"
#include "webserver.h"
#include "spi_slave_handler.h"
#include "ringbuffer.h"
#include "osi.h"
#include "date_time.h"
#include "cc3200_system.h"




extern long ConfigureSimpleLinkToDefaultState();
extern void InitializeAppVariables();

extern OsiSyncObj_t g_userconfig_init;
extern char ps_ip[];
extern uint32_t ps_port;
extern char p2p_key_char[];
extern char p2p_rn_char[];
extern char p2p_key_hex[];
extern char p2p_rn_hex[];
extern char str_ps_port[];
char timeoutbuffer[];
extern char mqtt_rece[];
myCinUserInformation_t g_mycinUser;

extern RingBuffer* ring_buffer_send;

char MYCIN_API_HOST_NAME[api_size] = {0};
char MYCIN_HOST_NAME[mqtt_size] = {0};
#ifdef NTP_CHINA
char g_acSNTPserver[NUM_NTP_SERVER][NTP_RUL_MAXLEN] = {"cn.pool.ntp.org", "cn.ntp.org.cn", "ntp1.aliyun.com", "ntp2.aliyun.com"};
#else
char g_acSNTPserver[NUM_NTP_SERVER][NTP_RUL_MAXLEN] = {"pool.ntp.org", "time.nist.gov", "time4.google.com", "nist1-nj2.ustiming.org", "ntp-nist.ldsbc.edu"};
#endif

//#ifdef NTP_CHINA
//char g_acSNTPserver_single[NTP_RUL_MAXLEN]; //not used
//#else
#define g_acSNTPserver_single (&g_acSNTPserver[0][0])
//#endif
char relay_url_ch[rms_size] = {0};
char SERVER_URL[stun_size] = {0};
char ak_version[7]={0};
char ak_version_format[9]={0};
int timezone_s=0;
int pairing_in_progress=0;

//#ifndef isprint
// #define in_range(c, lo, up)  ((uint8_t)c >= lo && (uint8_t)c <= up)
// #define isprint(c)           in_range(c, 0x20, 0x7f)
// #define isdigit(c)           in_range(c, '0', '9')
// #define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
// #define islower(c)           in_range(c, 'a', 'z')
// #define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == ',')

/* Wifi config parsing*/

/* Parse the wireless save*/
static char *ParseString (char *instring)
{
    char *ptr1 = instring, *ptr2 = instring;

    if (instring == NULL)
        return instring;

    while (isspace (*ptr1))
        ptr1++;

    while (*ptr1)
    {
        if (*ptr1 == '+')
        {
            ptr1++; *ptr2++ = ' ';
        }
        else if (*ptr1 == '%' && isxdigit (*(ptr1 + 1)) && isxdigit (*(ptr1 + 2)))
        {
            ptr1++;
            *ptr2    = ((*ptr1 >= '0' && *ptr1 <= '9') ? (*ptr1 - '0') : ((char)toupper(*ptr1) - 'A' + 10)) << 4;
            ptr1++;
            *ptr2++ |= ((*ptr1 >= '0' && *ptr1 <= '9') ? (*ptr1 - '0') : ((char)toupper(*ptr1) - 'A' + 10));
            ptr1++;
        }
        else
            *ptr2++ = *ptr1++;
    }

    /*while (ptr2 > instring && isspace(*(ptr2 - 1)))
        ptr2--;*/

    *ptr2 = '\0';

    return instring;
}

static int ParseWifiConfigString(unsigned char * strDataIn, nxcNetwork_st *wifiConfig)
{
    int retVal = 0;
    unsigned char lenSSID, lenKey, lenIP, lenSubnet, lenGW, lenwp, lenUsrN, lenPassW;
    int data_len = strlen((char const *)strDataIn);
    int index, check_count = 0;
    char strInt[10];

    unsigned char * strData = strDataIn;
    if (wifiConfig == NULL)
        return -1;
    memset((char*)wifiConfig, 0, sizeof(nxcNetwork_st));

    index = 0;
    //! get wifi mode
    switch (strData[index])
    {
    case '0':
        wifiConfig->NetworkType = 0;
        break;
    case '1':
        wifiConfig->NetworkType = 1;
        break;
    default:
        wifiConfig->NetworkType = 2;
        break;
    }
    index++;
    if (index > data_len)
    {
        return -1;
    }
    //! get channel to set in adhoc mode
    strncpy(strInt, (char const *)strData + index, 2);
    strInt[2] = '\0';
    wifiConfig->Channel = atoi(strInt);
    if ((wifiConfig->Channel <= 0) || (14 < wifiConfig->Channel))
    {
        wifiConfig->Channel = WIFI_DEFAULT_ADHOC_CHANNEL;//default channel
    }
    index += 2;
    if (index > data_len)
    {
        return -1;
    }
    //get authentication mode
    switch (strData[index++])
    {
    case '0':
    //                        wifiConfig->AuthenType = NETWORK_AUTH_NONE;
    //                       break;
    case '1':
        wifiConfig->AuthenType = 1;
        break;
    case '2':
        wifiConfig->AuthenType = 2;
        break;

    default:
        wifiConfig->AuthenType = 3;
        break;
    }
    if (index > data_len)
    {
        return -1;
    }
    //get keyindex if in open or shared mode
    if ((wifiConfig->AuthenType == 1))
    {
        switch (strData[index]) {
        case '0':
            wifiConfig->KeyIndex = 0;
            break;
        case '1':
            wifiConfig->KeyIndex = 1;
            break;
        case '2':
            wifiConfig->KeyIndex = 2;
            break;
        case '3':
            wifiConfig->KeyIndex = 3;
            break;
        default:
            break;
        }
    }
    index++;
    if (index > data_len)
    {
        return -1;
    }
    //get ip mode
    switch (strData[index++])
    {
    case '0':
        wifiConfig->IPMode = 0;
        break;
    case '1':
        wifiConfig->IPMode = 1;
        break;
    default:
        break;
    }
    if (index > data_len)
    {
        return -1;
    }
    //get length ssid
    strncpy(strInt, (char const *)strData + index, 3);
    strInt[3] = '\0';
    lenSSID = atoi(strInt);
    index += 3;
    if (index > data_len)
    {
        return -1;
    }
    //get length key
    strncpy(strInt, (char const *)strData + index, 2);
    strInt[2] = '\0';
    lenKey = atoi(strInt);
    if (lenKey == 0)
        wifiConfig->AuthenType = 0;
    index += 2;
    if (index > data_len)
    {
        return -1;
    }
    //get length ip
    strncpy(strInt, (char const *)strData + index, 2);
    strInt[2] = '\0';
    lenIP = atoi(strInt);
    index += 2;
    if (index > data_len)
    {
        return -1;
    }
    //get length subnet mask
    strncpy(strInt, (char const *)strData + index, 2);
    strInt[2] = '\0';
    lenSubnet = atoi(strInt);
    index += 2;
    if (index > data_len)
    {
        return -1;
    }
    //get length default gateway
    strncpy(strInt, (char const *)strData + index, 2);
    strInt[2] = '\0';
    lenGW = atoi(strInt);
    index += 2;
    if (index > data_len)
    {
        return -1;
    }
    //get length working port
    strncpy(strInt, (char const *)strData + index, 1);
    strInt[1] = '\0';
    lenwp = atoi(strInt);
    index += 1;
    if (index > data_len)
    {
        return -1;
    }
    //get length username
    strncpy(strInt, (char const *)strData + index, 2);
    strInt[2] = '\0';
    lenUsrN = atoi(strInt);
    index += 2;
    if (index > data_len)
    {
        return -1;
    }
    //get length password
    strncpy(strInt, (char const *)strData + index, 2);
    strInt[2] = '\0';
    lenPassW = atoi(strInt);
    index += 2;
    if (index > data_len)
    {
        return -1;
    }
    //! Check data available
    check_count = index + lenSSID + lenKey + lenIP + lenSubnet + lenGW + lenwp + lenUsrN + lenPassW;
    //UART_PRINT("Dump len: lenSSID=%d + lenKey=%d + lenIP=%d + lenSubnet=%d + lenGW=%d + lenwp=%d+ lenUsrN=%d + lenPassW=%d\n",
    //    lenSSID,lenKey,lenIP,lenSubnet,lenGW,lenwp,lenUsrN,lenPassW);
    if (check_count != data_len)
    {
        //UART_PRINT("Error->data_len=%d,correct_len=%d\n", data_len, check_count);
        return -1;
    }
    //get string ssid
    strncpy((char *)wifiConfig->ESSID, (char const *) strData + index, lenSSID);
    wifiConfig->ESSID[lenSSID] = '\0';
    index += lenSSID;
    //get string key
    strncpy((char *)wifiConfig->Key, (char const *) strData + index, lenKey);
    wifiConfig->Key[lenKey] = '\0';
    index += lenKey;
    //! Ignore if DHCP
    if (wifiConfig->IPMode == 1) {
        strncpy((char *)wifiConfig->DefaultIP, (char const *)strData + index, lenIP);
        wifiConfig->DefaultIP[lenIP] = '\0';
        index += lenIP;
        strncpy((char *)wifiConfig->SubnetMask, (char const *)strData + index, lenSubnet);
        wifiConfig->SubnetMask[lenSubnet] = '\0';
        index += lenSubnet;
        strncpy((char *)wifiConfig->Gateway, (char const *)strData + index, lenGW);
        wifiConfig->Gateway[lenGW] = '\0';
        index += lenGW;
        if (lenwp != 0) {
            //strncpy(wifiConfig->WorkPort, strData+index,lenwp);
            //wifiConfig->WorkPort[lenwp] = '\0';
        }
        else {
            //strcpy(wifiConfig->WorkPort,"80");
        }
        index += lenwp;
    }
    else {
        //strcpy(wifiConfig->WorkPort,"80");
        index += lenIP + lenSubnet + lenGW + lenwp;
    }

    if (wifiConfig->NetworkType == 1) {
        //strncpy(wifiConfig->Username, strData+index,lenUsrN);
        //wifiConfig->Username[lenUsrN] = '\0';
        index += lenUsrN;
        //strncpy(wifiConfig->Password, strData+index,lenPassW);
        //wifiConfig->Password[lenPassW] = '\0';
        index += lenPassW;
    }
    else {
        index += lenUsrN + lenPassW;
    }
    //! Dump

    return retVal;
}

void set_server_authen(char *api_key, char *timezone)
{
    memcpy(apiKey, api_key, strlen(api_key));
    memcpy(timeZone, timezone, strlen(timezone));
}

void set_wireless_save(char *wireless)
{
    #if 1
    if (wireless)
    {
        nxcNetwork_st wifiConfig;
        UART_PRINT("String=%s\r\n", wireless);
        ParseString(wireless);
        UART_PRINT("String_affter parse=%s\r\n", wireless);
        ParseWifiConfigString((unsigned char *)wireless, &wifiConfig);
        UART_PRINT("SSID=%s,Key=%s,IP=%s,Mask=%s,Mask=%s,GW=%s\r\n", wifiConfig.ESSID,wifiConfig.Key,wifiConfig.DefaultIP,wifiConfig.SubnetMask,wifiConfig.Gateway);
        set_wireless_config(wifiConfig.ESSID, wifiConfig.Key, NULL);
    }
    #else
    // NOTE: ERROR: FIXME: DKS: hardcode here
    // set_wireless_config("Hubble-Mir", "12345678@", NULL);
    set_wireless_config("Hubble-QA11", "qa13579135", NULL);
    #endif
}
int32_t set_wireless_config(char *ssid, char *pass, char *auth_type) {
    // auth_type not used
    if (ssid == NULL || pass == NULL) {
        return -1;
    }
    // URL decoder
    ParseString(ssid);
    ParseString(pass);
    UART_PRINT("SSID:%s, PASS:%s, SEC:%s\r\n", ssid, pass, auth_type);

    memset(ap_ssid, 0, MAX_AP_SSID_LEN);
    memset(ap_pass, 0, MAX_AP_PASS_LEN);
    memset(ap_sec, 0, MAX_AP_SEC_LEN);
    memcpy(ap_ssid, ssid, strlen((char const *)ssid));
    memcpy(ap_pass, pass, strlen((char const *)pass));
    memcpy(ap_sec, auth_type, strlen((char const *)auth_type));

    return 0;
}

int sec_string_to_num(){
	int l_len=strlen(ap_sec);
	int i;
	for(i=0;i<l_len;i++)
		ap_sec[i]=tolower(ap_sec[i]);
	UART_PRINT("Wifi Sec %s\r\n",ap_sec);
	if (strcmp (ap_sec, "open" )==0)
		return 0;
	if (strcmp (ap_sec, "wep" )==0)
		return 1;
	/*if (strcmp (ap_sec, "WPA" )==0)
		return 2;*/
	return 2;
}
/**
  * @brief  set_url_default down flash
  *
  * @return success : 0
  */
int set_url_default(int val_in)
{
/***********************************start set api default**********************************/
	if(val_in == 0)
	{
	memset(MYCIN_API_HOST_NAME, 0, strlen(MYCIN_API_HOST_NAME));
	strcpy(MYCIN_API_HOST_NAME, API_URL_DEFAULT);
	userconfig_set(MYCIN_API_HOST_NAME, strlen(MYCIN_API_HOST_NAME), API_URL);
	}
/*****************************************************************************************/


/***********************************start set mqtt default********************************/
	if(val_in == 1)
	{
	memset(MYCIN_HOST_NAME, 0, strlen(MYCIN_HOST_NAME));
	strcpy(MYCIN_HOST_NAME, MQTT_URL_DEFAULT);
	userconfig_set(MYCIN_HOST_NAME, strlen(MYCIN_HOST_NAME), MQTT_URL);
	}
/*****************************************************************************************/


/***********************************start set ntp default********************************/
	if(val_in == 2)
	{
	memset(g_acSNTPserver_single, 0, strlen(g_acSNTPserver_single));
	strcpy(g_acSNTPserver_single, NTP_URL_DEFAULT);
	userconfig_set(g_acSNTPserver_single, strlen(g_acSNTPserver_single), NTP_URL);
	}
/*****************************************************************************************/


/***********************************start set rms default********************************/
	if(val_in == 3)
	{
	memset(relay_url_ch, 0, strlen(relay_url_ch));
	strcpy(relay_url_ch, RMS_URL_DEFAULT);
	userconfig_set( relay_url_ch, strlen( relay_url_ch), RMS_URL);
	}
/*****************************************************************************************/


/***********************************start set stun default********************************/
	if(val_in == 4)
	{
	memset(SERVER_URL, 0, strlen(SERVER_URL));
	strcpy(SERVER_URL, STUN_URL_DEFAULT);
	userconfig_set(SERVER_URL, strlen(SERVER_URL), STUN_URL);
	}
/*****************************************************************************************/

	return 0;
}
/**
  * @brief  get_url_default if read from flash empty call function set_url_default
  *
  * @return  none
  */
int get_url_default()
{
	int  ret_val = 0, value_in = 0;
	int bool_to_write = 0;
	
	memset(MYCIN_API_HOST_NAME, 0, strlen(MYCIN_API_HOST_NAME));
	memset(MYCIN_HOST_NAME, 0, strlen(MYCIN_HOST_NAME));
	memset(g_acSNTPserver_single, 0, strlen(g_acSNTPserver_single));
	memset(relay_url_ch, 0, strlen(relay_url_ch));
	memset(SERVER_URL, 0, strlen(SERVER_URL));

/***************************************get_api_default ***************************************/
	ret_val = userconfig_get(MYCIN_API_HOST_NAME, api_size, API_URL);
	if(ret_val <= 0)
	{
		value_in = 0;
		set_url_default(value_in);
		ret_val = userconfig_get(MYCIN_API_HOST_NAME, api_size, API_URL);
		bool_to_write = 1;
	}
	//UART_PRINT("GET_API_VALUE %d %s\r\n", ret_val, MYCIN_API_HOST_NAME);

/*********************************************************************************************/


/**********************************************get_mqtt_default*******************************/
	ret_val = userconfig_get(MYCIN_HOST_NAME, mqtt_size, MQTT_URL);
	if (ret_val <= 0)
	{
		value_in = 1;
		set_url_default(value_in);
		ret_val = userconfig_get(MYCIN_HOST_NAME, mqtt_size, MQTT_URL);
		bool_to_write = 1;
	}
	//UART_PRINT("GET_MQTT_VALUE: %d %s\r\n", ret_val, MYCIN_HOST_NAME);

/******************************************************************************************/


/********************************************get_ntp_default********************************/

	ret_val = userconfig_get(g_acSNTPserver_single, ntp_size, NTP_URL);
	if (ret_val <= 0)
	{
		value_in = 2;
		set_url_default(value_in);
		ret_val = userconfig_get(g_acSNTPserver_single, ntp_size, NTP_URL);
		bool_to_write = 1;
	}
	//UART_PRINT("GET_NTP_VALUE: %d %s\r\n", ret_val, g_acSNTPserver);

/******************************************************************************************/


/*****************************************get_rms_default**********************************/

	ret_val = userconfig_get(relay_url_ch, rms_size, RMS_URL);
	if (ret_val <= 0)
	{
		value_in = 3;
		set_url_default(value_in);
		ret_val = userconfig_get(relay_url_ch, rms_size, RMS_URL);
		bool_to_write = 1;
	}
	//UART_PRINT("GET_RMS_VALUE: %d %s\r\n", ret_val, relay_url_ch);
 
/******************************************************************************************/


/*****************************************get_stun_default*********************************/

	ret_val = userconfig_get(SERVER_URL, stun_size, STUN_URL);
	if (ret_val <= 0)
	{
		value_in = 4;
		set_url_default(value_in);
		ret_val = userconfig_get(SERVER_URL, stun_size, STUN_URL);
		bool_to_write = 1;
	}
	//UART_PRINT("GET_STUN_VALUE: %d %s\r\n", ret_val, SERVER_URL);

	if (bool_to_write)
		userconfig_flash_write();
/******************************************************************************************/
	return;
}
/**
  * @brief  set_url from app sent down
  *
  * @param  api: data contains the parameter "api.cinatic.com".
  *
  * @param  mqtt: data contains the parameter "mqtt.cinatic.com".
  *
  * @param  ntp: data contains the parameter "ntp.inode.at"
  *
  * @param  rms: data contains the parameter "relay-monitor.cinatic.com"
  *
  * @param  stun: data contains the parameter "stun.cinatic.com"
  *
  * @return success = 0, fail = -1 is api, fail = -2 is mqtt
  *                      fail = -3 is ntp, fail = -4 is rms, fail = -5 is stun
  */
int set_url(char *api, char *mqtt, char *ntp, char *rms, char *stun)
{
	int ret_val = 0;

	memset(MYCIN_API_HOST_NAME, 0, strlen(MYCIN_API_HOST_NAME));
	memset(MYCIN_HOST_NAME, 0, strlen(MYCIN_HOST_NAME));
	memset(g_acSNTPserver_single, 0, strlen(g_acSNTPserver_single));
	memset(relay_url_ch, 0, strlen(relay_url_ch));
	memset(SERVER_URL, 0, strlen(SERVER_URL));

	ParseString(api);
	ParseString(mqtt);
	ParseString(ntp);
	ParseString(rms);
	ParseString(stun);

/***********************************start set api down flash**********************************/

	memcpy(MYCIN_API_HOST_NAME, api, strlen(api));
	ret_val = userconfig_set(MYCIN_API_HOST_NAME, strlen(MYCIN_API_HOST_NAME), API_URL);
	if(ret_val < 0)
	{
		return -1;
	}
/*********************************************************************************************/


/************************************start set mqtt down flash********************************/

	memcpy(MYCIN_HOST_NAME, mqtt, strlen(mqtt));
	ret_val = userconfig_set(MYCIN_HOST_NAME, strlen(MYCIN_HOST_NAME), MQTT_URL);
	if(ret_val < 0)
	{
		return -2;
	}
/*********************************************************************************************/


/***********************************start set ntp down flash**********************************/

	memcpy(g_acSNTPserver_single, ntp, strlen(ntp));
	ret_val = userconfig_set(g_acSNTPserver_single, strlen(g_acSNTPserver_single), NTP_URL);
	if(ret_val < 0)
	{
		return -3;
	}
/*********************************************************************************************/


/*************************************start set rms down flash*******************************/

	memcpy(relay_url_ch, rms, strlen(rms));
	ret_val = userconfig_set(relay_url_ch, strlen(relay_url_ch), RMS_URL);
	if(ret_val < 0)
	{
		return -4;
	}
/*********************************************************************************************/


/*************************************start set stun down flash******************************/

	memcpy(SERVER_URL, stun, strlen(stun));
	ret_val = userconfig_set(SERVER_URL, strlen(SERVER_URL), STUN_URL);
	if(ret_val < 0)
	{
		return -5;
	}
/*********************************************************************************************/
//	userconfig_flash_write();
	return 0;
}

/**
  * @brief  get_url from app sent down if read from flash empty call function set_url_default
  *
  * @retval api, mqtt, stp, rms, stun
  */
int get_url(char *rece_api, char *rece_mqtt, char *rece_ntp, char *rece_rms, char *rece_stun)
{
	 int  ret_val = 0, ret_val_default = 0;
	 memset(MYCIN_API_HOST_NAME, 0, strlen(MYCIN_API_HOST_NAME));
	 memset(MYCIN_HOST_NAME, 0, strlen(MYCIN_HOST_NAME));
	 memset(g_acSNTPserver_single, 0, strlen(g_acSNTPserver_single));
	 memset(relay_url_ch, 0, strlen(relay_url_ch));
	 memset(SERVER_URL, 0, strlen(SERVER_URL));

/***************************************get_api***********************************************/
	 ret_val = userconfig_get(MYCIN_API_HOST_NAME, api_size, API_URL);
	 if(ret_val <= 0)
	 {
		  ret_val_default = 0;
		  set_url_default(ret_val_default);
		  userconfig_get(MYCIN_API_HOST_NAME, api_size, API_URL);
	 }
	 UART_PRINT("GET_API_VALUE %d\r\n", ret_val);
	 memcpy(rece_api, MYCIN_API_HOST_NAME, strlen(MYCIN_API_HOST_NAME));

/*********************************************************************************************/


/**************************************get_mqtt***********************************************/
	 ret_val = userconfig_get(MYCIN_HOST_NAME, mqtt_size, MQTT_URL);
	 if (ret_val <= 0)
	 {
		 ret_val_default = 1;
		 set_url_default(ret_val_default);
		 userconfig_get(MYCIN_HOST_NAME, mqtt_size, MQTT_URL);
	 }
	 UART_PRINT("GET_MQTT_VALUE: %d\r\n", ret_val);
	 memcpy(rece_mqtt, MYCIN_HOST_NAME, strlen(MYCIN_HOST_NAME));

/*******************************************************************************************/


/**************************************get_ntp*********************************************/

	 ret_val = userconfig_get(g_acSNTPserver_single, ntp_size, NTP_URL);
	 if (ret_val <= 0)
	 {
		ret_val_default = 2;
		set_url_default(ret_val_default);
		userconfig_get(g_acSNTPserver_single, ntp_size, NTP_URL);
	 }
	 UART_PRINT("GET_NTP_VALUE: %d\r\n", ret_val);
	 memcpy(rece_ntp, g_acSNTPserver_single, strlen(g_acSNTPserver_single));

/******************************************************************************************/


/*************************************get_rms**********************************************/

	 ret_val = userconfig_get(relay_url_ch, rms_size, RMS_URL);
	 if (ret_val <= 0)
	 {
		ret_val_default = 3;
	 	set_url_default(ret_val_default);
	 	userconfig_get(relay_url_ch, rms_size, RMS_URL);
	 }
	 UART_PRINT("GET_RMS_VALUE: %d\r\n", ret_val);
	 memcpy(rece_rms, relay_url_ch, strlen(relay_url_ch));

/******************************************************************************************/


/*************************************get_stun*********************************************/

	  ret_val = userconfig_get(SERVER_URL, stun_size, STUN_URL);
	  if (ret_val <= 0)
	  {
		 ret_val_default = 4;
	  	 set_url_default(ret_val_default);
	  	 userconfig_get(SERVER_URL, stun_size, STUN_URL);
	  }
	  UART_PRINT("GET_STUN_VALUE: %d\r\n", ret_val);
	  memcpy(rece_stun,  SERVER_URL, strlen( SERVER_URL));
	userconfig_flash_write();
/******************************************************************************************/
	return ;
}
char cam_settings[CAM_LEN]={0};
int get_cam_default()
{
	int ret_val = 0;
	char temp_str[CAM_LEN]=CAM_SETTING_DEFAULT;
	ret_val = userconfig_get(cam_settings, CAM_LEN, CAM_ID);
	if(ret_val <= 0)
	{
		memset(cam_settings,0,CAM_LEN);
		memcpy(cam_settings,temp_str,CAM_LEN);
		//UART_PRINT("\r\nSet cam default: flicker %d UD %d LR %d brate %d kbps framerate %d resolution %d\r\n", cam_settings[0],cam_settings[1],cam_settings[2],cam_settings[3]*256+cam_settings[4],cam_settings[5],cam_settings[6]*256+cam_settings[7]);
		userconfig_set(cam_settings, CAM_LEN, CAM_ID);
		userconfig_flash_write();
	}
	//UART_PRINT("\r\nSetting ret=%d: flicker %d UD %d LR %d brate %d kbps framerate %d resolution %d\r\n", ret_val, cam_settings[0],cam_settings[1],cam_settings[2],cam_settings[3]*256+cam_settings[4],cam_settings[5],cam_settings[6]*256+cam_settings[7]);
	return 0;
}

void cam_settings_update()
{
	userconfig_set(cam_settings, CAM_LEN, CAM_ID);
	userconfig_flash_write();
}
extern void init_buffer();
extern int read_buffer_anyka(char l_response, int l_timeout);
extern int32_t spi_cmd_ps_info(char *out_buf);
// l_option=0->SPI on/off + setting/readback
// l_option=8x->SPI on/off + voice_prompt x
// l_option=1->Not SPI on/off + NTP
// l_option=2->Not SPI on/off + Setting/no_readback
int set_down_anyka(int l_option)
{
	int ret_val = 0;
	char temp_str[CAM_LEN]={0}, *l_spi_buf;
	int used_bytes=0;
	l_spi_buf=malloc(1024);
	int i;
	if(l_option&0x80){
		memset(l_spi_buf, 0x00, sizeof(l_spi_buf));
		l_spi_buf[0] = CMD_VOICEPROMPT;
		l_spi_buf[1] = (l_option&0x7f);
		spi_add_checksum(l_spi_buf);
		//RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		sendbuf_put_spi_buff(l_spi_buf);
		UART_PRINT("\r\nPlay %02x\r\n", l_option&0x7f); 
	}
		
	// Send Time
	if (l_option==1){
		uint32_t current_time_s = dt_get_time_s()+timezone_s;
		memset(l_spi_buf, 0x00, sizeof(l_spi_buf));

		l_spi_buf[0] = CMD_NTP;
		l_spi_buf[1] = SET_ID;
		l_spi_buf[2] = (current_time_s>>24)&0xff;
		l_spi_buf[3] = (current_time_s>>16)&0xff;
		l_spi_buf[4] = (current_time_s>>8)&0xff;
		l_spi_buf[5] = (current_time_s)&0xff;
		spi_add_checksum(l_spi_buf);
		//RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		//RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		sendbuf_put_spi_buff(l_spi_buf);
		sendbuf_put_spi_buff(l_spi_buf);
		UART_PRINT("\r\nNTP = %d | %d | %d\r\n", l_spi_buf[0], l_spi_buf[1], current_time_s);
		free(l_spi_buf);
		return 0;
	}

	if ((l_option==0)||(l_option==2)){
		ret_val = userconfig_get(temp_str, CAM_LEN, CAM_ID);
		UART_PRINT("\r\n userconfig_get ret_val %d\r\n", ret_val);
		if(ret_val > 0)
		{
			// Send flicker
			memset(l_spi_buf, 0x00, sizeof(l_spi_buf));
			l_spi_buf[0] = CMD_FLICKER;
			l_spi_buf[1] = SET_ID;
			memcpy(l_spi_buf+2,temp_str,CAM_LEN);
			spi_add_checksum(l_spi_buf);
			//RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
			sendbuf_put_spi_buff(l_spi_buf);
			for (i=0;i<CAM_LEN;i++)
				UART_PRINT("Byte %d, value %d\r\n",i,temp_str[i]);
			/*
			UART_PRINT("FLICKER = %d\r\n", temp_str[0]);
			UART_PRINT("FLIP_UD = %d\r\n", temp_str[1]);
			UART_PRINT("FLIP_LR = %d\r\n", temp_str[2]);
			UART_PRINT("BITRATE = %d\r\n", ((int)temp_str[3]<<8)+temp_str[4]);
			UART_PRINT("FRAMERATE = %d\r\n", temp_str[5]);
			UART_PRINT("RESOLUTION = %d\r\n", ((int)temp_str[6]<<8)+temp_str[7]);
			UART_PRINT("SPK_VOL = %d\r\n", temp_str[9]);
			UART_PRINT("MIC_VOL = %d\r\n", temp_str[10]);
			UART_PRINT("GOP = %d\r\n", temp_str[11]);
			UART_PRINT("Limitbitrate = %d\r\n", temp_str[13]);*/
			
			if (l_option==0)
				read_buffer_anyka(RESPONSE_FLICKER, 8);

			memset(l_spi_buf, 0x00, sizeof(l_spi_buf));
			l_spi_buf[0] = CMD_AESKEY;
			l_spi_buf[1] = SET_ID;
			//l_spi_buf[2] = 16;
			//memcpy(l_spi_buf + 3, p2p_key_char, 16);
			spi_cmd_ps_info(l_spi_buf + 2);
		    spi_add_checksum(l_spi_buf);
		    //RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		    sendbuf_put_spi_buff(l_spi_buf);
		    UART_PRINT("\r\nAESKEY");
			if (l_option==0)
				read_buffer_anyka(RESPONSE_AESKEY, 8);
				
			memset(l_spi_buf, 0x00, sizeof(l_spi_buf));
			l_spi_buf[0] = CMD_V2UDID;
			l_spi_buf[1] = SET_ID;
			l_spi_buf[2] = strlen(g_mycinUser.device_udid);
			memcpy(l_spi_buf + 3, g_mycinUser.device_udid, strlen(g_mycinUser.device_udid));
		    spi_add_checksum(l_spi_buf);
		    //RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		    sendbuf_put_spi_buff(l_spi_buf);
		    UART_PRINT("\r\nUDID");
			if (l_option==0)
				read_buffer_anyka(RESPONSE_V2UDID, 8);
				
			memset(l_spi_buf, 0x00, sizeof(l_spi_buf));
			l_spi_buf[0] = CMD_TOKEN;
			l_spi_buf[1] = SET_ID;
			l_spi_buf[2] = strlen(g_mycinUser.authen_token);
			memcpy(l_spi_buf + 3, g_mycinUser.authen_token, strlen(g_mycinUser.authen_token));
		    spi_add_checksum(l_spi_buf);
		    //RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		    sendbuf_put_spi_buff(l_spi_buf);
		    UART_PRINT("\r\nDTok");
			if (l_option==0)
				read_buffer_anyka(RESPONSE_TOKEN, 8);
				
			memset(l_spi_buf, 0x00, sizeof(l_spi_buf));
			l_spi_buf[0] = CMD_UPLOADURL;
			l_spi_buf[1] = SET_ID;
			l_spi_buf[2] = strlen(MYCIN_API_HOST_NAME);
			memcpy(l_spi_buf + 3, MYCIN_API_HOST_NAME, strlen(MYCIN_API_HOST_NAME));
		    spi_add_checksum(l_spi_buf);
		    //RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		    sendbuf_put_spi_buff(l_spi_buf);
		    UART_PRINT("\r\nMU_URL"); 
			if (l_option==0)
				read_buffer_anyka(RESPONSE_UPLOADURL, 8);
		}
	}

	free(l_spi_buf);
	return 0;
}


int set_down_anyka_cmd(char* l_data, int response_wait_s)
{
	spi_add_checksum(l_data);
	//RingBuffer_Write(ring_buffer_send, (char *)l_data, 1024);
	sendbuf_put_spi_buff(l_data);
	if (response_wait_s!=0)
		read_buffer_anyka(l_data[0]&0x7f, response_wait_s);
	return 0;
}

int32_t AKOTA_SPIWrite(uint32_t bytesReceived, char *buff, uint32_t len, uint32_t l_total_length)
{
	char l_spi_buf[1024];
	int l_len=8+len;
	int l_nextpost=0;
	l_nextpost = bytesReceived + len;
	int l_count=0;
	while(1){
		l_spi_buf[0] = CMD_AKOTA;
		l_spi_buf[1] = 0x01;
		// Len(bin+position+data)
		l_spi_buf[2] = (l_len>>8)&0xff;
		l_spi_buf[3] = l_len&0xff;
		// Bin length
		l_spi_buf[4] = (l_total_length>>24)&0xff;
		l_spi_buf[5] = (l_total_length>>16)&0xff;
		l_spi_buf[6] = (l_total_length>>8)&0xff;
		l_spi_buf[7] = (l_total_length)&0xff;
		// Position
		l_spi_buf[8] = (bytesReceived>>24)&0xff;
		l_spi_buf[9] = (bytesReceived>>16)&0xff;
		l_spi_buf[10] = (bytesReceived>>8)&0xff;
		l_spi_buf[11] = (bytesReceived)&0xff;
		// data
		memcpy(l_spi_buf+12, buff, len);
		spi_add_checksum(l_spi_buf);
		//RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
		sendbuf_put_spi_buff(l_spi_buf);
		UART_PRINT("\r\nOTA AK Send %d at %d len %d ", CMD_AKOTA, bytesReceived, len);
		if (read_buffer_anyka_1(l_nextpost)==l_nextpost){
			UART_PRINT("\r\nOTA AK BlkEnd");
			break;
		}
		l_count++;
		if (l_count>=20){
			UART_PRINT("\r\nOTA AK Timeout\r\n");
			return -1;
		}
	}
    return 0;
}
int32_t AKOTA_SPIWrite_MD5(char *buff)
{
	char l_spi_buf[1024];
	
	l_spi_buf[0] = CMD_AKOTA;
	l_spi_buf[1] = 0x05;
	l_spi_buf[2] = 0x00;
	l_spi_buf[3] = 1+32;
	l_spi_buf[4] = 1;
	memcpy(l_spi_buf+5, buff, 32);
	spi_add_checksum(l_spi_buf);
	//RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
	sendbuf_put_spi_buff(l_spi_buf);
	UART_PRINT("\r\nMD5 %s\r\n", buff);
	if (read_buffer_anyka(CMD_AKOTA, 70)==CMD_AKOTA)
		return 0;
	else
		return -1;
}
int32_t spi_write_raw(char *buff, int len)
{
	char l_spi_buf[1024]={0};
	memcpy(l_spi_buf,buff,len);
	spi_add_checksum(l_spi_buf);
	//RingBuffer_Write(ring_buffer_send, (char *)l_spi_buf, 1024);
	sendbuf_put_spi_buff(l_spi_buf);
	read_buffer_anyka(l_spi_buf[0]&0x7f, 4);
}
int32_t ak_get_version_spi()
{
	char l_spi_buf[1024];
	l_spi_buf[0]=CMD_VER;
	l_spi_buf[1]=SET_ID;
	set_down_anyka_cmd(l_spi_buf,4);
	UART_PRINT("AK ver %s\r\n", ak_version);
}
char g_version_8char[9]={0};
char g_strMacAddr[13]={0};
int system_init(void)
{
    int32_t ret_val = 0;
    
    // Power down AK module by default
    ak_power_set(0); 
    
    UART_PRINT("CC3200 version=%s\r\n", SYSTEM_VERSION);
    fw_version_format(g_version_8char,SYSTEM_VERSION);
    
    //BUG: can not pass user config init function
    // InitializeAppVariables();
    // ConfigureSimpleLinkToDefaultState();
    sl_Start(0, 0, 0);
    // Network_IF_InitDriver(ROLE_AP);
    // Don't print any log here, it will cause system hang
    userconfig_init();
    userconfig_flash_read();
    osi_SyncObjSignal(&g_userconfig_init);
    ret_val = userconfig_get(ap_ssid, MAX_AP_SSID_LEN, WIFI_SSID_ID);
    if (ret_val < 0)
    {
        UART_PRINT("Can not read WIFI SSID, error=%d\r\n", ret_val);
    }
    ret_val = userconfig_get(ap_pass, MAX_AP_PASS_LEN, WIFI_KEY_ID);
    if (ret_val < 0)
    {
        UART_PRINT("Can not read WIFI KEY, error=%d\r\n", ret_val);
    }
    ret_val = userconfig_get(ap_sec, MAX_AP_SEC_LEN, WIFI_SEC_ID);
    /*if (ret_val < 0)
    {
        UART_PRINT("Can not read WIFI SEC, error=%d\r\n", ret_val);
    }*/
    ret_val = userconfig_get(timeZone, 32, TIMEZONE_ID);
    timezone_s=atoi(timeZone);
    timezone_s=timezone_s*60*60;
    UART_PRINT("Ret %d, TIME ZONE is %s %d\r\n", ret_val, timeZone, timezone_s);
    get_url_default();
    get_cam_default();
    UART_PRINT("\r\nHOST_NAME = %s\r\n", MYCIN_API_HOST_NAME);
    // NOT USED
    // if (userconfig_get(topicID, sizeof(topicID), TOKEN_ID)<0)
    // {
    //   UART_PRINT("Can not read MQTT TOPIC");
    //   // sl_Stop(200);
    //   // NOT USED
    //   // return -1;
    // }
    // UART_PRINT("AP_SSID: %s\n", ap_ssid);
    // UART_PRINT("AP_PASS: %s\n", ap_pass);
    // Get user authen_token
    memset((char *)&g_mycinUser, 0, sizeof(g_mycinUser));
    ret_val = userconfig_get(g_mycinUser.authen_token, MAX_AUTHEN_TOKEN, MASTERKEY_ID);
    if (ret_val < 0) {
        UART_PRINT("Can not read authen_token\r\n");
        // FIXME: need return error
    }
    char device_udid[MAX_UID_SIZE] = {0};
    get_uid(device_udid);
    memcpy(g_mycinUser.device_udid, device_udid, strlen(device_udid));
    memcpy(g_mycinUser.user_token, g_mycinUser.authen_token, MAX_USER_TOKEN);
    get_mac_address(g_strMacAddr);
    
    
    UART_PRINT("mac addr: %s, udid: %s\n", g_strMacAddr, g_mycinUser.device_udid);
    return 0;
}

//*****************************************************************************
//@brief Function to reboot system
//
//*****************************************************************************

#include "state_machine.h"
#include "wlan.h"
void system_reboot()
{
	/*
    PRCMMCUReset(TRUE);
    // PRCMSOCReset();
	*/
    
	ak_power_down();
    
	// Another reboot method
	sl_WlanPolicySet(SL_POLICY_PM , SL_LOW_LATENCY_POLICY, NULL, 0);
	MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
	MAP_UtilsDelay(8000000);
	MAP_PRCMHibernateIntervalSet(330);
	MAP_PRCMHibernateEnter();
	
    while(1);
 
}

void system_reboot_1()
{

    PRCMMCUReset(TRUE);
    // PRCMSOCReset();
 
}

//*****************************************************************************
//@brief Function to check device register or not
//
//*****************************************************************************
int cam_mode = -1;
int system_IsRegistered()
{
    int modeID;
// sl_Start(0, 0, 0);
    //  userconfig_init();
//  userconfig_flash_read();
    if ((modeID = userconfig_get(NULL, 0, MODE_ID)) < 0)
    {
//    sl_Stop(200);
    	 UART_PRINT("MODE_GET_CONFIG_FAIL, error=%d\r\n", modeID);
        return -1;
    }
    if (modeID != DEV_REGISTERED)
    {
//    sl_Stop(200);
//    	UART_PRINT("MODE_PARING, error=%d\r\n", modeID);
    	cam_mode = DEV_NOT_REGISTERED;
        return -1;
    }
    else
    {
//	    UART_PRINT("MODE_ONLINE\r\n");
	    cam_mode = DEV_REGISTERED;
//    sl_Stop(200);
        return 0;
    }
}

int system_Deregister(void)
{
    int32_t lRetVal;
    userconfig_set(NULL, DEV_NOT_REGISTERED, MODE_ID);
    userconfig_flash_write();
    /*
    * Remove all conntection profile
    */
    
    lRetVal = sl_WlanProfileDel(0xFF);    
    //ASSERT_ON_ERROR(lRetVal);
    ConfigureSimpleLinkToDefaultState();
    /*InitializeAppVariables();*/
    return 0;
}

//******************************************************************************
//
//******************************************************************************
extern char timeZone[];
void uart_print_config_log(int l_param, int l_ret){
	UART_PRINT("Config %d %d\r\n",l_param,l_ret);
}
int system_registration()
{
    int lRetVal, i;

	char temp_str[CAM_LEN]=CAM_SETTING_DEFAULT;
	memcpy(cam_settings, temp_str, CAM_LEN);
	cam_settings[8]=UPGRADE_JUST_REGISTER;
    
#ifdef NTP_CHINA
    adc_bat_read();
#else
    adc_bat1_read();
#endif
    if (low_bat_push){
        g_input_state = LED_STATE_START_PAIR_MODE_LOWBAT;
    }else
        g_input_state=LED_STATE_START_MODE_WIFI_SETUP;
        
    UART_PRINT("Network init start\n");
    sl_WlanProfileDel(0xFF);    
    ConfigureSimpleLinkToDefaultState();
    InitializeAppVariables();
    
    // sl_Start(0, 0, 0);
    lRetVal = Network_IF_WifiSetMode(ROLE_STA);
    if (lRetVal < 0)
    {
        return REGISTRATION_ERROR_SETMODESTA;
    }
    UART_PRINT("Network init done\n");
    // Network_IF_InitDriver(ROLE_AP);
    Network_IF_provisionAP();
	set_down_anyka(0x80|PROMPT_BEEP);
    UART_PRINT("Scan wifi done\r\n");
    //Re-Start SimpleLink in AP Mode
    lRetVal = sl_Stop(0xFF);
    lRetVal = Network_IF_WifiSetMode(ROLE_AP);
    // lRetVal = sl_WlanSetMode(ROLE_AP);
    if (lRetVal < 0)
    {
        return REGISTRATION_ERROR_SETMODEAP;
    }
    //Stop Internal HTTP Server
    lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
    if (lRetVal < 0)
    {
        REGISTRATION_ERROR_STOPHTTP;
    }
    //Run APPS HTTP Server
    spi_thread_pause = 0;
    HttpServerInitAndRun(NULL);
	spi_thread_pause = 1;
	if(httpServerOperationTime>(10*REGISTRATION_TIMEOUT_S)){
		return REGISTRATION_ERROR_REGTIMEOUT;
	}
	osi_Sleep(100);
	set_down_anyka(0x80|PROMPT_JINGLE);
    g_input_state = LED_STATE_NONE;
    //Network_IF_DeInitDriver();

    #if 0
    // Initialize AP security params
    #define SSID                "Hubble-Mir"
    #define SSID_KEY            "12345678@"
    SlSecParams_t SecurityParams = {0};
    SecurityParams.Key = (signed char*)SSID_KEY;
    SecurityParams.KeyLen = strlen(SSID_KEY);
    SecurityParams.Type = SL_SEC_TYPE_WPA;

    //
    // Connect to the Access Point
    //
    lRetVal = Network_IF_ConnectAP(SSID,SecurityParams);
    if(lRetVal < 0)
    {
       Report("Connection to AP failed\n\r");
       LOOP_FOREVER();
    }

    #else
    i = 0;
    lRetVal = Network_IF_Connect2AP_static_ip(ap_ssid, ap_pass, 0);
    if (lRetVal<0){
        //osi_Sleep(2000);
        UART_PRINT("Connect to AP fail\n");
        g_input_state=LED_STATE_NONE;
        return REGISTRATION_ERROR_CONNECTAP;
    }
    #endif
    // register
    myCinApiRegister_t mycinRegister;
    UART_PRINT("User API key: %s\n", apiKey);

    int api_key_len = strlen(apiKey);
    if (api_key_len > MAX_USER_TOKEN) {
        api_key_len = MAX_USER_TOKEN - 1;
    }
    memset(g_mycinUser.user_token, 0x00, MAX_USER_TOKEN);
    strncpy(g_mycinUser.user_token, apiKey, api_key_len);
    // todo: debug, danger
    // strncpy(mycinUser.user_token, "R1C4vr8ttv8qXCVXkoc1", sizeof(mycinUser.user_token));
    //strncpy(mycinUser.device_udid, udid, sizeof(mycinUser.device_udid));

    char l_mqtt_clientid[MQTT_CLIENTID_LEN+1]={0};
    char l_mqtt_user[MQTT_USER_LEN+1]={0};
    char l_mqtt_pass[MQTT_PASS_LEN+1]={0};
    char l_mqtt_topic[MQTT_TOPIC_LEN+1]={0};
    char l_mqtt_servertopic[MQTT_SERVERTOPIC_LEN+1]={0};
    
    mycinRegister.mqtt_clientid=l_mqtt_clientid;
    mycinRegister.mqtt_user=l_mqtt_user;
    mycinRegister.mqtt_pass=l_mqtt_pass;
    mycinRegister.mqtt_topic=l_mqtt_topic;
    mycinRegister.mqtt_servertopic=l_mqtt_servertopic;

    if (myCinRegister(&g_mycinUser, &mycinRegister) >= 0) {
        //UART_PRINT("RegPostdone: device_id: %s, device_token %s\r\n", mycinRegister.registration_id,g_mycinUser.authen_token);
        // RESET VARIABLE
        memset(ps_ip, 0x00, P2P_PS_IP_LEN);
        ps_port = 0;
        memset(p2p_key_char, 0x00, P2P_KEY_CHAR_LEN);
        memset(p2p_rn_char, 0x00, P2P_RN_CHAR_LEN);
        
        while (p2p_init(1)<0){
            osi_Sleep(5000);
        }
        //UART_PRINT("Register to P2P server --- DONE\r\n");
        int ret_val = 0;
        // store data to flash
        ret_val = userconfig_set(g_mycinUser.authen_token, strlen(g_mycinUser.authen_token), MASTERKEY_ID);
        if (ret_val < 0) {
            UART_PRINT("Cannot write authen_token, error=%d\r\n", ret_val);
        }
        ret_val = userconfig_set(ap_ssid, strlen(ap_ssid), WIFI_SSID_ID);
        if (ret_val < 0) {
            //UART_PRINT("Cannot write ap_ssid, error=%d\r\n", ret_val);
            uart_print_config_log(WIFI_SSID_ID, ret_val);
        }
        ret_val = userconfig_set(ap_pass, strlen(ap_pass), WIFI_KEY_ID);
        if (ret_val < 0) {
            //UART_PRINT("Cannot write ap_pass, error=%d\r\n", ret_val);
            uart_print_config_log(WIFI_KEY_ID, ret_val);
        }
        ret_val = userconfig_set(timeZone, strlen(timeZone), TIMEZONE_ID);
        if (ret_val < 0) {
            UART_PRINT("Cannot write timeZone, error=%d\r\n", ret_val);
        }
        // p2p
        ret_val = userconfig_set(ps_ip, P2P_PS_IP_LEN, P2P_SERVER_IP_ID);
        if (ret_val < 0) {
            UART_PRINT("Cannot write ps_ip, error=%d\r\n", ret_val);
        }
        ret_val = userconfig_set(str_ps_port, strlen(str_ps_port), P2P_SERVER_PORT_ID);
        if (ret_val < 0) {
            UART_PRINT("Cannot write ps_port, error=%d\r\n", ret_val);
        }
        ret_val = userconfig_set(p2p_key_char, P2P_KEY_CHAR_LEN, P2P_KEY_ID);
        if (ret_val < 0) {
            UART_PRINT("Cannot write p2p_key_char, error=%d\r\n", ret_val);
        }
        ret_val = userconfig_set(p2p_rn_char, P2P_RN_CHAR_LEN, P2P_RAN_NUM_ID);
        if (ret_val < 0) {
            UART_PRINT("Cannot write p2p_rn_char, error=%d\r\n", ret_val);
        }

        ret_val = userconfig_set(mycinRegister.mqtt_clientid, MQTT_CLIENTID_LEN, MQTT_CLIENTID);
        if (ret_val < 0) {
            uart_print_config_log(MQTT_CLIENTID, ret_val);
        }
        ret_val = userconfig_set(mycinRegister.mqtt_user, MQTT_USER_LEN, MQTT_USER);
        if (ret_val < 0) {
            uart_print_config_log(MQTT_USER, ret_val);
        }
        ret_val = userconfig_set(mycinRegister.mqtt_pass, MQTT_PASS_LEN, MQTT_PASS);
        if (ret_val < 0) {
            uart_print_config_log(MQTT_PASS, ret_val);
        }
        ret_val = userconfig_set(mycinRegister.mqtt_topic, MQTT_TOPIC_LEN, MQTT_TOPIC);
        if (ret_val < 0) {
            uart_print_config_log(MQTT_TOPIC, ret_val);
        }
        ret_val = userconfig_set(mycinRegister.mqtt_servertopic, MQTT_SERVERTOPIC_LEN, MQTT_SERVERTOPIC_ID);
        if (ret_val < 0) {
            uart_print_config_log(MQTT_SERVERTOPIC_ID, ret_val);
        }
        
        ret_val = userconfig_set(ap_sec, strlen(ap_sec), WIFI_SEC_ID);
        if (ret_val < 0) {
            uart_print_config_log(WIFI_SEC_ID, ret_val);
        }
        
        // set register flag
        ret_val = userconfig_set(NULL, DEV_REGISTERED, MODE_ID);
        if (ret_val < 0) {
            UART_PRINT("Cannot write boot flag, error=%d\r\n", ret_val);
        }

        // Update camera setting as well as other changes
        cam_settings_update();
    }
    else {
        UART_PRINT("fail to register\n");
        lRetVal = REGISTRATION_ERROR_REGPOSTFAIL;
    }

    return lRetVal;
}
/*
myCinUserInformation_t* get_mycin_user(void) {
    return &g_mycinUser;
}*/

void escapeJsonString(char* in_str, char* out_str){
	int l_len=strlen(in_str);
	int i,j=0;
	for(i=0;i<l_len;i++){
		switch (in_str[i]){
			case '\\':
			case '"':
			case '/':
			case '\b':
			case '\f':
			case '\n':
			case '\r':
			case '\t':
				out_str[j++]='\\';
				break;
			default:
				break;
		}
		out_str[j++]=in_str[i];
	}
	out_str[j]=0x00;
}

void print_time(void)
{
	uint32_t current_time_s = dt_get_time_s()+timezone_s;
	UART_PRINT("Now:%d", current_time_s);
}
void print_manytimes(char *l_input){
	int i;
	for(i=0;i<10;i++)
		UART_PRINT("%s\r\n", l_input);
}

void fw_version_format(char* l_output, char* l_input){
	l_output[0]=l_input[0];
	l_output[1]=l_input[1];
	l_output[2]='.';
	l_output[3]=l_input[2];
	l_output[4]=l_input[3];
	l_output[5]='.';
	l_output[6]=l_input[4];
	l_output[7]=l_input[5];
	l_output[8]=0;
}
