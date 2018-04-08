/**
* @defgroup HttpCore
*
* @{
*/

#include "HttpResponse.h"
#include "HttpRequest.h"
#include "HttpDebug.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "webserver.h"
#include <stdio.h>
#include "system.h"
#include "cc3200_system.h"
#include "HttpString.h"
#include "url_parser.h"
#include "system.h"
// common interface includes
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "common.h"
#include "webserver.h"
#include "p2p_main.h"
// USING OLD SETWIRELESS SAVE COMMAND
#define OLD_STYLE_WIRELESS_HTTP_COMMAND       0
#define size_cmd    200

static struct HttpBlob nullBlob = {NULL, 0};


/*******************************************************************************
* @brief Function to handle http command
*
* @param[int]  strParam: command
* @param[out]  *response:  http respone to client
* @param[out]  *fileResp: flag to respone as a xml file
* @return:
//*/

#if (OLD_STYLE_HTTP_COMMAND)
#define   HTTP_COMMAND_URL_HEADER     "/?action=command&"
#else
#define   HTTP_COMMAND_URL_HEADER     "/?"
#endif

int HandleCommand(char *reqBuf, int reqBufLen, char *response, int *fileResponse)
{
  char *pHeadPtr = NULL;
  int respcode = 404;
  char command[size_cmd] = {0};
  size_t oSize;
  if(!response)
    return -1;
  if((pHeadPtr = strstr(reqBuf, HTTP_COMMAND_URL_HEADER)) != NULL)
  {
    pHeadPtr += strlen(HTTP_COMMAND_URL_HEADER);
    memcpy(command, pHeadPtr, reqBufLen - strlen(HTTP_COMMAND_URL_HEADER));
    HttpDebug("Command: '%s'\n\r", command);
    if (httpServerOperationTime)// reset timeout
        httpServerOperationTime=1;
    respcode = command_handler(command, oSize, HTTP_COMMAND, response, fileResponse);
//    HttpDebug("respcode: %d\n\r", respcode);
  }
  return respcode;
}
void SendResponse(
                  UINT16    uConnection, 
                  int       iRetCode, 
                  char     *strMessage,
                  int      fileResponse
                    )
{
  struct HttpBlob contentType = {0, NULL};
  struct HttpBlob respData = {0, NULL};
  
  switch(iRetCode)
  {
  case 200:
    if(fileResponse)
    {
      char strMimeType[16] = "text/plain"; // Currently, we only support command get_rt_list as file response. so only support this mime, need to extend incase support other mime
      //      char sendBuf[200] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"\
      //        "<testXML>\n"\
      //          "<note>\n"\
      //            "<header>\"hello world\"</header>\n"
      //              "<body>\"hello anh\"</body>\n"
      //                "<footer>\"good luck man\"</footer>\n"
      //                  "</note>\n"
      //                    "</testXML>\n";
      contentType.pData = strMimeType;
      contentType.uLength = strlen(strMimeType);
      if(!HttpResponse_Headers(uConnection, HTTP_STATUS_OK, 0, strlen(router_list), contentType, nullBlob))
      {
        HttpDebug("response header error\n");
      }
      respData.pData = router_list;
      // memcpy(respData.pData, "");
      respData.uLength = strlen(router_list);
      //          HttpDebug("uContentLeft: %d\n",g_state.connections[uConnection].uContentLeft);
      if(!HttpResponse_Content(uConnection, respData)){
        HttpDebug("response content error\n");       
      }
      HttpDebug("response content complete at line %d\n", __LINE__);
      
    }else
    {
      // sprintf(pContentBuf,   "HTTP/1.0 200 OK\r\nContent-type: text/html\r\n\r\n"
      //         "%s", strMessage);
      contentType.pData = (char *)"text/plain";
      contentType.uLength = strlen("text/plain");//text/html
      if(!HttpResponse_Headers(uConnection, HTTP_STATUS_OK, 0, strlen(strMessage), contentType, nullBlob))
      {
        HttpDebug("response header error\n");
      }
      respData.pData = strMessage;
      // memcpy(respData.pData, "");
      respData.uLength = strlen(strMessage);
      //      HttpDebug("uContentLeft: %d at LINE(%d) \n",g_state.connections[uConnection].uContentLeft, __LINE__);
      if(!HttpResponse_Content(uConnection, respData)){
        HttpDebug("response content error\n");       
      }
      //      HttpDebug("uContentLeft: %d at LINE(%d)\n",g_state.connections[uConnection].uContentLeft, __LINE__);
    }
    
    break;
  case 400:
    contentType.pData = (char *)"text/plain";
    contentType.uLength = strlen("text/plain");
    if(!HttpResponse_Headers(uConnection, HTTP_STATUS_ERROR_NOT_FOUND, 0, strlen(strMessage), contentType, nullBlob))
    {
      HttpDebug("response header error\n");
    }
    respData.pData = strMessage;
    respData.uLength = strlen(strMessage);
    if(!HttpResponse_Content(uConnection, respData)){
      HttpDebug("response content error\n");       
    }
    break;
  case 401:
    contentType.pData = (char *)"text/plain";
    contentType.uLength = strlen("text/plain");
    if(!HttpResponse_Headers(uConnection, HTTP_STATUS_ERROR_UNAUTHORIZED, 0, strlen(strMessage), contentType, nullBlob))
    {
      HttpDebug("response header error\n");
    }
    respData.pData = strMessage;
    respData.uLength = strlen(strMessage);
    if(!HttpResponse_Content(uConnection, respData)){
      HttpDebug("response content error\n");       
    }
    break;
  case 404:
    contentType.pData = (char *)"text/plain";
    contentType.uLength = strlen("text/plain");
    if(!HttpResponse_Headers(uConnection, HTTP_STATUS_ERROR_NOT_FOUND, 0, strlen("404 not found"), contentType, nullBlob))
    {
      HttpDebug("response header error\n");
    }
    respData.pData = "404 not found";
    respData.uLength = strlen("404 not found");
    //    HttpDebug("uContentLeft = %d\n",g_state.connections[uConnection].uContentLeft);
    if(!HttpResponse_Content(uConnection, respData)){
      HttpDebug("response content error\n");       
    }
    //    HttpDebug("uContentLeft = %d at LINE(%d) \n",g_state.connections[uConnection].uContentLeft, __LINE__);
    break;
  case 406:
    contentType.pData = (char *)"text/plain";
    contentType.uLength = strlen("text/plain");
    if(!HttpResponse_Headers(uConnection, HTTP_STATUS_ERROR_NOT_ACCEPTED, 0, strlen(strMessage), contentType, nullBlob))
    {
      HttpDebug("response header error\n");
    }
    respData.pData = strMessage;
    respData.uLength = strlen(strMessage);
    if(!HttpResponse_Content(uConnection, respData)){
      HttpDebug("response content error\n");       
    }
    break;
  case 500:
    contentType.pData = (char *)"text/plain";
    contentType.uLength = strlen("text/plain");
    if(!HttpResponse_Headers(uConnection, HTTP_STATUS_ERROR_INTERNAL, 0, strlen(strMessage), contentType, nullBlob))
    {
      HttpDebug("response header error\n");
    }
    respData.pData = strMessage;
    respData.uLength = strlen(strMessage);
    if(!HttpResponse_Content(uConnection, respData)){
      HttpDebug("response content error\n");       
    }
    break;
  default:
    contentType.pData = (char *)"text/plain";
    contentType.uLength = strlen("text/plain");
    if(fileResponse != -1)
    {
      if(!HttpResponse_Headers(uConnection, HTTP_STATUS_ERROR_NOT_FOUND, 0, strlen(strMessage), contentType, nullBlob))
      {
        HttpDebug("response header error\n");
      }
      respData.pData = strMessage;
      respData.uLength = strlen(strMessage);
      if(!HttpResponse_Content(uConnection, respData)){
        HttpDebug("response content error\n");       
      }      
    }
    else
    {
      HttpDebug("Restart system command\n");
      if(!HttpResponse_Headers(uConnection, HTTP_STATUS_OK, 0, strlen(strMessage), contentType, nullBlob))
      {
        HttpDebug("response header error\n");
      }
      respData.pData = strMessage;
      respData.uLength = strlen(strMessage);
      if(!HttpResponse_Content(uConnection, respData)){
        HttpDebug("response content error\n");       
      } 
    }

    break;
  }
}    // SendResponse

/// @}
