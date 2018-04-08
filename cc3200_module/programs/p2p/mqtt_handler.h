#ifndef __MQTT_HANDLER_H__
#define __MQTT_HANDLER_H__
#include <stdint.h>

extern int mqtt_status;
void MQTTClientTask(void *pvParameters);
int8_t  mqtt_pub(char *topic, char *msg);
void device_topic_generator(char *topic);
void client_id_generator(char *client_id);
#endif // __MQTT_HANDLER_H__