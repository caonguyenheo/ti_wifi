#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "user_common.h"

#include "cvhttpd.h"

#define OSI_STACK_SIZE      1024
#define PORT                3333
#define DEFAULT_AP_NAME     "nqdinhabcdefgh"

void NetworkTask(void *pvParameters);

static int SetAPName(const char* name, uint32_t nameLength)
{
    long   lRetVal = -1;

    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, nameLength,
                         (unsigned char*)name);
    ASSERT_ON_ERROR(lRetVal);

    /* Restart Network processor */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    return sl_Start(NULL, NULL, NULL);
}

void NetworkTask(void *pvParameters)
{
    long lRetVal = -1;

    Network_IF_ResetMCUStateMachine();

    lRetVal = Network_IF_InitDriver(ROLE_AP);

    if (lRetVal < 0) {
        UART_PRINT("Fail to start simplelink device\n");
        LOOP_FOREVER();
    }

    // set ap name
    lRetVal = SetAPName(DEFAULT_AP_NAME, strlen(DEFAULT_AP_NAME));
    if (lRetVal < 0) {
        UART_PRINT("Fail to set AP name\n");
        LOOP_FOREVER();
    }

    //Stop Internal HTTP Server
    lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
    if (lRetVal < 0) {
        UART_PRINT("Fail to stop internal HTTP server\n");
        LOOP_FOREVER();
    }

    UART_PRINT("Setup AP, waiting for connection at %d ...\n", PORT);
    if ((lRetVal = httpd_start(PORT)) < 0) {
        UART_PRINT("Fail to start httpd [%d]\n", lRetVal);
    }

    UART_PRINT("Httpd: end\n");

    //
    // Loop here
    //
    LOOP_FOREVER();
}

int main(void)
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
    if (lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the Network Task
    //
    lRetVal = osi_TaskCreate(NetworkTask, (const signed char *)"Network task",
                             OSI_STACK_SIZE * 1, NULL, 1, NULL);
    if (lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        LOOP_FOREVER();
    }

    //
    // Start the task scheduler
    //
    osi_start();
}
