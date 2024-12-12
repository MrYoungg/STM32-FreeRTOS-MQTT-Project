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
#include "main.h"

void printBuf(uint8_t *buf, uint32_t bufLen);

int main(void)
{
    MyUSART_Init();
    AT24C02_Init();
    W25Q64_Init();
    Delay_ms(20);

    LOG("BootLoader Start!\r\n");

    // 1、进入串口命令行（询问本地OTA）
    BootLoader_CMD();

    // 2、检测线上OTA
    // Detect_OnlineOTA();

    while (1) {
        switch (OTA_Status) {
            // 1、没有OTA事件
            case No_OTA: {
                // （1）跳转到APP
                LOG("未检测到OTA事件 \r\n");

                uint8_t ret = BootLoader_GoToAPP();

                // （2）跳转失败
                if (ret == false) {
                    LOG("跳转APP失败，APP程序可能已损坏，请重新下载APP程序 \r\n");
                    BootLoader_Reset();
                }
                break;
            }

            // 2、检测到线上OTA事件
            case Online_OTA: {
                LOG("Online OTA\r\n");
                // （1）从W25Q64搬运APP程序到内部flash
                // BootLoader_UpdateApp(Version_OTA);

                // （2）搬运完毕，复位OTA_Flag，跳转到APP
                uint8_t ret = BootLoader_GoToAPP();

                // （3）跳转失败
                if (ret == false) {
                    LOG("跳转APP失败，APP程序可能已损坏，请重新下载APP程序 \r\n");
                    BootLoader_Reset();
                }
                break;
            }

            // 3、检测到本地OTA事件
            case Local_OTA: {
                LOG("Local OTA\r\n");
                // （1）从W25Q64中选择相应版本的程序搬运到内部flash
                // BootLoader_UpdateApp(OTA_Update.APP_ProgVersion);

                // （2）搬运完毕，复位OTA_Flag，跳转到APP
                uint8_t ret = BootLoader_GoToAPP();
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
