#include "notification.h"
#include "server_api.h"
#include "server_http_client.h"
#include "cc3200_system.h"

#include "FreeRTOS.h"
#include "task.h"

#include "p2p_main.h"
#include "sleep_profile.h"
#include "userconfig.h"

// device driver
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "system.h"
#include "hw_types.h"
#include "prcm.h"
#include "rom_map.h"
#include "state_machine.h"
extern char MYCIN_API_HOST_NAME[];
// #include "date_time.h"
uint32_t start_time_ms = 0;

#define MAX_PUSH_URL_LEN			400
#define MAX_PUSH_PAYLOAD_LEN		50
#define MAX_PUSH_RECV_PAYLOAD_LEN	200
#define TEST_PUSH_NOTIFICATION		0
#if (TEST_PUSH_NOTIFICATION)
#define FAKE_MESG	"&key=5A6471473154656B4959563652336546&sip=54.173.89.126&sp=8000&rn=6571724A436F61416336684A"
#endif
//#define PUSH_QUERRY "/v1/devices/events/cameraservice?action=command&command=notification&alert=%i%s&udid=%s&auth_token=%s&mac=%s&time=%s&pns_type=2"
#define PUSH_QUERRY "/v1/devices/events/cameraservice?action=command&command=notification&alert=%i%s&udid=%s&auth_token=%s&mac=%s&time=%s&pns_type=%i"
#define PUSH_QUERY_URL "/v1/devices/events"
#define PUSH_QUERY_BODY "{\"device_id\":\"%s\",\"event_type\":%d,\"device_token\":\"%s\",\"is_ssl\":false}"

extern OsiMsgQ_t g_tConnectionFlag;
extern OsiMsgQ_t g_send_notification_flag;
extern OsiSyncObj_t g_start_streaming;
extern OsiSyncObj_t g_getNTP;
extern OsiSyncObj_t g_NTP_init_done_flag;

int32_t push_set_header(char *url, 
					uint32_t len, 
					myCinUserInformation_t *pmyCinUser,
					int32_t push_code,
					char *msg,
                    uint32_t msg_len,
                    uint32_t push_type);

#define IS_SECURITY			1
int32_t push_notification(myCinUserInformation_t *pmyCinUser, uint32_t push_code, char *msg, uint32_t msg_len, uint32_t push_type) {
#if (IS_SECURITY)
	bool secure = 1;
	uint16_t server_port = MYCIN_API_HOST_PORT;
#else
	bool secure = 0;
	uint16_t server_port = 80;
#endif
	int32_t lRetVal;
	HTTPCli_Struct httpClient;

	//UART_PRINT("\r\nConnect to %s port %d secure %d\r\n",MYCIN_API_HOST_NAME,server_port,secure);
	lRetVal = myCinHttpConnect(&httpClient, MYCIN_API_HOST_NAME, server_port, secure);
	if (lRetVal < 0) {
		UART_PRINT("Fail to connect to myCin\n");
		goto end;
	}
	// UART_PRINT("CHECK point 2\r\n");
	// construct otaUrl
	char push_url[MAX_PUSH_URL_LEN]={0};
	if (push_type==2)
		lRetVal = push_set_header(push_url, sizeof(push_url), pmyCinUser, push_code, msg, msg_len, push_type);
	else
		lRetVal = snprintf(push_url, sizeof(push_url), PUSH_QUERY_BODY, pmyCinUser->device_udid, push_code, pmyCinUser->authen_token);
		
	if (lRetVal < 0) {
		UART_PRINT("Fail to set url\n");
		goto end;
	}

	//UART_PRINT("url: %s\r\n", push_url);
	
	if (push_type==2)
		lRetVal = OtaHttpSendRequest(&httpClient, MYCIN_API_HOST_NAME, push_url, HTTPCli_METHOD_GET);
	else
		lRetVal = myCinHttpSendRequest(&httpClient, MYCIN_API_HOST_NAME, PUSH_QUERY_URL, HTTPCli_METHOD_POST, push_url);
		
	if (lRetVal < 0) {
		UART_PRINT("Fail to send push [%d]\n", lRetVal);
		goto end;
	}
	
	memset(push_url,0,sizeof(push_url));
	lRetVal = myCinHttpVersionGetResponse(&httpClient, push_url, sizeof(push_url));
	if (lRetVal < 0) {
	    UART_PRINT("Fail receive response [%d]\n", lRetVal);
	    goto end;
	} else
	{
		UART_PRINT("Return %d data %s\r\n", lRetVal, push_url);
		if (strstr(push_url,"200")!=NULL)
			lRetVal = 0;
		else
			lRetVal = -1;
	}
	
end:
  	myCinHttpClose(&httpClient);
	return lRetVal;
}
#define NOTIFICATION_TIME		"19700000099990000"
#define HARDCODE_MAC_ADDR		"000AE22F0000"

