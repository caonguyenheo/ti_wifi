/*
 * Nguyen Quoc Dinh
 * Nov, 2015
 */

#include "simplelink.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>

#include "server_http_client.h"
#include "common.h"
#include "uart_if.h"
#include "cc3200_system.h"
#include "system.h"

int8_t myCinHttpConnect(HTTPCli_Struct *pHttpClient, char *address, uint16_t port, uint8_t secure)
{
    uint32_t ulDestinationIP; // IP address of destination server
    int32_t lRetVal = -1;
    struct sockaddr_in addr;
    //
    // todo:
    // use Proxy?
    // 

    /* Resolve HOST NAME/IP */
    lRetVal = sl_NetAppDnsGetHostByName((signed char *)address,
                                        strlen((const char *)address),
                                        &ulDestinationIP,
                                        SL_AF_INET);
    if(lRetVal < 0) {
        return ERR_GET_HOST_IP_FAILED;
    }

    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = sl_Htonl(ulDestinationIP);

    uint16_t connectPort;
    if (port == 0) {
        connectPort = (secure)? DEFAULT_HTTPS_PORT: DEFAULT_HTTP_PORT;
    }
    else
        connectPort = port;

    addr.sin_port = htons(connectPort);

    /* Testing HTTPCli open call: handle, address params only */
    int32_t flag = 0;
    if (secure)
        flag |= HTTPCli_TYPE_TLS;

    HTTPCli_construct(pHttpClient);
    lRetVal = HTTPCli_connect(pHttpClient, (struct sockaddr *)&addr, flag, NULL);
    if (lRetVal < 0) {
        UART_PRINT("Connect status: %d\n", lRetVal);
        return ERR_SERVER_CONNECTION_FAILED;
    }    

    return 0;
}

int8_t myCinHttpClose(HTTPCli_Struct *pHttpClient)
{
    // Close connection
    HTTPCli_disconnect(pHttpClient);
    // Destroy the instance
    HTTPCli_destruct(pHttpClient);

    return 0;
}


/*
 * set url for the query
 */ 
int8_t myCinHttpVersionSetUrl(char *pmyCinUrl, uint32_t len, char *registerID, char *token)
{
    uint32_t lUrlLen = snprintf(pmyCinUrl, len, MYCIN_DEVICE_REGISTER, registerID, token);
    pmyCinUrl[lUrlLen] = '\0';
    return 0;
}
char DoorBell_name[32];
extern char timeZone[];
extern char g_local_ip[];
int8_t myCinHttpVersionSetPayload(char *pMessage, uint32_t len, char *udid)
{
    char ap_ssid_json[65]={0};
    escapeJsonString(ap_ssid, ap_ssid_json);
    uint32_t lMessLen = snprintf(pMessage, len,
        "{\"name\":\"%s\","
        "\"device_id\":\"%s\","
        "\"version\":\"%s\","
        "\"time_zone\":\"%s\","
        "\"mode\":\"upnp\","
        "\"local_ip\":\"%s\","
        "\"router_ssid\":\"%s\","
        "\"router_name\":\"%s\","
        "\"soc1_version\":\"%s\","
        "}",
        DoorBell_name,
        udid,
        g_version_8char,
        timeZone,
        g_local_ip,
        ap_ssid_json,"", ak_version_format);

    return 0;
}

/*
 * send the request
 */
