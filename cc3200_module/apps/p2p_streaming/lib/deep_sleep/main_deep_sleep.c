//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - Idle Profile
// Application Overview - Idle profile enables the user to measure current 
//                        values, power consumption and other such parameters 
//                        for CC3200, when the device is essentially idle(both 
//                        NWP and APPS subsystems in low power deep sleep 
//                        condition). The other main objective behind this 
//                        application is to introduce the user to the easily 
//                        configurable power management framework.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Idle_Profile_Application
// or
// docs\examples\CC32xx_Idle_Profile_Application.pdf
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup idle_profile
//! @{
//
//****************************************************************************

// Standard includes
#include <string.h>

//driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "prcm.h"
#include "wdt.h"
#include "utils.h"
#include "rom.h"
#include "rom_map.h"

// Simplelink includes
#include "simplelink.h"

//middleware includes
#include "cc_types.h"
#include "rtc_hal.h"
#include "gpio_hal.h"
#include "uart_drv.h"
#include "cc_timer.h"
#include "cc_pm_ops.h"
#include "cc3200_system.h"
// OS(Free-RTOS/TI-RTOS) includes
#include "osi.h"

// Common interface includes
#include "wdt_if.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"

#include "pinmux.h"
#include "pin.h"
#include "sleep_profile.h"

#define APPLICATION_VERSION "1.1.1"
//
// Values for below macros shall be modified as per access-point(AP) properties
// SimpleLink device will connect to following AP when application is executed
//
// #define SSID_NAME               "NXBroadbandV"    /* AP SSID */
// #define SECURITY_TYPE           SL_SEC_TYPE_WPA   /* Security type (OPEN or WEP or WPA) */
// #define SECURITY_KEY            "13579135"  /* Password of the secured AP */
// #define SPAWN_TASK_PRIORITY     9

#define GPIO_SRC_WKUP_PAIR      CALLGPIO
#define APP_UDP_PORT            5001    
#define LPDS_DUR_SEC            10
#define LPDS_DUR_NSEC           0
#define FOREVER                 1
#define BUFF_SIZE               1472
// #define SL_STOP_TIMEOUT         30
#define OSI_STACK_SIZE          1024
#define CONNECTION_RETRIES      5
// #define UART_PRINT(x)           uart_write(g_tUartHndl, x, strlen(x))

#define WD_PERIOD_MS                10000
#define MAP_SysCtlClockGet          80000000
#define MILLISECONDS_TO_TICKS(ms)   ((MAP_SysCtlClockGet/1000) * (ms))

enum ap_events{
    EVENT_CONNECTION = 0x1,
    EVENT_DISCONNECTION = 0x2,
    EVENT_IP_ACQUIRED = 0x4,
    WDOG_EXPIRED = 0x8,
    CONNECTION_FAILED = 0x10
};

//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
void WatchdogIntHandler(void);
int gpio_intr_hndlr(int gpio_num);
void TimerCallback(void *vParam);
cc_hndl SetTimerAsWkUp();
cc_hndl SetGPIOAsWkUp();
void UDPServerTask(void *pvParameters);
void TimerGPIOTask(void *pvParameters);

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#define AK_PLATFORM                1
#define DEBUG_GPIO
#ifdef DEBUG_GPIO
#define GPIO_09                    9
#define GPIO_10                    10
#if defined (GM_PLATFORM)
#define POWER_LED_GPIO      GPIO_09
#elif defined (AK_PLATFORM)
#define POWER_LED_GPIO      GPIO_10
#else
#define POWER_LED_GPIO      GPIO_09
#endif
cc_hndl tGPIODbgHndl;
#endif

extern OsiMsgQ_t g_tConnectionMaintainFlag;
extern OsiMsgQ_t g_tConnectionFlag;
extern OsiMsgQ_t g_MQTT_check_msg_flag;
extern OsiMsgQ_t g_send_notification_flag;

cc_hndl g_tUartHndl;
unsigned long g_ulIpAddr = 0;
unsigned char g_ucFeedWatchdog = 0;
unsigned char g_ucWdogCount = 0;
OsiMsgQ_t g_tConnection = 0;
// signed char g_cBuffer[BUFF_SIZE];
// char g_cErrBuff[100];
static cc_hndl g_tTimerHndl = NULL; 

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//                      Local Function Prototypes
//*****************************************************************************
extern void lp3p0_setup_power_policy(int power_policy);
extern int platform_init();

// Loop forever, user can change it as per application's requirement
// #define LOOP_FOREVER() \
//             {\
//                 while(1); \
//             }

