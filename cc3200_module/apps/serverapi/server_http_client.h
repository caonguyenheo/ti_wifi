#ifndef __OTAHTTPCLIENT_H__
#define __OTAHTTPCLIENT_H__

#include <http/client/httpcli.h>
#include <http/client/common.h>

#define MYCIN_DEVICE_REGISTER 			"/v1/devices/%s/register.json?access_token=%s"
// #define MYCIN_DEVICE_TOKEN				"FAXYNhHQek1S6HD6nYXq"

#define DEFAULT_HTTPS_PORT              443
#define DEFAULT_HTTP_PORT               80

#define ERR_GET_HOST_IP_FAILED          -10
#define ERR_SERVER_CONNECTION_FAILED    -11
#define MYCIN_API_REQUEST         -12
#define MYCIN_API_RESPONSE_STATUS -13
#define MYCIN_API_NOT_200         -14
#define MYCIN_API_FILTER_ID       -15
#define MYCIN_API_SHORT_BUF       -16
#define MYCIN_API_GET_BODY        -17
#define MYCIN_API_LEN_MATCH       -18
#define MYCIN_API_BODY_NOT_JSON   -19

#define ERR_VERSION_RECEIVE_INVALID     -100
#define ERR_VERSION_CONFIG_INVALID      -101

int8_t myCinHttpConnect(HTTPCli_Struct *pHttpClient, char *address, uint16_t port, uint8_t secure);
int8_t myCinHttpClose(HTTPCli_Struct *pHttpClient);
int8_t myCinHttpSendRequest(HTTPCli_Struct *pHttpClient, char *address, char *url, char *method, char *msg);
int8_t myCinHttpVersionSetUrl(char *pmyCinUrl, uint32_t len, char *registerID, char *token);
int8_t myCinHttpVersionSetPayload(char *pMessage, uint32_t len, char *udid);
int8_t myCinHttpVersionGetResponse(HTTPCli_Struct *pHttpClient, char *reqRecvBuf, uint32_t len);
int8_t myCinHttpSendGetRequest(HTTPCli_Struct *pHttpClient, char *address, char *url);

#endif
