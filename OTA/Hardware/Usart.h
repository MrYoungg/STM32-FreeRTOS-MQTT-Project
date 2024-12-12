#ifndef USART_H
#define USART_H
#include <stdio.h>
#include <stdarg.h>
#include "stm32f10x.h"
#include "RingBuffer.h"

#define CMD_USARTx            USART2
#define CMD_USART_GPIOx       GPIOA
#define CMD_USART_TX_PIN      GPIO_Pin_2
#define CMD_USART_RX_PIN      GPIO_Pin_3
#define CMD_USART_BAUD_RATE   115200
#define CMD_USARTx_IRQn       USART2_IRQn
#define CMD_USARTx_IRQ_PRIO   2
#define CMD_USARTx_IRQHandler USART2_IRQHandler

#define CMD_USART_DMAx          DMA1
#define CMD_USART_DMAy_CHANNELx DMA1_Channel6
#define CMD_USART_DMA_IRQn      DMA1_Channel6_IRQn
#define CMD_USARTx_DMA_IRQ_PRIO 1

#define MAX_USART_FRAME_SIZE 256

#define LOG(fmt, ...)                                                                              \
    do {                                                                                           \
        extern void CMD_USART_Printf(char *format, ...);                                           \
        CMD_USART_Printf("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);                       \
    } while (0)

// 1、不能用格式化字符发送16进制数，否则会发送16进制数对应的字符串;
// 2、例如：如果用格式化字符发送0x15：CMD_USART_Printf("%x",0x15)，
//    则发送出去的是0x31（1对应的ASCII码）和0x35（5对应的ASCII码）;
// 3、原则：如果接收方做解码（显示成ASCII字符），则发送方做编码（16进制转为ASCII）,
//    接收方不解码（纯粹接收16进制数），则发送方不编码（纯粹发送16进制数）
#if 0
#define Usart_SendData(fmt, ...)                                                                   \
    do {                                                                                           \
        extern void CMD_USART_Printf(char *format, ...);                                           \
        CMD_USART_Printf(fmt, ##__VA_ARGS__);                                                      \
    } while (0)
#endif

#define Usart_SendData(data)                                                                       \
    do {                                                                                           \
        extern void CMD_USART_SendByte(uint8_t);                                              \
        CMD_USART_SendByte(data);                                                                  \
    } while (0)

extern RingBuffer_t CMD_USART_RingBuffer;
void MyUSART_Init(void);

#endif
