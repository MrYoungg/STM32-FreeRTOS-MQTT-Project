#ifndef XMODEM_H
#define XMODEM_H
#include "stm32f10x.h"
#include "stdbool.h"
#include "string.h"
#include "Usart.h"
#include "InterFlash.h"
#include "Boot.h"

#define Xmodem_Start_CRC           Usart_SendData((uint8_t)'C');
#define Xmodem_Start_CheckSum      Usart_SendData(XMODEM_NAK);
#define Xmodem_SendACK             Usart_SendData(XMODEM_ACK);
#define Xmodem_SendNAK             Usart_SendData(XMODEM_NAK);
#define Xmodem_Stop                Usart_SendData(XMODEM_CAN);
#define XMODEM_CRC16_POLY          (uint16_t)0x1021
#define XMODEM_PACKET_SIZE         133
#define XMODEM_PACKETNUM_OFFSET    1
#define XMODEM_NE_PACKETNUM_OFFSET 2
#define XMODEM_DATA_OFFSET         3
#define XMODEM_DATA_SIZE           128
#define XMODEM_PACKETNUM_PRE_PAGE  (uint32_t)((INTERFLASH_PAGE_SIZE) / (XMODEM_DATA_SIZE))
#define XMODEM_CRC_OFFSET          131
#define XMODEM_SOH                 (uint8_t)0x01
#define XMODEM_STX                 (uint8_t)0x02
#define XMODEM_EOT                 (uint8_t)0x04
#define XMODEM_ACK                 (uint8_t)0x06
#define XMODEM_NAK                 (uint8_t)0x15
#define XMODEM_CAN                 (uint8_t)0X18

enum {
    Xmodem_SOH_Err = 0x19,
    Xmodem_EOT_Err,
    Xmodem_NumUncomplete_Err,
    Xmodem_NumWrongOrder_Err,
    Xmodem_RepeatedPacket_Err,
    Xmodem_Num_Correct,
    Xmodem_CRC_Err,
    Xmodem_Recv_Success,
};

typedef struct {
    uint8_t PageBuffer[INTERFLASH_PAGE_SIZE];
    uint32_t PacketNum;
    uint16_t CRC_Val;
} Xmodem_FCB_t;

// extern Xmodem_FCB_t Xmodem_FCB;
void USART_IAP(void);

#endif
