#ifndef __RGB_UTIL_H__
#define __RGB_UTIL_H__

#include <stdint.h>
#include "rgb_led.h"

int8_t RgbParse(const char *msg, uint32_t msgLen, Rgb_t *rgb);

#endif