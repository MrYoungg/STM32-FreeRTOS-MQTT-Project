#include "MQTTClient.h"

int MQTT_ClientConfig(MQTTClient_t *client)
{
    client->linkID = MQTT_DEFUALT_LINK_ID;
    client->protocal = PROTO_MQTT_OVER_TCP;

    // 1、clientID/username/password
    client->clienID = CLIENT_ID;
    client->username = USERNAME;
    client->password = MQTT_PASSWORD;

    // 2、host/port
    client->host = HOST;
    client->port = PORT;

    // 3、连接属性
    client->keepalive = KEEP_ALIVE;
    client->cleanSession = CLEAN_SESSION;
    client->reconnect = RECONNECT;
}

int MQTT_Connect(const MQTTClient_t *client)
{
    int ret;
    // 1.配置客户端信息
    ret = MQTT_UserConfig(client);

    // 2.配置连接属性
    ret = MQTT_ConnectConfig(client);

    // 3.连接服务器
    ret = MQTT_ConnectSever(client);

    return ret;
}

int MQTT_UserConfig(const MQTTClient_t *client)
{
    char command[AT_COMMAND_MAX_SIZE];
    int ret;

    snprintf(command,
             sizeof(command),
             "AT+MQTTUSERCFG=%d,%d,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
             client->linkID,
             client->protocal,
             client->clienID,
             client->username,
             client->password);
    ret = AT_SendCommand(command, portMAX_DELAY);

    if (ret != AT_OK) {
        DEBUG_LOG("user config:failed\r\n");
        ret = ERR_MQTT_USERCFG;
    }
    return ret;
}

int MQTT_ConnectConfig(const MQTTClient_t *client)
{
    char command[AT_COMMAND_MAX_SIZE];
    int ret;

#ifdef LWT
    snprintf(command,
             sizeof(command),
             "AT+MQTTCONNCFG=%d,%d,%d,\"%s\",\"%s\",%d,%d,",
             client->linkID,
             client->keepalive,
             client->cleanSession,
             client->lwtTopic,
             client->lwtMsg,
             client->lwtQos,
             client->lwtRetain);
#else
    snprintf(command,
             sizeof(command),
             "AT+MQTTCONNCFG=%d,%d,%d,\"\",\"\",0,0\r\n",
             client->linkID,
             client->keepalive,
             client->cleanSession);
#endif

    ret = AT_SendCommand(command, portMAX_DELAY);

    if (ret != AT_OK) {
        DEBUG_LOG("connect c:failed\r\n");
        ret = ERR_MQTT_CONNCFG;
    }
    return ret;
}

int MQTT_ConnectSever(const MQTTClient_t *client)
{
    char command[AT_COMMAND_MAX_SIZE];
    int ret;

    snprintf(command,
             sizeof(command),
             "AT+MQTTCONN=%d,\"%s\",%d,%d\r\n",
             client->linkID,
             client->host,
             client->port,
             client->reconnect);
    ret = AT_SendCommand(command, portMAX_DELAY);

    if (ret != AT_OK) {
        DEBUG_LOG("connect server:failed\r\n");
        ret = ERR_MQTT_CONN;
    }
    return ret;
}
