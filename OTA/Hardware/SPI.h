#ifndef SPI_H
#define SPI_H
#include "stm32f10x.h"
#include "stdlib.h"

#define SPIx         SPI2
#define SPI_GPIOx    GPIOB
#define SPI_SS_PIN   GPIO_Pin_12
#define SPI_SCK_PIN  GPIO_Pin_13
#define SPI_MISO_PIN GPIO_Pin_14
#define SPI_MOSI_PIN GPIO_Pin_15

#define SPI_DUMMY_BYTE 0xFF

void HWSPI_Init(void);
void SPI_Start(void);
void SPI_Stop(void);
uint8_t SPI_SwapByte(uint8_t ByteSend);
void SPI_Read(uint8_t *recvBuf, uint32_t recvBufLen, uint32_t dataLen);
void SPI_Write(uint8_t *sendBuf, uint32_t sendBufLen);

#endif
