#ifndef BOOT_H
#define BOOT_H
#include "stm32f10x.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "Usart.h"
#include "AT24C02.h"
#include "W25Q64.h"
#include "InterFlash.h"
#include "Xmodem.h"

#define OTA_INFO_BASE_ADDR (uint8_t)0
#define OTA_FLAG_ADDR      (uint8_t)(OTA_INFO_BASE_ADDR + 0)
#define OTA_FILE_SIZE_ADDR (uint8_t)(OTA_FLAG_ADDR + 1)
#define OTA_VERSION_ADDR   (uint8_t)(OTA_FILE_SIZE_ADDR + OTA_MAX_PROG_NUM)
#define OTA_INFO_SIZE      sizeof(OTA_Info_t)

#define OTA_FLAG_SET   0xAB
#define OTA_FLAG_RESET 0x00

#define APP_BASE_ADDR          (uint32_t)0x08005000
#define APP_PAGE_NUM           (uint32_t)44
#define APP_MAX_SIZE           (uint32_t)(APP_PAGE_NUM * INTERFLASH_PAGE_SIZE)
#define APP_INITIAL_SP         (uint32_t)(*(uint32_t *)APP_BASE_ADDR)
#define APP_RESET_VECTOR_ADDR  (APP_BASE_ADDR + 4)
#define APP_RESET_HANDLER_ADDR (void (*)(void))(*(uint32_t *)APP_RESET_VECTOR_ADDR)

#define VERSION_PATTERN ("VER-%*d.%*d.%*d-%*4d-%*2d-%*2d-%*2d.%2d")
#define VERSION_LEN     32

#define EXFLASH_APP_NUM 5

enum OTA_VERSION_VALUE {
    Version_OTA = 0,
    Version_V1,
    Version_V2,
    Version_V3,
    Version_V4,
    Version_V5,
    Version_V6,
};

enum OTA_FLAG_VALUE {
    No_OTA = 0,
    Online_OTA,
    Local_OTA,
};

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

extern OTA_Info_t OTA_Info;
extern uint8_t OTA_Status;

void Detect_OnlineOTA(void);
uint8_t Get_OTAFlag(void);
void Read_SavingOTAInfo(void);
void Saving_OTAInfo(void);
uint8_t BootLoader_GoToAPP(void);
void BootLoader_Reset(void);
FLASH_Status Erase_APP(void);
void USART_IAP(void);
void Set_APPVersion(void);
void Get_APPVersion(void);
void ExFlash_APPDownload(void);
#endif
