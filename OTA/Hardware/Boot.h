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

#define OTA_INFO_BASE_ADDR (uint8_t)0
#define OTA_FLAG_ADDR      (uint8_t)(OTA_INFO_BASE_ADDR + 0)
#define OTA_FILE_SIZE_ADDR (uint8_t)(OTA_FLAG_ADDR + 1)
#define OTA_MAX_PROG_NUM   7
#define OTA_VERSION_ADDR   (uint8_t)(OTA_FILE_SIZE_ADDR + OTA_MAX_PROG_NUM)
#define OTA_INFO_SIZE      sizeof(OTA_Info_t)

#define OTA_FLAG_SET   0xAB
#define OTA_FLAG_RESET 0x00

#define APP_BASE_ADDR          (uint32_t)0x08005000
#define APP_PAGE_NUM           (uint32_t)44
#define APP_MAX_SIZE           (uint32_t)(INTERFLASH_PAGE_SIZE * APP_PAGE_NUM)
#define APP_INITIAL_SP         (uint32_t)(*(uint32_t *)APP_BASE_ADDR)
#define APP_RESET_VECTOR_ADDR  (APP_BASE_ADDR + 4)
#define APP_RESET_HANDLER_ADDR (void (*)(void))(*(uint32_t *)APP_RESET_VECTOR_ADDR)

enum {
    Version_OTA = 0,
    Version_V1,
    Version_V2,
    Version_V3,
    Version_V4,
    Version_V5,
    Version_V6,
};

enum {
    No_OTA = 0,
    Online_OTA,
    Local_OTA,

};

typedef struct {
    // 标记是否有OTA事件发生
    uint8_t OTA_Flag;

    // 记录W25Q64中各个不同版本的程序的大小（单位：字节），其中第0个成员始终保留给OTA使用
    uint32_t OTA_File_Size[OTA_MAX_PROG_NUM];

    // 服务器程序版本号
    // uint8_t Sever_OTA_Version;

} OTA_Info_t;

extern OTA_Info_t OTA_Info;
extern uint8_t OTA_Status;

void Detect_OnlineOTA(void);
uint8_t Get_OTAFlag(void);
void Read_OTAInfo(void);
void Write_OTAInfo(void);
uint8_t BootLoader_GoToAPP(void);
void BootLoader_Reset(void);
FLASH_Status Erase_APP(void);
#endif
