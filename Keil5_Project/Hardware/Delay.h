#ifndef __DELAY_H
#define __DELAY_H
#include "stm32f10x.h"
#include "FreeRTOS.h"

void Systick_Init(void);
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);

#endif
