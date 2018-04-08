#ifndef __NOTIFICATION_H__
#define __NOTIFICATION_H__
#include <stdint.h>
#include "server_api.h"

int32_t push_notification(myCinUserInformation_t *pmyCinUser, uint32_t push_code, char *msg, uint32_t msg_len, uint32_t push_type);

void notification_task(void *pvParameters);
#endif
