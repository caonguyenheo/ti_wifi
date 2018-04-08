#ifndef __SLEEP_PROFILE_H__
#define __SLEEP_PROFILE_H__

#include <stdint.h>

void TimerConfigNStart();
int32_t enable_deep_sleep(void);
int32_t disable_deep_sleep(void);
#endif