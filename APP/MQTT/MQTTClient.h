#ifndef __MQTTCLIENT_H
#define __MQTTCLIENT_H
#include "Sensor.h"
#include "AT_USART.h"
#include "AT.h"
#include "Device.h"

#define PROTO_MQTT_OVER_TCP  1
#define MQTT_DEFUALT_LINK_ID 0
#define PORT                 1883
#define KEEP_ALIVE           300
#define CLEAN_SESSION        CLEAN_SESSION_ENABLE
#define RECONNECT            RECONNECT_DISABLE

#if 0
#define CLIENT_ID     "k1yd9zVwFaz.Smart_Home|securemode=2\\,signmethod=hmacsha256\\,timestamp=1730969572037|"
#define USERNAME      "Smart_Home&k1yd9zVwFaz"
#define MQTT_PASSWORD "a9c9ee55611b1c8c67f2d9a4d5115d6eb87412296704e838d80203fc92530777"
#define HOST          "iot-06z00d4h7nbwh15.mqtt.iothub.aliyuncs.com"

#define PARAM_SET_TOPIC        "/sys/k1yd9zVwFaz/Smart_Home/thing/service/property/set"
#define POST_DATA_TOPIC        "/sys/k1yd9zVwFaz/Smart_Home/thing/event/property/post"
#define POST_DATA_REPLY_TOPIC  "/sys/k1yd9zVwFaz/Smart_Home/thing/event/property/post_reply"
#define POST_VERSION_TOPIC     "/ota/device/inform/k1yd9zVwFaz/Smart_Home"
#define REQUEST_OTA_INFO_TOPIC "/sys/k1yd9zVwFaz/Smart_Home/thing/ota/firmware/get"
#define RECV_OTA_INFO_TOPIC    "/sys/k1yd9zVwFaz/Smart_Home/thing/ota/firmware/get_reply"
#define REQUEST_DOWNLOAD_TOPIC "/sys/k1yd9zVwFaz/Smart_Home/thing/file/download"
#define RECV_DOWNLOAD_TOPIC    "/sys/k1yd9zVwFaz/Smart_Home/thing/file/download_reply"
#define RECV_OTA_PUSH_TOPIC    "/ota/device/upgrade/k1yd9zVwFaz/Smart_Home"
#endif

#define CLIENT_ID     "a1bVU2nIf06.D001|securemode=2\\,signmethod=hmacsha256\\,timestamp=1735988304803|"
#define USERNAME      "D001&a1bVU2nIf06"
#define MQTT_PASSWORD "96f26fbd56c316f0944c4184555779f232726ff8da353569d9baa36edd573f8d"
#define HOST          "a1bVU2nIf06.iot-as-mqtt.cn-shanghai.aliyuncs.com"

#define PARAM_SET_TOPIC        "/sys/a1bVU2nIf06/D001/thing/service/property/set"
#define POST_DATA_TOPIC        "/sys/a1bVU2nIf06/D001/thing/event/property/post"
#define POST_DATA_REPLY_TOPIC  "/sys/a1bVU2nIf06/D001/thing/event/property/post_reply"
#define POST_VERSION_TOPIC     "/ota/device/inform/a1bVU2nIf06/D001"
#define REQUEST_OTA_INFO_TOPIC "/sys/a1bVU2nIf06/D001/thing/ota/firmware/get"
#define RECV_OTA_INFO_TOPIC    "/sys/a1bVU2nIf06/D001/thing/ota/firmware/get_reply"
#define REQUEST_DOWNLOAD_TOPIC "/sys/a1bVU2nIf06/D001/thing/file/download"
#define RECV_DOWNLOAD_TOPIC    "/sys/a1bVU2nIf06/D001/thing/file/download_reply"
#define RECV_OTA_PUSH_TOPIC    "/ota/device/upgrade/a1bVU2nIf06/D001"

#define MQTT_SUBNEWS_BUFFER_SIZE AT_RESPONSE_BUFFER_SIZE

#define JSON_STRING_MAX_LENGTH  256
#define JSON_NAME_MAX_LENGTH    32
#define VALUE_STRING_MAX_LENGTH 32

enum {
    CLEAN_SESSION_ENABLE = 0,
    CLEAN_SESSION_DISABLE,
};

enum {
    RECONNECT_DISABLE = 0,
    RECONNECT_ENABLE,
};

enum ErrMQTT {
    ERR_MQTT_USERCFG = 0,
    ERR_MQTT_CONNCFG,
    ERR_MQTT_CONN,
    ERR_MQTT_PUB,
    ERR_MQTT_SUB,
    ERR_MQTT_UNSUB,
    MQTT_SUCCESS,
};

enum Qos {
    QoS_0 = 0,
    QoS_1,
    QoS_2,
};

enum {
    PUB_RETAIN_DISBLE,
    PUB_RETAIN_ENABLE,
};

typedef enum {
    UNKNOWN = 0,
    PARAM_SET,
    RECV_DOWNLOAD_INFO,
    RECV_DOWNLOAD,
    RECV_OTA_PUSH,
} SubTopic_t;

typedef enum {
    MQTT_DISCONNECTED = 0,
    MQTT_CONNECTED,
} MQTT_ConnectState_t;

typedef struct {
    int protocal;
    int linkID;

    char *clienID;
    char *username;
    char *password;

    char *host;
    int port;

    int keepalive;
    int cleanSession;
    int reconnect;
    volatile int connectState;

    int Qos;

#ifdef LWT
    char *lwtTopic;
    char *lwtMsg;
    char *lwtQos;
    char *lwtRetain;
#endif
} MQTTClient_t;

extern MQTTClient_t mqttClient;
// extern char MQTT_JsonDataBuffer[MAX_USART_FRAME_SIZE];
extern char MQTT_SubNewsBuffer[MQTT_SUBNEWS_BUFFER_SIZE];

void MQTT_ClientConfig(MQTTClient_t *client);
int MQTT_ConnectServer(const MQTTClient_t *client);
int MQTT_UserConfig(const MQTTClient_t *client);
int MQTT_ConnectConfig(const MQTTClient_t *client);
int MQTT_Connect(const MQTTClient_t *client);
uint8_t MQTT_Pubish(char *topic, char *data, uint32_t dataLen);
void MQTT_PostData(SensorData_t SensorData, DeviceStatus_t DeviceStatus);
int MQTT_Subscribe(char *Topic);
int MQTT_UnSubscribe(char *Topic);
void MQTT_DealingSubNews(void);
uint8_t MQTT_GetJSONValue_Str(char *JSONStr, char *ItemName, char *ItemValueStr, uint32_t ValueStrLen);

#endif
