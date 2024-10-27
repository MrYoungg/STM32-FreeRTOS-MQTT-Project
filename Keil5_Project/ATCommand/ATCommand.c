#include "ATCommand.h"

static platform_mutex_t AT_Send_Mutex;
static platform_mutex_t AT_DataProcess_Mutex;

static int AT_Status;
static AT_Ring_Buffer AT_RespDataPacket;

void AT_Init(void)
{
    // 初始化AT命令串口
    AT_USART_Init();

    // 初始化AT命令发送线程mutex(注意初始化后先上锁)
    platform_mutex_init(&AT_Send_Mutex);
    platform_mutex_lock_timeout(&AT_Send_Mutex, 0);

    // 初始化AT数据处理线程mutex
    platform_mutex_init(&AT_DataProcess_Mutex);
    platform_mutex_lock_timeout(&AT_DataProcess_Mutex, 0);
}

static void Set_AT_Status(int status)
{
    AT_Status = status;
}

static int Get_AT_Status(void)
{
    return AT_Status;
}

/// @brief 向ESP8266发送AT命令
/// @param command AT命令字符串
/// @param respBuffer
/// @param respBuffer_Length
/// @param timeout
/// @return
int AT_SendCommand(char *command, TickType_t timeout)
{

    volatile int mutexStatus;
    volatile int respStatus;

    // 发送命令
    AT_USART_SendString(command);

    // 等待response(基于mutex的唤醒)
    DEBUG_LOG("send mutex lock\r\n");
    mutexStatus = platform_mutex_lock_timeout(&AT_Send_Mutex, timeout);
    DEBUG_LOG("send mutex unlock\r\n");

    // Response超时
    if (mutexStatus != pdPASS) {
        DEBUG_LOG("Send_Task Timeout\r\n");
        return AT_RESPONSE_TIMEOUT;
    }
    
    // 成功唤醒
    DEBUG_LOG("Send_Task Awakened\r\n");
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
void AT_ReceiveResponse(void)
{
    uint8_t responseBuffer[AT_RESPONSE_BUFFER_SIZE];
    volatile int rxStatus;

    rxStatus = USART_Read_Buffer(responseBuffer,AT_RESPONSE_BUFFER_SIZE, portMAX_DELAY);

    // responseBuffer中有完整的响应数据
    if (rxStatus != RX_BUFFER_OVERFLOW) {
        DEBUG_LOG("-------- responseBuffer --------\r\n");
        DEBUG_LOG(responseBuffer);
        DEBUG_LOG("-------- responseBuffer --------\r\n");
    }

    // 解析数据
    DEBUG_LOG("parse response\r\n");
    AT_ParseResponse(responseBuffer);

    // 唤醒发送任务mutex
    if (Get_AT_Status() != AT_COMMAND_UNCOMPLETE) {
        DEBUG_LOG("wake up send task\r\n");
        platform_mutex_unlock(&AT_Send_Mutex);
    }
}

/// @brief 解析AT响应
/// @param responseBuffer AT响应缓存区
void AT_ParseResponse(char *responseBuffer)
{
    bool isGetOK = strstr(responseBuffer, "\r\nOK\r\n");
    bool isGetError = strstr(responseBuffer, "\r\nERROR\r\n");
    bool isGetDataRequest = strstr(responseBuffer, ">");
    bool isGetDataFromHost = strstr(responseBuffer, "+IPD,");

    if (isGetOK) {
        Set_AT_Status(AT_OK);
        DEBUG_LOG("parse:isGetOK\r\n");
        // 收到数据请求
        if (isGetDataRequest) {
            Set_AT_Status(AT_DATA_REQUEST);
            DEBUG_LOG("parse:isGetDataRequest\r\n");
            return;
        }
    }
    else if (isGetError) {
        Set_AT_Status(AT_ERROR);
        DEBUG_LOG("parse:isGetError\r\n");
    }
    // 收到主机传来的数据包
    else if (isGetDataFromHost) {
        Set_AT_Status(AT_DATA_FROM_HOST);
        DEBUG_LOG("parse:isGetDataFromHost\r\n");
        // 1、保存某些信息到环形缓冲区 AT_RespDataPacket（考虑利用正则表达式来解析包）
        // SaveResponseData(responseBuffer);

        // 2、唤醒数据包处理线程mutex
        platform_mutex_unlock(&AT_DataProcess_Mutex);

    }
    else {
        Set_AT_Status(AT_COMMAND_UNCOMPLETE);
        DEBUG_LOG("parse:uncomplete\r\n");
    }
}

/// @brief 处理收到的数据包
/// @param  
void AT_ProcessData(void){
    char recvData[AT_RESPONSE_BUFFER_SIZE];
    while(AT_RespDataPacket.DataSize == 0){
        DEBUG_LOG("process data lock\r\n");
        platform_mutex_lock_timeout(&AT_DataProcess_Mutex,portMAX_DELAY);
        DEBUG_LOG("process data unlock\r\n");
    }
    DEBUG_LOG("RespDataPacket size:%d\r\n",AT_RespDataPacket.DataSize);
    
    // 1、读取收到的数据包到recvData

    // 2、根据收到的数据执行对应功能

}


