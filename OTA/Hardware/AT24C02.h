#ifndef AT24C02_H
#define AT24C02_H
#include "stm32f10x.h"
#include "IIC.h"

#define AT24C02_READ_ADDR                0xA1
#define AT24C02_WRITE_ADDR               0xA0
#define AT24C02_BYTE_NUM                 256
#define AT24C02_PAGE_NUM                 32
#define AT24C02_PAGE_SIZE                8 // AT24C02每页8字节，共32页
#define AT24C02_GET_PAGE_ADDR(pageIndex) (uint8_t)((pageIndex) * AT24C02_PAGE_SIZE)
#define AT24C02_WIAT_FOR_WRITE()           Delay_ms(5)

uint8_t AT24C02_WriteByte(uint8_t byteAddr, uint8_t data);
uint8_t AT24C02_WritePage(uint8_t pageAddr, uint8_t *data);
uint8_t AT24C02_ReadByte(uint8_t byteAddr);

#endif
