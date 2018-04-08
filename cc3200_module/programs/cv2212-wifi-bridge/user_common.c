#include <stdio.h>
#include <string.h>

#include "pinmux.h"
#include "user_common.h"

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

#define UART_PRINT              Report

static void
DisplayBanner(char * AppName)
{
    DBG_PRINT("\n\n\n\r");
    DBG_PRINT("\t\t *************************************************\n\r");
    DBG_PRINT("\t\t       CC3200 %s Application       \n\r", AppName);
    DBG_PRINT("\t\t *************************************************\n\r");
    DBG_PRINT("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
void
BoardInit(void)
{
    // In case of TI-RTOS vector table is initialize by OS itself
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

    //
    // Pinmux for UART & LED
    //
    PinMuxConfig();

#ifndef NOTERM
    //
    // Configuring UART
    //
    InitTerm();

    // Show up the project name
    DisplayBanner(PROJECT);
#endif
}

int32_t
set_time(const SlDateTime_t *cur_time)
{
    return sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                      SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                      sizeof(SlDateTime_t),
                      (unsigned char *)(cur_time));
}
