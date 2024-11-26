#include "ATCommand.h"

volatile static SemaphoreHandle_t AT_Send_Mutex;
volatile static SemaphoreHandle_t AT_DataPacketRead_Mutex;

static int AT_Status;
static RingBuffer_t AT_DataPacketBuffer;

void AT_Init(void)
{
    // 初始化AT命令串口
    AT_USART_Init();

    // 初始化AT数据包缓冲区
    RingBuffer_Init(&AT_DataPacketBuffer, AT_DATA_PACKET_SIZE);

    // 初始化AT命令发送线程mutex(注意初始化后先上锁)
    // platform_mutex_init(&AT_Send_Mutex);
    AT_Send_Mutex = xSemaphoreCreateMutex();
    // platform_mutex_lock_timeout(&AT_Send_Mutex, 0);
    xSemaphoreTake(AT_Send_Mutex, 0);

    // 初始化AT数据处理线程mutex
    AT_DataPacketRead_Mutex = xSemaphoreCreateMutex();
    xSemaphoreTake(AT_DataPacketRead_Mutex, 0);
}

void AT_Reset(void)
{
    AT_SendCommand("AT+RST\r\n", portMAX_DELAY);
}

static void Set_AT_Status(int status)
{
    AT_Status = status;
}

static int Get_AT_Status(void)
{
    return AT_Status;
}

int AT_SendData(char *data, TickType_t timeout)
{
    return AT_Send(data, timeout);
}

int AT_SendCommand(char *command, TickType_t timeout)
{
    if (strstr(command, "\r\n") == NULL) {
        DEBUG_LOG("AT command missing \\r\\n \r\n");
        return AT_RESP_UNCOMPLETE;
    }
    return AT_Send(command, timeout);
}

/// @brief 向ESP8266发送AT命令
/// @param command AT命令字符串
/// @param timeout 超时时间
/// @return
static int AT_Send(char *c, TickType_t timeout)
{
    volatile int mutexStatus;
    volatile int respStatus;

    // 发送
    AT_USART_SendString(c);

    // 等待response(基于mutex的唤醒)
    // DEBUG_LOG("send mutex lock\r\n");
    mutexStatus = xSemaphoreTake(AT_Send_Mutex, timeout);
    // DEBUG_LOG("send mutex unlock\r\n");

    // Response超时
    if (mutexStatus != pdPASS) {
        // DEBUG_LOG("Send_Task Timeout\r\n");
        return AT_RESPONSE_TIMEOUT;
    }

    // 成功唤醒
    // DEBUG_LOG("Send_Task Awakened\r\n");
    // 读取AT命令状态
    respStatus = Get_AT_Status();

    if (respStatus == AT_OK) {
        DEBUG_LOG("send task get: OK\r\n");
    }
    else if (respStatus == AT_ERROR) {
        DEBUG_LOG("send task get: ERROR\r\n");
    }
    else if (respStatus == AT_DATA_REQUEST) {
        DEBUG_LOG("send task get: data request\r\n");
    }
    else if (respStatus == AT_DATA_FROM_HOST) {
        DEBUG_LOG("send task get: data from host\r\n");
    }
    else {
        DEBUG_LOG("send task get: unrecognized\r\n");
    }

    return respStatus;
}

/// @brief 读取AT响应
/// @param
int AT_ReceiveResponse(void)
{
    uint8_t responseBuffer[AT_RESPONSE_BUFFER_SIZE];
    volatile int rxStatus;

    rxStatus = USART_Read_Buffer(responseBuffer, AT_RESPONSE_BUFFER_SIZE, portMAX_DELAY);

    // responseBuffer的数据溢出
    if (rxStatus == RX_BUFFER_OVERFLOW) {
        DEBUG_LOG("receive task:usart buffer overflow\r\n");
        return pdFAIL;
    }

    DEBUG_LOG("\r\n-------- responseBuffer begin--------\r\n");
    DEBUG_LOG("%s\r\n", responseBuffer);
    DEBUG_LOG("\r\n-------- responseBuffer end--------\r\n");

    // 解析数据
    // DEBUG_LOG("parse response\r\n");
    AT_ParseResponse((char *)responseBuffer);

    // if (Get_AT_Status() != AT_RESP_UNCOMPLETE &&) {
    //     // DEBUG_LOG("wake up send task\r\n");
    //     platform_mutex_unlock(&AT_Send_Mutex);
    // }

    return pdPASS;
}

