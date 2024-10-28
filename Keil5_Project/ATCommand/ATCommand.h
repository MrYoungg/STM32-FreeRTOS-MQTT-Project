#ifndef ATCOMMAND_H
#define ATCOMMAND_H
#include "AT_USART.h"
#include "platform_mutex.h"
#include "Debug_USART.h"
#include "Delay.h"
#include <stdbool.h>
#include <string.h>

#define AT_RESPONSE_BUFFER_SIZE 128
#define AT_SEND_TIMEOUT         portMAX_DELAY
#define AT_RECEIVE_TIMEOUT      pdMS_TO_TICKS(1000)
#define AT_DATA_PACKET_SIZE     128

enum { AT_OK, AT_ERROR, AT_DATA_REQUEST, AT_DATA_FROM_HOST, AT_COMMAND_UNCOMPLETE, AT_RESPONSE_TIMEOUT };

// AT响应数据环形缓冲区
typedef struct {
    char buffer[AT_DATA_PACKET_SIZE]; // 缓冲区
    volatile uint32_t readIndex;      // 指向上一次读出的最后一个位置
    volatile uint32_t writeIndex;     // 指向上次写入的最后一个位置
    volatile uint32_t DataSize;       // 已填充数据的大小
} AT_Ring_Buffer;

void AT_Init(void);
int AT_SendCommand(char *command, TickType_t timeout);
int AT_ReceiveResponse(void);
void AT_ParseResponse(char *responseBuffer);
void AT_ProcessData(void);

#endif
