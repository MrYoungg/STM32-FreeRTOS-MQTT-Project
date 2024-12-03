#include "stm32f10x.h"
#include "string.h"
#include "Usart.h"
#include "Key.h"
#include "LED.h"
#include "AT24C02.h"
#include "W25Q64.h"
#include "InterFlash.h"

#define PAGE_NUM 4

void printBuf(uint32_t *buf, uint32_t bufLen);
uint32_t buf[1024];
uint32_t wbuf[1024];

int main(void)
{
    MyUSART_Init();
    Key_Init();
    LED_Init();
    AT24C02_Init();
    W25Q64_Init();

    DEBUG_LOG("RST\r\n");

    // 擦除后4页
    DEBUG_LOG("erase\r\n");
    for (int i = 0; i < PAGE_NUM; i++) {
        InterFlash_ErasePage(INTERFLASH_GET_PAGE_ADDR(60 + i));
    }
    InterFlash_ReadBuf_Word(INTERFLASH_GET_PAGE_ADDR(60), 1024, buf, 1024);
    printBuf(buf, 1024);

    // 写后4页
    DEBUG_LOG("write\r\n");
    for (uint32_t i = 0; i < 1024; i++) {
        wbuf[i] = i;
    }
    InterFlash_WriteBuf_Word(INTERFLASH_GET_PAGE_ADDR(60), wbuf, 1024);
    InterFlash_ReadBuf_Word(INTERFLASH_GET_PAGE_ADDR(60), 1024, buf, sizeof(buf));
    printBuf(buf, 1024);

    while (1) {
    }
}

void printBuf(uint32_t *buf, uint32_t bufLen)
{
    for (uint32_t i = 0; i < bufLen; i++) {
        DEBUG_LOG("buf[%d]:%.8x\r\n", i, buf[i]);
    }
}
