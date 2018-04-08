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

#include "user_common.h"

#include "tcpserver.h"

#define OSI_STACK_SIZE      1024
#define PORT                5001
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

    return sl_Start(NULL,NULL,NULL);
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

    UART_PRINT("Setup AP, waiting for connection ...\n");

    if ((lRetVal = httpd_start(PORT)) < 0) {
        UART_PRINT("Fail to start httpd [%d]\n", lRetVal);
    }

    //
    // Loop here
    //
    LOOP_FOREVER();
}

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