// check the error code and handle it
// #define ASSERT_ON_ERROR(error_code)\
//             {\
//                  if(error_code < 0) \
//                    {\
//                         sprintf(g_cErrBuff,"Error [%d] at line [%d] in "\
//                                 "function [%s]", error_code,__LINE__,\
//                                 __FUNCTION__);\
//                         UART_PRINT(g_cErrBuff);\
//                         UART_PRINT("\n\r");\
//                         return error_code;\
//                  }\
//             }



//*****************************************************************************
//
//! Application defined idle task hook
//! 
//! \param  none
//! 
//! \return none
//!
//*****************************************************************************
// void vApplicationIdleHook( void )
// {
//     cc_idle_task_pm();
// }





//*****************************************************************************
//
//! The interrupt handler for the watchdog timer
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
void WatchdogIntHandler(void)
{
    //
    // If we have been told to stop feeding the watchdog, return immediately
    // without clearing the interrupt.  This will cause the system to reset
    // next time the watchdog interrupt fires.
    //
    if(!g_ucFeedWatchdog)
    {
        return;
    }

    unsigned char ucQueueMsg = 0;
    if(g_ucWdogCount < CONNECTION_RETRIES)
    {
        g_ucWdogCount++;
        ucQueueMsg = (unsigned char)WDOG_EXPIRED;
        MAP_WatchdogIntClear(WDT_BASE);
    }
    else
    {
        g_ucFeedWatchdog = 0;
        ucQueueMsg = (unsigned char)CONNECTION_FAILED;
        MAP_WatchdogIntClear(WDT_BASE);
    }
    if(g_tConnection != 0)
    {
        osi_MsgQWrite(&g_tConnection, &ucQueueMsg, OSI_NO_WAIT);
    }
    else
    {
        UART_PRINT("Error: WatchdogIntHandler: Queue does not exist\n\r");
    }
}

//*****************************************************************************
//
//! \brief callback function for gpio interrupt handler
//!
//! \param gpio_num is the gpio number which has triggered the interrupt
//!
//! \return 0
//
//*****************************************************************************
extern int call_button_activate;
int gpio_intr_hndlr(int gpio_num)
{
    int queue_msg = gpio_num;
/*
    if(GPIO_SRC_WKUP == gpio_num)
    {
        osi_MsgQWrite(&g_send_notification_flag, &queue_msg, OSI_NO_WAIT);
    }*/
    
    //if(GPIO_SRC_WKUP_PAIR == gpio_num) {
	if (call_button_activate)
        osi_MsgQWrite(&g_send_notification_flag, &queue_msg, OSI_NO_WAIT);
    //}
    return 0;
}

//*****************************************************************************
//
//! \brief callback function for timer interrupt handler
//!
//! \param vParam is a general void pointer (not used here)
//!
//! \return None
//
//*****************************************************************************
void TimerCallback(void *vParam)
{
    unsigned char ucQueueMsg = 1;
    unsigned char ucConnectionMaintainMsg = 0;
    unsigned char uc_MQTT_check_msg = 0;
    // osi_MsgQWrite(&g_tWkupSignalQueue, &ucQueueMsg, OSI_NO_WAIT);
    // osi_MsgQWrite(&g_tConnectionMaintainFlag, &ucConnectionMaintainMsg, 
        // OSI_NO_WAIT);
    osi_MsgQWrite(&g_MQTT_check_msg_flag, &uc_MQTT_check_msg, OSI_NO_WAIT);
    return;
}

//*****************************************************************************
//
//! \brief set GPIO as a wake up source from low power modes.
//!
//! \param none
//!
//! \return handle for the gpio used
//
//*****************************************************************************
cc_hndl SetGPIOAsWkUp()
{
    cc_hndl tGPIOHndl;
    cc_hndl tGPIOHndl_pair;
    #if 0
    // Activate internal pull-up resistor on PIN_59
    MAP_PinConfigSet(PIN_18, PIN_STRENGTH_4MA, PIN_TYPE_STD_PD);
    //
    // setting up GPIO as a wk up source and configuring other related
    // parameters
    //
    tGPIOHndl = cc_gpio_open(GPIO_SRC_WKUP, GPIO_DIR_INPUT);
    cc_gpio_enable_notification(tGPIOHndl, GPIO_SRC_WKUP, INT_RISING_EDGE,
                                (GPIO_TYPE_NORMAL | GPIO_TYPE_WAKE_SOURCE));
    #endif
    #if 1
    // pair button
    // Activate internal pull-up resistor on PIN_59
#if (CALLGPIO==4)
    MAP_PinConfigSet(PIN_59, PIN_STRENGTH_4MA, PIN_TYPE_STD_PD);
#endif
#if (CALLGPIO==11)
    MAP_PinConfigSet(PIN_02, PIN_STRENGTH_4MA, PIN_TYPE_STD_PD);
#endif
    //
    // setting up GPIO as a wk up source and configuring other related
    // parameters
    //
    tGPIOHndl_pair = cc_gpio_open(GPIO_SRC_WKUP_PAIR, GPIO_DIR_INPUT);
    cc_gpio_enable_notification(tGPIOHndl_pair, GPIO_SRC_WKUP_PAIR, INT_RISING_EDGE,
                                (GPIO_TYPE_NORMAL | GPIO_TYPE_WAKE_SOURCE));
    #endif
    return(tGPIOHndl);
}

