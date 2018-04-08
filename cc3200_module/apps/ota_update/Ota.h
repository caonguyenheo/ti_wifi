/*
 * Nguyen Quoc Dinh
 * Nov, 2015
 */

#ifndef __OTA_H__
#define __OTA_H__

// #include "simplelink.h"
#ifdef __linux__
#elif defined(TARGET_IS_CC3200)
#include "user_config.h"
#endif

#include "OtaUtil.h"

#define MAX_SERVER_NAME     48
#define MAX_PROTOCOL_STR    7
#define MAX_VERSION_STR     12

#define MAX_OTA_URL_STR     100

#define MAX_CDN_HOST_STR    48
#define MAX_CDN_URL_STR     100

#define MAX_VERSION_RCV_BUF 1000
#define MAX_IMAGE_NAME      21          // /sys/mcubootinfo.bin

#define PROTOCOL_HTTP   "http:"
#define PROTOCOL_HTTPS  "https:"

#define ALIGNMENT_READ_MASK             0x0F
#define SIZE_100K                       (100*1024)          /* Serial flash file size 100 KB */
#define MAX_FILE_SIZE       SIZE_100K

// #define MAX_PATH_PREFIX     48
// #define MAX_REST_HDRS_SIZE  96
// #define MAX_USER_NAME_STR   8
// #define MAX_USER_PASS_STR   8
// #define MAX_CERT_STR        32
// #define MAX_KEY_STR         32

// check the error code and handle it

#define DEFAULT_HTTPS_PORT              443
#define DEFAULT_HTTP_PORT               80
#define DEFAULT_HTTP_TIMEOUT            10               // 5 secs timeout

#define ERR_OTA_IS_RUNNING              -1
#define ERR_START_SL                    -2
#define ERR_STOP_SL                     -3

#define ERR_GET_HOST_IP_FAILED          -10
#define ERR_SERVER_CONNECTION_FAILED    -11
#define ERR_OTA_VERSION_REQUEST         -12
#define ERR_OTA_VERSION_RESPONSE_STATUS -13
#define ERR_OTA_VERSION_NOT_200         -14
#define ERR_OTA_VERSION_FILTER_ID       -15
#define ERR_OTA_VERSION_SHORT_BUF       -16
#define ERR_OTA_VERSION_GET_BODY        -17
#define ERR_OTA_VERSION_LEN_MATCH       -18
#define ERR_OTA_VERSION_BODY_NOT_JSON   -19
#define ERR_OTA_VERSION_PARSE_RESPONSE  -20

#define ERR_VERSION_RECEIVE_INVALID     -100
#define ERR_VERSION_CONFIG_INVALID      -101

#define ERR_ACCESS_IMG_BOOT_INFO        -200
#define ERR_WRITE_IMG_BOOT_INFO         -201
#define ERR_CLOSE_IMG_BOOT_INFO         -202
#define ERR_INVALID_IMAGE_NAME          -203

#define ERR_CDN_RESPONSE_STATUS         -301
#define ERR_CDN_NOT_200                 -302
#define ERR_CDN_FILTER_ID               -303

#define ERR_FLASH_OPEN                  -400
#define ERR_FLASH_WRITE                 -401


typedef struct {
    char version[MAX_VERSION_STR];
    char address[MAX_SERVER_NAME];
    uint16_t port;
    uint8_t secure;
    uint32_t interval;          // in second
} otaInfo_t;

int32_t StartOta(otaInfo_t *);

#endif
