#include "stm32f10x.h"
#include "string.h"
#include "Usart.h"
#include "Key.h"
#include "LED.h"
#include "AT24C02.h"
#include "W25Q64.h"
#include "InterFlash.h"
#include "Boot.h"
#include "CMD.h"
#include "Xmodem.h"
#include "OnlineOTA.h"

void printBuf(uint8_t *buf, uint32_t bufLen);
uint8_t buf[1024] = {0};

int main(void)
{
    MyUSART_Init();
    AT24C02_Init();
    W25Q64_Init();
    Delay_ms(20);

    LOG("BootLoader Start!\r\n");

    // 先读AT24C02中的参数
    Read_SavingOTAInfo();
    // LOG("%s\r\n", OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].version);
    // LOG("%d\r\n", OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].size);
    // while(1);

    // 1、串口命令行（本地IAP）
    BootLoader_CMD();

    // 2、检测线上OTA
    Detect_OnlineOTA();

    while (1) {
        switch (OTA_Status) {
            // 1、没有OTA事件
            case No_OTA: {
                uint8_t ret = 0;
                // （1）跳转到APP
                LOG("未检测到OTA事件 \r\n");

                ret = BootLoader_GoToAPP();

                // （2）跳转失败
                if (ret == false) {
                    LOG("跳转APP失败，APP程序可能已损坏，请重新下载APP程序 \r\n");
                    BootLoader_Reset();
                }
                break;
            }

            // 2、检测到线上OTA事件
            case Online_OTA: {
                uint8_t ret = 0;
                LOG("Online OTA\r\n");
                // （1）从W25Q64第0块搬运APP程序到内部flash
                ret = OnlineOTA_UpdateAPP();
                if (ret != true) {
                    OTA_Info.OTA_Flag = OTA_FLAG_RESET;
                    Saving_OTAInfo();
                    LOG("搬运程序出错,准备重启\r\n");
                    BootLoader_Reset();
                }
                LOG("APP程序加载完成\r\n");

                // （2）搬运完毕，复位OTA_Flag，跳转到APP
                OnlineOTA_ConfigInfo();
                ret = BootLoader_GoToAPP();
                // （3）跳转失败
                if (ret == false) {
                    LOG("跳转APP失败，APP程序可能已损坏，请重新下载APP程序 \r\n");
                    BootLoader_Reset();
                }
                break;
            }
            default:
                break;
        }
    }
}

void printBuf(uint8_t *buf, uint32_t bufLen)
{
    for (uint32_t i = 0; i < bufLen; i++) {
        LOG("buf[%x]:%.8x\r\n", i, buf[i]);
    }
}
