#ifndef __SERVER_UTIL_H__
#define __SERVER_UTIL_H__

int8_t myCinRegisterParse(const char *json, uint32_t json_len, myCinUserInformation_t *pmyCinUser,  myCinApiRegister_t *pmyCinRegister);

#endif