int8_t myCinHttpSendRequest(HTTPCli_Struct *pHttpClient, char *address, char *url, char *method, char *msg)
{
    /*
     * set header for the request
     * for this version, set API key
     */
    HTTPCli_Field fields[4] = {
                                {HTTPCli_FIELD_NAME_HOST, address},
                                // {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_USER_AGENT, "curl/7.24.0"},
                                // {HTTPCli_FIELD_NAME_CACHE_CONTROL, "no-cache"},
                                {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/json"},
                                {NULL, NULL}
                            };
    
    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(pHttpClient, fields);

    // then send request
    bool moreFlags = 1;

    int32_t lRetVal = HTTPCli_sendRequest(pHttpClient, method, url, moreFlags);

    if (lRetVal < 0) {
        return MYCIN_API_REQUEST;
    }

    char tmpBuf[4];
    sprintf((char *)tmpBuf, "%d", strlen(msg));
    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    bool lastFlag = 1;
    lRetVal = HTTPCli_sendField(pHttpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
    if(lRetVal < 0) {
        // UART_PRINT("Failed to send HTTP POST request header.\n\r");
        return MYCIN_API_REQUEST;
    }


    /* Send POST data/body */
    lRetVal = HTTPCli_sendRequestBody(pHttpClient, msg, strlen(msg));
    if(lRetVal < 0) {
        // UART_PRINT("Failed to send HTTP POST request body.\n\r");
        return MYCIN_API_REQUEST;
    }
    
    return 0;
}

int8_t myCinHttpVersionGetResponse(HTTPCli_Struct *pHttpClient, char *recvBuf, uint32_t bufLen)
{

    long lRetVal = 0;
    int bytesRead = 0;
    int id = 0;
    unsigned long bodyLen = 0;
    bool moreFlags = 1;
    const char *ids[4] = {
                            HTTPCli_FIELD_NAME_CONTENT_LENGTH,
                            HTTPCli_FIELD_NAME_CONNECTION,
                            HTTPCli_FIELD_NAME_CONTENT_TYPE,
                            NULL
                         };

    /* Read HTTP POST request status code */
    lRetVal = HTTPCli_getResponseStatus(pHttpClient);
    UART_PRINT("Response status: %d\n", lRetVal);
    if (lRetVal < 0) {
        lRetVal = MYCIN_API_RESPONSE_STATUS;
        goto end;
    }

    if (lRetVal/100 != 2) {
        lRetVal = MYCIN_API_NOT_200;
        goto end;
    }

    /* then we have Status = 200 */
    /*
        Set response header fields to filter response headers. All
        other than set by this call we be skipped by library.
     */
    HTTPCli_setResponseFields(pHttpClient, (const char **)ids);

    /* Read filter response header and take appropriate action. */
    /* Note:
        1. id will be same as index of fileds in filter array setted
        in previous HTTPCli_setResponseFields() call.

        2. moreFlags will be set to 1 by HTTPCli_getResponseField(), if  field
        value could not be completely read. A subsequent call to
        HTTPCli_getResponseField() will read remaining field value and will
        return HTTPCli_FIELD_ID_DUMMY. Please refer HTTP Client Libary API
        documenation @ref HTTPCli_getResponseField for more information.
     */
    while((id = HTTPCli_getResponseField(pHttpClient, (char *)recvBuf, bufLen, &moreFlags))
            != HTTPCli_FIELD_ID_END)
    {
        switch(id) {
            case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
                bodyLen = strtoul((char *)recvBuf, NULL, 0);
                break;

            case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
                break;

            case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
                /*if(strncmp((const char *)recvBuf, "application/json",
                        strlen("application/json"))) {
                    #ifndef USING_FIRMWARE_UBISEN
                    // must be a json
                    lRetVal = MYCIN_API_BODY_NOT_JSON;
                    goto end;    
                    #endif
                }*/
                break;

            default:
                lRetVal = MYCIN_API_FILTER_ID;
                goto end;
        }
    }

    if (bodyLen > bufLen) {
        UART_PRINT("Failed to allocate memory %d\n\r", bodyLen);
        lRetVal = MYCIN_API_SHORT_BUF;
        goto end;
    }
    /* Read response data/body */
    /* Note:
            moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
            data is available Or in other words content length > length of buffer.
            The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
            Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
            for more information

     */
    bytesRead = HTTPCli_readResponseBody(pHttpClient, (char *)recvBuf, bufLen, &moreFlags);

    if (bytesRead < 0) {
        UART_PRINT("Failed to received response body\n\r");
        lRetVal = MYCIN_API_GET_BODY;
        goto end;
    }
    else if (bytesRead < bodyLen || moreFlags) {
        UART_PRINT("Mismatch in content length and received data length\n\r");
        lRetVal = MYCIN_API_LEN_MATCH;
        goto end;
    }
    recvBuf[bytesRead] = '\0';

    lRetVal = 0;

end:
    return lRetVal;
}

///*
// * send the get request
// */
//int8_t myCinHttpSendGetRequest(HTTPCli_Struct *pHttpClient, char *address, char *url) {
//    long lRetVal = 0;
//    HTTPCli_Field fields[4] = {
//                                {HTTPCli_FIELD_NAME_HOST, address},
//                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
//                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
//                                {NULL, NULL}
//                            };
//    bool        moreFlags;
//    
//    
//    /* Set request header fields to be send for HTTP request. */
//    HTTPCli_setRequestFields(pHttpClient, fields);
//
//    /* Send GET method request. */
//    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
//       at later stage. Please refer HTTP Library API documentaion @ HTTPCli_sendRequest
//       for more information.
//    */
//    moreFlags = 0;
//    lRetVal = HTTPCli_sendRequest(pHttpClient, HTTPCli_METHOD_GET, url, moreFlags);
//    if(lRetVal < 0)
//    {
//        UART_PRINT("Failed to send HTTP GET request.\n\r");
//        return lRetVal;
//    }
//
//
//    // lRetVal = readResponse(pHttpClient);
//
//    return lRetVal;
//}
