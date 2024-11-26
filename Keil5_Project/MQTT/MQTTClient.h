#ifndef __MQTTCLIENT_H
#define __MQTTCLIENT_H

#include "ATCommand.h"
#include "Sensor.h"
#include "JSON.h"

#define PROTO_MQTT_OVER_TCP  1
#define MQTT_DEFUALT_LINK_ID 0
#define MQTT_PORT            1883
#define KEEP_ALIVE_SECOND    60

enum { CLEAN_SESSION_ENABLE = 0, CLEAN_SESSION_DISABLE };
enum { RECONNECT_DISABLE = 0, RECONNECT_ENABLE };
enum { ERR_MQTT_USERCFG = 0, ERR_MQTT_CONNCFG, ERR_MQTT_CONN, ERR_MQTT_PUB };
enum { QoS_0 = 0, QoS_1, QoS_2 };
enum { PUB_RETAIN_DISBLE, PUB_RETAIN_ENABLE };

#define CLIENT_ID                                                                                  \
    "k1yd9zVwFaz.Smart_Home|securemode=2\\,signmethod=hmacsha256\\,timestamp="                     \
    "1730969572037|"
#define USERNAME        "Smart_Home&k1yd9zVwFaz"
#define MQTT_PASSWORD   "a9c9ee55611b1c8c67f2d9a4d5115d6eb87412296704e838d80203fc92530777"
#define HOST            "iot-06z00d4h7nbwh15.mqtt.iothub.aliyuncs.com"
#define DATA_POST_TOPIC "/sys/k1yd9zVwFaz/Smart_Home/thing/event/property/post"

#define PORT          MQTT_PORT
#define KEEP_ALIVE    KEEP_ALIVE_SECOND
#define CLEAN_SESSION CLEAN_SESSION_ENABLE
#define RECONNECT     RECONNECT_DISABLE

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
    char *dataPostTopic;
    int Qos;
#ifdef LWT
    char *lwtTopic;
    char *lwtMsg;
    char *lwtQos;
    char *lwtRetain;
#endif
} MQTTClient_t;

void MQTT_ClientConfig(MQTTClient_t *client);
int MQTT_Connect(const MQTTClient_t *client);
int MQTT_UserConfig(const MQTTClient_t *client);
int MQTT_ConnectConfig(const MQTTClient_t *client);
int MQTT_ConnectSever(const MQTTClient_t *client);
int MQTT_Publish(const SensorData_t data);

#endif
