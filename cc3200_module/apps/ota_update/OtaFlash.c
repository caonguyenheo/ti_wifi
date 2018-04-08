#include <stdint.h>

#include "OtaClient.h"
#include "OtaUtil.h"
#include "Ota.h"

#include "flc.h"

#ifdef TARGET_IS_CC3200
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_gprcm.h"
#include "simplelink.h"
#include "bootmgr.h"
#endif


//*****************************************************************************
// Checks if the device is secure
// This function checks if the device is a secure device or not.
// \return Returns \b true if device is secure, \b false otherwise
//*****************************************************************************
static tBoolean IsSecureMCU()
{
    unsigned long ulChipId;

    ulChipId =(HWREG(GPRCM_BASE + GPRCM_O_GPRCM_EFUSE_READ_REG2) >> 16) & 0x1F;

    if((ulChipId != DEVICE_IS_CC3101RS) &&(ulChipId != DEVICE_IS_CC3101S))
    {
        //
        // Return non-Secure
        //
        return false;
    }

    //
    // Return secure
    //
    return true;
}

int32_t OtaGetNextImageAddress(imageInfo_t *pImageInfo)
{
    int32_t iRetVal = -1;
    int32_t lFileHandle;
    uint32_t ulBootInfoToken;
    sBootInfo_t sBootInfo;

    iRetVal = sl_FsOpen((unsigned char *)IMG_BOOT_INFO,
                    FS_MODE_OPEN_READ,
                    &ulBootInfoToken,
                    &lFileHandle);
    // If we fail to open IMG_BOOT_INFO, it should be a serious error
    // since the appplication booloader should create this file when booting
    if (iRetVal >= 0) {
        iRetVal = sl_FsRead(lFileHandle,0,
                            (unsigned char *)&sBootInfo,
                            sizeof(sBootInfo_t));

        // UART_PRINT("ucActiveImg = %d\n", sBootInfo.ucActiveImg);
        // UART_PRINT("ulImgStatus = %lu\n", sBootInfo.ulImgStatus);
    }
    else {
        // UART_PRINT("Error when reading IMG_BOOT_INFO: %s\n", IMG_BOOT_INFO);
        return ERR_ACCESS_IMG_BOOT_INFO;
    }
    
    iRetVal = sl_FsClose(lFileHandle, NULL, NULL, 0);
    if (iRetVal < 0)
        return ERR_CLOSE_IMG_BOOT_INFO;

    int32_t newImageIndex;

    // Assume sBootInfo is already filled in init time (by sl_extlib_FlcCommit)
    // if USER1 --> USER2
    //    USER2 --> USER1
    //    FACTORY_DEFAULT --> USER1
    //
    switch(sBootInfo.ucActiveImg)
    {
        case IMG_ACT_USER1:
            newImageIndex = IMG_ACT_USER2;
            break;

        case IMG_ACT_USER2:
        default:
            newImageIndex = IMG_ACT_USER1;
            break;
    }

    // well, we don't intend to check the new image
    // todo: check image, then commit new image?
    pImageInfo->status = IMG_STATUS_NOTEST;

    pImageInfo->image = newImageIndex;

    // get the name, should be IMG_USER_1/2
    if (pImageInfo->image == IMG_ACT_USER1)
      strncpy(pImageInfo->name, IMG_USER_1, sizeof(pImageInfo->name));
    else if (pImageInfo->image == IMG_ACT_USER2)
      strncpy(pImageInfo->name, IMG_USER_2, sizeof(pImageInfo->name));
    else
      return ERR_INVALID_IMAGE_NAME;

    return 0;
}

int32_t OtaSaveNextImageAddress(imageInfo_t *pImageInfo)
{
    int32_t iRetVal = -1;
    int32_t lFileHandle;
    uint32_t ulBootInfoToken;

    iRetVal = sl_FsOpen((unsigned char *)IMG_BOOT_INFO,
                    FS_MODE_OPEN_WRITE,
                    &ulBootInfoToken,
                    &lFileHandle);
    // If we fail to open IMG_BOOT_INFO, it should be a serious error
    // since the appplication booloader should create this file when booting
    if (iRetVal < 0) {
        // UART_PRINT("Error when reading IMG_BOOT_INFO: %s\n", IMG_BOOT_INFO);
        return ERR_ACCESS_IMG_BOOT_INFO;
    }

    sBootInfo_t sBootInfo = {
        .ucActiveImg = pImageInfo->image,
        .ulImgStatus = pImageInfo->status
    };

    iRetVal = sl_FsWrite(lFileHandle, 0,
                        (unsigned char *)&sBootInfo,
                        sizeof(sBootInfo_t));

    if (iRetVal < 0)
        return ERR_WRITE_IMG_BOOT_INFO;

    iRetVal = sl_FsClose(lFileHandle, NULL, NULL, 0);
    if (iRetVal < 0)
        return ERR_CLOSE_IMG_BOOT_INFO;

    return 0;
}

static int32_t fileHandle = -1;
static uint32_t Token = 0;

#define MAX_OTA_IMAGE_SIZE (150*1024)
int32_t OtaFlashInit(char *pFileName)
{
    int32_t lRetVal = 0;
    uint32_t ulBootInfoCreateFlag;

    //
    // Initialize boot info file create flag
    //
    ulBootInfoCreateFlag  = _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE;

    //
    // Check if its a secure MCU
    //
    if (IsSecureMCU()) {
        ulBootInfoCreateFlag  = _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_OPEN_FLAG_SECURE|
                                _FS_FILE_OPEN_FLAG_NO_SIGNATURE_TEST|
                                _FS_FILE_PUBLIC_WRITE|_FS_FILE_OPEN_FLAG_VENDOR;
    }

    // Open file to save the downloaded file
    lRetVal = sl_FsOpen((uint8_t *)pFileName, FS_MODE_OPEN_WRITE, &Token, &fileHandle);
    if(lRetVal < 0) {
        // File Doesn't exit create new
        UART_PRINT("File %s doesn't exit, create\r\n", pFileName);
        lRetVal = sl_FsOpen((unsigned char *)pFileName,
                           FS_MODE_OPEN_CREATE(MAX_OTA_IMAGE_SIZE,  ulBootInfoCreateFlag),
                           &Token, &fileHandle);
    }

    if (lRetVal < 0)
        lRetVal = ERR_FLASH_OPEN;

    return lRetVal;
}

int32_t OtaFlashWrite(uint32_t bytesReceived, char *buff, uint32_t len, uint32_t l_total_length)
{
	UART_PRINT("|CC @%d len%d|", bytesReceived, len);
    int32_t lRetVal = sl_FsWrite(fileHandle, bytesReceived, (unsigned char *)buff, len);

    if (lRetVal < 0)
        return lRetVal;

    if ((uint32_t)lRetVal < len) {
        // UART_PRINT("Failed during writing the file, Error-code: %d\r\n", FILE_WRITE_ERROR);
        return -1;
    }
    return 0;
}

int32_t OtaFlashClose(int8_t save)
{
    int32_t lRetVal = -1;

    if (save) {
        lRetVal = sl_FsClose(fileHandle, 0, 0, 0);
    }
    else {
        lRetVal = sl_FsClose(fileHandle, 0, (unsigned char*) "A", 1);
    }
    
    return 0;
}
