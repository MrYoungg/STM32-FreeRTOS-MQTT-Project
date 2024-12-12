#include "CMD.h"
#define CMD_TIMEOUT 5000
#define ENTER_CMD   'c'

typedef enum {
    InFlash_EraseApp = 1,
    Usart_UpdateAPP,
    Set_OTAVersion,
    Get_OTAVersion,
    ExFlash_Download,
    ExFlash_Use,
    Exit_CMD,
    Reset,
    End_of_Enum,
} Function_t;

static uint8_t CMD_Entrance(void)
{
    LOG("按下 '%c' 进入 BootLoader CMD \r\n", ENTER_CMD);

    uint16_t timeout = CMD_TIMEOUT;
    uint8_t cmd = 0;
    while (timeout--) {
        RingBuffer_ReadFrame(&CMD_USART_RingBuffer, &cmd, sizeof(cmd));
        if (cmd == ENTER_CMD) {
            return true;
        }
        Delay_ms(1);
    }
    return false;
}

static uint8_t CMD_Menu(void)
{
    LOG("------BootLoader CMD------\r\n");
    LOG("[%d]擦除A区 \r\n", InFlash_EraseApp);
    LOG("[%d]串口IAP下载程序 \r\n", Usart_UpdateAPP);
    LOG("[%d]设置OTA版本号 \r\n", Set_OTAVersion);
    LOG("[%d]获取OTA版本号 \r\n", Get_OTAVersion);
    LOG("[%d]向外部Flash下载程序 \r\n", ExFlash_Download);
    LOG("[%d]读取外部Flash程序 \r\n", ExFlash_Use);
    LOG("[%d]退出命令行 \r\n", Exit_CMD);
    LOG("[%d]重新启动 \r\n", Reset);
    LOG("\r\n");

    uint8_t func = 0;
    while (func == 0) RingBuffer_ReadFrame(&CMD_USART_RingBuffer, &func, sizeof(func));
    func -= '0';
    if (func >= 1 && func < End_of_Enum) {
        return func;
    }
    return false;
}

void BootLoader_CMD(void)
{
    uint8_t entry = CMD_Entrance();
    if (entry != true) return;
    uint8_t SelectedFunc = 0;
menu:
    SelectedFunc = CMD_Menu();
    if (SelectedFunc == false) {
        LOG("请选择正确的功能 \r\n");
        goto menu;
    }

    switch (SelectedFunc) {
        case InFlash_EraseApp:
            LOG("擦除A区 \r\n");
            if (Erase_APP() != FLASH_COMPLETE) {
                LOG("A区擦除出错 \r\n");
                while (1);
            }
            LOG("A区擦除完毕 \r\n");
            Delay_s(1);
            goto menu;
        case Usart_UpdateAPP:
            LOG("串口IAP下载程序\r\n");
            USART_IAP();
            Delay_s(1);
            goto menu;
        case Set_OTAVersion:
            LOG("设置OTA版本号 \r\n");
            Set_APPVersion();
            Delay_s(1);
            goto menu;
        case Get_OTAVersion:
            LOG("获取OTA版本号 \r\n");
            Delay_s(1);
            goto menu;
        case ExFlash_Download:
            LOG("下载程序到外部flash \r\n");
            Delay_s(1);
            goto menu;
        case ExFlash_Use:
            LOG("使用外部flash程序 \r\n");
            Delay_s(1);
            goto menu;
        case Exit_CMD:
            LOG("退出命令行 \r\n");
            return;
        case Reset:
            LOG("重新启动 \r\n");
            BootLoader_Reset();
        default:
            break;
    }
}
