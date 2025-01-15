#ifndef DEBUG_H
#define DEBUG_H
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void Debug_USART_Printf(char *format, ...);
void Debug_USART_Init(void);

#define DEBUG_LOG(fmt, ...)                                                                        \
    do {                                                                                           \
        Debug_USART_Printf("[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__);                       \
    } while (0)

#endif