//*****************************************************************************
//
//! \brief set Timer as a wake up source from low power modes.
//!
//! \param none
//!
//! \return handle for the Timer setup as a wakeup source
//
//*****************************************************************************
cc_hndl SetTimerAsWkUp()
{
    cc_hndl tTimerHndl;
    struct cc_timer_cfg sRealTimeTimer;
    struct u64_time sInitTime, sIntervalTimer;
    //
    // setting up Timer as a wk up source and other timer configurations
    //
    sInitTime.secs = 0;
    sInitTime.nsec = 0;
    cc_rtc_set(&sInitTime);

    sRealTimeTimer.source = HW_REALTIME_CLK;
    sRealTimeTimer.timeout_cb = TimerCallback;
    sRealTimeTimer.cb_param = NULL;

    tTimerHndl = cc_timer_create(&sRealTimeTimer);

    sIntervalTimer.secs = LPDS_DUR_SEC;
    sIntervalTimer.nsec = LPDS_DUR_NSEC;
    cc_timer_start(tTimerHndl, &sIntervalTimer, OPT_TIMER_PERIODIC);
    return(tTimerHndl);
}

void RemoveTimerAsWkUp() {
    UART_PRINT("REMOVE TIMER\r\n");
    cc_timer_stop(g_tTimerHndl);
    cc_timer_delete(g_tTimerHndl);
}
//*****************************************************************************
//
//! \brief Task Created by main fucntion.This task starts simpleink, set NWP
//!        power policy, connects to an AP. Give Signal to the other task about
//!        the connection.wait for the message form the interrupt handlers and
//!        the other task. Accordingly print the wake up cause from the low
//!        power modes.
//!
//! \param pvParameters is a general void pointer (not used here).
//!
//! \return none
//
//*****************************************************************************
void TimerGPIOTask(void *pvParameters)
{
    // cc_hndl tTimerHndl = NULL; 
    cc_hndl tGPIOHndl = NULL;
    unsigned char ucQueueMsg = 0;
    int iRetVal = 0;
  

    //UART_PRINT("Start TimerGPIOTask\r\n");
    
    // SYNC with network task
    unsigned char ucSyncMsg = 0;
    // osi_MsgQRead(&g_tConnectionFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
    // osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg, OSI_NO_WAIT);

    //
    // Queue management related configurations
    //
    // iRetVal = osi_MsgQCreate(&g_tWkupSignalQueue, NULL,
    //                          sizeof( unsigned char ), 10);
    // if (iRetVal < 0)
    // {
    //     UART_PRINT("unable to create the msg queue\n\r");
    //     LOOP_FOREVER();
    // }

    //
    // setting Timer as one of the wakeup source
    //
    g_tTimerHndl = SetTimerAsWkUp();
    
    //
    // setting some GPIO as one of the wakeup source
    //
    tGPIOHndl = SetGPIOAsWkUp();

    // DkS: set up RTC
    //
    // setting up Timer as a wk up source and other timer configurations
    //
    // struct u64_time sInitTime;
    // sInitTime.secs = 0;
    // sInitTime.nsec = 0;
    // cc_rtc_set(&sInitTime);
    // cc_rtc_get(&sInitTime);

    // UART_PRINT("time s=%d, ms=%d\r\n",sInitTime.secs, sInitTime.nsec/1000000);
    // int i;
    // int j;
    // for (i = 0; i < 10000000; i++) {
    //     j++;
    // }
    // UART_PRINT("time s=%d, ms=%d\r\n",sInitTime.secs, sInitTime.nsec/1000000);
    
    /* handles, if required, can be used to stop the timer, but not used here*/
    // UNUSED(tTimerHndl);
    // UNUSED(tGPIOHndl);
    //
    // setting Apps power policy
    //
    #if (LPDS_MODE)
    lp3p0_setup_power_policy(POWER_POLICY_STANDBY);
    #else
    lp3p0_setup_power_policy(POWER_POLICY_HIBERNATE);
    #endif
    disable_deep_sleep();
#if 0    
    while(FOREVER)
    {
        //
        // waits for the message from the various interrupt handlers(GPIO,
        // Timer) and the UDPServerTask.
        //
        osi_MsgQRead(&g_tWkupSignalQueue, &ucQueueMsg, OSI_WAIT_FOREVER);
        switch(ucQueueMsg){
        case 1:
            UART_PRINT("timer\n\r");
            break;
        case 2:
            UART_PRINT("GPIO\n\r");
            break;
        case 3:
            UART_PRINT("host irq\n\r");
            break;
        default:
            UART_PRINT("invalid msg\n\r");
            break;
        }
    }
#endif
}

