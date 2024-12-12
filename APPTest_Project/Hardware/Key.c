#include "Delay.h"
#include "stm32f10x.h" // Device header

void key_init(void) // 初始化按键
{
    // 开启外设时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_initstructure;
    GPIO_initstructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_initstructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_initstructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_initstructure);
}

uint8_t key_getnum(void) // 读取按键值
{
    uint8_t keynum = 0; // 初始化返回值
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) {
        Delay_ms(20);                                         // 消除按下抖动
        while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) // 等待按键松开
        {
        }
        Delay_ms(20); // 消除松手抖动
        keynum = 1;
    }
    return keynum;
}
