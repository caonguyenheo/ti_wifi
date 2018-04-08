#ifndef __MYCIN_API_H__
#define __MYCIN_API_H__

#include <stdint.h>

#define MAX_REGISTRATION_ID  27
#define MAX_MQTT_TOPIC_CODE  27
#define MAX_USER_TOKEN       (64+4)
#define MAX_UDID             27
#define MAX_AUTHEN_TOKEN	 MAX_USER_TOKEN

#define MAX_OTA_URL_STR     100
#define MAX_OTA_PAYLOAD_REQ 300
// DkS
#define MAX_OTA_PAYLOAD_RCV 1200

#define MYCIN_API_HOST_PORT           443

#define MAX_FW_CHECKSUM                (64+4)
#define MAX_FW_FILES                   128
#define MAX_FW_VERSION                 (8+4)
#define MAX_FW_ID                      4
#define MAX_G_FW_ID                    8
typedef struct {
    char *mqtt_clientid;
    char *mqtt_user;
    char *mqtt_pass;
    char *mqtt_topic;
    char *mqtt_servertopic;
} myCinApiRegister_t;

typedef struct {
    char user_token[MAX_USER_TOKEN];
    char device_udid[MAX_UDID];
    char authen_token[MAX_AUTHEN_TOKEN];
} myCinUserInformation_t;


typedef struct {
    char fw_id[MAX_FW_ID];
    char fw_version[MAX_FW_VERSION];
    char fw_files[MAX_FW_FILES];
    char fw_checksum[MAX_FW_CHECKSUM];
} firmware_t;

typedef struct {
    firmware_t ota_firmware[2];
} ota_firmware_t;

int8_t myCinRegister(myCinUserInformation_t *pmyCinUser, myCinApiRegister_t *pmyCinRegister);
int8_t myCinOtaParse(const char *json, uint32_t json_len, ota_firmware_t *l_ota_firmware);
#endif
