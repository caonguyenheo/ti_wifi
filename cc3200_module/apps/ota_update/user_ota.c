#include "stdint.h"

#include "FreeRTOS.h"
#include "task.h"

#include "OtaUtil.h"
#include "Ota.h"
#include "flc.h"
/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "simplelink.h"
#include "hw_types.h"

#include "network_if.h"
#include "cc3200_system.h"
#include "system.h"

extern OsiMsgQ_t g_tConnectionFlag;
int ota_done = 0;
extern char cam_settings[];
void start_mycin_ota(void *pvParameters)
{
    int l_head=0, l_tail=0;
    // SYNC with network task
    unsigned char ucSyncMsg;
    osi_MsgQRead(&g_tConnectionFlag, &ucSyncMsg, OSI_WAIT_FOREVER);
    osi_MsgQWrite(&g_tConnectionFlag, &ucSyncMsg, OSI_NO_WAIT);
    
    UART_PRINT("\r\nCurrent version %s\r\n", SYSTEM_VERSION);
    otaInfo_t otaInfo;
    strncpy(otaInfo.version, SYSTEM_VERSION, sizeof(otaInfo.version));
    otaInfo.secure = true;
    otaInfo.port = OTA_SERVER_PORT;       // use default
    otaInfo.interval = OTA_INTERVAL;   // onetime
    strncpy(otaInfo.address, OTA_SERVER_DOMAIN, sizeof(otaInfo.address));

	l_head = cam_settings[8]&0xF0;
	l_tail = cam_settings[8]&0x0F;
	if (l_head==UPGRADE_CMDRCV_HEAD){
		if (l_tail<5) { // Upgrade 5 times
			l_tail++;
			cam_settings[8]=(l_head|l_tail);
			cam_settings_update();
			StartOta(&otaInfo);
		} else {
			cam_settings[8]=UPGRADE_DEFAULT;
			cam_settings_update();
		}
	}
	UART_PRINT("\r\nOTA%d\r\n",cam_settings[8]);
	ota_done = 1;
	vTaskDelete( NULL );
}
