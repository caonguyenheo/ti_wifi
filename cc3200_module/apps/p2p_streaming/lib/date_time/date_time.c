#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
#include "hw_types.h"
#include <driverlib/prcm.h>
#include "date_time.h"
#include "cc_types.h"
#include "rtc_hal.h"


#define DATE_TIME_STATE_UNINIT 			0
#define DATE_TIME_STATE_START			1
// UNINIT -> START

#define STOP_WATCH_STATE_UNINIT			0
#define STOP_WATCH_STATE_START			1
#define STOP_WATCH_STATE_STOP  			2

// UNINIT -> START -> STOP -> START
#define SEC_PER_HOUR					3600
#define SEC_PER_MIN						60
static volatile uint32_t delta_ntp_ms = 0;



static uint32_t stop_watch_timer_s = 0;
static uint16_t stop_watch_timer_ms = 0;
// static volatile uint32_t dt_time_epoch = 0;
static volatile int32_t time_zone_adjust_s = 0;
static uint8_t dt_state = DATE_TIME_STATE_UNINIT;
static uint8_t sw_state = STOP_WATCH_STATE_UNINIT;



//*****************************************************************************
//
//! start date time calendar
//!
//! \brief  This function starts RTC counting on hw
//!
//! \param  none
//!
//! \return 0 : success, -1 : failure
//!
//
//*****************************************************************************
int32_t dt_start(void) {
	//  first init date time
	if (DATE_TIME_STATE_UNINIT == dt_state) {
		UART_PRINT("Start RTC\n");
		// NOTE: Already init in other thread
		// PRCMRTCInUseSet();
	}

	// if already start, return and do nothing
	if (DATE_TIME_STATE_START == dt_state) {
		return DATE_TIME_ERROR_OK;
	}

	// start date time
	// UART_PRINT("Set RTC\n");
	// PRCMRTCSet(0, 0);
	dt_state = DATE_TIME_STATE_START;
	return DATE_TIME_ERROR_OK;
}
//*****************************************************************************
//
//! set date time calendar
//!
//! \brief  This function set time in epoch format to RTC counter
//!
//! \param  time_epoch is epoch time format
//! \param  msec is milisecond
//!
//! \return 0 : success, -1 : failure
//!
//
//*****************************************************************************
int32_t dt_set_time(uint32_t time_epoch, uint32_t msec) {
	struct u64_time current_time;	
	if (DATE_TIME_STATE_UNINIT == dt_state) {
		return DATE_TIME_ERROR_NOT_STARTED;
	}
	int32_t ret_val = 0;
	ret_val = cc_rtc_get(&current_time);
	delta_ntp_ms = time_epoch - current_time.secs;
	UART_PRINT("delta_ntp_ms=%u, timezone %u\r\n", delta_ntp_ms, time_zone_adjust_s);
	// PRCMRTCSet(time_epoch, msec);
	return DATE_TIME_ERROR_OK;
}
//*****************************************************************************
//
//! get current time in epoch format
//!
//! \brief  This function gets epoch time
//!
//! \param  none
//!
//! \return milisecond
//!
//
//*****************************************************************************
uint32_t g_lasttimeget=0;
uint32_t dt_get_time_s(void) {
	// uint32_t cur_date_s = 0;
	// uint16_t cur_date_ms = 0;
	struct u64_time current_time;
	if (DATE_TIME_STATE_UNINIT == dt_state) {
		return DATE_TIME_ERROR_NOT_STARTED;
	}
	cc_rtc_get(&current_time);
//	UART_PRINT("Current_time = %u, timezone %u\r\n", current_time.secs + delta_ntp_ms + time_zone_adjust_s,time_zone_adjust_s);
	g_lasttimeget = current_time.secs + delta_ntp_ms + time_zone_adjust_s;
	return g_lasttimeget;

	// PRCMRTCGet(&cur_date_s, &cur_date_ms);
	// return cur_date_s + time_zone_adjust_s;
}
//*****************************************************************************
//
//! set timezone
//!
//! \brief  This function sets timezone
//!
//! \param  time_zone
//!
//! \return 0 : success, -1 : failure
//!
//
//*****************************************************************************
int32_t dt_set_timezone(int32_t time_zone_h, int32_t time_zone_m) {
	if (time_zone_m < -30 || time_zone_m > 30) {
		return DATE_TIME_ERROR_UNKNOWN;
	}
	if (time_zone_h < -12 || time_zone_h > 12) {
		return DATE_TIME_ERROR_UNKNOWN;
	}
	time_zone_adjust_s = time_zone_h * SEC_PER_HOUR + time_zone_m * SEC_PER_MIN;
	return 0;
}
//*****************************************************************************
//
//! get timezone
//!
//! \brief  This function gets timezone
//!
//! \param  time_zone is output
//!
//! \return 0 : success, -1 : failure
//!
//
//*****************************************************************************
int32_t dt_get_timezone(int32_t *time_zone_h, int32_t *time_zone_m) {
	*time_zone_h = time_zone_adjust_s / SEC_PER_HOUR;
	*time_zone_m = (time_zone_adjust_s % SEC_PER_HOUR) / SEC_PER_MIN;
	return DATE_TIME_ERROR_OK;
}

//*****************************************************************************
//                 STOP WATCH -- Start
//*****************************************************************************
//*****************************************************************************
//
//! start stop watch
//!
//! \brief  This function to start stop watch
//!
//! \param  none
//!
//! \return 0 : success, -1 : failure
//!
//
//*****************************************************************************
int32_t stopwatch_start(void) {
	if (STOP_WATCH_STATE_UNINIT == sw_state) {
		if (DATE_TIME_STATE_UNINIT == dt_state) {
			return DATE_TIME_ERROR_NOT_STARTED;
		}
	}

	switch (sw_state) {
	case STOP_WATCH_STATE_START:
		break;
	default:
		// save the start point
		PRCMRTCGet(&stop_watch_timer_s, &stop_watch_timer_ms);
		sw_state = STOP_WATCH_STATE_START;
	}

	return DATE_TIME_ERROR_OK;
}
//*****************************************************************************
//
//! stop stop watch
//!
//! \brief  This function to get stop watch
//!
//! \param  none
//!
//! \return 0 : success, -1 : failure
//!
//
//*****************************************************************************
int32_t stopwatch_stop(void) {
	if (STOP_WATCH_STATE_UNINIT == sw_state) {
		if (DATE_TIME_STATE_UNINIT == dt_state) {
			return DATE_TIME_ERROR_NOT_STARTED;
		}
	}

	switch (sw_state) {
	case STOP_WATCH_STATE_STOP:
		break;
	default:
		// RESET VALUE
		stop_watch_timer_s = 0;
		stop_watch_timer_ms = 0;
		sw_state = STOP_WATCH_STATE_STOP;
	}

	return DATE_TIME_ERROR_OK;
}
//*****************************************************************************
//
//! get stop watch value
//!
//! \brief  get current value of stop watch
//!
//! \param  none
//!
//! \return msecond
//!
//
//*****************************************************************************
uint32_t stopwatch_get_ms(void) {
	uint32_t current_s = 0;
	uint16_t current_ms = 0;
	uint32_t delta_ms = 0;

	if (STOP_WATCH_STATE_UNINIT == sw_state || DATE_TIME_STATE_UNINIT == dt_state) {
		return 0;
	}
	PRCMRTCGet(&current_s, &current_ms);
	delta_ms = current_s - stop_watch_timer_s;
	delta_ms = delta_ms * 1000 + current_ms;
	return delta_ms;
}
//*****************************************************************************
//                 STOP WATCH -- End
//*****************************************************************************
