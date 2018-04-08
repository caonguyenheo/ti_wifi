#include <stdint.h>
#include <string.h>

#include "server_api.h"
#include "server_http_client.h"
#include "server_util.h"

// common interface includes
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
extern char MYCIN_API_HOST_NAME[];
int8_t myCinRegister(myCinUserInformation_t *pmyCinUser, myCinApiRegister_t *pmyCinRegister)
{
  bool secure = 1;
  int32_t lRetVal;
  HTTPCli_Struct httpClient;

  lRetVal = myCinHttpConnect(&httpClient, MYCIN_API_HOST_NAME, MYCIN_API_HOST_PORT, secure);
  if (lRetVal < 0) {
    UART_PRINT("Fail connect to HOST_NAME = %s PORT %d\r\n", MYCIN_API_HOST_NAME, MYCIN_API_HOST_PORT);
    //goto end;
    return lRetVal;
  }

  // construct l_Url
  char l_Url[MAX_OTA_URL_STR];
  lRetVal = myCinHttpVersionSetUrl(l_Url, sizeof(l_Url), pmyCinUser->device_udid, pmyCinUser->user_token);
  if (lRetVal < 0) {
    UART_PRINT("Fail to set url\n");
    goto end;
  }

  char regisPayload[MAX_OTA_PAYLOAD_REQ];
  lRetVal = myCinHttpVersionSetPayload(regisPayload, sizeof(regisPayload), pmyCinUser->device_udid);
  if (lRetVal < 0) {
    UART_PRINT("Fail to set payload\n");
    goto end;
  }
  UART_PRINT("url:%s, payload:%s\n", l_Url, regisPayload);

  lRetVal = myCinHttpSendRequest(&httpClient, MYCIN_API_HOST_NAME, l_Url, HTTPCli_METHOD_POST, regisPayload);
  if (lRetVal < 0) {
      UART_PRINT("Fail to post to myCin [%d]\n", lRetVal);
      goto end;
  }

  char recvPayload[MAX_OTA_PAYLOAD_RCV];
  lRetVal = myCinHttpVersionGetResponse(&httpClient, recvPayload, sizeof(recvPayload));
  if (lRetVal < 0) {
      UART_PRINT("Fail receive response [%d]\n", lRetVal);
      goto end;
  }

  UART_PRINT("Payload: %s\n", recvPayload);

  lRetVal = myCinRegisterParse(recvPayload, strlen(recvPayload), pmyCinUser, pmyCinRegister);
  UART_PRINT("Registration ret %d\r\n", lRetVal);
  if (lRetVal < 0) {
      goto end;
  }

  lRetVal = 0;

end:
  myCinHttpClose(&httpClient);

  return lRetVal;
}
