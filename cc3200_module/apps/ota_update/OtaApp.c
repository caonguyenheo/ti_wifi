#include <stdint.h>
#include <stdbool.h>

#include "string.h"
#include "OtaClient.h"
#include "OtaUtil.h"
#include "Ota.h"
#include "OtaFlash.h"
#include "CdnClient.h"
#include "cc3200_system.h"
#include "server_api.h"
#include "system.h"

static bool isRunning = false;
extern ota_firmware_t g_ota_firmware;

#define CHECK_ON_ERROR(error_code)\
{\
    if(error_code < 0) \
    {\
        ERR_PRINT(error_code);\
        goto end;\
    }\
}
void system_reboot(void);
int32_t StartOta(otaInfo_t *otaInfo)
{
    int32_t lReturn = -1;
    int ak_status=0;
    // if (otaInfo->interval > 0) {
    //     //todo
    //     DEBUG("OtaApp: Not support interval yet\n");
    //     return 0;
    // }
    if (isRunning == true)
        return ERR_OTA_IS_RUNNING;

    isRunning = true;

    // check new version
    // cdnInfo contains all information to download new version
    cdnInfo_t cdnInfo;
    lReturn = OtaVersion(otaInfo, &cdnInfo, 0, 0);
    if (lReturn<0)
        goto end;
    lReturn = OtaVersion(otaInfo, &cdnInfo, 2, 4);
    if (lReturn<0)
        goto end;
	ak_power_up();
	INFO("\r\nAK Current version %s vs online version %s\r\n", ak_version, (g_ota_firmware.ota_firmware[0]).fw_version);

	if ((atoi(ak_version) < atoi((g_ota_firmware.ota_firmware[0]).fw_version)) && (atoi(ak_version)!=0)) {
		if (AkGetImage(&cdnInfo)!=0){
			system_reboot();
		}
		ak_power_down();
		fw_version_format(ak_version_format,(g_ota_firmware.ota_firmware[0]).fw_version);
	} else {
		ak_power_down();
		if (atoi(ak_version)==0)
			return -1;
	}
	// Reach this line if AK has been upgraded successfully or AK already latest
    INFO("\r\nCC Current version %s vs online version %s\r\n", SYSTEM_VERSION, cdnInfo.version);

    if (atoi(cdnInfo.version) <= atoi(SYSTEM_VERSION)) {
        INFO("OtaApp: no new OTA available\r\n");
        // Bootup query
        lReturn = OtaVersion(otaInfo, &cdnInfo, 1, 0);
        lReturn |= OtaVersion(otaInfo, &cdnInfo, 2, 2);
        lReturn |= OtaVersion(otaInfo, &cdnInfo, 2, 1);
        if (lReturn==0){
            cam_settings[8]=UPGRADE_DEFAULT;
	        cam_settings_update();
        }
        return lReturn;
    }

    // ok, get the file name to save
    // if we have error in getting the next image address
    // it should be a serious error
    imageInfo_t imageInfo;
    lReturn = OtaGetNextImageAddress(&imageInfo);
    CHECK_ON_ERROR(lReturn);

    INFO("OTA: Will save to %s\r\n", imageInfo.name);

    // get new image, save them to next image
    lReturn = CdnGetImage(&cdnInfo, &imageInfo);
    if (lReturn!=0x6A7B8C9D) { // If something wrong during getting the image, don't update path to new image
        INFO("Fails to get complete image->reboot\r\n");
        osi_Sleep(1000);
        system_reboot();
    }
    // check the file with MD5. Sorry, no more Code Space
    // todo

    // commit file
    lReturn = OtaSaveNextImageAddress(&imageInfo);
    if (lReturn<0)
        goto end;

    fw_version_format(g_version_8char,cdnInfo.version);
    
    lReturn = OtaVersion(otaInfo, &cdnInfo, 1, 0);
    lReturn |= OtaVersion(otaInfo, &cdnInfo, 2, 2);
    lReturn |= OtaVersion(otaInfo, &cdnInfo, 2, 1);
    if (lReturn==0){
        cam_settings[8]=UPGRADE_DEFAULT;
        cam_settings_update();
    }

    for(lReturn=0;lReturn<10;lReturn++)
        INFO("We did get new image->reboot.\r\n");
    osi_Sleep(1000);
    system_reboot();
end:
    isRunning = false;
    return lReturn;
}
