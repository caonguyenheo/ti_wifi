#ifndef __CVHTTPD_H__
#define __CVHTTPD_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#elif defined(TARGET_IS_CC3200)
#include "simplelink.h"
//#include "user_common.h"
#endif

#ifdef __linux__
#define MIN_SOCKET  0
#define BUFSIZE     2560

#elif defined(TARGET_IS_CC3200)
#define MIN_SOCKET  16          // socket fd is from 16 to 23
#define BUFSIZE     200

#define O_NONBLOCK      SL_SO_NONBLOCKING
//#define printf(...)     UART_PRINT(__VA_ARGS__)
#define printf(...)

#define sleep(x)        osi_Sleep((x)*1000)
#endif

#define HAS_EXENSION_CHECK  0

// Application specific status/error codes
typedef enum {
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    SOCKET_CREATE_ERROR = -0x7D0,
    BIND_ERROR = SOCKET_CREATE_ERROR - 1,
    LISTEN_ERROR = BIND_ERROR - 1,
    SOCKET_OPT_ERROR = LISTEN_ERROR - 1,
    CONNECT_ERROR = SOCKET_OPT_ERROR - 1,
    ACCEPT_ERROR = CONNECT_ERROR - 1,
    SEND_ERROR = ACCEPT_ERROR - 1,
    RECV_ERROR = SEND_ERROR - 1,
    SOCKET_CLOSE_ERROR = RECV_ERROR - 1,
    DEVICE_NOT_IN_STATION_MODE = SOCKET_CLOSE_ERROR - 1,
    SELECT_ERROR = DEVICE_NOT_IN_STATION_MODE - 1,
    STATUS_CODE_MAX = -0xBB8
} e_AppStatusCodes;

typedef enum {
    HTTPD_IDLE = 0,
    HTTPD_RUNNING,
    HTTPD_EXIT
} httpd_status_t;

extern httpd_status_t httpd_status;

#define HTTPVERSION         23
#define HTTPERROR           42
#define HTTPLOG             44
#define HTTPFORBIDDEN       403
#define HTTPNOTFOUND        404

#define FORBIDDEN_MSG "HTTP/1.1 403 HTTPFORBIDDEN\nContent-Length: 105\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 HTTPFORBIDDEN</title>\n</head><body>\n<h1>Forbidden</h1>\nNot allow.\n</body></html>\n"

#define NOTFOUND_MSG "HTTP/1.1 404 Not Found\nContent-Length: 101\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nNot Found.\n</body></html>\n"

#define DEFAULT_MSG "HTTP/1.1 200 OK\nContent-Length: 99\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>cvhttpd</title>\n</head><body>\n<h1>Welcome</h1>\nThis is cvhttpd.\n</body></html>\n"

int32_t httpd_start(uint16_t);
int handle_request(int fd, const char*, int len);
int logger(int type, const char *s1, const char *s2, int socket_fd);

#endif
