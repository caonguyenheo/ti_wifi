#include "Ota.h"
#include "OtaClient.h"
#include "OtaUtil.h"
#include "OtaFlash.h"
#include "CdnClient.h"
#include "OtaHttpClient.h"

// HTTP Client lib
#include <http/client/httpcli.h>
#include <http/client/common.h>
#include "server_api.h"
#include "server_util.h"
#include "cc3200_system.h"
#include "userconfig.h"

extern myCinUserInformation_t g_mycinUser;
ota_firmware_t g_ota_firmware;
char g_fw_id[MAX_G_FW_ID]={0};
/**
  * @brief  search_str
  *
  * @@param (*str_input) = input, (*str_host) = output, (*str_url) = output
  *
  * @return success : 0, fail : -1 when not found "//", fail : -2 when not found "/"
 */
int search_str(char *str_input, char *str_host, char *str_url)
{
	char *str_host_temp, *str_url_temp;
	int length = 0;

	str_host_temp = strstr(str_input, "//");
	if(str_host_temp != NULL)
	{
		str_url_temp = strstr(str_host_temp + 2, "/");
		if(str_url_temp != NULL)
		{
			/****************taken string for str_url***********/
			strncpy(str_url, str_url_temp, strlen(str_url_temp));

			/***************taken string for str_host***********/
			length = str_url_temp - (str_host_temp + 2);// length = 15
			strncpy(str_host, str_host_temp + 2, length);
		}
		else
		{
			return -2;
		}
	}
	else
	{
		return -1;
	}

	return 0;
}
/*
 * input: ota infor
 * return cdn infor
 */
extern char MYCIN_API_HOST_NAME[];
/*l_cmd=0: Get OTA version
       =1: Update Cam info
       =2: Update OTA status*/
