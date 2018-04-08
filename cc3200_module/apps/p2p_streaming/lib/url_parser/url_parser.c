#include <string.h>
#include "common.h"
#ifndef NOTERM
#include "uart_if.h" 
#endif
#include "url_parser.h"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
// TODO: If add new command, add here and define in url_parser.h (enum eCommandID)
// must sync with each other
char *CMD_STRING[MAX_COMMAND_ID] = {
	"get_session_key",
	"set_broadcast_stream_url",
	"get_broadcast_stream_url",
	"start_broadcast_stream",
	"stop_broadcast_stream",
	"get_version",
	"get_spk_volume",
	/*************************/
	"get_mac_address",
	"get_udid",
	"check_fw_upgrade",
//	"request_fw_upgrade",
	"restart_system",
	"get_rt_list",
	"set_server_auth",
	"setup_wireless_save",
	"get_wifi_connection_state",
	"url_set",
	"url_get",
	"set_flicker",
	"set_city_timezone",
	"set_flipup",
	"set_bitrate",
	"set_framerate",
	"set_resolution",
	"p2p_file_xfer",
	"get_wifi_strength",
//	"get_spkmic_volume",
	"set_spkmic_volume",
	"play_prompt",
//	"get_framerate",
//	"get_resolution",
//	"get_bitrate",
	"set_gop",
//	"get_gop",
	"value_flipup",
//	"get_bitratelimitopt",
	"set_bitratelimitopt",
	"close_p2p_rtsp_stun",
//	"get_flicker",
//	"get_agc",
	"set_agc",
	"ftest",
	"set_callwait",
//	"get_callwait",
	"get_caminfo",
	"set_pwm",
};

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//*****************************************************************************
static int requestParser(char *req, url_token *parsed_req, int *num_parsed_req, int max_token);
static char *endString(char *str);

void unit_test(void) {
	int  i = 0, j = 0;
	int  iRetCode = 0;
	int  strLen;
	char charBuff[512];
	char* argv[4] = {
		"mode=combine&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627&==&&234=+&&&=&=",
		"2app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=combine&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627&",
		"&&==2app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=combine&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627",
		"2app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=combine&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627"
	};
	// char * argv = "2app_topic_sub=/android-app/paho34727881002438/sub&time=1463054694586&action=command&command=get_session_key&mode=combine&port1=42627&ip=115.79.62.15&streamname=CC3A61D7DEC6_42627";
	url_parser parsed_url;
	for (j = 0; j < 4; j++) {
		// UART_PRINT("SIZEOF=%d\n", sizeof(url_parser));
		memset((void *)&parsed_url, 0x00, sizeof(url_parser));
		UART_PRINT("STRING: %s\n", argv[j]);
		iRetCode = parse_url(argv[j], &parsed_url);
		if (iRetCode != ERROR_NONE) {
			UART_PRINT("FAILED \n");
			continue;
			// return;
		}
		/////////////////////// TEST ONLY //////////////////////////////////////////
		UART_PRINT("DkS_____________________________\r\n");
		for (i = 0; i < parsed_url.num_token; i++) {
			strLen = parsed_url.parsed_token[i].key.pTail - parsed_url.parsed_token[i].key.pHead;
			memcpy(charBuff, parsed_url.parsed_token[i].key.pHead, strLen);
			charBuff[strLen] = '\0';
			UART_PRINT("DEBUG_REQUEST_PARSER  key: %s\r\n", charBuff);

			strLen = parsed_url.parsed_token[i].value.pTail - parsed_url.parsed_token[i].value.pHead;
			memcpy(charBuff, parsed_url.parsed_token[i].value.pHead, strLen);
			charBuff[strLen] = '\0';
			UART_PRINT("DEBUG_REQUEST_PARSER  value: %s\r\n", charBuff);
			UART_PRINT("DkS_____________________________\r\n");
		}
		////////////////////////////////////////////////////////////////////////////
	}
}

//*****************************************************************************
//                      FUNCTION IMPLEMENT
//*****************************************************************************
int parse_url(char *req, url_parser *parsed_url) {
	int num_parsed_req = 0;
	int ret_val;
	if (req == NULL || parsed_url == NULL) {
		ret_val = -1;
		goto exit;
	}
	ret_val = requestParser(req, parsed_url->parsed_token, &num_parsed_req, MAX_TOKEN_NUM);
	if (ret_val < 0) {
		goto exit;
	}
	parsed_url->num_token = num_parsed_req;
	parsed_url->pChUrl = req;

exit:
	return ret_val;
}

