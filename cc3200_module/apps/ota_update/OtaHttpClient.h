
#ifndef __OTAHTTPCLIENT_H__
#define __OTAHTTPCLIENT_H__

#include "Ota.h"
#include <http/client/httpcli.h>
#include <http/client/common.h>

typedef int32_t (*OtaFlashWriteCall)(uint32_t bytesReceived, char *buff, uint32_t len, uint32_t l_total_length);

int32_t OtaHttpConnect(HTTPCli_Struct *pHttpClient, char *address, uint16_t port, uint8_t secure);
int32_t OtaHttpClose(HTTPCli_Struct *pHttpClient);
int32_t OtaHttpVersionSetUrl(char *pOtaUrl, uint32_t len);

int32_t OtaHttpSendRequest(HTTPCli_Struct *pHttpClient, char *address, char *url, char *method);
int32_t OtaHttpVersionGetResponse(HTTPCli_Struct *pHttpClient, char *reqRecvBuf, uint32_t len);

int32_t OtaHttpCdnGetFile(HTTPCli_Struct *pHttpClient, OtaFlashWriteCall writeCb);

#endif
