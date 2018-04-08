#include <stdio.h>
#include <stdint.h>

// simplelink includes
#include "simplelink.h"

// mycin mqtt client
#include "server_mqtt.h"

// Free-RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portmacro.h"
#include "osi.h"

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_apps_rcm.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "rom.h"
#include "rom_map.h"
#include "timer.h"
#include "utils.h"
#include "prcm.h"

#include "pinmux.h"

#include "common.h"

// common interface includes
#include "network_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif

#include "rgb_led.h"

extern OsiSyncObj_t g_ConnectedToAP;

static void messageArrived(MessageData* md)
{
    MQTTMessage* message = md->message;

    UART_PRINT("%.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
    UART_PRINT("%.*s\n", (int)message->payloadlen, (char*)message->payload);

    SetRgbLeds((char*)message->payload, (int32_t)message->payloadlen);
}


#define TOPIC       "%02X%02X%02X%02X%02X%02X/rgb"
#define CLIENT_ID   "%02X%02X%02X%02X%02X%02X"

void myCinMqttClient(void *pvParameters)
{
    UART_PRINT("Start mycin mqtt client\n");

    osi_SyncObjWait(&g_ConnectedToAP, OSI_WAIT_FOREVER);

    char macAddressVal[SL_MAC_ADDR_LEN];

    unsigned char macAddressLen = SL_MAC_ADDR_LEN;
    sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &macAddressLen,(unsigned char *)macAddressVal);

    // #define TOPIC       "%s/rgb"
    // char macAddressVal[12+1];
    // strncpy(macAddressVal, "000AE2200008", sizeof(macAddressVal));

    uint32_t iMacCount = 0;
    UART_PRINT("Mac address: ");
    for (iMacCount = 0; iMacCount < SL_MAC_ADDR_LEN; iMacCount++) {
        UART_PRINT("%02X:", macAddressVal[iMacCount]);
    }
    UART_PRINT("\n");

    char topic[30];
    char clientID[13];
    uint32_t topLen = snprintf(topic, sizeof(topic), TOPIC,
        macAddressVal[0],
        macAddressVal[1],
        macAddressVal[2],
        macAddressVal[3],
        macAddressVal[4],
        macAddressVal[5]
        );
    topic[topLen] = 0;
    UART_PRINT("Gonna subscribe topic %s\n", topic);

    uint32_t clientIDLen = snprintf(clientID, sizeof(clientID), CLIENT_ID,
        macAddressVal[0],
        macAddressVal[1],
        macAddressVal[2],
        macAddressVal[3],
        macAddressVal[4],
        macAddressVal[5]
        );
    clientID[clientIDLen] = 0;
    UART_PRINT("Client ID %s\n", clientID);

    //
    // mqtt do its job
    //
    mycinMqtt_t client;

    while (1) {

        if (mycinMqttInit(&client, clientID) < 0) {
            UART_PRINT("Fail to start mycin mqtt client\n");
            goto end;
        }

        if (mycinMqttSub(&client, topic, QOS1, messageArrived) < 0) {
            UART_PRINT("Fail to subsribe mycin mqtt server\n");
            goto end;
        }

        // if (mycinMqttHeartbeat(&client) < 0) {
        //     UART_PRINT("Fail to start mycin mqtt heartbeat\n");
        //     return -1;
        // }
        
        uint8_t count = 150;
        while(1) {
            // char s_sensor[50];
            // snprintf(s_sensor, sizeof(s_sensor), "{\"r\":%d,\"g\":%d,\"b\":%d}", count, count, count);
            // count += 10;
            // if (count <= 150)
            //     count = 150;
            // UART_PRINT("Message: %s\n", s_sensor);

            // mycinMqttPub(&client, topic, QOS1, s_sensor);

            if (MQTTYield(&client.hMQTTClient, 1000*2) < 0) {
                UART_PRINT("Fail to yield\n");
            }

            osi_Sleep(1000*2);
        }

    end:
        mycinMqttDeinit(&client);
    }

    while(1) {
        osi_Sleep(1000*10);
    }

    LOOP_FOREVER();
}
