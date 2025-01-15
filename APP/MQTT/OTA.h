#ifndef __OTA_H
#define __OTA_H
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define OTA_DATA_FRAME_SIZE  256
#define OTA_DATA_PACKET_SIZE 512
#define OTA_BASE_ADDR        (uint32_t)0x00000000
#define OTA_BLOCK_NUM        0

#define VERSION_LEN        32
#define EXFLASH_APP_NUM    5
#define OTA_INFO_SIZE      sizeof(OTA_Info_t)
#define OTA_INFO_BASE_ADDR (uint8_t)0x00

#define OTA_FLAG_SET   0xAB
#define OTA_FLAG_RESET 0x00

#define IBM_CRC16_POLY           (uint16_t)0x8005
#define ALIYUN_JSON_BYTES_LENGTH 2

#define SWAP_BYTE(ByteAddr1, ByteAddr2)                                                            \
    do {                                                                                           \
        uint8_t tmp = *(ByteAddr1);                                                                \
        *(ByteAddr1) = *(ByteAddr2);                                                               \
        *(ByteAddr2) = (tmp);                                                                      \
    } while (0)

typedef enum {
    No_OTA = 1,
    Online_OTA,
    Local_OTA,
} OTA_FLAG_VALUE;

typedef enum {
    OTA_POST_VERSION_ERR = 1,
    OTA_GET_INFO_ERR,
    OTA_GET_DOWNLOAD_ERR,
} OTA_ERR;

typedef enum {
    OTA_STATE_NONEWS = 0,
    OTA_STATE_GET_INFO,
    OTA_STATE_GET_BIN,
} OTA_State_t;

typedef struct ExFlash_Info {
    uint8_t version[VERSION_LEN];
    uint32_t size;
} ExFlash_Info_t;

typedef struct {
    // 标记是否有OTA事件发生
    uint8_t OTA_Flag;

    // 程序版本信息
    uint8_t CurAPP_Version[VERSION_LEN];

    // 记录W25Q64中各个不同版本的程序的大小（单位：字节），其中第0个成员始终保留给OTA使用
    ExFlash_Info_t ExFlash_InfoArr[EXFLASH_APP_NUM];

} OTA_Info_t;

typedef struct {
    uint32_t FileSize;
    uint32_t RecvFileSize;
    char Version[VERSION_LEN];
    uint32_t StreamID;
    uint32_t FileID;

    uint8_t BinFrameBuf[OTA_DATA_FRAME_SIZE];
    uint16_t CRC16_Value;
} OTA_Download_t;

extern OTA_Info_t OTA_Info;
extern volatile SemaphoreHandle_t OTA_Mutex;
extern OTA_Download_t OTA_Download_CB;

void OTA_Init(void);
uint8_t OTA_GetBinDataFrame(void);
void Read_SavingOTAInfo(void);
void Save_OTAInfo(void);
void OTA_ConfigInfo(void);
void OTA_Reset(void);
uint8_t OTA_PostVersion(void);
uint8_t OTA_RequestDownloadInfo(void);
void OTA_GetDownloadInfo(void);
void OTA_RequestDataFrame(void);
uint16_t OTA_GetBuf_CRC16IBM(uint8_t *Buf, uint16_t BufLen);
OTA_State_t Get_OTA_State(void);
void Set_OTA_State(OTA_State_t state);
void OTA_SaveBinDataFrame(void);
void OTA_DealingInfo(void);
void OTA_DealingBinData(void);

#endif
