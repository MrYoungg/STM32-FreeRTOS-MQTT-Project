#include "Boot.h"

#define VERSION_PATTERN 

OTA_Info_t OTA_Info;
uint8_t OTA_Status;

void Detect_OnlineOTA(void)
{
    Read_OTAInfo();

    // 1、检测到线上OTA事件
    if (OTA_Info.OTA_Flag == OTA_FLAG_SET) {
        LOG("OTA_Flag:%x, recieve ota event\r\n", OTA_Info.OTA_Flag);
        OTA_Status = Online_OTA;
        // OTA_Update.APP_ProgVersion = Version_OTA;
    }
    // 2、无OTA事件
    else {
        OTA_Status = No_OTA;
        LOG("OTA_Flag:%x, no ota event\r\n", OTA_Info.OTA_Flag);
    }
}

uint8_t Get_OTAFlag(void)
{
    uint8_t flag = 0;
    flag = AT24C02_ReadByte(OTA_INFO_BASE_ADDR);
    OTA_Info.OTA_Flag = flag;
    return flag;
}

void Read_OTAInfo(void)
{
    memset(&OTA_Info, 0, OTA_INFO_SIZE);
    AT24C02_Read(OTA_INFO_BASE_ADDR, OTA_INFO_SIZE, (uint8_t *)&OTA_Info, OTA_INFO_SIZE);
}

void Write_OTAInfo(void)
{
    AT24C02_Write(OTA_INFO_BASE_ADDR, (uint8_t *)&OTA_Info, OTA_INFO_SIZE);
}

static void DeInit_Periph(void)
{
    GPIO_DeInit(CMD_USART_GPIOx);
    GPIO_DeInit(IIC_GPIOx);
    GPIO_DeInit(SPI_GPIOx);

    USART_DeInit(CMD_USARTx);
    SPI_I2S_DeInit(SPIx);
}

// 通过inline展开，防止在SP改变后函数调用出错
static inline uint8_t Redirect_SP(void)
{
    if (APP_INITIAL_SP >= 0x20000000 && APP_INITIAL_SP <= 0x20004FFF) {
        __set_MSP(APP_INITIAL_SP);
        return true;
    }

    LOG("Fail to go to APP: Invail SP\r\n");
    return false;
}

static inline uint8_t Redirect_PC(void)
{
    if ((uint32_t)APP_RESET_HANDLER_ADDR >= APP_BASE_ADDR &&
        (uint32_t)APP_RESET_HANDLER_ADDR <= 0x08010000) {
        void (*APP_ResetHandler)(void) = APP_RESET_HANDLER_ADDR;
        APP_ResetHandler();
    }

    LOG("Fail to go to APP: Invail PC\r\n");
    return false;
}

uint8_t BootLoader_GoToAPP(void)
{
    LOG("即将跳转到APP \r\n");
    for (uint8_t i = 0; i < 3; i++) {
        LOG("%d\r\n", 3 - i);
        Delay_s(1);
    }

    uint8_t SP_Ret = true;
    uint8_t PC_Ret = true;
    SP_Ret = Redirect_SP();
    PC_Ret = Redirect_PC();
    if (!SP_Ret || !PC_Ret) return false;
    return true;
}

void BootLoader_Reset(void)
{
    LOG("设备即将重启... \r\n");
    for (uint8_t i = 0; i < 3; i++) {
        LOG("%d\r\n", 3 - i);
        Delay_s(1);
    }
    NVIC_SystemReset();
}

FLASH_Status Erase_APP(void)
{
    FLASH_Status state;
    for (uint8_t i = 0; i < APP_PAGE_NUM; i++) {
        uint32_t addr = APP_BASE_ADDR + i * INTERFLASH_PAGE_SIZE;
        state = InterFlash_ErasePage(addr);
        if (state != FLASH_COMPLETE) return state;
    }
    return state;
}

// 版本号标准格式：VER-1.0.0-2024/12/12-14:14
void Set_APPVersion(void)
{
    uint8_t versionStr[30] = {0};
    LOG("请输入版本: \r\n");

    while (is_FCBList_Empty(&CMD_USART_RingBuffer));
    RingBuffer_ReadFrame(&CMD_USART_RingBuffer, versionStr, sizeof(versionStr));


}
