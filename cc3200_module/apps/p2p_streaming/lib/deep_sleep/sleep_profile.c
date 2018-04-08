#include "sleep_profile.h"

#include "simplelink.h"
// driverlib includes 
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "timer.h"

// common interface includes 
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "timer_if.h"

#define ENABLE_DEEP_SLEEP           (1)
//middleware includes
// #include "cc_types.h"
// #include "rtc_hal.h"
// #include "gpio_hal.h"
// #include "uart_drv.h"
// #include "cc_timer.h"
// #include "cc_pm_ops.h"

// #define LPDS_DUR_SEC            60
// #define LPDS_DUR_NSEC           0

// void TimerConfigNStart();
// void TimerDeinitStop();
// void sleep_profile_TimerPeriodicIntHandler(void);
// cc_hndl SetTimerAsWkUp();

static volatile uint8_t g_is_deep_sleep = 0;
#if (ENABLE_DEEP_SLEEP)
//
//! \brief Application defined idle task hook
//! 
//! \param  none
//! 
//! \return none
//!
//*****************************************************************************
#include "cc_pm.h"
void
vApplicationIdleHook( void)
{
	// if (g_is_deep_sleep == 1) {
    cc_idle_task_pm();
    // UART_PRINT("GO THERE\r\n");
    // }
}
#endif

//****************************************************************************
//
//! Function to configure and start timer to blink the LED while device is
//! trying to connect to an AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
// void TimerConfigNStart()
// {
//   //
//   // Configure Timer for blinking the LED for IP acquisition
//   //
//   Timer_IF_Init(PRCM_TIMERA0,TIMERA0_BASE,TIMER_CFG_PERIODIC,TIMER_A,0);
//   Timer_IF_IntSetup(TIMERA0_BASE,TIMER_A,sleep_profile_TimerPeriodicIntHandler);
//   Timer_IF_Start(TIMERA0_BASE,TIMER_A,15000);     // time value is in mSec
// }

//****************************************************************************
//
//! Disable the LED blinking Timer as Device is connected to AP
//!
//! \param none
//!
//! return none
//
//****************************************************************************
// void TimerDeinitStop()
// {
//   //
//   // Disable the LED blinking Timer as Device is connected to AP
//   //
//   Timer_IF_Stop(TIMERA0_BASE,TIMER_A);
//   Timer_IF_DeInit(TIMERA0_BASE,TIMER_A);

// }

//*****************************************************************************
//
//! Periodic Timer Interrupt Handler
//!
//! \param None
//!
//! \return None
//
//*****************************************************************************
// void
// sleep_profile_TimerPeriodicIntHandler(void)
// {
//     unsigned long ulInts;
    
//     //
//     // Clear all pending interrupts from the timer we are
//     // currently using.
//     //
//     ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
//     MAP_TimerIntClear(TIMERA0_BASE, ulInts);
//     DBG_PRINT("interrupt\n\r");

//     //
//     // Increment our interrupt counter.
//     //
//     // g_usTimerInts++;
//     // if(!(g_usTimerInts & 0x1))
//     // {
//         //
//         // Off Led
//         //
//         // GPIO_IF_LedOff(MCU_RED_LED_GPIO);
//     // }
//     // else
//     // {
//         //
//         // On Led
//         //
//         // GPIO_IF_LedOn(MCU_RED_LED_GPIO);
//     // }
// }
extern int ota_done;
int32_t enable_deep_sleep(void) {
    //
    // setting Timer as one of the wakeup source
    //
    // cc_hndl tTimerHndl = NULL; 
    // tTimerHndl = SetTimerAsWkUp();
    // Deep sleep
	// g_is_deep_sleep = 1;
	if (ota_done==1)
    	cc_app_resume_pm();
	return 0;
}

int32_t disable_deep_sleep(void) {
	// g_is_deep_sleep = 0;
    cc_app_putoff_pm();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
/*
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
    // osi_MsgQWrite(&g_tWkupSignalQueue, &ucQueueMsg, OSI_NO_WAIT);
    UART_PRINT("TIMER FIRE\r\n");
    return;
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

*/
void idle_task(void *arg) {
    UART_PRINT("IDLE TASK IS RUNNING\r\n");
    while (1) {
        // if (g_is_deep_sleep == 1) {
            // UART_PRINT("SLEEP\r\n");
            cc_idle_task_pm();
            // osi_Sleep(100);
        // }
    }
    LOOP_FOREVER();
    return;
}
