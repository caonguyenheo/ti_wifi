#ifndef _USER_CONFIGURATION_
#define _USER_CONFIGURATION_

#define LINUX   0
#define NRF51   1
#define REALTEK 2
#define CC3200     3
#define PLATFORM CC3200

/*Define for specific flash*/
#if (PLATFORM ==LINUX)
    int32_t test_array[256] = {0};
    #define FLASH_BASEADDR test_array // the start address of the block use for storing user information
#elif (PLATFORM == NRF51)
    #include "nrf.h"
    #include "bsp.h"
    #include "nordic_common.h"
    #define FLASH_BASEADDR  (uint32_t *)(BOOTLOADER_REGION_START - NRF_FICR->CODEPAGESIZE)
    #define GSM_APN_SUPPORT
#elif (PLATFORM == REALTEK)
    #include "flash_api.h"
    //#define FLASH_BASEADDR 0x0006FE00
    #define FLASH_BASEADDR 0x000FE000
#endif

#if((PLATFORM == REALTEK)||(PLATFORM == CC3200))
#define WIFI_CONFIG_SUPPORT
#endif
    
#define WORD_LEN                    4
#define MASTERKEY_LEN               (WORD_LEN*8)
#define CHANNELTOKEN_LEN            (WORD_LEN*8)
#define TIMEZONE_LEN                (WORD_LEN*2)
#ifdef GSM_APN_SUPPORT
    #define GSMAPN_LEN              (WORD_LEN*8)
#endif
#ifdef WIFI_CONFIG_SUPPORT
    #define WIFI_SSID_LEN           (WORD_LEN*8)
    #define WIFI_KEY_LEN            (WORD_LEN*16)
#endif
#define SERIAL_LEN                  (WORD_LEN*8)
#define DATECODE_LEN                (WORD_LEN*2)

#define P2P_SERVER_IP_LEN           (WORD_LEN*5)
#define P2P_SERVER_PORT_LEN         (WORD_LEN*2)
#define P2P_KEY_LEN                 (WORD_LEN*5)
#define P2P_RAN_NUM_LEN             (WORD_LEN*5)
#define TIMEOUT_LEN                 (WORD_LEN*1)

#define API_LEN                     (WORD_LEN*10)
#define MQTT_LEN                    (WORD_LEN*10)
#define NTP_LEN                     (WORD_LEN*10)
#define RMS_LEN                     (WORD_LEN*10)
#define STUN_LEN                    (WORD_LEN*10)
#define CAM_LEN                     (WORD_LEN*5)

#define MQTT_CLIENTID_LEN           (WORD_LEN*16)
#define MQTT_USER_LEN               (WORD_LEN*16)
#define MQTT_PASS_LEN               (WORD_LEN*16)
#define MQTT_TOPIC_LEN              (WORD_LEN*24)
#define MQTT_SERVERTOPIC_LEN        (WORD_LEN*24)

#define WIFI_SEC_LEN                (WORD_LEN*2)
/* Define all unique id of all element in userconfig struct*/
enum{
    MODE_ID = 0,
    MASTERKEY_ID,
    TOKEN_ID,
    TIMEZONE_ID,
#ifdef GSM_APN_SUPPORT
    GSM_APN_ID,
#endif
#ifdef WIFI_CONFIG_SUPPORT
    WIFI_SSID_ID,
    WIFI_KEY_ID,
#endif
    SERIAL_ID,
    DATECODE_ID,
    // P2P config
    P2P_SERVER_IP_ID,
    P2P_SERVER_PORT_ID,
    P2P_KEY_ID,
    P2P_RAN_NUM_ID,
    TIMEOUT_ID,
	API_URL,
	MQTT_URL,
	NTP_URL,
	RMS_URL,
	STUN_URL,
	CAM_ID,
	MQTT_CLIENTID,
	MQTT_USER,
	MQTT_PASS,
	MQTT_TOPIC,
	WIFI_SEC_ID,
	MQTT_SERVERTOPIC_ID,
    MAX_ELEMENT 
};

typedef struct _UserConfig 
{
    int mode;
    
    int master_key_len;
    unsigned char master_key[MASTERKEY_LEN];
    int master_key_checksum;
    
    int channel_token_len;
    unsigned char channel_token[CHANNELTOKEN_LEN];
    int channel_token_checksum;
    
    int time_zone_len;
    unsigned char time_zone[TIMEZONE_LEN]; 
#ifdef GSM_APN_SUPPORT
    int gsm_apn_len;
    unsigned char gsm_apn[GSMAPN_LEN];
#endif
#ifdef WIFI_CONFIG_SUPPORT
    int wifi_ssid_len;
    unsigned char wifi_ssid[WIFI_SSID_LEN];

    int wifi_key_len;
    unsigned char wifi_key[WIFI_KEY_LEN];
    
#endif
    int serial_len;
    unsigned char serial[SERIAL_LEN];
    
    int datecode_len;
    unsigned char datecode[DATECODE_LEN];

    // P2P config
    int p2p_server_ip_len;
    unsigned char p2p_server_ip[P2P_SERVER_IP_LEN];

    int p2p_server_port_len;
    unsigned char p2p_server_port[P2P_SERVER_PORT_LEN];

    int p2p_key_len;
    unsigned char p2p_key[P2P_KEY_LEN];

    int p2p_ran_num_len;
    unsigned char p2p_ran_num[P2P_RAN_NUM_LEN];

    int api_len;
	unsigned char api_num[API_LEN];

	int mqtt_len;
	unsigned char mqtt_num[MQTT_LEN];

	int ntp_len;
	unsigned char ntp_num[NTP_LEN];

	int rms_len;
	unsigned char rms_num[RMS_LEN];

	int stun_len;
	unsigned char stun_num[STUN_LEN];

	int cam_len;
	unsigned char cam_num[CAM_LEN];

	int mqtt_clientid_len;
	unsigned char mqtt_clientid[MQTT_CLIENTID_LEN];

	int mqtt_user_len;
	unsigned char mqtt_user[MQTT_USER_LEN];
    
	int mqtt_pass_len;
	unsigned char mqtt_pass[MQTT_USER_LEN];
	
	int mqtt_topic_len;
	unsigned char mqtt_topic[MQTT_TOPIC_LEN];
	
	int wifi_sec_len;
	unsigned char wifi_sec[WIFI_SEC_LEN];
	
	int mqtt_servertopic_len;
	unsigned char mqtt_servertopic[MQTT_SERVERTOPIC_LEN];
} UserConfig;

int userconfig_init(void);
int userconfig_set(char *data_buffer, int data_len, int id);
int userconfig_get(char *data_buffer, int buf_sz, int id);
int userconfig_flash_read();
int userconfig_flash_write();
unsigned char *get_userconfig_pointer();
int get_userconfig_size();

#if (PLATFORM == NRF51)
// Bootloader
void bootloader_call(void);
#endif
#endif // end _USER_CONFIGURATION_
