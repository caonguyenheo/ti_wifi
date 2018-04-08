#ifndef __TEST_FLASH_H__
#define __TEST_FLASH_H__

#include <stdbool.h>
#include <stdint.h>

int32_t fileRemove(char *fileName);
int32_t fileCreate(char *fileName);
bool fileIsExist(char *fileName);
int32_t fileWrite(char *fileName, char *message, uint32_t len);
int32_t fileRead(char *fileName, char *message, uint32_t len);

#endif