
#include <string.h>

// SimpleLink includes
#include "simplelink.h"

// driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "utils.h"
#include "interrupt.h"

// common interface includes
#include "uart_if.h"
#include "common.h"
#include "pinmux.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>
// JSON Parser
#include "jsmn.h"
#include "httpclientapp.h"
// #include "NetworkAPI.h"

#include "system.h"
#include <stdio.h>

#define APPLICATION_VERSION "1.1.1"
#define APP_NAME            "HTTP Client"

//#define POST_REQUEST_URI 	"/post.php?dir=example"
#define POST_REQUEST_URI 	"/v4/devices/register.json?api_key=82mFvVHXFsxK4zWHzF-2"
#define POST_DATA           "{\n\"name\":\"xyz\",\n\"address\":\n{\n\"plot#\":12,\n\"street\":\"abc\",\n\"city\":\"ijk\"\n},\n\"age\":30\n}"

#define DELETE_REQUEST_URI 	"/delete"


#define PUT_REQUEST_URI 	"/put"
#define PUT_DATA            "PUT request."

// #define GET_REQUEST_URI 	"/BMS/cameraservice?action=command&command=notification&alert=%02d&val=%s&auth_token=%s&mac=%s&time=%s"
#define GET_REQUEST_URI     "/v1/devices/events/cameraservice?action=command&command=notification&alert=%02d&val=%s&auth_token=%s&mac=%s&time=%s"


//#define HOST_NAME       	"www.howsmyssl.com" //"<host name>"
#define HOST_NAME       	"posttestserver.com" //"<host name>"
//#define HOST_NAME       	"dev-api.mycin.in" //"<host name>"

//#define HOST_PORT           443

#define PROXY_IP       	    <proxy_ip>
#define PROXY_PORT          <proxy_port>

#define READ_SIZE           1450
#define MAX_BUFF_SIZE       1460*2


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

