#include "stm32f10x.h"
#include "Usart.h"
#include "Key.h"
#include "LED.h"
#include "AT24C02.h"

int main(void)
{
    MyUSART_Init();
    Key_Init();
    LED_Init();
    IIC_Init();

    DEBUG_LOG("RST\r\n");
    uint8_t buf[256] = {0};
    for (uint16_t i = 0; i < AT24C02_BYTE_NUM; i++) {
        buf[i] = AT24C02_ReadByte((uint8_t)i);
        DEBUG_LOG("buf[%d]:%d\r\n", i, buf[i]);
    }

    for (uint16_t i = 0; i < AT24C02_BYTE_NUM; i++) {
        AT24C02_WriteByte((uint8_t)i, (uint8_t)(i));
        AT24C02_WIAT_FOR_WRITE();
    }
    for (uint16_t i = 0; i < AT24C02_BYTE_NUM; i++) {
        buf[i] = AT24C02_ReadByte((uint8_t)i);
        DEBUG_LOG("buf[%d]:%d\r\n", i, buf[i]);
    }

    while (1) {
#if 0
        uint8_t buf[MAX_USART_FRAME_SIZE] = {0};
        Read_DataFrame(&CMD_USART_Buffer, buf, sizeof(buf));
        DEBUG_LOG("%s\r\n", buf);

        FCB_ListItem_t *item = CMD_USART_Buffer.FCBListHead;

        int size = sizeof((*item));
        int addr1 = &(item->frameBegin);
        int addr2 = &(item->frameSize);
        ;
        int addr3 = &(item->next);
        ;
        DEBUG_LOG("sizeof FCB_ListItem_t:%d\r\n", size);
        DEBUG_LOG("addr1:%8x\r\n", addr1);
        DEBUG_LOG("addr2:%8x\r\n", addr2);
        DEBUG_LOG("addr3:%8x\r\n", addr3);

        Delay_s(3);
#endif
    }
}
