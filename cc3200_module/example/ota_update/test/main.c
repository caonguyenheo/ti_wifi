#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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
#include "OtaFlash.h"

#define OSI_STACK_SIZE          5000

/****************************************************************************/
/*                      LOCAL FUNCTION PROTOTYPES                           */
/****************************************************************************/
void UserTask(void *pvParameters);

void CheckImageInfo(imageInfo_t *pImageInfo)
{
    // if we have error in getting the next image address
    // it should be a serious error
    int32_t lReturn;


    lReturn = OtaGetNextImageAddress(pImageInfo);
    // commit file
    // lReturn = OtaSaveNextImageAddress(pImageInfo);
}

void CheckSaveData()
{
    int32_t lReturn = -1;
    bool save;

    char image1[] = "/sys/mcuimg2.bin";

    lReturn = OtaFlashInit(image1);
    if (lReturn < 0)
        goto close;

    char msg[] = "hello";
    lReturn = OtaFlashWrite(0, msg, strlen(msg));
    if (lReturn < 0)
        goto close;

close:
    save = (lReturn < 0)? false: true;
    OtaFlashClose(save);
}

void UserTask(void *pvParameters)
{
    // int32_t lReturn = -1;
    // imageInfo_t imageInfo;
    // CheckImageInfo(&imageInfo);
    CheckSaveData();

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
#ifndef NOTERM
    //
    // Configuring UART
    //
    InitTerm();
#endif
    UART_PRINT("Main started\n");

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
    // Start the HIBUDPBroadcast task
    //
    lRetVal = osi_TaskCreate(UserTask, (const signed char *)"User task",
                OSI_STACK_SIZE, NULL, 1, NULL );
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