/// @brief 解析AT响应
/// @param responseBuffer AT响应缓存区
void AT_ParseResponse(char *responseBuffer)
{
    bool isGetOK = strstr(responseBuffer, "OK");
    bool isGetReady = strstr(responseBuffer, "ready");
    bool isGetError = strstr(responseBuffer, "ERROR") || strstr(responseBuffer, "FAIL");
    bool isGetDataRequest = strstr(responseBuffer, ">");
    bool isGetDataFromHost = strstr(responseBuffer, "+MQTTSUBRECV:");

    if (isGetOK || isGetReady) {
        Set_AT_Status(AT_OK);
        DEBUG_LOG("parse:isGetOK\r\n");

        // 1、收到数据请求
        if (isGetDataRequest) {
            Set_AT_Status(AT_DATA_REQUEST);
            DEBUG_LOG("parse:isGetDataRequest\r\n");
        }

        // 唤醒发送任务
        // platform_mutex_unlock(&AT_Send_Mutex);
        xSemaphoreGive(AT_Send_Mutex);
    }
    // 2、收到错误
    else if (isGetError) {
        Set_AT_Status(AT_ERROR);
        DEBUG_LOG("parse:isGetError\r\n");

        // 唤醒发送任务
        // platform_mutex_unlock(&AT_Send_Mutex);
        xSemaphoreGive(AT_Send_Mutex);
    }
    // 3、收到服务器传来的数据包
    else if (isGetDataFromHost) {
        Set_AT_Status(AT_DATA_FROM_HOST);
        DEBUG_LOG("parse:isGetDataFromHost\r\n");

        // 1、将保存数据到环形缓冲区AT_DataPacketBuffer
        SaveJSONDataToBuffer(responseBuffer);
        DEBUG_LOG("save data finished\r\n");

        // 2、唤醒数据包处理线程
        DEBUG_LOG("wake up read data task\r\n");
        xSemaphoreGive(AT_DataPacketRead_Mutex);
    }
    else {
        Set_AT_Status(AT_RESP_UNCOMPLETE);
        DEBUG_LOG("parse:uncomplete\r\n");
    }
}

/// @brief 将收到的数据保存到数据环形缓冲区
/// @param buf
/// 收到的数据，格式为：+MQTTSUBRECV:<linkID>,"topic",<dataSize>,<JSONData>，只保存<JSONData>部分
static int SaveJSONDataToBuffer(char *buf)
{
    volatile int i = 0;
    volatile int ret;
    while (buf[++i] != '{');

    while (buf[i] != '\0') {
        ret = Write_RingBuffer(&AT_DataPacketBuffer, &buf[i++], 1);
        if (ret != pdPASS) {
            break;
        }
    }
    return ret;
}

/// @brief 读取数据包环形缓冲区中的数据
/// @param buf 存放数据的buf
/// @param dataLen 读取的数据长度
/// @param bufLen buf的长度
/// @param timeout 超时时间
int AT_Read_DataPacketBuffer(char *buf, int bufLen, int timeout)
{
    volatile int ret;
    volatile int dataLen;
    // 1、读取AT数据缓冲区，如果缓冲区空则休眠
    while (is_RingBuffer_Empty(&AT_DataPacketBuffer)) {
        DEBUG_LOG("read data lock\r\n");
        // platform_mutex_lock_timeout(&AT_DataPacketRead_Mutex, timeout);
        xSemaphoreTake(AT_DataPacketRead_Mutex, timeout);
        Delay_ms(100);
        DEBUG_LOG("read data unlock\r\n");
    }
    // DEBUG_LOG("RespDataPacket size:%d\r\n", AT_DataPacketBuffer.dataSize);

    // 2、AT数据缓冲区中有数据，被唤醒，读取收到的数据包到buf
    dataLen = AT_DataPacketBuffer.dataSize;
    DEBUG_LOG("data pack size:%d\r\n", dataLen);
    ret = Read_RingBuffer(&AT_DataPacketBuffer, buf, dataLen, bufLen);
    return ret;
}
