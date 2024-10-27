#ifndef AT_USART_H
#define AT_USART_H
#include "stm32f10x.h"
#include "platform_mutex.h"
#include "Debug_USART.h"
#include "Delay.h"
#include "RingBuffer.h"

#define USART_BUFFER_SIZE 128

// 数据接收状态
enum {
    RX_DATA_RECEIVED = 0, // 成功接收
    RX_BUFFER_OVERFLOW,   // 环形缓冲区溢出
};

void AT_USART_Init(void);
void AT_USART_SendString(char *String);
int USART_Read_Buffer(uint8_t *responseBuffer, uint32_t responseBufferLen, TickType_t timeout);
#endif
