#ifndef __UTIL_H__
#define __UTIL_H__

// common interface includes
#include "stdint.h"

#ifdef __linux__
#define DEBUG(...)      printf(__VA_ARGS__)
#define INFO(...)       printf(__VA_ARGS__)

#elif defined(TARGET_IS_CC3200)
#include "common.h"
#include "uart_if.h"
#define DEBUG(...) 		UART_PRINT(__VA_ARGS__)
#define INFO(...) 		UART_PRINT(__VA_ARGS__)
#endif

#define VERSION_DIGIT_MAX   9             // max value of 1B
#define VERSION_BYTE        3               // MAJOR.MINOR.PATCH

#define CHECK_RETURN(x) do { \
    if (x < 0) {      \
        return (x);   \
    }                 \
} while (0)

int32_t parse_fota(const char *json, uint32_t len, char **version, char **host, char **path, char **protocol);
int32_t convert_version(const char *version_str, uint32_t len, uint32_t *version_bin);

int32_t ParseVersionResponse(char *message, char *version, uint32_t verLen, char *protocol, uint32_t proLen, char *host, uint32_t hostLen, char *url, uint32_t urlLen);

#endif
