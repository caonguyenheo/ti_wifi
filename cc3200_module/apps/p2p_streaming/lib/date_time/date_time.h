#ifndef __DATE_TIME_H__
#define __DATE_TIME_H__
#include <stdint.h>

#define DATE_TIME_ERROR_OK     			0
#define DATE_TIME_ERROR_UNKNOWN			-1
#define DATE_TIME_ERROR_NOT_STARTED		-2

int32_t dt_start(void);
int32_t dt_set_time(uint32_t time_epoch, uint32_t msec);
uint32_t dt_get_time_s(void);
int32_t dt_set_timezone(int32_t time_zone_h, int32_t time_zone_m);
int32_t dt_get_timezone(int32_t *time_zone_h, int32_t *time_zone_m);

int32_t stopwatch_start(void);
int32_t stopwatch_stop(void);
uint32_t stopwatch_get_ms(void);
#endif//__DATE_TIME_H__