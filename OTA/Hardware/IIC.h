#ifndef I2C_H
#define I2C_H
#include "stm32f10x.h"
#include "Delay.h"

#define IIC_HIGH           1
#define IIC_LOW            0
#define IIC_ACK            0
#define IIC_NOACK          1

void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
void IIC_SendByte(uint8_t Byte);
uint8_t IIC_ReceiveByte(void);
void IIC_SendAck(uint8_t AckBit);
uint8_t IIC_ReceiveAck(int16_t timeout);

#endif