// Alert define
#define MOTION_DETECTED 1
#define SOUND_DETECTED 2
#define HIGH_TEMP 3
#define LOW_TEMP 4
#define STATUS_CHANGE 5
#define RINGING_DETECTED 6
#define LOWBAT_DETECTED 7

// Push type define
#define NORMAL_PUSH 1
#define VOIP_PUSH 2




int32_t push_set_header(char *url, 
						uint32_t len, 
						myCinUserInformation_t *pmyCinUser,
						int32_t push_code,
						char *msg,
                        uint32_t msg_len,
                        uint32_t push_type
						) {
	int32_t ret_val;
	char mac_addr[MAX_MAC_ADDR_SIZE] = {0};

	memset(mac_addr, 0, sizeof(mac_addr));
	get_mac_address(mac_addr);

	// memcpy(mac_addr, HARDCODE_MAC_ADDR,sizeof(HARDCODE_MAC_ADDR));

	ret_val = snprintf(url, len, PUSH_QUERRY, push_code
		                , msg
						, pmyCinUser->device_udid
						, pmyCinUser->authen_token
						, mac_addr
                        , NOTIFICATION_TIME
                        , push_type);
	// UART_PRINT("len: %d\r\n", ret_val);
	// UART_PRINT("udid: %s\r\n", pmyCinUser->device_udid);
	// UART_PRINT("authen_token: %s\r\n",pmyCinUser->authen_token);
	// UART_PRINT("mac: %s\r\n", mac_addr);
	// UART_PRINT("time: %s\r\n", NOTIFICATION_TIME);
	// UART_PRINT("url: %s\r\n", url);
	if (ret_val >= len) {
		return -1;
	}

	url[ret_val] = '\0';
	return 0;
}

