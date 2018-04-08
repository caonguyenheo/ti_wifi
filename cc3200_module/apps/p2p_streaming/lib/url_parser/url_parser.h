#ifndef __URL_PARSER_H__
#define __URL_PARSER_H__

// ERROR_CODE
#define      ERROR_NONE                 0
#define      ERROR_UNKNOWN              -1
#define      ERROR_OUT_OF_BUFFER        -2

#define      PARAMS_SPLITTER            "&"
#define      PARAMS_EQUAL               "="

#define MAX_TOKEN_NUM                   10
#define URL_BUFFER_SIZE                 256

typedef enum eCommandID {
	CMD_GET_SESSION_KEY = 0,
	CMD_SET_BROADCAST_URL,
	CMD_GET_BROADCAST_URL,
	CMD_START_BROADCAST_STREAM,
	CMD_STOP_BROADCAST_STREAM,
    CMD_GET_VERSION,
    CMD_GET_SPK_VOLUME,
    /*******************************/
    CMD_ID_GET_MAC,
    CMD_ID_GET_UDID,
    CMD_ID_CHECK_FIRMWARE_UPGRADE,
//    CMD_ID_REQUEST_FIRMWARE_UPGRADE,
    CMD_ID_RESTART_SYSTEM,
    CMD_ID_GET_RT_LIST,
    CMD_ID_SET_SERVER_AUTH,
    CMD_ID_SETUP_WIRELESS_SAVE,
    CMD_ID_GET_WIFI_CONNECTION_STATE,
    CMD_ID_URL_SET,
    CMD_ID_URL_GET,
    CMD_ID_SET_FLICKER,
    CMD_ID_SET_CITY_TIMEZONE,
    CMD_ID_SET_VIDEO_FLIP,
    CMD_ID_SET_BITRATE,
    CMD_ID_SET_FRAMERATE,
    CMD_ID_SET_RESOLUTION,
    CMD_ID_FILETRANSFER,
    CMD_ID_GET_WIFISTRENGTH,
//    CMD_ID_GET_VOLUME,
    CMD_ID_SET_VOLUME,
    CMD_ID_SET_PLAY_VOICE_PROMPT,
//    CMD_ID_GET_FRAMERATE,
//    CMD_ID_GET_RESOLUTION,
//    CMD_ID_GET_BITRATE,
    CMD_ID_SET_GOP,
//    CMD_ID_GET_GOP,
    CMD_ID_GET_FLIPUP,
//    CMD_ID_GET_BITRATELIMITOPT,
    CMD_ID_SET_BITRATELIMITOPT,
    CMD_STOP_SESSION,
//    CMD_ID_GET_FLICKER,
//    CMD_ID_GET_AGC,
    CMD_ID_SET_AGC,
    CMD_ID_FTEST,
    CMD_ID_SET_CALLWAIT,
//    CMD_ID_GET_CALLWAIT,
    CMD_ID_GET_CAMINFO,
    CMD_ID_SET_PWM,
	MAX_COMMAND_ID
} eCommandID;

typedef struct url_segment_t {
    char *pHead;
    char *pTail;
}url_segment;

typedef struct url_token_t {
    url_segment key;
    url_segment value;
}url_token;

typedef struct url_parser_t {
    int num_token;
    char *pChUrl;
    url_token parsed_token[MAX_TOKEN_NUM];
} url_parser;

int parse_url(char *req, url_parser *parsed_url);
int getValueFromKey(const url_parser *parsed_url, const char *inKey, char *outVal, int buf_sz);
int command_string_2_token(char *str_command);
void unit_test(void);
#endif // __URL_PARSER_H__
