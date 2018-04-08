
#ifndef bool
#define bool int
#define false 0
#endif

#ifndef NULL
#define NULL 0
#endif

enum{
	eCOMMAND_ID_GET_VERSION = 0,
	eCOMMAND_ID_GET_MAC  = 1,
	eCOMMAND_ID_GET_UDID = 2,
	eCOMMAND_ID_CHECK_FIRMWARE_UPGRADE = 3,
	eCOMMAND_ID_REQUEST_FIRMWARE_UPGRADE = 4,
	eCOMMAND_ID_RESTART_SYSTEM = 5,
	eCOMMAND_ID_GET_RT_LIST = 6,
	eCOMMAND_ID_SET_SERVER_AUTH = 7,
	eCOMMAND_ID_SETUP_WIRELESS_SAVE = 8,
	eCOMMAND_ID_GET_WIFI_CONNECTION_STATE = 9,
	eCOMMAND_ID_CNT = 10
}enumCOMMAND_ID;

typedef struct {
	bool has_command_id_parsed;
	int command_id_parsed;
	bool has_value_parsed;
	char value_parsed[65];
	bool has_level_parsed;
	char level_parsed[17];
	bool has_timezone_parsed;
	char timezone_parsed[17];
	bool has_setup_parsed;
	char setup_parsed[129];
} urlResponse_struct;

#define default_urlResponse_struct {{0}, {0}, false, -1, false, {0}, false, {0}, false, {0}, false, {0}}

extern int command_parser(char* input);
extern int command_parser_unittest(void);

#define WIFI_DEFAULT_ADHOC_CHANNEL 6

#define NETWORK_TYPE_UAP        0
#define NETWORK_TYPE_INFRA      1
typedef struct nxcNetwork_st{
	int NetworkType;
	unsigned char ESSID[36];//max 32
	int AuthenType;
	int Channel;
	unsigned char Key[64];//63 printable
	int KeyIndex; //1,2,3
	int IPMode;//DHCP, STATIC
	unsigned char Gateway[16];//192.168.2.1
	unsigned char SubnetMask[16];//255.255.255.255
	unsigned char DefaultIP[16];//192.168.2.1
}nxcNetwork_st;

extern urlResponse_struct urlResponse;
extern nxcNetwork_st wifiConfig_nonos;
