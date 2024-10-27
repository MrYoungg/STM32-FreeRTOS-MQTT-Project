#include <Key.h>

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN | KEY2_PIN | KEY3_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIOX, &GPIO_InitStructure);
}

uint8_t Key_Scan(void)
{
    uint8_t Key_Value = 0;

    if (IS_KEY1_PRESS == PRESS) {
        vTaskDelay(pdMS_TO_TICKS(20));
        while (IS_KEY1_PRESS == PRESS) {
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        Key_Value = KEY1_VALUE;
    }

    if (IS_KEY2_PRESS == PRESS) {
        vTaskDelay(pdMS_TO_TICKS(20));
        while (IS_KEY2_PRESS == PRESS) {
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        Key_Value = KEY2_VALUE;
    }

    if (IS_KEY3_PRESS == PRESS) {
        vTaskDelay(pdMS_TO_TICKS(20));
        while (IS_KEY3_PRESS == PRESS) {
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        Key_Value = KEY3_VALUE;
    }

    return Key_Value;
}
