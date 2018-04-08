#ifndef __CC3200_SYSTEM_H__
#define __CC3200_SYSTEM_H__
#include <stdint.h>


#define CALLGPIO 11
//#define NTP_CHINA

#define MAX_MAC_ADDR_SIZE  13
#define MAX_UID_SIZE       25
#define SYSTEM_VERSION     "020256"
#define VENDOR				0
#define TYPE_NUM			0
#define FAKE_CCAM 			0
#if (FAKE_CCAM)
#define STR_MODEL			"001"
#else
#define STR_MODEL			"003"
#endif
#define SCAN_TABLE_SIZE 15
#define LANGUAGE_ENGLISH 0x10
#define LANGUAGE_CHINESE 0x20
#define PROMPT_BEEP 1
#define PROMPT_JINGLE 2
#define PROMPT_SETUPFAIL 1
#define PROMPT_SETUPSUCCESS 2
#define PROMPT_SETUPTIMEOUT 3

#define PROMPT_SORRYNOTFREETOMEETNOW	4
#define PROMPT_YOUCANLEAVEIT			5
#define WELCOMEICOMINGOVERNOW			6
#define HOWCANIHELP						7
#define SORRYNOTATHOMENOW				8
#define PROMPT_FACTORY					12

#define CAM_LANGUAGE LANGUAGE_ENGLISH

int get_mac_address(char * mac_addr);
int set_mac_address(char *str_mac_addr);
int get_uid(char *uid);
int get_version(char *l_version);
void set_gm_pin(uint8_t gpio_value);
void ak_init_gpio(void);
void ak_power_up(void);
void ak_power_down(void);
void ak_set_read_gpio(char val);
void ak_read_interrupt(void);
extern char g_local_ip[];
extern char g_version_8char[];
extern char g_strMacAddr[];
void ak_led1_set(int val);
void ak_bat1_set(int val);
void ak_bat2_set(int val);
void ak_batlvlsel1_set(int val);
void ak_batlvlsel2_set(int val);
void ak_speakerEnable_set(int val);
extern char ak_version[];
extern char ak_version_format[];
//#define P2P_FORCE_EU
#define SERVER_MQTTS

#ifdef SERVER_MQTTS
#define MQTT_KEEP_A_LIVE_TIME	45
#define MQTT_YIELD_PERIOD   (1000*5)
#define DEFAULT_MQTT_NOKL_TORESET_SLEEP 11
#define DEFAULT_MQTT_NOOK_TORESET_WAKEUP 60
#else
#define MQTT_KEEP_A_LIVE_TIME	30
#define MQTT_YIELD_PERIOD   (1000*10)
#define DEFAULT_MQTT_NOKL_TORESET_SLEEP 5
#define DEFAULT_MQTT_NOOK_TORESET_WAKEUP 40
#endif
#ifdef NTP_CHINA
#define BAT_LOW_THRESHOLD_MV 2400
#else
#define BAT_LOW_THRESHOLD_MV 3500
#endif

extern int low_bat_push;
extern int low_bat_push2;
extern int count_to_reboot;
extern int ak_power_status;
extern unsigned int g_simeple_time; // in ms/10
extern unsigned int push_timestamp; // in ms/10
extern unsigned int PERIOD_REBOOT_TIME_MS;
#endif // __CC3200_SYSTEM_H__
