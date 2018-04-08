//*****************************************************************************
//
// Application Name     - CV2212 Wifi Bridge
// Application Overview - 
//
//*****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "server_mqtt.h"
#include "user_common.h"

#define OSI_STACK_SIZE          1024

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
void NetworkTask(void *pvParameters);
void MQTTClientTask(void *pvParameters);

#define MYCINLINK_SYS  "hl/sys"
#define CLIENT_ID       "cc3200_xxx"

#define MYCINLINK_CUSTOM_TOPIC "hl/custom"

OsiSyncObj_t g_ObjConnectedAP;

void hl_system_topic(MessageData* md)
{
    MQTTMessage* message = md->message;

    UART_PRINT("Sub %.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
    UART_PRINT("%.*s\n", (int)message->payloadlen, (char*)message->payload);
}

void hl_custom_topic(MessageData* md)
{
    MQTTMessage* message = md->message;

    UART_PRINT("*Sub %.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
    UART_PRINT("%.*s\n", (int)message->payloadlen, (char*)message->payload);
}

#define RETRIE_AFTER_DISCONNECT_AP  5000

void NetworkTask(void *pvParameters)
{
    long lRetVal;

    //
    //
    // Reset The state of the machine
    //
    Network_IF_ResetMCUStateMachine();

    uint32_t mcuState;

    //
    // Start the driver
    //
    lRetVal = Network_IF_InitDriver(ROLE_STA);
    if(lRetVal < 0)
    {
       UART_PRINT("Failed to start SimpleLink Device\n\r");
       LOOP_FOREVER();
    }

    SlDateTime_t cur_time = TIME_CURRENT_DEFAULT;
    if (set_time(&cur_time) < 0) {
        UART_PRINT("Failed to set time\n");
    }

    SlSecParams_t SecurityParams = {0}; // AP Security Parameters
    // Initialize AP security params
    SecurityParams.Key = (signed char*)SECURITY_KEY;
    SecurityParams.KeyLen = strlen(SECURITY_KEY);
    SecurityParams.Type = SECURITY_TYPE;

    while (1) {
        mcuState = Network_IF_CurrentMCUState();
        if (!IS_CONNECTED(mcuState) || !IS_IP_ACQUIRED(mcuState)) {
            // Connect to the Access Point
            UART_PRINT("MCU connecting to AP\n");

            lRetVal = Network_IF_ConnectAP(SSID_NAME, SecurityParams);
            if(lRetVal < 0) {
                UART_PRINT("Connection to AP failed\n");
            }
        }
        else {
            osi_SyncObjSignal(&g_ObjConnectedAP);
        }
        osi_Sleep(RETRIE_AFTER_DISCONNECT_AP);
    }

    //
    // Loop here
    //
    LOOP_FOREVER();
}

void MQTTClientTask(void *pvParameters)
{
    osi_SyncObjWait(&g_ObjConnectedAP, OSI_WAIT_FOREVER);

    mycinMqtt_t client;
    uint32_t count;
    char s_sensor[10];

    while(1) {

        UART_PRINT("Start a MQTT section\n");

        while (mycinMqttInit(&client, CLIENT_ID) < 0) {
            UART_PRINT("Fail to initialize mycin mqtt client\n");
            osi_Sleep(1000);
        }

        if (mycinMqttSub(&client, MYCINLINK_SYS, QOS2, hl_system_topic) < 0) {
            UART_PRINT("Fail to subsribe mycin mqtt server\n");
            // exit the following code, continue next loop

            goto section_terminal;
        }

        if (mycinMqttSub(&client, MYCINLINK_CUSTOM_TOPIC, QOS2, hl_custom_topic) < 0) {
            UART_PRINT("Fail to subsribe mycin mqtt server\n");
            // exit the following code, continue next loop

            goto section_terminal;
        }

        count = 0;
        while(1) {
            if (count++ % 4 == 0) {
                snprintf(s_sensor, sizeof(s_sensor), "%04x", count/4);
                UART_PRINT("Pub: %s\n", s_sensor);
                if (mycinMqttPub(&client, MYCINLINK_SYS, QOS2, s_sensor) < 0) {
                    UART_PRINT("... fail to pub\n");
                    goto section_terminal;
                }
            }

            if (MQTTYield(&(client.hMQTTClient), MQTT_YIELD_PERIOD) < 0) {
                UART_PRINT("... fail to yield\n");
                goto section_terminal;
            }

            osi_Sleep(MQTT_YIELD_PERIOD);
        }

section_terminal:
        mycinMqttDeinit(&client);
        UART_PRINT("End a MQTT section\n");

        // sleep for a while
        osi_Sleep(MQTT_YIELD_PERIOD);
    }

    //
    // Loop here
    //
    LOOP_FOREVER();
}

//****************************************************************************
//
//! Main function
//!
//! \param none
//!
//! \return None.
//
//****************************************************************************
void main()
{
    long lRetVal = -1;

    //
    // Initialize board confifurations
    //
    BoardInit();

    //
    // Start the SimpleLink Host
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);

    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    // Create object to notify Wifi AP connection
    osi_SyncObjCreate(&g_ObjConnectedAP);

    //
    // Start the Network Task
    //
    lRetVal = osi_TaskCreate(NetworkTask, (const signed char *)"Network task",
                OSI_STACK_SIZE*1, NULL, 1, NULL);
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the MQTT client task
    //
    lRetVal = osi_TaskCreate(MQTTClientTask, (const signed char *)"MQTT client task",
                OSI_STACK_SIZE*3, NULL, 1, NULL);

    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the task scheduler
    //
    osi_start();
}
