#ifndef AT_USART_H
#define AT_USART_H
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "stdlib.h"

// #define USART_BUFFER_SIZE 256
#define MAX_USART_FRAME_SIZE 512

// 数据接收状态
enum {
    RX_RECV_SUCCESE = 0, // 成功接收
    RX_BUFFER_OVERFLOW,  // 环形缓冲区溢出
};

void AT_USART_Init(void);
void AT_USART_SendDataBuf(char *Buf, uint16_t BufLen);
void AT_USART_SendString(char *String);
void AT_USART_WaitForInput(char *buf, size_t bufLen);
int USART_Read_Buffer(TickType_t timeout);
#endif
