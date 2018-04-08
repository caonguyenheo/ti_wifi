#ifndef __SERVER_MQTT_H__
#define __SERVER_MQTT_H__

#include <stdint.h>

#include "MQTTClient.h"
#include "system.h"

#define MYCIN_SEC_METHOD       SL_SO_SEC_METHOD_TLSV1_2
#define MYCIN_CIPHER           SL_SEC_MASK_SECURE_DEFAULT
#define MYCIN_CER_FILE         "crt/hbmqtt"
#define MYCIN_SERVER_VERIFY    1

#define MQTT_COMMAND_TIMEOUT_MS     5000

// #define MQTT_BACKGROUND_PRIORITY    1
// #define MQTT_BACKGROUND_STACK_SIZE  (1024*2)
/*
#ifdef SERVER_MQTTS
#define MQTT_KEEP_A_LIVE_TIME	30
#define MQTT_YIELD_PERIOD   (1000*5)
#define DEFAULT_MQTT_NOKL_TORESET_SLEEP 8
#define DEFAULT_MQTT_NOOK_TORESET_WAKEUP 60
#else
#define MQTT_KEEP_A_LIVE_TIME	30
#define MQTT_YIELD_PERIOD   (1000*10)
#define DEFAULT_MQTT_NOKL_TORESET_SLEEP 5
#define DEFAULT_MQTT_NOOK_TORESET_WAKEUP 40
#endif*/
#define MQTT_BUF_SIZE       450
#define MQTT_READBUF_SIZE   350

// NOTE: mycinMqttHeartbeat should be called after mycinMqttInit and mycinMqttSub

typedef struct {
    unsigned int msg_id;
    Network n;
    Client hMQTTClient;
    unsigned char buf[MQTT_BUF_SIZE];
    unsigned char readbuf[MQTT_READBUF_SIZE];
} mycinMqtt_t;

int8_t mycinMqttInit(mycinMqtt_t *pmyCinMqtt, char *clientID);
int8_t mycinMqttDeinit(mycinMqtt_t *pmyCinMqtt);
int8_t mycinMqttSub(mycinMqtt_t *pmyCinMqtt, char *topic, int32_t qos, void *callback);
int8_t mycinMqttPub(mycinMqtt_t *pmyCinMqtt, char *topic, int32_t qos, char *message);

/*
int8_t mycinMqttHeartbeatStart(mycinMqtt_t *pvParameters);
void MqttBackgroundTask(void *pvParameters);
*/

#endif //  __MQTT__
