#ifndef __SYSTEM_H_
#define __SYSTEM_H_

#include "server_api.h"
#include "cc3200_system.h"
#include <stdint.h>

#define NTP_URL_DEFAULT         "pool.ntp.org"
#define API_URL_DEFAULT         "api.cinatic.com"
#define MQTT_URL_DEFAULT        "mqtt.cinatic.com"
#define RMS_URL_DEFAULT         "relay-monitor.cinatic.com"
#define STUN_URL_DEFAULT        "stun.cinatic.com"
#ifdef NTP_CHINA
#define NUM_NTP_SERVER          4
#else
#define NUM_NTP_SERVER          5
#endif
#define NTP_RUL_MAXLEN          32
#define WIFI_DEFAULT_ADHOC_CHANNEL  6
#define STARTUP_MODE_INFRA          249
#define DEV_NOT_REGISTERED          0xC3
#define DEV_REGISTERED              0xAB
#define FLICKER_DEFAULT             50
#ifdef NTP_CHINA
#define BITRATE_DEFAULT             300
#else
#define BITRATE_DEFAULT             800
#endif
#define FRAMERATE_DEFAULT           12
#ifdef NTP_CHINA
#define RESOLUTION_DEFAULT          360
#define CDS_DELAY                   10
#else
#define RESOLUTION_DEFAULT          720
#define CDS_DELAY                   70
#endif
#define IMAGEFLIP_UD_DEFAULT        0
#define IMAGEFLIP_LR_DEFAULT        0
#define SPK_VOL_DEFAULT             10
#define MIC_VOL_DEFAULT             10
#define GOP_DEFAULT                 12
#define AUDIO_DUPLEX_DEFAULT        0
#define AUTO_BITRATE_DEFAULT        1
#define AUDIO_AGC_DEFAULT           1
#define CALLWAIT_DEFAULT            15
#define PWM_DEFAULT                 30
//#define AUDIO_DROP_PERIOD           300
//#define TALKBACK_DROP_PERIOD        300
#define UPGRADE_DEFAULT             0
#define UPGRADE_JUST_REGISTER       0xAB
//#define UPGRADE_CMDRCV              0xAC
#define UPGRADE_CMDRCV_HEAD         0xA0



#define  api_size  		40
#define  mqtt_size  	40
#define  ntp_size 	  	40
#define  rms_size  		40
#define  stun_size 		40
typedef struct nxcNetwork_st{
	int NetworkType;
	unsigned char ESSID[36];//max 32
	int AuthenType;
	int Channel;
	unsigned char Key[64];//63 printable
	int KeyIndex; //1,2,3
	int IPMode;//DHCP, STATIC
	unsigned char Gateway[16];//192.168.2.1
	unsigned char SubnetMask[16];//255.255.255.255
	unsigned char DefaultIP[16];//192.168.2.1
}nxcNetwork_st;
enum {

	ID_SET_FLICKER = 0,
	ID_SET_IMAGE_FLIP = 1,
	ID_SET_BITRATE = 2,
	ID_SET_FRAMERATE = 3,
	ID_SET_RESOLUTION = 4,
};
//  |1 byte flicker|1 byte flip updown|1 byte flip leftright|2 bytes bitrate|1 byte frame rate|2 byte resolutions|1 byte reboot after register|1 byte speaker volume|1 byte microphone volume|1 byte GOP|1 byte DUPLEX|1 byte auto bitrate|1byte AGC on/off|1byte CallWait|1byte PWM
#define CAM_SETTING_DEFAULT {FLICKER_DEFAULT,IMAGEFLIP_UD_DEFAULT,IMAGEFLIP_LR_DEFAULT,BITRATE_DEFAULT>>8,BITRATE_DEFAULT&0xff,FRAMERATE_DEFAULT,RESOLUTION_DEFAULT>>8,RESOLUTION_DEFAULT&0xff,UPGRADE_DEFAULT,SPK_VOL_DEFAULT,MIC_VOL_DEFAULT,GOP_DEFAULT,AUDIO_DUPLEX_DEFAULT,AUTO_BITRATE_DEFAULT,AUDIO_AGC_DEFAULT,CALLWAIT_DEFAULT,PWM_DEFAULT,0,0,0}
extern char cam_settings[];
/*******************************************************************************
* @brief Function to get device MAC address
*
*/
// void get_mac_address(char *mac_addr_str);

