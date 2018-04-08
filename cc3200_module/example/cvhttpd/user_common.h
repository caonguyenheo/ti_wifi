#ifndef __FREERTOS_COMMON_H__
#define __FREERTOS_COMMON_H__

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

#include "../../user/ssid_config.h"

void BoardInit(void);
void DisplayBanner(char * AppName);

#endif