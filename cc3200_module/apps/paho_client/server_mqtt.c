#include <stdint.h>

#include "server_mqtt.h"
#include "MQTTClient.h"
#include "common.h"
#include "system.h"
#include "userconfig.h"

// driverlib includes 
#include "rom_map.h"
#include "utils.h"
// common interface includes 
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "server_http_client.h"
#include "system.h"

extern char MYCIN_HOST_NAME[];
#ifndef SERVER_MQTTS
int MYCIN_HOST_PORT=1885;
#else
int MYCIN_HOST_PORT=8885;
HTTPCli_Struct mqttsClient;
#endif
int8_t mycinMqttInit(mycinMqtt_t *pmyCinMqtt, char *clientID)
{
    int rc = 0, ret_val;
    char mqtt_user[MQTT_USER_LEN+1] = {0};
    char mqtt_pass[MQTT_PASS_LEN+1] = {0};
    char *p_ctemp;
    NewNetwork(&pmyCinMqtt->n);
    // UART_PRINT("Connecting Socket\n\r");
    ret_val = userconfig_get(mqtt_user, MQTT_USER_LEN, MQTT_USER);
    ret_val |= userconfig_get(mqtt_pass, MQTT_PASS_LEN, MQTT_PASS);
    UART_PRINT("MQTT user:%s,pass:%s\r\n", mqtt_user, mqtt_pass);
    
    p_ctemp=strstr(MYCIN_HOST_NAME,":");
    if (p_ctemp!=NULL){
        MYCIN_HOST_PORT=atoi(p_ctemp+1);
#ifndef SERVER_MQTTS
        if ((MYCIN_HOST_PORT/1000)!=1)
            MYCIN_HOST_PORT=1000+(MYCIN_HOST_PORT%1000);
#else
        if ((MYCIN_HOST_PORT/1000)!=8)
            MYCIN_HOST_PORT=8000+(MYCIN_HOST_PORT%1000);
#endif
        *p_ctemp=0x00;//Change ":" to string termination
    }
    //UART_PRINT("MQTT URL:%s, port:%d\r\n", MYCIN_HOST_NAME, MYCIN_HOST_PORT);
    //Works - Unsecured
#ifndef SERVER_MQTTS
    rc = ConnectNetwork(&pmyCinMqtt->n, MYCIN_HOST_NAME, MYCIN_HOST_PORT);
    if (rc < 0)
        return -1;
#else
/*    rc =TLSConnectNetwork(&pmyCinMqtt->n,
                            MYCIN_HOST_NAME,
                            MYCIN_HOST_PORT,
                            NULL,
                            MYCIN_SEC_METHOD,
                            MYCIN_CIPHER,
                            0);
                            //MYCIN_SERVER_VERIFY);*/
    rc = myCinHttpConnect(&mqttsClient, MYCIN_HOST_NAME, MYCIN_HOST_PORT, 1);
    UART_PRINT("Connect mqtts ret: %d\r\n", rc);
    if (rc < 0)
        return -1;
#endif
    
    MQTTClient(&pmyCinMqtt->hMQTTClient,
        &pmyCinMqtt->n,
        MQTT_COMMAND_TIMEOUT_MS,
        pmyCinMqtt->buf, sizeof(pmyCinMqtt->buf),
        pmyCinMqtt->readbuf, sizeof(pmyCinMqtt->readbuf));

    MQTTPacket_connectData cdata = MQTTPacket_connectData_initializer;
    cdata.MQTTVersion = 3;
    cdata.keepAliveInterval = MQTT_KEEP_A_LIVE_TIME;
    cdata.clientID.cstring = (char*)clientID;
    cdata.username.cstring = mqtt_user;
    cdata.password.cstring = mqtt_pass;

    rc = MQTTConnect(&pmyCinMqtt->hMQTTClient, &cdata);

    if (rc != 0) {
        UART_PRINT("MQTT Connect fail: %d\n", rc);
        return -1;
    } else {
        UART_PRINT("Connected to MQTT broker\n\r");
    }
    // UART_PRINT("\tsocket=%d\n\r", n.my_socket);

    pmyCinMqtt->msg_id = 0;

    return 0;
}

int8_t mycinMqttDeinit(mycinMqtt_t *pmyCinMqtt)
{
    MQTTDisconnect(&pmyCinMqtt->hMQTTClient);
    
    return sl_Close(pmyCinMqtt->n.my_socket);
}

int8_t mycinMqttSub(mycinMqtt_t *pmyCinMqtt, char *topic, int32_t qos, void *callback)
{
    return MQTTSubscribe(&(pmyCinMqtt->hMQTTClient), topic, qos, callback);
}

int8_t mycinMqttPub(mycinMqtt_t *pmyCinMqtt, char *topic, int32_t qos, char *message)
{
    MQTTMessage msg;
    msg.dup = 0;
    msg.id = pmyCinMqtt->msg_id++;
    msg.qos = qos;
    msg.retained = 0;

    msg.payload = message;
    msg.payloadlen = strlen(message);
    
    
    int32_t rc = MQTTPublish(&pmyCinMqtt->hMQTTClient, topic, &msg);
    // UART_PRINT("SW3, rc=%d\n\r", rc);
    if (rc < 0)
        return -1;

    return 0;
}

/*
int8_t mycinMqttHeartbeatStart(mycinMqtt_t *pmyCinMqtt)
{

    // create new process to handle background activities
    int32_t rc = osi_TaskCreate(MqttBackgroundTask, (const signed char *)"Mqtt background task",
                MQTT_BACKGROUND_STACK_SIZE, (void*)pmyCinMqtt, MQTT_BACKGROUND_PRIORITY, NULL );
    if(rc < 0) {
        UART_PRINT("Fail to start mqtt background task\n");
        return -1;
    }

    return 0;
}

void MqttBackgroundTask(void *pvParameters)
{
    mycinMqtt_t *pmyCinMqtt = (mycinMqtt_t*)pvParameters;

    uint32_t count = 0;

    while (1) {
        if (pmyCinMqtt->hMQTTClient.isconnected) {
            UART_PRINT("... mqtt heartbeat %d\n", count++);
            int32_t rc = MQTTYield(&(pmyCinMqtt->hMQTTClient), MQTT_YIELD_PERIOD);
            if (rc != 0) {
                UART_PRINT("Fail to yield\n");
                // return -1;
                continue;
            }
        }

        osi_Sleep(MQTT_YIELD_PERIOD);
    } 
}
*/
