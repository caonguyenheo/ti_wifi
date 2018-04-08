#include "stdint.h"

#include "OtaUtil.h"
#include "Ota.h"
#include "flc.h"

#include "simplelink.h"
#include "hw_types.h"

uint8_t start_mycin_ota(void)
{
    // uint32_t curVersion;
    // if (convert_version(VERSION, strlen(VERSION), &curVersion) < 0)
    //     LOOP_FOREVER();

    UART_PRINT("\r\nCurrent version %s\r\n", VERSION);
    otaInfo_t otaInfo;
    strncpy(otaInfo.version, VERSION, sizeof(otaInfo.version));
    otaInfo.secure = false;
    otaInfo.port = OTA_SERVER_PORT;       // use default
    otaInfo.interval = OTA_INTERVAL;   // onetime
    strncpy(otaInfo.address, OTA_SERVER_DOMAIN, sizeof(otaInfo.address));

    StartOta(&otaInfo);

    return 0;
}
