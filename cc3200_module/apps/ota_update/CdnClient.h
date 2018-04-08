
#ifndef __CDNCLIENT_H__
#define __CDNCLIENT_H__

#include "Ota.h"
#include "OtaFlash.h"

typedef struct {
    char version[MAX_VERSION_STR];
    char protocol[MAX_PROTOCOL_STR];
    char host[MAX_CDN_HOST_STR];
    char url[MAX_CDN_URL_STR];
    char host_ak[MAX_CDN_HOST_STR]; // For AK chip
    char url_ak[MAX_CDN_URL_STR]; // For AK chip
} cdnInfo_t;

int32_t CdnGetImage(cdnInfo_t *, imageInfo_t *);

#endif
