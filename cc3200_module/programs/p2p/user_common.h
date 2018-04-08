#ifndef __FREERTOS_COMMON_H__
#define __FREERTOS_COMMON_H__

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
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif

// common interface includes
#include "network_if.h"
#include "gpio_if.h"
#include "utils_if.h"
#include "timer_if.h"

#include "../../user/ssid_config.h"

#define DATE                5    /* Current Date */
#define MONTH               10     /* Month 1-12 */
#define YEAR                2015  /* Current year */
#define HOUR                12    /* Time - hours */
#define MINUTE              32    /* Time - minutes */
#define SECOND              0     /* Time - seconds */

#define TIME_CURRENT_DEFAULT { \
        .sl_tm_day = (uint32_t)DATE, \
        .sl_tm_mon = (uint32_t)MONTH, \
        .sl_tm_year = (uint32_t)YEAR, \
        .sl_tm_sec = (uint32_t)HOUR, \
        .sl_tm_hour = (uint32_t)MINUTE, \
        .sl_tm_min = (uint32_t)SECOND \
    }

void BoardInit(void);
int32_t set_time(const SlDateTime_t *);

#endif
