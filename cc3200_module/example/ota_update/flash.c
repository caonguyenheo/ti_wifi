
#include <stdbool.h>
#include <stdint.h>

#include "simplelink.h"         /* file system */
#include "flash.h"

#define MAX_FILE_SIZE       (40*1024)

int32_t fileRemove(char *fileName)
{
    uint32_t ulToken = 0;
    if (fileIsExist(fileName)) {
        return sl_FsDel((uint8_t*) fileName, ulToken);
    }
    else
        return 0;
}

bool fileIsExist(char *fileName)
{
    SlFsFileInfo_t fsFileInfo;
    uint32_t ulToken = 0;
    int32_t lReturn;
    lReturn = sl_FsGetInfo((uint8_t*)fileName, ulToken, &fsFileInfo);

    if (lReturn < 0)
        return FALSE;
    else
        return TRUE;
}

int32_t fileCreate(char *fileName)
{
    uint32_t ulToken;
    int32_t lFileHandler;
    int32_t lReturn;

    uint32_t ulBootInfoCreateFlag = _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE;

    lReturn = sl_FsOpen((uint8_t *)fileName, FS_MODE_OPEN_WRITE, &ulToken, &lFileHandler);
    if (lReturn < 0) {
        lReturn = sl_FsOpen((uint8_t *)fileName,
                           FS_MODE_OPEN_CREATE(MAX_FILE_SIZE,  ulBootInfoCreateFlag),
                           &ulToken, &lFileHandler);
    }
    lReturn = sl_FsClose(lFileHandler, 0, 0, 0);

    return (lReturn <0)? -1:0;
}

int32_t fileWrite(char *fileName, char *message, uint32_t len)
{
    int32_t lReturn = -1;
    int32_t lFileHandler;
    uint32_t ulBootInfoToken;
    int32_t lWroteBytes;

    lReturn = sl_FsOpen((uint8_t *)fileName,
                    FS_MODE_OPEN_WRITE,
                    &ulBootInfoToken,
                    &lFileHandler);

    if (lReturn < 0)
        return lReturn;

    lReturn = sl_FsWrite(lFileHandler,
                    0,
                    (uint8_t *)message,
                    len);

    if (lReturn < 0) {
        lReturn = sl_FsClose(lFileHandler, 0, 0, 0);
        return lReturn;
    }

    lWroteBytes = lReturn;
    
    // close and save
    lReturn = sl_FsClose(lFileHandler, 0, 0, 0);

    if (lReturn < 0)
        return lReturn;

    return lWroteBytes;
}

int32_t fileRead(char *fileName, char *message, uint32_t len)
{
    int32_t lReturn = -1;
    int32_t lFileHandler;
    uint32_t ulBootInfoToken;
    int32_t lReadBytes;

    lReturn = sl_FsOpen((uint8_t *)fileName,
                    FS_MODE_OPEN_READ,
                    &ulBootInfoToken,
                    &lFileHandler);

    if (lReturn < 0) {
        sl_FsClose(lFileHandler, 0, 0, 0);
        return lReturn;
    }

    lReturn = sl_FsRead(lFileHandler,
                    0,
                    &message[0],
                    len);

    if (lReturn < 0) {
        sl_FsClose(lFileHandler, 0, 0, 0);
        return lReturn;
    }

    lReadBytes = lReturn;
    
    lReturn = sl_FsClose(lFileHandler, 0, 0, 0);

    return lReadBytes;
}
