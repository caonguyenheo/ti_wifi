#ifndef __HTTPCLIENTAPP_H__
#define __HTTPCLIENTAPP_H__

#include <http/client/httpcli.h>

//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************
int ConnectToHTTPServer(HTTPCli_Handle httpClient, char *host, int HOST_PORT);

//*****************************************************************************
//
//! \brief HTTP DELETE Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTPDeleteMethod(HTTPCli_Handle httpClient);

//*****************************************************************************
//
//! \brief HTTP GET Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTPGetMethod(HTTPCli_Handle httpClient, 
  char*          url, 
  char*          post_data, 
  char*          resp_data
  );

//*****************************************************************************
//
//! \brief HTTP POST Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTPPostMethod(HTTPCli_Handle httpClient, 
 char*          url, 
 char*          post_data, 
 char*          resp_data
 );

//*****************************************************************************
//
//! \brief HTTP PUT Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTPPutMethod(HTTPCli_Handle httpClient);


//*****************************************************************************
//
//! \brief Function to package push notification datas
//!
//*****************************************************************************
void packageDataToNotify(char* dataToSend, uint32_t dataLen);

//*****************************************************************************
//
//! \brief HTTP GET to push notification datas
//!
//! \param[in]  httpClient - Pointer to http client
//! \param[in]  host_name  - Host name 
//! \param[in]  respData   - REsponse data from server 
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTPPushNotification(HTTPCli_Handle httpClient, 
 char*          host_name, 
 char*          respData
 );


//*****************************************************************************
//
//! \brief HTTP Close connection with server
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return NULL
//!
//*****************************************************************************
void HTTPCloseConnection(HTTPCli_Handle httpCli);

//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************

int 
https_connect(HTTPCli_Handle httpClient, char* host_ip, int host_port);

//*****************************************************************************
//
//! \brief      Request GET data to server 
//!
//! \param[in]  httpClient - http client handle
//! \param[in]  host_name  - host name 
//! \param[in]  req_data   - request data to server 
//! \param[in]  respData   - response data from server 
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int 
HTTP_Get(HTTPCli_Handle httpClient, 
         char*          host_name,
         char*          req_data,
         char*          respData
 );


//*****************************************************************************
//
//! \brief      Request POST data to server 
//!
//! \param[in]  httpClient - http client handle
//! \param[in]  host_name  - host name 
//! \param[in]  post_uri   - post url 
//! \param[in]  post_data  - request data to server 
//! \param[out]  respData   - response data from server 
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int HTTP_Post(HTTPCli_Handle httpClient, 
              char*          host_name,
              char*          post_uri,
              char*          post_data,
              char*          respData
 );

//*****************************************************************************
//
//! \brief      Parse response data with specific key 
//!
//! \param[in]  resp_buf      - response buffer 
//! \param[in]  resp_buf_len  - response buffer length 
//! \param[in]  key           - key word to get value  
//! \param[out] value         - value return 
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
int
parse_response(char* resp_buf,
 int   resp_buf_len,
 char* key,
 char* value
 );

#endif
