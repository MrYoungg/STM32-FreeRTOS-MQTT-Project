#include "OnlineOTA.h"
#include "stm32f10x.h"
#include "stdlib.h"
#include "Boot.h"

uint8_t OnlineOTA_UpdateAPP(void)
{
    uint32_t Moved_KBs = 0;

    uint8_t buf[1024] = {0};
    uint32_t BlockAddr = 0;
    uint32_t ExPageAddr = 0;
    uint32_t InPageAddr = 0;
    FLASH_Status ret = FLASH_COMPLETE;
    ExFlash_Info_t *info = &OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM];

    LOG("准备将0号外存块写入内部Flash \r\n");
    if (info->size == 0) {
        LOG("0号存储块没有程序 \r\n");
        return false;
    }
    else if (info->size > APP_MAX_SIZE) {
        LOG("0号存储块程序过大 \r\n");
        return false;
    }

    BlockAddr = OTA_BLOCK_NUM * W25Q64_BLOCK_SIZE;
    ExPageAddr = BlockAddr;
    InPageAddr = APP_BASE_ADDR;

    // 3、写入内部Flash中（读1k写1k）
    info = &(OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM]);
    Moved_KBs = ((info->size) + (1024 - 1)) / 1024; // 实现除法的向上取整
    for (uint8_t i = 0; i < Moved_KBs; i++) {

        memset(buf, 0, sizeof(buf));
        W25Q64_ReadPages(ExPageAddr, buf, sizeof(buf), INTERFLASH_PAGE_SIZE / W25Q64_PAGE_SIZE);

        ret = InterFlash_ErasePage(InPageAddr);
        if (ret != FLASH_COMPLETE) return false;
        ret = InterFlash_WritePage(InPageAddr, (uint32_t *)buf); // 注意要按字写入
        if (ret != FLASH_COMPLETE) return false;

        ExPageAddr += INTERFLASH_PAGE_SIZE;
        InPageAddr += INTERFLASH_PAGE_SIZE;
    }

    return true;
}

void OnlineOTA_ConfigInfo(void)
{
    OTA_Info.OTA_Flag = OTA_FLAG_RESET;
    memcpy(
        OTA_Info.CurAPP_Version, OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].version, VERSION_LEN - 1);
    OTA_Info.CurAPP_Version[VERSION_LEN - 1] = '\0';
    Saving_OTAInfo();
}
