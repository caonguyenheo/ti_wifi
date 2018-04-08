
#ifndef __OTAFLASH_H__
#define __OTAFLASH_H__

#include "Ota.h"

typedef struct {
  uint8_t image;
  uint32_t status;
  char name[MAX_IMAGE_NAME];
} imageInfo_t;

int32_t OtaGetNextImageAddress(imageInfo_t *);
int32_t OtaSaveNextImageAddress(imageInfo_t *);
// we have only one flash
int32_t OtaFlashInit(char *pFileName);
int32_t OtaFlashWrite(uint32_t bytesReceived, char *buff, uint32_t len, uint32_t l_total_length);
int32_t OtaFlashClose(int8_t save);

#endif