#if 0
//*****************************************************************************
//
//! \brief Task Created by main fucntion. This task creates a udp server and
//!        wait for packets. Upon receiving the packet, signals the other task.
//!
//! \param pvParameters is a general void pointer (not used here).
//!
//! \return none
//
//*****************************************************************************
void UDPServerTask(void *pvParameters)
{
    unsigned char ucSyncMsg;
    unsigned char ucQueueMsg = 3;
    int iSockDesc = 0;
    int iRetVal = 0;
    sockaddr_in sLocalAddr;
    sockaddr_in sClientAddr;
    unsigned int iAddrSize = 0;
       
    //
    // waiting for the other task to start simplelink and connection to the AP
    //
    osi_MsgQRead(&g_tConnectionFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
    osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg, OSI_NO_WAIT);

    //
    // configure the Server
    //
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = sl_Htons((unsigned short)APP_UDP_PORT);
    sLocalAddr.sin_addr.s_addr = 0;
    
    iAddrSize = sizeof(sockaddr_in);
    
    //
    // creating a UDP socket
    //
    iSockDesc = sl_Socket(SL_AF_INET,SL_SOCK_DGRAM, 0);

    if(iSockDesc < 0)
    {
        UART_PRINT("sock error\n\r");
        LOOP_FOREVER();
    }
    
    //
    // binding the socket
    //
    iRetVal = sl_Bind(iSockDesc, (SlSockAddr_t *)&sLocalAddr, iAddrSize);
    if(iRetVal < 0)
    {
        UART_PRINT("bind error\n\r");
        LOOP_FOREVER();
    }
    
    while(FOREVER)
    {
        //
        // waiting on a UDP packet
        //
        iRetVal = sl_RecvFrom(iSockDesc, g_cBuffer, BUFF_SIZE, 0,
                              ( SlSockAddr_t *)&sClientAddr,
                              (SlSocklen_t*)&iAddrSize );
        if(iRetVal > 0)
        {
            //
            // signal the other task about receiving the UDP packet
            //
            osi_MsgQWrite(&g_send_notification_flag, &ucQueueMsg, OSI_WAIT_FOREVER);
            UART_PRINT("recv network wakeup\n\r");
        }
        else
        {
            UART_PRINT("recv error\n\r");
            LOOP_FOREVER();
        }
    }  
}
#endif
//****************************************************************************
//                            MAIN FUNCTION
//****************************************************************************
void init_power_manager(void)
{
    int iRetVal;

    //
    // Initialize the platform
    //
    platform_init();

#ifdef DEBUG_GPIO
    //
    // setting up GPIO for indicating the entry into LPDS
    //
    tGPIODbgHndl = cc_gpio_open(POWER_LED_GPIO, GPIO_DIR_OUTPUT);
    cc_gpio_write(tGPIODbgHndl, POWER_LED_GPIO, 1);
#endif
    //
    // Task creation for providing host_irq as a wake up source(LPDS)
    //
    // iRetVal =  osi_TaskCreate(UDPServerTask,
    //                           (const signed char *)"UDP Server waiting to recv"\
    //                           "packets", OSI_STACK_SIZE, NULL, 12, NULL );
    // if(iRetVal < 0)
    // {
    //     UART_PRINT("First Task creation failed\n\r");
    //     LOOP_FOREVER();
    // }
#if 0
    //
    // setting up timer and gpio as source for wake up from lPDS
    //
    iRetVal =  osi_TaskCreate(TimerGPIOTask,
                              (const signed char*)"Configuring Timer and GPIO as"\
                              " wake src", OSI_STACK_SIZE*2, NULL, 13, NULL );
    if(iRetVal < 0)
    {
        UART_PRINT("Second Task creation failed\n\r");
        LOOP_FOREVER();
    }
#else
    TimerGPIOTask(NULL);
#endif
#if 0
    //
    // Task creation for providing host_irq as a wake up source(LPDS)
    //
    iRetVal =  osi_TaskCreate(UDPServerTask,
                              (const signed char *)"UDP Server waiting to recv"\
                              "packets", OSI_STACK_SIZE, NULL, 1, NULL );
    if(iRetVal < 0)
    {
        UART_PRINT("First Task creation failed\n\r");
        LOOP_FOREVER();
    }
#endif
}
