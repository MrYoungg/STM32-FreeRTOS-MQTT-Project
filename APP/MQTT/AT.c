#include "AT.h"
#include "stdbool.h"
#include "string.h"
#include "AT_USART.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "Debug_USART.h"
#include "Delay.h"
#include "JSON.h"
#include "MQTTClient.h"
#include "OTA.h"

char AT_ResponseBuffer[AT_RESPONSE_BUFFER_SIZE];
volatile SemaphoreHandle_t AT_Send_Mutex;
volatile EventGroupHandle_t AT_EventGroup;
volatile static int AT_State;

void AT_Init(void)
{
    // 1、初始化AT命令串口
    AT_USART_Init();

    // 2、初始化AT_Send_Mutex
    AT_Send_Mutex = xSemaphoreCreateMutex();
    // xSemaphoreTake(AT_Send_Mutex, 0);
    if (AT_Send_Mutex == NULL) {
        DEBUG_LOG("系统空间不足，互斥量创建失败 \r\n");
        while (1);
    }

    // 3、初始化事件组
    AT_EventGroup = xEventGroupCreate();
    if (AT_EventGroup == NULL) {
        DEBUG_LOG("系统空间不足，事件组创建失败 \r\n");
        while (1);
    }
}

void AT_Reset(void)
{
    int ret = 0;
    ret = AT_SendCommand("AT+RST\r\n", AT_SEND_TIMEOUT);
    if (ret != AT_OK) {
        DEBUG_LOG("RST失败 \r\n");
    }
}

void AT_ExitPassthroughMode(void)
{
    Delay_ms(200);
    AT_USART_SendString("+++");
    Delay_s(2);
}

void Set_AT_State(int status)
{
    AT_State = status;
}

int Get_AT_Status(void)
{
    int status = AT_State;
    AT_State = 0;
    return status;
}

/// @brief 向ESP8266发送AT命令
/// @param command AT命令字符串
/// @param timeout 超时时间
/// @return
static int AT_Send_Timeout(char *buf, TickType_t timeout)
{
    volatile int mutexStatus = 0;
    volatile int respStatus = 0;

    // 1、上锁，发送
    DEBUG_LOG("send mutex lock\r\n");
    xSemaphoreTake(AT_Send_Mutex, portMAX_DELAY);

    AT_USART_SendString(buf);

    // 2、等待在Resp事件标志位上
    xEventGroupWaitBits(AT_EventGroup, SEND_RESP_EVENTBIT, pdTRUE, pdFALSE, timeout);

    // 3、读取AT命令状态
    respStatus = Get_AT_Status();

    if (respStatus == AT_OK) {
        DEBUG_LOG("send task get: OK\r\n");
    }
    else if (respStatus == AT_ERROR) {
        DEBUG_LOG("send task get: ERROR\r\n");
    }
    else if (respStatus == AT_DATA_REQUEST) {
        DEBUG_LOG("send task get: Data Request\r\n");
    }
    else if (respStatus == AT_GET_SUB_NEWS) {
        DEBUG_LOG("send task get: Data from Host\r\n");
    }
    else {
        DEBUG_LOG("send task get: Unrecognized\r\n");
    }

    // 4、解锁
    xSemaphoreGive(AT_Send_Mutex);
    DEBUG_LOG("send mutex unlock\r\n");

    return respStatus;
}

/// @brief 普通模式下发送数据给AT模块
/// @param data 数据
/// @param timeout 超时时间
/// @return 回复情况
int AT_SendData_NormalMode(char *data, TickType_t timeout)
{
    return AT_Send_Timeout(data, timeout);
}

/// @brief 透传模式下发送数据给AT模块
/// @param dataBuf 数据
/// @param BufLen 数据长度
void AT_SendData_PassthroughMode(char *dataBuf, uint16_t BufLen)
{
    AT_USART_SendDataBuf(dataBuf, BufLen);
}

int AT_SendCommand(char *command, TickType_t timeout)
{
    if (strstr(command, "\r\n") == NULL) {
        DEBUG_LOG("AT command missing \\r\\n \r\n");
        return AT_RESP_UNCOMPLETE;
    }
    return AT_Send_Timeout(command, timeout);
}

/// @brief 解析AT响应
static void AT_ParseResponse(void)
{
    bool isGetOK = strstr(AT_ResponseBuffer, "OK");
    bool isGetReady = strstr(AT_ResponseBuffer, "ready");
    bool isGetError = strstr(AT_ResponseBuffer, "ERROR") || strstr(AT_ResponseBuffer, "FAIL");
    bool isGetDataRequest = strstr(AT_ResponseBuffer, ">");
    bool isGetMQTTSubNews = strstr(AT_ResponseBuffer, "+MQTTSUBRECV:");
    bool lostMQTTConnect = strstr(AT_ResponseBuffer, "+MQTTDISCONNECTED:0");

    if (isGetOK) {
        Set_AT_State(AT_OK);
        DEBUG_LOG("parse:isGetOK\r\n");

        // 1、收到数据请求
        if (isGetDataRequest) {
            Set_AT_State(AT_DATA_REQUEST);
            DEBUG_LOG("parse:isGetDataRequest\r\n");
        }
        // 唤醒发送消息的线程
        xEventGroupSetBits(AT_EventGroup, SEND_RESP_EVENTBIT);
    }
    // 2、收到错误
    else if (isGetError) {
        Set_AT_State(AT_ERROR);
        DEBUG_LOG("parse:isGetError\r\n");
        // 唤醒发送消息的线程
        xEventGroupSetBits(AT_EventGroup, SEND_RESP_EVENTBIT);
    }
    // 3、收到订阅主题的消息
    else if (isGetMQTTSubNews) {
        Set_AT_State(AT_GET_SUB_NEWS);
        DEBUG_LOG("收到订阅主题的消息 \r\n");

        // 处理消息（分辨主题，保存消息，唤醒相应线程）
        MQTT_DealingSubNews();
    }
    else if (lostMQTTConnect) {
        DEBUG_LOG("MQTT连接断开，准备重连 \r\n");
        // 唤醒连接线程
        Delay_s(3);
        mqttClient.connectState = MQTT_DISCONNECTED;
        xEventGroupSetBits(AT_EventGroup, DISCONNECT_EVENTBIT);
    }
    else {
        // Set_AT_State(AT_RESP_UNCOMPLETE);
        DEBUG_LOG("parse:uncomplete\r\n");
    }
}

/// @brief 读取AT响应
/// @param
int AT_ReceiveResponse(void)
{
    // uint8_t responseBuffer_local[AT_RESPONSE_BUFFER_SIZE] = {0};
    volatile int rxStatus = 0;

    rxStatus = USART_Read_Buffer(portMAX_DELAY);

    // responseBuffer的数据溢出
    if (rxStatus == RX_BUFFER_OVERFLOW) {
        DEBUG_LOG("receive task:usart buffer overflow\r\n");
        return pdFAIL;
    }

    // DEBUG_LOG("Receive response successfully\r\n");
    DEBUG_LOG("\r\n-------- responseBuffer begin--------\r\n");
    DEBUG_LOG("%s\r\n", AT_ResponseBuffer);
    DEBUG_LOG("\r\n-------- responseBuffer end--------\r\n");

    // 解析数据
    AT_ParseResponse();

    return pdPASS;
}