static int requestParser(char *req, url_token *parsed_req, int *num_parsed_req, int max_token) {
	char *pChReq = NULL;
	char *pEqualChar = NULL;
	char *pTokenChar = NULL;

	char *pChStart = NULL;
	char *pChStop = NULL;

	int ret_val = 0;

//	UART_PRINT("DEBUG_REQUEST_PARSER  requestParser\r\n");

	/* Check input */
	if (req == NULL || parsed_req == NULL) {
		ret_val = -1;
		goto exit;
	}
//	UART_PRINT("DEBUG_REQUEST_PARSER  requestParser passed input\r\n");

	/* Parser */
	pChReq = req;
	*num_parsed_req = 0;
	while (pChReq != NULL && pChReq != '\0' && *num_parsed_req < max_token) {

//		UART_PRINT("DEBUG_REQUEST_PARSER  requestParser parse num=%d\r\n", *num_parsed_req);
		// Find Token Char
		pTokenChar = strstr(pChReq, PARAMS_SPLITTER);

		// Cannot find "&" char check if there is exist "=" char
		if (pTokenChar == NULL) {
			pEqualChar = strstr(pChReq, PARAMS_EQUAL);
			// Cannot find "=" char
			if (pEqualChar == NULL) {
				UART_PRINT("DEBUG_REQUEST_PARSER  requestParser failed\r\n");
				ret_val = -1;
				goto exit;
			}
			// UART_PRINT("DEBUG_REQUEST_PARSER  requestParser the last one\r\n");
			// Key from begin to equal char
			parsed_req[*num_parsed_req].key.pHead = pChReq;
			parsed_req[*num_parsed_req].key.pTail = pEqualChar;
			// Value the rest of string
			parsed_req[*num_parsed_req].value.pHead = pEqualChar + 1;
			parsed_req[*num_parsed_req].value.pTail = (char *)endString(pEqualChar);
			(*num_parsed_req)++;
			goto exit;
		}

		pEqualChar = strstr(pChReq, PARAMS_EQUAL);

		// This is an invalid case, cannot find any "=" char
		if (pEqualChar == NULL) {
			UART_PRINT("DEBUG_REQUEST_PARSER  requestParser cannot find = case\r\n");
			ret_val = -1;
			goto exit;
		}
		// This is an invalid case, current token lack of "=" char, continue with next token
		if (pEqualChar > pTokenChar) {
			UART_PRINT("DEBUG_REQUEST_PARSER  requestParser find = but invalid case\r\n");
			goto next_token;
		}

//		UART_PRINT("DEBUG_REQUEST_PARSER  requestParser normal case\r\n");

		// Key from begin to equal char
		parsed_req[*num_parsed_req].key.pHead = pChReq;
		parsed_req[*num_parsed_req].key.pTail = pEqualChar;
		// Value from equal char +1 to token char
		parsed_req[*num_parsed_req].value.pHead = pEqualChar + 1;
		parsed_req[*num_parsed_req].value.pTail = pTokenChar;
		// Increase number of parsed token
		(*num_parsed_req)++;

next_token:
		pChReq = pTokenChar + 1;
	}

exit:
	return 0;
}

static char *endString(char *str) {
	char * pStr = NULL;
	pStr = str;
	while (*pStr != '\0') {
		pStr++;
	}
	return pStr;
}

int getValueFromKey(const url_parser *parsed_url, const char *inKey, char *outVal, int buf_sz) {
	int i;
	int strLen;
	int valueLen;
	int numToken;
	const url_token *inToken;

	inToken = parsed_url->parsed_token;
	numToken = parsed_url->num_token;
	for (i = 0; i < numToken; i++) {
		strLen = inToken[i].key.pTail - inToken[i].key.pHead;
		if (strLen == strlen(inKey)) {
			if (memcmp(inToken[i].key.pHead, inKey, strLen) == 0) {
				valueLen = inToken[i].value.pTail - inToken[i].value.pHead;
				if (valueLen > buf_sz) {
					return ERROR_OUT_OF_BUFFER;
				}
				memcpy(outVal, inToken[i].value.pHead, valueLen);
				outVal[inToken[i].value.pTail - inToken[i].value.pHead] = '\0';
				return ERROR_NONE;
			}
		}
	}

	return ERROR_UNKNOWN;
}

int command_string_2_token(char *str_command) {
	int i;
	int cmd_len = 0;

	cmd_len = strlen(str_command);
	for (i = 0; i < MAX_COMMAND_ID; i++) {
		if (cmd_len == strlen(CMD_STRING[i])) {
			if (strncmp(str_command, CMD_STRING[i], cmd_len) == 0) {
				return i;
			}
		}
	}
	return -1;
}
