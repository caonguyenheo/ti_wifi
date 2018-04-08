#include <stdio.h>
#include <string.h>
#include <stdint.h>

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
#include "user_ota.h"

#define OSI_STACK_SIZE          (1024)

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
void UserTask(void *pvParameters);


#include "flash.h"
#include "flc.h"
int32_t InitFileSystem()
{
    if (fileCreate(IMG_BOOT_INFO) < 0)
        return -1;

    sBootInfo_t boot = {
        .ucActiveImg = IMG_ACT_FACTORY,
        .ulImgStatus = IMG_STATUS_TESTREADY
    };

    if (fileWrite(IMG_BOOT_INFO, (uint8_t*)&boot, sizeof(boot)) < 0)
        return -1;

    if (fileCreate(IMG_FACTORY_DEFAULT) < 0)
        return -1;

    if (fileCreate(IMG_USER_1) < 0)
        return -1;

    if (fileCreate(IMG_USER_2) < 0)
        return -1;

    return 0;
}
//*****************************************************************************
//@brief Function to reboot system
//
//*****************************************************************************
void system_reboot()
{
  PRCMMCUReset(TRUE);
  // PRCMSOCReset(); 
}
/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
void NetworkTask(void *pvParameters);
OsiSyncObj_t g_ObjConnectedAP;
// void spi_main_task(void *pvParameters);
int network_on=0;

#define RETRIE_AFTER_DISCONNECT_AP  5000

void NetworkTask(void *pvParameters)
{
    long lRetVal;
    int count=0;
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
       //LOOP_FOREVER();
       system_reboot();
    }

    /*
    SlDateTime_t cur_time = TIME_CURRENT_DEFAULT;
    if (set_time(&cur_time) < 0) {
        UART_PRINT("Failed to set time\n");
        if (set_time(&cur_time) < 0) {
            UART_PRINT("Failed to set time\n");
            system_reboot();
        }
    }*/


    SlSecParams_t SecurityParams = {0}; // AP Security Parameters
    // Initialize AP security params
    SecurityParams.Key = (signed char*)SECURITY_KEY;
    SecurityParams.KeyLen = strlen(SECURITY_KEY);
    SecurityParams.Type = SECURITY_TYPE;

    while (1) {
        mcuState = Network_IF_CurrentMCUState();
        if (!IS_CONNECTED(mcuState) || !IS_IP_ACQUIRED(mcuState)) {
            // Connect to the Access Point
            UART_PRINT("MCU connecting to AP %s password %s\n",SSID_NAME,SecurityParams.Key);

            lRetVal = Network_IF_ConnectAP(SSID_NAME, SecurityParams);

            if(lRetVal < 0) {
                UART_PRINT("Connection to AP failed\n");
                count++;
            }
            else{
	            network_on=1;
                count = 0;
            }
            if (count==5)
                system_reboot();
        }
        osi_Sleep(RETRIE_AFTER_DISCONNECT_AP);
    }

    //
    // Loop here
    //
    //LOOP_FOREVER();
    system_reboot();
}


void UserTask(void *pvParameters)
{
    long lRetVal;
    int i;
	/*
	uint32_t mcuState;
	int count=0;
	

    //
    //
    // Reset The state of the machine
    //
    Network_IF_ResetMCUStateMachine();

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

    //
    // Connect to the Access Point
    //
    UART_PRINT("\r\nConnecting to AP %s password %s\r\n", SSID_NAME, SECURITY_KEY);
    
        while (1) {
        mcuState = Network_IF_CurrentMCUState();
        if (!IS_CONNECTED(mcuState) || !IS_IP_ACQUIRED(mcuState)) {
            // Connect to the Access Point
            UART_PRINT("MCU connecting to AP %s password %s\r\n",SSID_NAME,SecurityParams.Key);

            lRetVal = Network_IF_ConnectAP(SSID_NAME, SecurityParams);

            if(lRetVal < 0) {
                UART_PRINT("Connection to AP failed\r\n");
                count++;
            }
            else
            	{
	            UART_PRINT("Connection to AP succeeded\r\n");
                count = 0;
                break;
            }
            if (count==5)
                system_reboot();
        }
        osi_Sleep(RETRIE_AFTER_DISCONNECT_AP);
    }
    
    */
    /*
    lRetVal = Network_IF_ConnectAP(SSID_NAME, SecurityParams);
    if (lRetVal < 0) {
        UART_PRINT("Connection to AP failed %d\n", lRetVal);
        LOOP_FOREVER();
    }*/

    // lRetVal = InitFileSystem();
    // if (lRetVal < 0) {
    //     Report("Fail to setup file system %d\n", lRetVal);
    //     LOOP_FOREVER();
    // }
	//osi_SyncObjWait(&g_ObjConnectedAP, OSI_WAIT_FOREVER);
	i=0;
	while(network_on==0){
		if (i==20){
			UART_PRINT("Wait for network too long, break loop to try OTA, if fail then reboot\r\n");
			break;
		}
		osi_Sleep(1000);
		i++;
	}
    start_mycin_ota();
	system_reboot();
	
    //
    // Loop here
    //
    //LOOP_FOREVER();
}

//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//! This function  
//!    1. Invokes the SLHost task
//!    2. Invokes the UserTask
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
    //
    // Configuring UART
    //
    InitTerm();

    DisplayBanner(PROJECT);

    //
    // Start the SimpleLink Host
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        system_reboot();
    }

    //
    // Start the Network Task
    //
    lRetVal = osi_TaskCreate(NetworkTask, (const signed char *)"Network task",
                OSI_STACK_SIZE*2, NULL, 15, NULL);
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
        system_reboot();
    }
    
    //
    // Start the HIBUDPBroadcast task
    //
    lRetVal = osi_TaskCreate(UserTask, (const signed char *)"User task",
                OSI_STACK_SIZE*3, NULL, 1, NULL );
    if(lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
        system_reboot();
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
