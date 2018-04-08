#include "mqtt_handler.h"
#include "server_mqtt.h"
#ifndef NOTERM
#include "common.h"
#endif
#include "uart_if.h"
#include "network_if.h"
#include "cc3200_aes.h"
#include "cc3200_system.h"
#include "p2p_main.h"
#include "sleep_profile.h"
#include "userconfig.h"

// DkS: Test notification
#include "notification.h"
#include "server_api.h"
#include "network_if.h"
#include <string.h>
#include "system.h"
// DkS: end test


#define TOPIC_PREFIX            "dev/"
#define TOPIC_SURFIX            "/sub"

#define STR_APP_TOPIC           "app_topic_sub"
#define RESPONSE_TOPIC_SZ       128
#define RESPONSE_MESS_SZ        1024
#define CLIENT_ID_LEN           15

extern OsiMsgQ_t g_tConnectionFlag;
extern OsiMsgQ_t g_MQTT_connection_flag;
extern OsiMsgQ_t g_MQTT_check_msg_flag;
// extern OsiMsgQ_t g_NTP_init_done_flag;

static mycinMqtt_t client;
int mqtt_status=0;

void hl_system_topic(MessageData* md)
{
    char *ptr_end_payload = NULL;

    MQTTMessage* message = md->message;

    UART_PRINT("Sub %.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
    UART_PRINT("%.*s\n", (int)message->payloadlen, (char*)message->payload);

    // add terminated string char
    ptr_end_payload = (char *)message->payload + message->payloadlen;
    *ptr_end_payload = '\0';

    // Command handler
    command_handler((char *)message->payload, message->payloadlen, 
        MQTT_COMMAND, NULL, NULL);
    // NOTE: Command respose is send from command_handler
}

extern int ota_done;
unsigned int yield_count=0;
unsigned int keepalive_count=0;
int mqtt_connect_status=0;
int mqtt_noKL_toreset=DEFAULT_MQTT_NOKL_TORESET_SLEEP;
extern void wdt_clear(void);
#define SECOND_PERDAY (24*3600)
#define PERIOD_REBOOT_TIME_MS (7*SECOND_PERDAY*1000)
int count_to_reboot=(PERIOD_REBOOT_TIME_MS/MQTT_YIELD_PERIOD);
void MQTTClientTask(void *pvParameters)
{
    char topic_name[MQTT_TOPIC_LEN+1] = {0};
    char client_id[MQTT_CLIENTID_LEN+1] = {0};
    int ret_val;
    unsigned char ucSyncMsg;
    int i;
    
    osi_Sleep(1000);//Now allow led_control to run so early
    while ((ota_done==0) || (get_ntp_status==1)){
    	wdt_clear();
    	osi_Sleep(100);
#ifndef NTP_CHINA
    	led_control(100);
#endif
    	if (httpServerOperationTime)
    		httpServerOperationTime++;
	}
    //osi_MsgQRead(&g_tConnectionFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
    //osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg, OSI_NO_WAIT);

    // unsigned char uc_NTP_status = 0;
    // osi_MsgQRead(&g_NTP_init_done_flag, &uc_NTP_status, OSI_WAIT_FOREVER);
    // osi_MsgQWrite(&g_NTP_init_done_flag, &uc_NTP_status, OSI_NO_WAIT);
    uint32_t count;
    char s_sensor[10];

    while(1) {
        ret_val = userconfig_get(client_id, MQTT_CLIENTID_LEN, MQTT_CLIENTID);
        ret_val |= userconfig_get(topic_name, MQTT_TOPIC_LEN, MQTT_TOPIC);
        UART_PRINT("Start MQTT client:%s,topic:%s\r\n", client_id, topic_name);
        //client_id_generator(client_id);
        while (mycinMqttInit(&client, client_id) < 0) {
            print_manytimes("Fail to initialize mycin mqtt client->Reboot");
            system_reboot();
        }
        //device_topic_generator(topic_name);
        // SYNC with p2p task
        unsigned char uc_MQTT_connection = 0;
        osi_MsgQWrite(&g_MQTT_connection_flag, &uc_MQTT_connection, OSI_NO_WAIT);

        if (mycinMqttSub(&client, topic_name, QOS2, hl_system_topic) < 0) {
            print_manytimes("Fail to subsribe mycin mqtt server");
            system_reboot();
        }

        count = 0;
        // UART_PRINT("Test 1\r\n");
        // UART_PRINT("Test 2\r\n");
        // enable_deep_sleep();
        while(1) {
        wdt_clear();
        // Yield many times but no KeepAlive or it's time to periodically reboot
        if ((yield_count>=mqtt_noKL_toreset) || (count_to_reboot<=0)){
			if (count_to_reboot<=0){
				print_manytimes("Periodic reboot");
            }else{
            	print_manytimes("MqttYReboot");
        	}
            system_reboot();
        }
            
        yield_count++;
        keepalive_count=0;
        UART_PRINT("Yield ...%d %d %d ",yield_count,keepalive_count,count_to_reboot);
        count_to_reboot--; // Each mqtt loop reduce timeleft to reboot
        if (mqtt_connect_status==0){
            osi_Sleep(1000);
            mqtt_connect_status=1;
        }
        #if 1
            if (MQTTYield(&(client.hMQTTClient), MQTT_YIELD_PERIOD) < 0) {
                print_manytimes("... fail to yield");
                system_reboot();
            }
        #endif
        UART_PRINT("DONE\r\n");
        osi_MsgQRead(&g_MQTT_check_msg_flag, &ucSyncMsg, OSI_WAIT_FOREVER);
        }

//section_terminal:
//        mycinMqttDeinit(&client);
    }
}

int8_t  mqtt_pub(char *topic, char *msg) {
	return mycinMqttPub(&client, topic, QOS0, msg);
}
/*
void client_id_generator(char *client_id)
{
    char strMacAddr[13] = {0};
    get_mac_address(strMacAddr);
    if(client_id)
    {
        sprintf(client_id, "00%s", strMacAddr); // client 64 bytes
    }
}*/

//*****************************************************************************
//                      GENERATE TOPIC
//*****************************************************************************
inline char CharToAlphabet (unsigned char val)
{
    return (val&0x80)?(val%26+'A'):(val%26+'a');
}
int StringToAlphabet(char *in, char *out, int len)
{
    int i;
    if(!in || !out)
        return -1;
    for(i=0; i<len; i++)
    {
        out[i] = CharToAlphabet((unsigned char)in[i]);
    }
    return 0;
}
/*
#define HARDCODE_DATA "aaaaaaaaaaaaaaaa"
void device_topic_generator(char *topic)
{
    char strMacAddr[MAX_MAC_ADDR_SIZE] = {0}, strUDID[MAX_UID_SIZE+2] = {0};
    char enc_buf[17], iv[16]={0};
    char expandStr[9] = {0};
    char dev_topic_code[21] = {0};
    char *ptr_print = NULL;
    size_t out_size = 0;

    get_mac_address(strMacAddr);
    get_uid(strUDID);
    memset(enc_buf, 0, sizeof(enc_buf));
    aes_encrypt_cbc(strUDID + 10, iv, HARDCODE_DATA, enc_buf, 16, &out_size);
    StringToAlphabet(enc_buf, expandStr, 8);
    sprintf(dev_topic_code, "%s%s", strMacAddr, expandStr);

    UART_PRINT("dev_topic_code %s,expandStr %s\r\n", dev_topic_code, expandStr);
    if(topic)
    {
        ptr_print = topic;
        ptr_print += sprintf(ptr_print, "%s", TOPIC_PREFIX);
        ptr_print += sprintf(ptr_print, "%s", dev_topic_code);
        ptr_print += sprintf(ptr_print, "%s", TOPIC_SURFIX);
    }
}*/
