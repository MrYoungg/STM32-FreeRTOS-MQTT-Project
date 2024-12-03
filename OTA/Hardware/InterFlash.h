#ifndef INTERFLASH_H
#define INTERFLASH_H
#include "stm32f10x.h"
#include "stdlib.h"
#include "Usart.h"

#define INTERFLASH_PAGE_NUM  64
#define INTERFLASH_PAGE_SIZE 1024
#define INTERFLASH_GET_PAGE_ADDR(page)                                                             \
    (uint32_t)((uint32_t)0x08000000 + (uint32_t)(page) * INTERFLASH_PAGE_SIZE)
#define INTERFLASH_MAX_ADDR 0x08010000

uint32_t InterFlash_ReadWord(uint32_t Address);
uint16_t InterFlash_ReadHalfWord(uint32_t Address);
uint8_t InterFlash_ReadByte(uint32_t Address);
void InterFlash_ReadBuf_Word(uint32_t StartAddress,
                             uint32_t DataLen,
                             uint32_t *RecvBuf,
                             uint32_t BufLen);
FLASH_Status InterFlash_ErasePage(uint32_t Address);
FLASH_Status InterFlash_WriteHalfWord(uint32_t Address, uint16_t Data);
FLASH_Status InterFlash_WriteWord(uint32_t Address, uint32_t Data);
FLASH_Status InterFlash_WriteBuf_Word(uint32_t StartAddress, uint32_t *DataBuf, uint32_t DataLen);

#endif
