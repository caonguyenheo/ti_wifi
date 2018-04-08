
void p2p_main(void *pvParameters);

typedef enum eMediaType
    {
    eMediaTypeNull = 0,
    eMediaTypeVideo,
    eMediaTypeAudio,
    eMediaTypeCommand,
    eMediaTypeFile,                 //No more supported
    eMediaTypeACK,
    eMediaTypeTalkback,
    eMediaTypeUserDefine,
    }eMediaType;

typedef enum eMediaSubType
    {
    eMediaSubTypeNULL               = eMediaTypeNull * 10,
    eMediaSubTypeVideoIFrame        = eMediaTypeVideo * 10,
    eMediaSubTypeVideoBFrame,
    eMediaSubTypeVideoPFrame,
    eMediaSubTypeVideoJPEG,
    eMediaSubTypeAudioAlaw          = eMediaTypeAudio * 10,
    eMediaSubTypeAudioPCM,
    eMediaSubTypeAudioADPCM,
    eMediaSubTypeAudioG722,
    eMediaSubTypeCommandRequest     = eMediaTypeCommand * 10,
    eMediaSubTypeCommandResponse,
    eMediaSubTypeCommandOpenStream,
    eMediaSubTypeCommandStopStream,
    eMediaSubTypeCommandAccessStream,
    eMediaSubTypeCommandCloseStream,
    eMediaSubTypeMediaACK           = eMediaTypeACK * 10,
    eMediaSubTypeMediaFileACK,
    eMediaSubTypeTalkback           = eMediaTypeTalkback * 10,
    eMediaSubTypeUserDefine         = eMediaTypeUserDefine * 10,
    }eMediaSubType;

typedef enum command_type
    {
    P2P_COMMAND = 0,
    MQTT_COMMAND,
    HTTP_COMMAND,
    }command_type;

#define P2P_KEY_CHAR_LEN        17
#define P2P_RN_CHAR_LEN         13
#define P2P_KEY_HEX_LEN         33
#define P2P_RN_HEX_LEN          25
#define P2P_PS_IP_LEN           20
#define P2P_PS_URL_LEN          40
#define P2P_PS_PORT_LEN         8

#define STR_IP_BUFFER_SZ        20
#define STR_PORT_BUFFER_SZ      20
#define MAX_STR_LEN             40
#define MAX_STR_PARAM_LEN       72


#define STR_RELAY               "relay"
#define STR_COMMAND             "command"
#define STR_REQ                 "req"
#define STR_MODE                "mode"
#define STR_REMOTE              "remote"
#define STR_COMBINE             "combine"
#define STR_LOCAL               "local"
#define STR_PORT_1              "port1"
#define STR_IP                  "ip"
#define STR_STREAM_NAME         "streamname"
#define STR_TIME                "time"


#define STR_CMD_VALUE           "value"
#define STR_CMD_URL     		"url_set"
#define STR_CMD_API     		"api_url"
#define STR_CMD_MQTT    		"mqtt_url"
#define STR_CMD_NTP     		"ntp_url"
#define STR_CMD_RMS     		"rms_url"
#define STR_CMD_STUN    		"stun_url"
#define STR_CMD_AUTH          	"auth"
#define STR_CMD_SSID            "ssid"
#define STR_CMD_KEY             "key"
#define STR_CMD_INDEX           "index"
#define STR_CMD_TIMEZONE        "timezone"

#define SIZE_FLIP        4
#define SIZE_BITRATE     8
#define SIZE_FRAMERATE   4
#define SIZE_FLICKER     4
#define SIZE_RESOLUTION  4
//#define SIZE_CAM_ALL     12
#define SIZE_VOICEPROMPT 4

int command_handler(char* input, int length, command_type commandtype, char* response, int* response_len);
int32_t get_ps_info(char *out_buf, uint32_t out_buf_sz, uint32_t *used_bytes);
int32_t mqtt_get_ps_info(char *out_buf, uint32_t out_buf_sz, uint32_t *used_bytes);
int32_t fast_ps_mode();