static myCinUserInformation_t *mycinUser;
extern myCinUserInformation_t g_mycinUser;
int32_t init_notification() {
	        // DkS: test push notification
        // #define TEST_USER_TOKEN   "be93cf741d7cc6644426268701ebdc96"
        // #define AUTHEN_TOKEN      "be93cf741d7cc6644426268701ebdc96"
		// dev board
		// #if (1)
  //       #define TEST_USER_TOKEN   "d835b4706682e3b39d497dea159b9ffa"
  //       #define AUTHEN_TOKEN      "102aabfdd9e1dceb9ca747f3f56bf1b0"
  //       /*#define TEST_USER_TOKEN   "97cb297be200dfda760ac399c926c8b8"*/
  //       /*#define AUTHEN_TOKEN      "97cb297be200dfda760ac399c926c8b8"*/
		// #else
		// // cv2212
  //       #define TEST_USER_TOKEN   "9806d103cfecb0dc60efa9170595f428"
  //       #define AUTHEN_TOKEN      "9806d103cfecb0dc60efa9170595f428"
		// #endif
		
        
  //       char device_udid[MAX_UID_SIZE] = {0};

  //       memset(device_udid, 0, sizeof(device_udid));
  //       get_uid(device_udid);

  //       memset((char *)&mycinUser, 0, sizeof(mycinUser));
  //       memcpy(mycinUser.user_token, TEST_USER_TOKEN, sizeof(TEST_USER_TOKEN));
  //       // memcpy(mycinUser.device_udid, TEST_UDID, sizeof(TEST_UDID));
  //       memcpy(mycinUser.device_udid, device_udid, strlen(device_udid));
  //       memcpy(mycinUser.authen_token, AUTHEN_TOKEN, sizeof(AUTHEN_TOKEN));
		mycinUser = &g_mycinUser;
        return 0;
}
extern int ota_done;
int play_dingdong_trigger=0;
int low_bat_push=0;
int low_bat_push2=0;
unsigned int push_timestamp=0; // in ms/10
void notification_task(void *pvParameters) {
	int32_t ret_val = 0;
	uint32_t used_bytes = 0;
	uint32_t timeout, i;
	//char notification_msg[128] = {0};
	//int i;
	char l_mqtt_servertopic[96] = {0};
	char l_mqtt_pushmessage[256] = {0};
    unsigned char ucSyncMsg;
    int notify_msg;
    
    while (ota_done==0)
    	osi_Sleep(1000);
    // DkS
    #if 1
    osi_MsgQRead(&g_tConnectionFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
    osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg, OSI_NO_WAIT);

    #endif
    init_notification();
    ret_val = userconfig_get(l_mqtt_servertopic, MQTT_SERVERTOPIC_LEN, MQTT_SERVERTOPIC_ID);
	//ret_val = get_ps_info(notification_msg, sizeof(notification_msg), &used_bytes);
	// enable_deep_sleep();
// #if (LPDS_MODE)
#if (1)
    while (1) {
    	osi_MsgQRead(&g_send_notification_flag, &notify_msg, OSI_WAIT_FOREVER);
    	timeout = 4;
        signal_event(eEVENT_START_STREAMING);
		play_dingdong_trigger = DINGDONG_COUNT_MAX;
    	used_bytes = 0;
		#if 1
		//i=0;
		/* Enable this fast alone push required
		ret_val=-1;
		while ((ret_val<0) && (i<2)){
			ret_val = push_notification(mycinUser, RINGING_DETECTED, notification_msg, used_bytes, VOIP_PUSH); // CALL ALERT
			i++;
		}*/

    	// start STREAM
		osi_SyncObjSignal(&g_getNTP);
        osi_SyncObjWait(&g_NTP_init_done_flag, OSI_WAIT_FOREVER);
		if ((push_timestamp==0) || ((g_simeple_time-push_timestamp)>140000)){
			mqtt_get_ps_info(l_mqtt_pushmessage, sizeof(l_mqtt_pushmessage), &used_bytes);
			//UART_PRINT("MQTT: %s %s \r\n",l_mqtt_servertopic,l_mqtt_pushmessage);
			UART_PRINT("START MQTT PUSH\r\n");
			ret_val = mqtt_pub(l_mqtt_servertopic, l_mqtt_pushmessage);
			push_timestamp = g_simeple_time;
			//UART_PRINT("MQTT: %d\r\n",push_timestamp);
		}
		UART_PRINT("START PS STREAM\r\n");
		fast_ps_mode();
		//UART_PRINT("Wait for 3 seconds!\r\n");
		i = 0;
		while (timeout--) {
			osi_MsgQRead(&g_send_notification_flag, &notify_msg, OSI_NO_WAIT);
			osi_Sleep(500);
			if (i<1) //Send x time PS open more
				fast_ps_mode();
			i++;
		}
    	if (low_bat_push==1){
            UART_PRINT("LOW bat detected\r\n");
            push_notification(mycinUser, LOWBAT_DETECTED, NULL, 0, NORMAL_PUSH);
            low_bat_push=2;
        } 

		// osi_SyncObjSignal(&g_start_streaming);
			
        #endif
		// enable_deep_sleep();
    }
#else
    if (MAP_PRCMSysResetCauseGet() != PRCM_HIB_EXIT) {
    	while(1) {
	    	UART_PRINT("wait for event\r\n");
	    	osi_MsgQRead(&g_send_notification_flag, &notify_msg, OSI_WAIT_FOREVER);
	    	UART_PRINT("Got GPIO interrupt, pin=%d\r\n", notify_msg);
    	}
    	// DO nothing
    }
    else {
		used_bytes = 0;
		memset(notification_msg, 0x00, sizeof(notification_msg));
		// wait for event
		// enable_deep_sleep();
    	// osi_MsgQRead(&g_send_notification_flag, &notify_msg, OSI_WAIT_FOREVER);
    	// disable_deep_sleep();
		uint32_t end_time_ms = 0;
		// start_time_ms = stopwatch_get_ms();
	#if (TEST_PUSH_NOTIFICATION)
		memset(notification_msg, 0x00, sizeof(notification_msg));
		memcpy(notification_msg, FAKE_MESG, sizeof(FAKE_MESG));
		used_bytes = strlen(notification_msg);
	#else
		do {
			ret_val = get_ps_info(notification_msg, sizeof(notification_msg), &used_bytes);
		} while(ret_val != 0);
	#endif
		// UART_PRINT("msg: %s\r\n", notification_msg);
		#if 1
        ret_val = push_notification(mycinUser, 1, notification_msg, used_bytes); // SOUND ALERT
        if (ret_val == 0) {
			// end_time_ms = stopwatch_get_ms();
			// UART_PRINT("TIME_TO_SEND = %d\r\n", end_time_ms - start_time_ms);
        	// start SPI
      		// spi_init();

        	// start STREAM
			do {
				UART_PRINT("START PS STREAM\r\n");
				ret_val = fast_ps_mode();
			} while(ret_val != 0);
			// osi_Sleep(5000);
        }
        #endif
		// enable_deep_sleep();
    	osi_MsgQRead(&g_send_notification_flag, &notify_msg, OSI_WAIT_FOREVER);
    	disable_deep_sleep();
	}
#endif // LPDS_MODE

	vTaskDelete(NULL);
	// LOOP_FOREVER();
}

