#include "LED.h"

void LED_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APBxPERIPH_GPIOx, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = LED_PINx;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIOx, &GPIO_InitStructure);
}

void LED_Blink(void)
{
    int times = 3;
    int delayMs = 1000;
    while (times--) {
        LED_ON;
        Delay_ms(delayMs);
        LED_OFF;
        Delay_ms(delayMs);
    }
}
