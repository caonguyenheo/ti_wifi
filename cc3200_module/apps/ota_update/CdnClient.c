#include "Ota.h"
#include "OtaUtil.h"
#include "CdnClient.h"
#include "OtaHttpClient.h"
#include "server_api.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>
/*
 * this function download the file in pCdnInfo,
 * save it to a filename declare in pImageInfo
 */
int32_t CdnGetImage(cdnInfo_t *pCdnInfo, imageInfo_t *pImageInfo) {
    int8_t save=false;

    // check cdnInfo and imageInfo
    // todo

    HTTPCli_Struct httpClient;
    // connect to ota server
    // get port
    uint16_t port = 0;
    uint32_t secure = 0;
    if (strncmp(pCdnInfo->protocol, PROTOCOL_HTTPS, strlen(pCdnInfo->protocol)) == 0) {
        secure = 1;
    }

    int32_t lReturn = -1;

    lReturn = OtaFlashInit(pImageInfo->name);
    if (lReturn < 0){
	    UART_PRINT("OtaFlashInit fails\r\n");
        goto close_file;
	}
    lReturn = myCinHttpConnect(&httpClient, pCdnInfo->host, port, secure);
    if (lReturn < 0){
	    UART_PRINT("myCinHttpConnect fails\r\n");
        goto close;
    }

    // sclose request
    lReturn = OtaHttpSendRequest(&httpClient, pCdnInfo->host, pCdnInfo->url, HTTPCli_METHOD_GET);
    if (lReturn < 0){
	    UART_PRINT("OtaHttpSendRequest fails\r\n");
        goto close;
    }

    // get file, save to flash
    lReturn = OtaHttpCdnGetFile(&httpClient, OtaFlashWrite);
    if (lReturn != 0x6A7B8C9D){
	    UART_PRINT("OtaHttpCdnGetFile fails\r\n");
    }

close:
    // finally, close connection
    myCinHttpClose(&httpClient);

close_file:
    // and close flash
    save = (lReturn != 0x6A7B8C9D)? false: true;
    OtaFlashClose(save);
    return lReturn;
}
extern int32_t AKOTA_SPIWrite(uint32_t bytesReceived, char *buff, uint32_t len, uint32_t l_total_length);
extern ota_firmware_t g_ota_firmware;
int32_t AkGetImage(cdnInfo_t *pCdnInfo){
    HTTPCli_Struct httpClient;
    // connect to ota server
    // get port
    uint16_t port = 0;
    uint32_t secure = 0;
    if (strncmp(pCdnInfo->protocol, PROTOCOL_HTTPS, strlen(pCdnInfo->protocol)) == 0) {
        secure = 1;
    }

    int32_t lReturn = -1;
    UART_PRINT("pCdnInfo->host_ak %s\r\n", pCdnInfo->host_ak);
    UART_PRINT("pCdnInfo->url_ak %s\r\n", pCdnInfo->url_ak);
    lReturn = myCinHttpConnect(&httpClient, pCdnInfo->host_ak, port, secure);
    if (lReturn < 0)
        goto close;

    // sclose request
    lReturn = OtaHttpSendRequest(&httpClient, pCdnInfo->host_ak, pCdnInfo->url_ak, HTTPCli_METHOD_GET);
    if (lReturn < 0)
        goto close;

    // get file, save to flash
    lReturn = OtaHttpCdnGetFile(&httpClient, AKOTA_SPIWrite);
    if (lReturn < 0){
	    UART_PRINT("Get file error\r\n");
        goto close;
    }
    if (AKOTA_SPIWrite_MD5((g_ota_firmware.ota_firmware[0]).fw_checksum)==0)
        lReturn = 0;
    else
        lReturn = -1;

close:
    // finally, close connection
    myCinHttpClose(&httpClient);
  
    return lReturn;
}