// Application specific status/error codes
typedef enum{
 /* Choosing this number to avoid overlap with host-driver's error codes */
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,       
    DEVICE_START_FAILED = DEVICE_NOT_IN_STATION_MODE - 1,
    INVALID_HEX_STRING = DEVICE_START_FAILED - 1,
    TCP_RECV_ERROR = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED = INVALID_SERVER_RESPONSE - 1,
    FILE_OPEN_FAILED = FORMAT_NOT_SUPPORTED - 1,
    FILE_WRITE_ERROR = FILE_OPEN_FAILED - 1,
    INVALID_FILE = FILE_WRITE_ERROR - 1,
    SERVER_CONNECTION_FAILED = INVALID_FILE - 1,
    GET_HOST_IP_FAILED = SERVER_CONNECTION_FAILED  - 1,
    
    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

//******************************************************************************
//
//
//
//
//******************************************************************************
//static unsigned char g_buff[MAX_BUFF_SIZE+1];


//*****************************************************************************
//
//! \brief Flush response body.
//!
//! \param[in]  httpClient - Pointer to HTTP Client instance
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
//static int FlushHTTPResponse(HTTPCli_Handle httpClient)
//{
//    const char *ids[2] = {
//                            HTTPCli_FIELD_NAME_CONNECTION, /* App will get connection header value. all others will skip by lib */
//                            NULL
//                         };
//    char buf[128];
//    int id;
//    int len = 1;
//    bool moreFlag = 0;
//    char ** prevRespFilelds = NULL;
//
//
//    /* Store previosly store array if any */
//    prevRespFilelds = HTTPCli_setResponseFields(httpClient, ids);
//
//    /* Read response headers */
//    while ((id = HTTPCli_getResponseField(httpClient, buf, sizeof(buf), &moreFlag))
//            != HTTPCli_FIELD_ID_END)
//    {
//
//        if(id == 0)
//        {
//            if(!strncmp(buf, "close", sizeof("close")))
//            {
//                UART_PRINT("Connection terminated by server\n\r");
//            }
//        }
//
//    }
//
//    /* Restore previosuly store array if any */
//    HTTPCli_setResponseFields(httpClient, (const char **)prevRespFilelds);
//
//    while(1)
//    {
//        /* Read response data/body */
//        /* Note:
//                moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
//                data is available Or in other words content length > length of buffer.
//                The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
//                Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
//                for more information.
//        */
//        HTTPCli_readResponseBody(httpClient, buf, sizeof(buf) - 1, &moreFlag);
//        ASSERT_ON_ERROR(len);
//
//        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n'){
//            break;
//        }
//
//        if(!moreFlag)
//        {
//            /* There no more data. break the loop. */
//            break;
//        }
//    }
//    return 0;
//}


//*****************************************************************************
//
//! \brief Handler for parsing JSON data
//!
//! \param[in]  ptr - Pointer to http response body data
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
/*
static int ParseJSONData(char *ptr)
{
	long lRetVal = 0;
    int noOfToken;
    jsmn_parser parser;
    jsmntok_t   *tokenList;


    // Initialize JSON PArser 
    jsmn_init(&parser);

    // Get number of JSON token in stream as we we dont know how many tokens need to pass
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), NULL, 10);
    if(noOfToken <= 0)
    {
    	UART_PRINT("Failed to initialize JSON parser\n\r");
    	return -1;

    }

    // Allocate memory to store token
    tokenList = (jsmntok_t *) malloc(noOfToken*sizeof(jsmntok_t));
    if(tokenList == NULL)
    {
        UART_PRINT("Failed to allocate memory %d\n\r", __LINE__);
        return -1;
    }

    // Initialize JSON Parser again
    jsmn_init(&parser);
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), tokenList, noOfToken);
    if(noOfToken < 0)
    {
    	UART_PRINT("Failed to parse JSON tokens\n\r");
    	lRetVal = noOfToken;
    }
    else
    {
    	UART_PRINT("Successfully parsed %ld JSON tokens\n\r", noOfToken);
    }

    free(tokenList);

    return lRetVal;
}*/

/*!
    \brief This function read respose from server and dump on console

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
//static int readResponse(HTTPCli_Handle httpClient, char *resp_data)
//{
//	long lRetVal = 0;
//	int bytesRead = 0;
//	int id = 0;
//	unsigned long len = 0;
//	int json = 0;
//	char *dataBuffer=NULL;
//	bool moreFlags = 1;
//	const char *ids[4] = {
//	                        HTTPCli_FIELD_NAME_CONTENT_LENGTH,
//			                HTTPCli_FIELD_NAME_CONNECTION,
//			                HTTPCli_FIELD_NAME_CONTENT_TYPE,
//			                NULL
//	                     };
//
//	/* Read HTTP POST request status code */
//	lRetVal = HTTPCli_getResponseStatus(httpClient);
//	if(lRetVal > 0)
//	{
//        // UART_PRINT("status code %d\n", lRetVal);
//		switch(lRetVal)
//		{
//		case 200:
//		{
//			UART_PRINT("HTTP Status 200\n\r");
//			/*
//                 Set response header fields to filter response headers. All
//                  other than set by this call we be skipped by library.
//			 */
//			HTTPCli_setResponseFields(httpClient, (const char **)ids);
//
//			/* Read filter response header and take appropriate action. */
//			/* Note:
//                    1. id will be same as index of fileds in filter array setted
//                    in previous HTTPCli_setResponseFields() call.
//
//                    2. moreFlags will be set to 1 by HTTPCli_getResponseField(), if  field
//                    value could not be completely read. A subsequent call to
//                    HTTPCli_getResponseField() will read remaining field value and will
//                    return HTTPCli_FIELD_ID_DUMMY. Please refer HTTP Client Libary API
//                    documenation @ref HTTPCli_getResponseField for more information.
//			 */
//			while((id = HTTPCli_getResponseField(httpClient, (char *)g_buff, sizeof(g_buff), &moreFlags))
//					!= HTTPCli_FIELD_ID_END)
//			{
//
//				switch(id)
//				{
//				case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
//				{
//					len = strtoul((char *)g_buff, NULL, 0);
//				}
//				break;
//				case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
//				{
//				}
//				break;
//				case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
//				{
//					if(!strncmp((const char *)g_buff, "application/json",
//							sizeof("application/json")))
//					{
//						json = 1;
//					}
//					else
//					{
//						/* Note:
//                                Developers are advised to use appropriate
//                                content handler. In this example all content
//                                type other than json are treated as plain text.
//						 */
//						json = 0;
//					}
//					UART_PRINT(HTTPCli_FIELD_NAME_CONTENT_TYPE);
//					UART_PRINT(" : ");
//					UART_PRINT("application/json\n\r");
//				}
//				break;
//				default:
//				{
//					UART_PRINT("Wrong filter id\n\r");
//					lRetVal = -1;
//					goto end;
//				}
//				}
//			}
//			bytesRead = 0;
//			if(len > sizeof(g_buff))
//			{
//				dataBuffer = (char *) malloc(len);
//				if(dataBuffer)
//				{
//					UART_PRINT("Failed to allocate memory %d\n\r", __LINE__);
//					lRetVal = -1;
//					goto end;
//				}
//			}
//			else
//			{
//				dataBuffer = (char *)g_buff;
//			}
//
//			/* Read response data/body */
//			/* Note:
//                    moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
//		            data is available Or in other words content length > length of buffer.
//		            The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
//		            Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
//		            for more information
//
//			 */
//			bytesRead = HTTPCli_readResponseBody(httpClient, (char *)dataBuffer, len, &moreFlags);
//			if(bytesRead < 0)
//			{
//				UART_PRINT("Failed to received response body\n\r");
//				lRetVal = bytesRead;
//				goto end;
//			}
//			else if( bytesRead < len || moreFlags)
//			{
//				UART_PRINT("Mismatch in content length and received data length\n\r");
//				goto end;
//			}
//			dataBuffer[bytesRead] = '\0';
//
//			if(json)
//			{
//				/* Parse JSON data */
////				lRetVal = ParseJSONData(dataBuffer);
//                          UART_PRINT("resp data: %s\n", dataBuffer);
//                          memcpy(resp_data, dataBuffer, strlen(dataBuffer));
////				if(lRetVal < 0)
////				{
////					goto end;
////				}
//			}
//			else
//			{
//				/* treating data as a plain text */
//                          UART_PRINT("url encode format\n");
////                          UART_PRINT("resp data: %s\n", dataBuffer);
//                          memcpy(resp_data, dataBuffer, strlen(dataBuffer));
//			}
//
//		}
//		break;
//
//		case 404:
//			UART_PRINT("404 File not found. \r\n");
//			/* Handle response body as per requirement.
//                  Note:
//                    Developers are advised to take appopriate action for HTTP
//                    return status code else flush the response body.
//                    In this example we are flushing response body in default
//                    case for all other than 200 HTTP Status code.
//			 */
//		default:
//			/* Note:
//              Need to flush received buffer explicitly as library will not do
//              for next request.Apllication is responsible for reading all the
//              data.
//			 */
//			FlushHTTPResponse(httpClient);
//			break;
//		}
//	}
//	else
//	{
//		UART_PRINT("Failed to receive data from server.\r\n");
//		goto end;
//	}
//
//	lRetVal = 0;
//
//end:
//    if(len > sizeof(g_buff) && (dataBuffer != NULL))
//	{
//	    free(dataBuffer);
//    }
//    return lRetVal;
//}

//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************

//#define SECURE 1
//
//int ConnectToHTTPServer(HTTPCli_Handle httpClient, char *host, int HOST_PORT)
//{
//    long lRetVal = -1;
//    struct sockaddr_in addr;
//    unsigned int g_ulDestinationIP;
//    
//    
//#ifdef SECURE
//    SlDateTime_t dt;
//
//    /* Set current Date to validate certificate */
//    dt.sl_tm_day = 10;
//    dt.sl_tm_mon = 11;
//    dt.sl_tm_year = 2015;
//    dt.sl_tm_hour = 12;
//    dt.sl_tm_min = 32;
//    dt.sl_tm_sec = 0;
//    sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION, SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME, sizeof(SlDateTime_t), (unsigned char *)(&dt));
//#endif
//    
//    /* Resolve HOST NAME/IP */
//    lRetVal = sl_NetAppDnsGetHostByName((signed char *)host,
//                                          strlen((const char *)host),
//                                          (_u32 *)&g_ulDestinationIP,SL_AF_INET);
//    if(lRetVal < 0)
//    {
//        ASSERT_ON_ERROR(GET_HOST_IP_FAILED);
//    }
//
//    
//#ifdef SECURE
//#define SL_SSL_CA_CERT	"cert/cacert.der"
////    struct HTTPCli_SecureParams sparams;
////    /* Set secure TLS connection  */
////    /* Security parameters */
////    sparams.method.secureMethod = SL_SO_SEC_METHOD_TLSV1_2;
////    sparams.mask.secureMask = SL_SEC_MASK_SECURE_DEFAULT;
//////    strncpy(sparams.cafile, SL_SSL_CA_CERT, sizeof(SL_SSL_CA_CERT));
////    sparams.cafile[0] = 0;
////    sparams.privkey[0] = 0;
////    sparams.cert[0] = 0;
////    sparams.dhkey[0] = 0;
////    HTTPCli_setSecureParams(&sparams);
//#endif
//    
//    /* Set up the input parameters for HTTP Connection */
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(HOST_PORT);
//    addr.sin_addr.s_addr = sl_Htonl(g_ulDestinationIP);
//
//    UART_PRINT("serverIP: %d.%d.%d.%d\n",
//                      (g_ulDestinationIP>>24)&0xFF,
//                      (g_ulDestinationIP>>16)&0xFF,
//                      (g_ulDestinationIP>>8)&0xFF,
//                      g_ulDestinationIP&0xFF);
//    /* Testing HTTPCli open call: handle, address params only */
//    HTTPCli_construct(httpClient);
//#ifdef SECURE
//    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, HTTPCli_TYPE_TLS, NULL);
//#else
//    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
//#endif
//    
//    if (lRetVal < 0)
//    {
//        UART_PRINT("Connection to server failed. error(%d)\n\r", lRetVal);
//        ASSERT_ON_ERROR(SERVER_CONNECTION_FAILED);
//    }    
//    else
//    {
//        UART_PRINT("Connection to server created successfully\r\n");
//    }
//
//    return 0;
//}
//*****************************************************************************
//
//! \brief HTTP POST Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
//int HTTPPostMethod(HTTPCli_Handle httpClient, char* url, char *post_data, char *resp_data)
//{
//    bool moreFlags = 1;
//    bool lastFlag = 1;
//    char tmpBuf[4];
//    long lRetVal = 0;
//    HTTPCli_Field fields[4] = {
//                                {HTTPCli_FIELD_NAME_HOST, url},
//                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
//                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/json"},
//                                {NULL, NULL}
//                            };
//    
//
//    /* Set request header fields to be send for HTTP request. */
//    HTTPCli_setRequestFields(httpClient, fields);
//
//    /* Send POST method request. */
//    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
//       other than setted in previous call HTTPCli_setRequestFields() at later stage.
//       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
//    */
//    moreFlags = 1;
//    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, POST_REQUEST_URI, moreFlags);
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP POST request header. at LINE[%d]\n\r", __LINE__);
//        return lRetVal;
//    }
//
//    sprintf(tmpBuf, "%d", (strlen(post_data)));
//    UART_PRINT("post_data: \n %.*s\n", strlen(post_data), post_data);
//    /* Here we are setting lastFlag = 1 as it is last header field.
//       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
//    */
//    lastFlag = 1;
//    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP POST request header. at LINE[%d]\n\r", __LINE__);
//        return lRetVal;
//    }
//
//
//    /* Send POST data/body */
//    lRetVal = HTTPCli_sendRequestBody(httpClient, post_data, (strlen(post_data)));
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP POST request body.\n\r");
//        return lRetVal;
//    }
//
//
//    lRetVal = readResponse(httpClient, resp_data);
//
//    return lRetVal;
//}


//*****************************************************************************
//
//! \brief HTTP DELETE Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
//int HTTPDeleteMethod(HTTPCli_Handle httpClient)
//{
//  
//    long lRetVal = 0;
//    HTTPCli_Field fields[3] = {
//                                {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
//                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
//                                {NULL, NULL}
//                            };
//    bool moreFlags;
//
//
//    /* Set request header fields to be send for HTTP request. */
//    HTTPCli_setRequestFields(httpClient, fields);
//
//    /* Send DELETE method request. */
//    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
//       at later stage. Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest
//       for more information.
//    */
//    moreFlags = 0;
//    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_DELETE, DELETE_REQUEST_URI, moreFlags);
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP DELETE request header.\n\r");
//        return lRetVal;
//    }
//
////    lRetVal = readResponse(httpClient, resp_data);
//
//    return lRetVal;
//}

//*****************************************************************************
//
//! \brief HTTP GET Demonstration
//!
//! \param[in]  httpClient - Pointer to http client
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
//int HTTPGetMethod(HTTPCli_Handle httpClient, char* url, char *post_data, char *resp_data)
//{
//  
//    long lRetVal = 0;
//    HTTPCli_Field fields[4] = {
//                                {HTTPCli_FIELD_NAME_HOST, url},
//                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
//                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
//                                {NULL, NULL}
//                            };
//    bool        moreFlags;
//    
//    
//    /* Set request header fields to be send for HTTP request. */
//    HTTPCli_setRequestFields(httpClient, fields);
//
//    /* Send GET method request. */
//    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
//       at later stage. Please refer HTTP Library API documentaion @ HTTPCli_sendRequest
//       for more information.
//    */
//    moreFlags = 0;
//    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI, moreFlags);
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP GET request.\n\r");
//        return lRetVal;
//    }
//
//
//    lRetVal = readResponse(httpClient, resp_data);
//
//    return lRetVal;
//}


//*****************************************************************************
//
//! Function to connect to HTTP server
//!
//! \param  httpClient - Pointer to HTTP Client instance
//!
//! \return Error-code or SUCCESS
//!
//*****************************************************************************

//int 
//https_connect(HTTPCli_Handle httpClient, char* host_ip, int host_port)
//{
//    long lRetVal = -1;
//    struct sockaddr_in addr;
//    int g_ulDestinationIP = atoi(host_ip);
//    
//    
//#ifdef SECURE
//    SlDateTime_t dt;
//
//    /* Set current Date to validate certificate */
//    dt.sl_tm_day = 10;
//    dt.sl_tm_mon = 11;
//    dt.sl_tm_year = 2015;
//    dt.sl_tm_hour = 12;
//    dt.sl_tm_min = 32;
//    dt.sl_tm_sec = 0;
//    sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION, SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME, sizeof(SlDateTime_t), (unsigned char *)(&dt));
//#endif
//    
//    // /* Resolve HOST NAME/IP */
//    // lRetVal = sl_NetAppDnsGetHostByName((signed char *)host,
//    //                                       strlen((const char *)host),
//    //                                       &g_ulDestinationIP,SL_AF_INET);
//    // if(lRetVal < 0) {
//    //     ASSERT_ON_ERROR(GET_HOST_IP_FAILED);
//    // }
//
//    
//#ifdef SECURE
//#define SL_SSL_CA_CERT  "cert/cacert.der"
////    struct HTTPCli_SecureParams sparams;
////    /* Set secure TLS connection  */
////    /* Security parameters */
////    sparams.method.secureMethod = SL_SO_SEC_METHOD_TLSV1_2;
////    sparams.mask.secureMask = SL_SEC_MASK_SECURE_DEFAULT;
//////    strncpy(sparams.cafile, SL_SSL_CA_CERT, sizeof(SL_SSL_CA_CERT));
////    sparams.cafile[0] = 0;
////    sparams.privkey[0] = 0;
////    sparams.cert[0] = 0;
////    sparams.dhkey[0] = 0;
////    HTTPCli_setSecureParams(&sparams);
//#endif
//    
//    /* Set up the input parameters for HTTP Connection */
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(host_port);
//    addr.sin_addr.s_addr = sl_Htonl(g_ulDestinationIP);
//
//    UART_PRINT("serverIP: %d.%d.%d.%d\n",
//                      (g_ulDestinationIP>>24)&0xFF,
//                      (g_ulDestinationIP>>16)&0xFF,
//                      (g_ulDestinationIP>>8)&0xFF,
//                      g_ulDestinationIP&0xFF);
//    /* Testing HTTPCli open call: handle, address params only */
//    HTTPCli_construct(httpClient);
//#ifdef SECURE
//    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, HTTPCli_TYPE_TLS, NULL);
//#else
//    lRetVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
//#endif
//    
//    if (lRetVal < 0)
//    {
//        UART_PRINT("Connection to server failed. error(%d)\n\r", lRetVal);
//        ASSERT_ON_ERROR(SERVER_CONNECTION_FAILED);
//    }    
//    else
//    {
//        UART_PRINT("Connection to server created successfully\r\n");
//    }
//
//    return 0;
//}
//*****************************************************************************
//
//! \brief      Request GET data to server 
//!
//! \param[in]  httpClient - http client handle
//! \param[in]  host_name  - host name 
//! \param[in]  req_data   - request data to server 
//! \param[out]  respData   - response data from server 
//!
//! \return 0 on success else error code on failure
//!
//*****************************************************************************
//int 
//HTTP_Get(HTTPCli_Handle httpClient, 
//         char*          host_name,
//         char*          req_data,
//         char*          respData
//)
//{
//  long lRetVal = 0;
//  HTTPCli_Field fields[4] = {
//    {HTTPCli_FIELD_NAME_HOST, host_name},
//    {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
//    {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
//    {NULL, NULL}
//  };
//  bool        moreFlags;
//  /* Set request header fields to be send for HTTP request. */
//  HTTPCli_setRequestFields(httpClient, fields);
//  
//  /* Send GET method request. */
//  /* Here we are setting moreFlags = 0 as there are no more header fields need to send
//  at later stage. Please refer HTTP Library API documentaion @ HTTPCli_sendRequest
//  for more information.
//  */
//  moreFlags = 0;
//  lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, req_data, moreFlags);
////  lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, "/", moreFlags);
//  if(lRetVal < 0)
//  {
//    UART_PRINT("Failed to send HTTP GET request %d\n\r", lRetVal);
//    return lRetVal;
//  }
//  
//  
//  lRetVal = readResponse(httpClient, respData);
//  
//  return lRetVal;
//  
//}

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
//int HTTP_Post(HTTPCli_Handle httpClient, 
//              char*          host_name,
//              char*          post_uri,
//              char*          post_data,
//              char*          respData
// )
//{
//
//    bool moreFlags = 1;
//    bool lastFlag = 1;
//    char tmpBuf[4];
//    long lRetVal = 0;
//    HTTPCli_Field fields[4] = {
//                                {HTTPCli_FIELD_NAME_HOST, host_name},
//                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
//                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/json"},
//                                {NULL, NULL}
//                            };
//    
//
//    /* Set request header fields to be send for HTTP request. */
//    HTTPCli_setRequestFields(httpClient, fields);
//
//    /* Send POST method request. */
//    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
//       other than setted in previous call HTTPCli_setRequestFields() at later stage.
//       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
//    */
//    moreFlags = 1;
//    lRetVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, post_uri, moreFlags);
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP POST request header. at LINE[%d]\n\r", __LINE__);
//        return lRetVal;
//    }
//
//    snprintf(tmpBuf, sizeof tmpBuf, "%d", (strlen(post_data)));
//    // UART_PRINT("post_data: \n %.*s\n", strlen(post_data), post_data);
//    /* Here we are setting lastFlag = 1 as it is last header field.
//       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
//    */
//    lastFlag = 1;
//    lRetVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP POST request header. at LINE[%d]\n\r", __LINE__);
//        return lRetVal;
//    }
//
//
//    /* Send POST data/body */
//    lRetVal = HTTPCli_sendRequestBody(httpClient, post_data, (strlen(post_data)));
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP POST request body.\n\r");
//        return lRetVal;
//    }
//
//
//    lRetVal = readResponse(httpClient, respData);
//
//    return lRetVal;
//
//}

/*void HTTPCloseConnection(HTTPCli_Handle httpCli)
{
  HTTPCli_disconnect(httpCli);
  HTTPCli_destruct(httpCli);
}*/

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
  )
{
    char* key_ptr;
    char* end_ptr;
    int   value_len = 0;
    key_ptr = strstr(resp_buf, key);
    if (NULL != key_ptr) {
        if(0 == strcmp(key, "sip")) {      // case key is 'sip'
            value_len = resp_buf_len - (key_ptr + strlen(key) + 1 - resp_buf);
            strncpy(value, key_ptr + strlen(key) + 1, value_len);
        } else {                          // other case
            end_ptr = strstr(key_ptr, "&");
            if(NULL == end_ptr) {
                return -1;
            }
            value_len =  end_ptr - key_ptr - strlen(key) - 1;
            strncpy(value, key_ptr + strlen(key) + 1, value_len);
        }
        return 0;
    } else {
        return -1;
    }
}