int8_t OtaVersion(otaInfo_t *pOtaInfo, cdnInfo_t *pCdnInfo, int l_cmd, int l_ota_status)
{
    HTTPCli_Struct httpClient;
    int  lRetVal = 0, length = 0, PORT = 0;
    char l_version[1024], PROTOCOL[7]= {0};
    char first_str[200]={0}, str_host_fl[48]={0}, str_url_fl[100]={0}, udid[MAX_UID_SIZE]={0};
    char ssl_secure=0;
    
	lRetVal = myCinHttpConnect(&httpClient, MYCIN_API_HOST_NAME, 443, 1);
	if (lRetVal < 0) {
		UART_PRINT("Fail to connect to port %d url:%s\r\n",443,MYCIN_API_HOST_NAME);
		goto close_socket;
	}
  
    get_uid(udid);
    lRetVal = userconfig_get(g_mycinUser.authen_token, MAX_AUTHEN_TOKEN, MASTERKEY_ID);
    if (lRetVal < 0)
    {
        UART_PRINT("Can not read authen_token\r\n");
        goto close_socket;
    }
    if (l_cmd==1){
		snprintf(first_str, sizeof(first_str),"/v1/devices/%s/update_info?device_token=%s", udid, g_mycinUser.authen_token);
		snprintf(l_version, sizeof(l_version),"{\"version\": \"%s\",\"local_ip\":\"%s\",\"soc1_version\":\"%s\"}",g_version_8char,g_local_ip,ak_version_format);
		UART_PRINT("\r\nRequest: %s%s %s\r\n", MYCIN_API_HOST_NAME, first_str, l_version);
		lRetVal = myCinHttpSendRequest(&httpClient, MYCIN_API_HOST_NAME, first_str, HTTPCli_METHOD_PUT, l_version);
	} else if (l_cmd==0) {
		snprintf(first_str, sizeof(first_str),"/v1/devices/%s/check_firmware_ext?device_token=%s", udid, g_mycinUser.authen_token);
		UART_PRINT("\r\nRequest: %s\r\n", first_str);
		lRetVal = OtaHttpSendRequest(&httpClient, MYCIN_API_HOST_NAME, first_str, HTTPCli_METHOD_GET);
	} else {
		snprintf(first_str, sizeof(first_str),"/v1/devices/%s/update_firmware?device_token=%s", udid, g_mycinUser.authen_token);
		snprintf(l_version, sizeof(l_version),"{\"fw_id\":%s,\"fw_status\":%d}", g_fw_id, l_ota_status);
		UART_PRINT("\r\nRequest: %s%s %s\r\n", MYCIN_API_HOST_NAME, first_str, l_version);
		lRetVal = myCinHttpSendRequest(&httpClient, MYCIN_API_HOST_NAME, first_str, HTTPCli_METHOD_PUT, l_version);
	}
	if (lRetVal < 0) {
		UART_PRINT("Fail to post to myCin [%d]\n", lRetVal);
		goto close_socket;
	}

	memset(l_version,0x00,sizeof(l_version));
	lRetVal = myCinHttpVersionGetResponse(&httpClient, l_version, sizeof(l_version));
	if (lRetVal < 0) {
		UART_PRINT("Fail to get response [%d]\n", lRetVal);
		goto close_socket;
	}  

    CHECK_RETURN(myCinHttpClose(&httpClient));
    UART_PRINT("\r\n Response : %s\r\n", l_version);
    if (l_cmd>=1){
	    if (strstr(l_version,"200")!=NULL)
    		return 0;
    	else
    		return -1;
	}
    lRetVal = myCinOtaParse(l_version, strlen(l_version), &g_ota_firmware);
    /*
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[0]).fw_id);
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[0]).fw_version);
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[0]).fw_files);
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[0]).fw_checksum);
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[1]).fw_id);
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[1]).fw_version);
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[1]).fw_files);
    UART_PRINT("%s\r\n", (g_ota_firmware.ota_firmware[1]).fw_checksum);
    */

    if (lRetVal != 8)
    {
    	UART_PRINT("Fail to parse return data, return %d\r\n",lRetVal);
    	lRetVal = -1;
    	goto close_socket;
    }
    
	if((g_ota_firmware.ota_firmware[1]).fw_files[4] == 's')
	{
		strcpy(PROTOCOL, PROTOCOL_HTTPS);
		PORT = 443;
		ssl_secure=1;
	}
	else
	{
		strcpy(PROTOCOL, PROTOCOL_HTTP);
		PORT = 80;
		ssl_secure=0;
	}

    lRetVal = search_str((g_ota_firmware.ota_firmware[1]).fw_files, str_host_fl, str_url_fl);
    if (lRetVal < 0){
		UART_PRINT("Fail to parse ota CC file %s\r\n",(g_ota_firmware.ota_firmware[1]).fw_files);
		goto close_socket;
	}

    strncpy(pCdnInfo->version, (g_ota_firmware.ota_firmware[1]).fw_version, sizeof(pCdnInfo->version));
    strncpy(pCdnInfo->protocol, PROTOCOL, sizeof(pCdnInfo->protocol));
    strncpy(pCdnInfo->host, str_host_fl, sizeof(pCdnInfo->host));
    strncpy(pCdnInfo->url, str_url_fl, sizeof(pCdnInfo->url));
    
    memset(str_host_fl,0,sizeof(str_host_fl));
    memset(str_url_fl,0,sizeof(str_url_fl));
    
    lRetVal = search_str((g_ota_firmware.ota_firmware[0]).fw_files, str_host_fl, str_url_fl);
    UART_PRINT("%s\r\n", str_host_fl);
    UART_PRINT("%s\r\n", str_url_fl);
    if (lRetVal < 0){
		UART_PRINT("Fail to parse ota AK file %s\r\n",(g_ota_firmware.ota_firmware[0]).fw_files);
		goto close_socket;
	}

    strncpy(pCdnInfo->host_ak, str_host_fl, sizeof(pCdnInfo->host_ak));
    strncpy(pCdnInfo->url_ak, str_url_fl, sizeof(pCdnInfo->url_ak));

close_socket:
    UART_PRINT("Close socket return %d func ret %d\r\n",myCinHttpClose(&httpClient), lRetVal);
    return lRetVal;
}
