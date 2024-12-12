#include "stm32f10x.h" // Device header
#include "Delay.h"
#include "LED.h"
#include "Key.h"

uint8_t keynum;

int main(void)
{
    LED_Init();
    key_init();

    while (1) {
        LED_ON();
        Delay_s(1);
        LED_OFF();
        Delay_s(1);
    }
}
