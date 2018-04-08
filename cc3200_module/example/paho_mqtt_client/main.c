//*****************************************************************************
//
// Application Name     - OpenDoor sensor
// Application Overview - Most of time the node is in hibernate to save battery
//                        An event occurs, e.g. from external interrupt at SW3
//                        the node wake up, push a mqtt to myCin server
//                        then 
//
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Hibernate_Application
// or
// docs\examples\CC32xx_Hibernate_Application.pdf
//
//*****************************************************************************


//****************************************************************************
//
//! \addtogroup hib
//! @{
//
//****************************************************************************

#include <stdint.h>
#include <stdio.h>
#include <string.h>


#include "server_mqtt.h"

// simplelink includes
#include "device.h"

// driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_hib1p2.h"
#include "timer.h"
#include "interrupt.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"


// common interface includes
#include "network_if.h"
#include "gpio_if.h"
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "utils_if.h"
#include "timer_if.h"
#include "pinmux.h"

#include "user_common.h"

#define OSI_STACK_SIZE          1024

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
void NetworkTask(void *pvParameters);
void MQTTClientTask(void *pvParameters);

#define MQTT_TOPIC  "ticc3200/moisture"
#define CLIENT_ID   "cc3200_xxx"    

OsiSyncObj_t g_ObjConnectedAP;

void messageArrived(MessageData* md)
{
    MQTTMessage* message = md->message;

    UART_PRINT("Sub %.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
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

    SlSecParams_t SecurityParams = {0}; // AP Security Parameters
    // Initialize AP security params
    SecurityParams.Key = (signed char*)SECURITY_KEY;
    SecurityParams.KeyLen = strlen(SECURITY_KEY);
    SecurityParams.Type = SECURITY_TYPE;

    while (1) {
        mcuState = Network_IF_CurrentMCUState();
        if (!IS_CONNECTED(mcuState) || !IS_IP_ACQUIRED(mcuState)) {
            //
            // Connect to the Access Point
            //
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

    // UART_PRINT("Start MQTT client task\n");

    mycinMqtt_t client;
    uint32_t count;
    char s_sensor[10];

    while(1) {

        UART_PRINT("Start a MQTT section\n");

        while (mycinMqttInit(&client, CLIENT_ID) < 0) {
            UART_PRINT("Fail to initialize mycin mqtt client\n");
            osi_Sleep(1000);
        }

        if (mycinMqttSub(&client, MQTT_TOPIC, QOS1, messageArrived) < 0) {
            UART_PRINT("Fail to subsribe mycin mqtt server\n");
            // exit the following code, continue next loop

            goto section_terminal;
        }

        // if (mycinMqttHeartbeat(&client) < 0) {
        //     UART_PRINT("Fail to start mycin mqtt heartbeat\n");
        // }
        // if (osi_TaskCreate(MqttBackgroundTask, (const signed char *)"Mqtt background task",
        //             MQTT_BACKGROUND_STACK_SIZE, (void*)&client, MQTT_BACKGROUND_PRIORITY, NULL) < 0) {
        //     UART_PRINT("Fail to start mqtt background task\n");
        //     // return -1;
        // }

        count = 0;
        while(1) {
            if (count++ % 4 == 0) {
                snprintf(s_sensor, sizeof(s_sensor), "%04x", count/4);
                UART_PRINT("Pub: %s\n", s_sensor);
                if (mycinMqttPub(&client, MQTT_TOPIC, QOS1, s_sensor) < 0) {
                    UART_PRINT("... fail to pub\n");
                    // goto section_terminal;
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
    // Pinmux for UART & LED
    //
    PinMuxConfig();
#ifndef NOTERM
    //
    // Configuring UART
    //
    InitTerm();
#endif
    //
    // Start the SimpleLink Host
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    osi_SyncObjCreate(&g_ObjConnectedAP);

    //
    // Start the Network Task
    //
    lRetVal = osi_TaskCreate(NetworkTask, (const signed char *)"Network task",
                OSI_STACK_SIZE*1, NULL, 1, NULL);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the Network Task
    //
    lRetVal = osi_TaskCreate(MQTTClientTask, (const signed char *)"MQTT client task",
                OSI_STACK_SIZE*3, NULL, 1, NULL);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the task scheduler
    //
    osi_start();
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
