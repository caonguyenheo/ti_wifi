#ifndef __CVHTTPD_H__
#define __CVHTTPD_H__

#include <stdint.h>
#include <stdlib.h>

#ifdef __linux__
#include <fcntl.h>
// #include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#elif defined(TARGET_IS_CC3200)
#include "simplelink.h"
#include "user_common.h"
#endif

#ifdef __linux__
#define MIN_SOCKET  0

#elif defined(TARGET_IS_CC3200)
#define O_NONBLOCK SL_SO_NONBLOCKING
#define printf(...)  UART_PRINT(__VA_ARGS__)

#define MIN_SOCKET  16          // socket fd is from 16 to 23
#endif

// Application specific status/error codes
typedef enum {
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    SOCKET_CREATE_ERROR = -0x7D0,
    BIND_ERROR = SOCKET_CREATE_ERROR - 1,
    LISTEN_ERROR = BIND_ERROR -1,
    SOCKET_OPT_ERROR = LISTEN_ERROR -1,
    CONNECT_ERROR = SOCKET_OPT_ERROR -1,
    ACCEPT_ERROR = CONNECT_ERROR - 1,
    SEND_ERROR = ACCEPT_ERROR -1,
    RECV_ERROR = SEND_ERROR -1,
    SOCKET_CLOSE_ERROR = RECV_ERROR -1,
    DEVICE_NOT_IN_STATION_MODE = SOCKET_CLOSE_ERROR - 1,
    SELECT_ERROR = DEVICE_NOT_IN_STATION_MODE -1,
    STATUS_CODE_MAX = -0xBB8
} e_AppStatusCodes;


int32_t httpd_start(uint16_t);

#endif