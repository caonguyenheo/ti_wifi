//****************************************************************************
//
//
//****************************************************************************

#include <stdio.h>
#include <stdint.h>

// simplelink includes
#include "simplelink.h"

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

#include "httpserverapp.h"
#include "led_control.h"
#include "rgb_led.h"
#include "button.h"

/*Spawn task priority and OSI Stack Size*/
#define OSI_STACK_SIZE          5000
#define BUTTON_STACK_SIZE       2000
#define MYCINMQTT_STACK_SIZE   3000

void RgbLedControl(void *pvParameters);
void NetworkInterfaceControl(void *pvParameters);
void myCinMqttClient(void *pvParameters);
void myCinApiClient(void *pvParameters);

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

OsiSyncObj_t g_ConnectedToAP;

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}


//****************************************************************************
//
//! Demonstrates the controlling of LED brightness using PWM
//!
//! \param none
//! 
//! This function  
//!    1. Pinmux the GPIOs that drive LEDs to PWM mode.
//!    2. Initializes the timer as PWM.
//!    3. Loops to continuously change the PWM value and hence brightness 
//!       of LEDs.
//!
//! \return None.
//
//****************************************************************************
void main()
{
    //
    // Board Initialisation
    //
    BoardInit();
    
    //
    // Configure the pinmux settings for the peripherals exercised
    //
    PinMuxConfig();    

    //
    // Configuring UART
    //
    InitTerm();

    //
    // Initialize the PWMs used for driving the LEDs
    //
    InitPWMModules();
    
    //
    // LED
    //
    InitRgbLed();

    UART_PRINT("Main: starting\n");

    //
    // Create sync object to signal display refresh
    //
    osi_SyncObjCreate(&g_ConnectedToAP);

    int32_t lRetVal;

    //
    // Start the SimpleLink Host
    //

    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(lRetVal < 0) {
        UART_PRINT("Main: Fail to start simplelink task\n");
        LOOP_FOREVER();
    }

    //
    // Start the network interface
    //
    lRetVal = osi_TaskCreate(HttpServerAppTask,
                            (const signed char *)"Network interface control",
                            OSI_STACK_SIZE, NULL, 2, NULL );
    if(lRetVal < 0) {
        UART_PRINT("Main: Fail to start network interface task\n");
        LOOP_FOREVER();
    }

    //
    // Start the button handling
    //
    lRetVal = osi_TaskCreate(ButtonHandingTask,
                            (const signed char *)"Button handling task",
                            BUTTON_STACK_SIZE, NULL, 2, NULL );
    if(lRetVal < 0) {
        UART_PRINT("Main: Fail to start button handling task\n");
        LOOP_FOREVER();
    }
    
    //
    // Start the led dimming task
    //
    lRetVal = osi_TaskCreate(myCinMqttClient,
                            (const signed char *)"Rgb led control task",
                            MYCINMQTT_STACK_SIZE, NULL, 2, NULL );
    if(lRetVal < 0) {
        UART_PRINT("Main: Fail to start rgb led control task");
        LOOP_FOREVER();
    }

    //
    // Start the task scheduler
    //
    osi_start();

    //
    // loop, should not be here
    //
    while (1) {
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
