/*
 * Nguyen Quoc Dinh
 */

#include "Ota.h"
#include "simplelink.h"

// HTTP Client lib
#include "http/client/httpcli.h"
#include <http/client/common.h>

#include "OtaHttpClient.h"


/*
 * send the request
 */
int32_t OtaHttpSendRequest(HTTPCli_Struct *pHttpClient, char *address, char *url, char *method)
{
    /*
     * set header for the request
     * for this version, set API key
     */
#ifdef USING_FIRMWARE_UBISEN
    HTTPCli_Field fields[5] = {
                                {HTTPCli_FIELD_NAME_HOST, address},
                                {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
                                {CINATIC_OTA_API_KEY, CINATIC_OTA_API_KEY_VALUE},
                                {NULL, NULL}
                            };

    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(pHttpClient, fields);
#endif

    // then send request
    bool moreFlags = 0;

    int32_t lRetVal = HTTPCli_sendRequest(pHttpClient, method, url, moreFlags);

    if (lRetVal < 0) {
        return ERR_OTA_VERSION_REQUEST;
    }

    return 0;
}

int32_t OtaHttpCdnGetFile(HTTPCli_Struct *pHttpClient, OtaFlashWriteCall writeCb)
{
    long lRetVal = 0;
    int id = 0;
    unsigned long bodyLen = 0;
    bool moreFlags = 1;
    char recvBuf[MAX_VERSION_RCV_BUF];
    const char *ids[3] = {
                            HTTPCli_FIELD_NAME_CONTENT_LENGTH,
                            HTTPCli_FIELD_NAME_CONNECTION,
                            // HTTPCli_FIELD_NAME_CONTENT_TYPE,
                            NULL
                         };

    /* Read HTTP POST request status code */
    lRetVal = HTTPCli_getResponseStatus(pHttpClient);
    if (lRetVal < 0) {
        lRetVal = ERR_CDN_RESPONSE_STATUS;
        goto end;
    }

    if (lRetVal != 200) {
        lRetVal = ERR_CDN_NOT_200;
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
    while((id = HTTPCli_getResponseField(pHttpClient, (char *)recvBuf, sizeof(recvBuf), &moreFlags))
            != HTTPCli_FIELD_ID_END)
    {
        switch(id) {
            case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
                bodyLen = strtoul((char *)recvBuf, NULL, 0);
                break;

            case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
                break;

            // case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
            //     if(strncmp((const char *)recvBuf, "application/json",
            //             strlen("application/json"))) {
            //         #ifndef USING_FIRMWARE_UBISEN
            //         // must be a json
            //         lRetVal = ERR_OTA_VERSION_BODY_NOT_JSON;
            //         goto end;
            //         #endif
            //     }
            //     break;

            default:
                lRetVal = ERR_CDN_FILTER_ID;
                goto end;
        }
    }

    /* Read response data/body */
    /* Note:
            moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
            data is available Or in other words content length > length of buffer.
            The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
            Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
            for more information
     */


    int32_t bytesRead = 0;
    int32_t totalBytes = 0;
    moreFlags = 1;
    
    //
    // need many loop to received full file
    //
    while (bytesRead >= 0 && moreFlags == 1 && totalBytes < bodyLen) {
        bytesRead = HTTPCli_readResponseBody(pHttpClient, (char *)recvBuf, sizeof(recvBuf), &moreFlags);
        // save to flash
        if (bytesRead > 0) {
             if (writeCb(totalBytes, recvBuf, bytesRead, bodyLen) < 0) {
                 lRetVal = ERR_FLASH_WRITE;
                 goto end;
             }
            totalBytes += bytesRead;
            //INFO("Received %d bytes, got %d/%d bytes\r\n", bytesRead, totalBytes, bodyLen);
        }
    }
    INFO("\r\Download length %d, bodyLen in header %d, equal and >60*1024\r\n",totalBytes,bodyLen);
    if ((totalBytes==bodyLen) && (bodyLen>60*1024)) // All images >60kB
        lRetVal = 0x6A7B8C9D;
    else
        lRetVal = -1;
    
end:
    return lRetVal;
}
