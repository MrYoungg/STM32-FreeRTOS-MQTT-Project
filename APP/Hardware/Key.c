#include "Key.h"

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    // 使能AFIO的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIOX, &GPIO_InitStructure);

    // 配置外部中断线0（PA0）
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    // 配置外部中断
    EXTI_InitTypeDef EXTI_InitStruct;
    EXTI_InitStruct.EXTI_Line = EXTI_Line0;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising; // 下降沿触发
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStruct);

    // 配置NVIC
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

uint8_t Key_Scan(void)
{
    uint8_t Key_Value = 0;

    if (IS_LIGHT_KEY_PRESS == PRESS) {
        Delay_ms(20);
        while (IS_LIGHT_KEY_PRESS == PRESS) {
        }
        Delay_ms(20);
        Key_Value = KEY1_VALUE;
    }

    return Key_Value;
}

void EXTI0_IRQHandler(void)
{
    Delay_ms(10);
    if (EXTI_GetITStatus(EXTI_Line0) == SET) {
        if (LED_STATUS == 0) {
            LED_ON;
        }
        else {
            LED_OFF;
        }
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
