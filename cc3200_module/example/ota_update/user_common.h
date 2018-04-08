#ifndef __FREERTOS_COMMON_H__
#define __FREERTOS_COMMON_H__

#undef SSID_NAME
#undef SECURITY_TYPE
#undef SECURITY_KEY
#undef SSID_LEN_MAX
#undef BSSID_LEN_MAX

#define SSID_NAME           "myCin-QA12"    /* AP SSID */
// #define SSID_NAME           "NXBroadbandV"    /* AP SSID */
#define SECURITY_TYPE       SL_SEC_TYPE_WPA /* Security type (OPEN or WEP or WPA*/
#define SECURITY_KEY        "13579135"              /* Password of the secured AP */
// #define SECURITY_KEY        "13579135"              /* Password of the secured AP */

#define SSID_LEN_MAX        32
#define BSSID_LEN_MAX       32

void BoardInit(void);
void DisplayBanner(char * AppName);

#endif
