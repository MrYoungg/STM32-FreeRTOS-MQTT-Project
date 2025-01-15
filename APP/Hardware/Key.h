#ifndef __KEY_H
#define __KEY_H
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stm32f10x.h>

#define KEY_GPIOX     GPIOA
#define KEY1_PIN      GPIO_Pin_0
#define KEY2_PIN      GPIO_Pin_2
#define KEY3_PIN      GPIO_Pin_4
#define PRESS         0
#define UNPRESS       1
#define IS_KEY1_PRESS GPIO_ReadInputDataBit(KEY_GPIOX, KEY1_PIN)
#define IS_KEY2_PRESS GPIO_ReadInputDataBit(KEY_GPIOX, KEY2_PIN)
#define IS_KEY3_PRESS GPIO_ReadInputDataBit(KEY_GPIOX, KEY3_PIN)
#define KEY1_VALUE    1
#define KEY2_VALUE    2
#define KEY3_VALUE    3

void Key_Init(void);

uint8_t Key_Scan(void);

#endif
