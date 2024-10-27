#ifndef DEBUG_H
#define DEBUG_H
#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define DEBUG_LOG Debug_USART_Printf

void Debug_USART_Init(void);
void Debug_USART_Printf(char *format, ...);

#endif
