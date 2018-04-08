#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define USING_FIRMWARE_UBISEN

#ifdef USING_FIRMWARE_UBISEN
#define CINATIC_OTA_API_KEY         "api-key"
#define CINATIC_OTA_API_KEY_VALUE   "7fd171732e9838fb1fb8e00b2326df546b690f014674aaa19e798892d5c55bda"
#define CINATIC_GET_REQUEST_URI 	"/api/%s/versions/image1"

#define OTA_SERVER_DOMAIN           "192.168.1.69"
#define OTA_SERVER_PORT             3000
#define OTA_INTERVAL                5           //sec
#endif

#endif