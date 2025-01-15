#include "MQTTClient.h"
#include "stdbool.h"
#include "AT_USART.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "AT.h"
#include "Sensor.h"
#include "JSON.h"
#include "OTA.h"

MQTTClient_t mqttClient;
// char MQTT_JsonDataBuffer[MAX_USART_FRAME_SIZE];
char MQTT_SubNewsBuffer[MQTT_SUBNEWS_BUFFER_SIZE];

void MQTT_ClientConfig(MQTTClient_t *client)
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

    // 4、数据上报属性（服务质量）
    client->Qos = QoS_0;

    // 5、连接状态
    client->connectState = MQTT_DISCONNECTED;
}

int MQTT_ConnectServer(const MQTTClient_t *client)
{
    int ret;
    // 1.配置客户端信息
    ret = MQTT_UserConfig(client);

    // 2.配置连接属性
    ret = MQTT_ConnectConfig(client);

    // 3.连接服务器
    ret = MQTT_Connect(client);

    return ret;
}

int MQTT_UserConfig(const MQTTClient_t *client)
{
    char command[AT_COMMAND_MAX_SIZE] = {0};
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
    char command[AT_COMMAND_MAX_SIZE] = {0};
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

int MQTT_Connect(const MQTTClient_t *client)
{
    char command[AT_COMMAND_MAX_SIZE] = {0};
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

uint8_t MQTT_Pubish(char *topic, char *data, uint32_t dataLen)
{
    char command[AT_COMMAND_MAX_SIZE] = {0};
    uint8_t ret = 0;

    snprintf(command,
             sizeof(command),
             "AT+MQTTPUBRAW=%d,\"%s\",%d,%d,%d\r\n",
             mqttClient.linkID,
             topic,
             dataLen,
             mqttClient.Qos,
             PUB_RETAIN_DISBLE);

    DEBUG_LOG("设置期望主题 \r\n");
    ret = AT_SendCommand(command, AT_SEND_TIMEOUT);
    if (ret != AT_DATA_REQUEST) {
        DEBUG_LOG("MQTT try to publish:failed\r\n");
        return ERR_MQTT_PUB;
    }

    DEBUG_LOG("发送JSON数据 \r\n");
    ret = AT_SendData_NormalMode(data, AT_SEND_TIMEOUT);
    if (ret != AT_OK) {
        DEBUG_LOG("MQTT publish data:failed\r\n");
        return ERR_MQTT_PUB;
    }

    return MQTT_SUCCESS;
}

void MQTT_PostData(SensorData_t SensorData, DeviceStatus_t DeviceStatus)
{

    char JSONStr[JSON_STRING_MAX_LENGTH] = {0};
    uint16_t JSONStrLength = 0;
    uint8_t ret = 0;

    JSONStrLength = snprintf(JSONStr,
                             sizeof(JSONStr),
                             "{\"method\":\"thing.event.property.post\",\"id\":\"1\",\"params\":{\"%s\":"
                             "%d,\"%s\":%d,\"%s\":"
                             "%d,\"%s\":%d,\"%s\":%d},\"version\":\"1.0.0\"}",
                             FOOD_PARAM_NAME,
                             SensorData.foodRemain,
                             WATER_PARAM_NAME,
                             SensorData.waterRemain,
                             LIGHT_FUNC_NAME,
                             DeviceStatus.Light_Status,
                             FOOD_FUNC_NAME,
                             DeviceStatus.Food_Status,
                             WATER_FUNC_NAME,
                             DeviceStatus.Water_Status);

    ret = MQTT_Pubish(POST_DATA_TOPIC, JSONStr, JSONStrLength);
    if (ret != MQTT_SUCCESS) {
        DEBUG_LOG("上报数据出错 \r\n");
    }
}

/// @brief 订阅MQTT主题
/// @param topic 主题名
int MQTT_Subscribe(char *topic)
{
    char command[AT_COMMAND_MAX_SIZE] = {0};
    int ret = 0;

    snprintf(
        command, sizeof(command), "AT+MQTTSUB=%d,\"%s\",%d\r\n", mqttClient.linkID, topic, mqttClient.Qos);

    ret = AT_SendCommand(command, portMAX_DELAY);
    if (ret != AT_OK) {
        DEBUG_LOG("MQTT 订阅出错 \r\n");
        return ERR_MQTT_SUB;
    }

    DEBUG_LOG("MQTT 订阅成功 \r\n");
    return ret;
}

/// @brief 取消订阅MQTT主题
/// @param topic
int MQTT_UnSubscribe(char *topic)
{
    char command[AT_COMMAND_MAX_SIZE] = {0};
    int ret = 0;

    snprintf(command, sizeof(command), "AT+MQTTUNSUB=%d,\"%s\"\r\n", mqttClient.linkID, topic);

    ret = AT_SendCommand(command, portMAX_DELAY);
    if (ret != AT_OK) {
        DEBUG_LOG("MQTT 取消订阅出错 \r\n");
        return ERR_MQTT_UNSUB;
    }

    DEBUG_LOG("MQTT 取消订阅成功 \r\n");
    return ret;
}

#if 1
static void MQTT_SaveSubNews(void)
{
    uint16_t subNewsLen = 0;
    char lenStr[4] = {0};
    char *start = NULL;

    start = strstr(AT_ResponseBuffer, "\",") + strlen("\",");
    for (uint8_t i = 0; (*start) != ',' && i < sizeof(lenStr); start++, i++) {
        lenStr[i] = *start;
    }

    if (*start != ',') {
        DEBUG_LOG("数据长度过长，无法处理\r\n");
        return;
    }
    subNewsLen = strtoul(lenStr, NULL, 10);
    DEBUG_LOG("接收到主题消息，长度为：%d\r\n", subNewsLen);

    start++;
    memset(MQTT_SubNewsBuffer, 0, sizeof(MQTT_SubNewsBuffer));
    memcpy(MQTT_SubNewsBuffer, start, subNewsLen);
}
#endif

uint8_t MQTT_CheckSubTopic(void)
{
    char *start = NULL;
    start = strstr(AT_ResponseBuffer, "\"");

    if (memcmp(start + 1, PARAM_SET_TOPIC, sizeof(PARAM_SET_TOPIC) - 1) == 0) {
        return PARAM_SET;
    }
    else if (memcmp(start + 1, RECV_OTA_INFO_TOPIC, sizeof(RECV_OTA_INFO_TOPIC) - 1) == 0) {
        return RECV_DOWNLOAD_INFO;
    }
    else if (memcmp(start + 1, RECV_DOWNLOAD_TOPIC, sizeof(RECV_DOWNLOAD_TOPIC) - 1) == 0) {
        return RECV_DOWNLOAD;
    }
    else if (memcmp(start + 1, RECV_OTA_PUSH_TOPIC, sizeof(RECV_OTA_PUSH_TOPIC) - 1) == 0) {
        return RECV_OTA_PUSH;
    }
    else {
        return UNKNOWN;
    }
}

#if 0
void MQTT_SetParams(void)
{
    // 1、解析JSON数据（获取阿里云下发的param对象）
    JSON_t *itemHead = NULL;
    JSON_GetParams(MQTT_JsonDataBuffer, &itemHead);
    // 打印链表数据
    JSON_PrintItemList(itemHead);

    // 2、根据对象参数执行相应的操作

    // 2.1 开灯（喂水、喂粮等操作本质一样）
    JSON_t *item = isInItemList(itemHead, "LightSwitch");
    if (item != NULL) {
        int lightSwitch = getItemValueNumber(item);
        if (lightSwitch)
            LED_ON;
        else
            LED_OFF;
    }

    // 2.2 设定时间
    // SetTime();

    // 3、释放链表内存
    JSON_FreeList(itemHead);
}
#endif

void MQTT_DealingSubNews(void)
{
    uint8_t topic = 0;

    // 1、判断下发消息的主题：(1)属性设置; (2)OTA升级包信息; (3)OTA升级包分片
    topic = MQTT_CheckSubTopic();

    // 2、保存收到的主题消息到 MQTT_SubNewsBuffer
    MQTT_SaveSubNews();
    DEBUG_LOG("保存的主题消息：\r\n");
    DEBUG_LOG("%s\r\n", MQTT_SubNewsBuffer);

    switch (topic) {
        // (1)收到属性设置请求
        case PARAM_SET: {
            DEBUG_LOG("云端设置设备属性 \r\n");
            // 唤醒 DeviceCtrl 线程
            xEventGroupSetBits(AT_EventGroup, ONLINE_DEVICE_CTRL_EVENTBIT);
            break;
        }

        // (2)云端回复OTA升级包信息
        case RECV_DOWNLOAD_INFO: {
            DEBUG_LOG("云端回复OTA升级包信息 \r\n");
            // 设置OTA_Status，并唤醒OTA线程
            Set_OTA_State(OTA_STATE_GET_INFO);
            // xSemaphoreGive(OTA_Mutex);
            xEventGroupSetBits(AT_EventGroup, OTA_EVENTBIT);
            break;
        }

        // (3)收到云端OTA升级推送
        case RECV_OTA_PUSH: {
            DEBUG_LOG("云端推送OTA升级 \r\n");
            // 设置OTA_Status，并唤醒OTA线程
            Set_OTA_State(OTA_STATE_GET_INFO);
            // xSemaphoreGive(OTA_Mutex);
            xEventGroupSetBits(AT_EventGroup, OTA_EVENTBIT);
            break;
        }

        // (4)收到云端OTA分片升级包
        case RECV_DOWNLOAD: {
            DEBUG_LOG("收到云端OTA升级包数据片 \r\n");
            // 设置OTA_Status，并唤醒OTA线程
            Set_OTA_State(OTA_STATE_GET_BIN);
            // xSemaphoreGive(OTA_Mutex);
            xEventGroupSetBits(AT_EventGroup, OTA_EVENTBIT);
            break;
        }

        default:
            break;
    }
}

uint8_t MQTT_GetJSONValue_Str(char *JSONStr, char *ItemName, char *ItemValueStr, uint32_t ValueStrLen)
{
    char *start = NULL;
    char *end = NULL;

    start = strstr(JSONStr, ItemName);
    if (start == NULL) {
        DEBUG_LOG("JSON 字符串中没有%s对象 \r\n", ItemName);
        return false;
    }

    while (*start != ':') start++;
    while (*start == ':' || *start == '"') start++;
    for (end = start; *end != ',' && *end != '"' && *end != '}'; end++);

    if ((uint8_t)(end - start) > ValueStrLen) {
        DEBUG_LOG("%s对象的值长度过大，无法获取 \r\n", ItemName);
        return false;
    }

    if (!memset(ItemValueStr, 0, ValueStrLen)) return false;
    if (!memcpy(ItemValueStr, start, (size_t)(end - start))) return false;

    return true;
}