/*******************************************************************************
* @brief Function to backup API key and time zone from client
*
* @param[in] api_key: api key
* @param[in] timezone: time zone
*/
void set_server_authen(char *api_key, char *timezone);

/*******************************************************************************
* @brief Function to backup wireless information as SSID, password
*
*/
void set_wireless_save(char *wireless);
/*******************************************************************************
* @brief Function to backup wireless information as SSID, password
*
*/
int32_t set_wireless_config(char *ssid, char *pass, char *auth_type);

/*******************************************************************************
* @brief Registration device
*
*/
int system_registration(void);

/*******************************************************************************
* @brief Function to read factory information
*
*/
int system_init(void);

//*****************************************************************************
//@brief Function to reboot system
//
//*****************************************************************************
void system_reboot();

/*******************************************************************************
* @brief Function to check device whether register with server or not
*
*/
int system_IsRegistered();
int system_Deregister(void);

#define MAX_AP_SSID_LEN         32
#define MAX_AP_PASS_LEN         64
#define MAX_AP_SEC_LEN          8
extern char ap_ssid[];
extern char ap_pass[];
extern char ap_sec[];

extern char apiKey[];
extern char timeZone[];
extern char topicID[];
extern char authenToken[];
myCinUserInformation_t* get_mycin_user(void);
extern myCinUserInformation_t g_mycinUser;
extern char g_strMacAddr[];

#define REGISTRATION_ERROR_SETMODESTA (-50)
#define REGISTRATION_ERROR_SETMODEAP (-51)
#define REGISTRATION_ERROR_STOPHTTP (-52)
#define REGISTRATION_ERROR_INITDRVSTA (-53)
#define REGISTRATION_ERROR_CONNECTAP (-54)
#define REGISTRATION_ERROR_REGPOSTFAIL (-55)
#define REGISTRATION_ERROR_REGTIMEOUT (-56)

// Time in registration mode
extern int httpServerOperationTime;
#define REGISTRATION_TIMEOUT_S 150

extern int sd_file_status;
extern int server_req_upgrade;
extern int req_set_volume;
extern int get_ntp_status;
void escapeJsonString(char* in_str, char* out_str);
int sec_string_to_num(void);

enum led_input_state {
    LED_STATE_INVALID = -1,
    LED_STATE_NONE = 0,
    LED_STATE_POWER_UP,
    LED_STATE_AP_CONNECTING,
    LED_STATE_FACTORY_CONNECTED,
//    LED_STATE_FW_UPGRADE,
//    LED_STATE_VIDEO_STREAMING,
    LED_STATE_RE_CONNECTED_ROUTER,
    LED_STATE_USER_CONNECTED,
//    LED_STATE_PING_ROUTER_FAILED,
    LED_STATE_START_PAIR_MODE,
    LED_STATE_FACTORY_SEARCHING,
//    LED_STATE_CALL_PRESSED_BATTERY_GOOD,
//    LED_STATE_CALL_PRESSED_BATTERY_lOW,
    LED_STATE_START_MODE_WIFI_SETUP,
    LED_STATE_CALL_LOWBAT,
    LED_STATE_CALL_NORMALBAT,
    LED_STATE_START_PAIR_MODE_LOWBAT,
    LED_STATE_FTEST,
};
extern int g_input_state;
extern int g_current_state;
#define DINGDONG_COUNT_MAX 100
extern int true_bitrate;
extern unsigned int adc_bat_read(void);
void print_manytimes(char *l_input);
extern int pairing_in_progress;
void fw_version_format(char* l_output, char* l_input);
extern int timezone_s;
#endif
