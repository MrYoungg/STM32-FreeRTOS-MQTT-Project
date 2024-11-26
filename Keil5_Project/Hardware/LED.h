#ifndef LED_H
#define LED_H
#include "stm32f10x.h"
#include "Delay.h"

#define RCC_APBxPERIPH_GPIOx RCC_APB2Periph_GPIOA
#define LED_GPIOx            GPIOA
#define LED_PINx             GPIO_Pin_4
#define LED_ON               GPIO_SetBits(LED_GPIOx, LED_PINx)
#define LED_OFF              GPIO_ResetBits(LED_GPIOx, LED_PINx)
#define LED_STATUS           GPIO_ReadOutputDataBit(LED_GPIOx, LED_PINx)

void LED_Init(void);

void LED_Blink(void);

#endif
