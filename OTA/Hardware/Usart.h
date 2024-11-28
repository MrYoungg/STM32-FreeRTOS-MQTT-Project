#ifndef USART_H
#define USART_H
#include <stdio.h>
#include <stdarg.h>
#include "stm32f10x.h"
#include "RingBuffer.h"

#define MAX_USART_FRAME_SIZE 256

#define DEBUG_LOG(fmt, ...)                                                                        \
    do {                                                                                           \
        extern void CMD_USART_Printf(char *format, ...);                                           \
        CMD_USART_Printf("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);                       \
    } while (0)
    
extern RingBuffer_t CMD_USART_Buffer;
void MyUSART_Init(void);

#endif
