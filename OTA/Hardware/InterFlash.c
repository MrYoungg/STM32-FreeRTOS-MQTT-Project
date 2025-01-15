#include "InterFlash.h"

/// @brief 从Flash指定地址中读取一个字（32bits）
/// @param Address
/// @return 读取到的数据
uint32_t InterFlash_ReadWord(uint32_t Address)
{
    return (*(__IO uint32_t *)Address);
}

/// @brief 从Flash指定地址中读取一个半字（16bits）
/// @param Address
/// @return 读取到的数据
uint16_t InterFlash_ReadHalfWord(uint32_t Address)
{
    return (*(__IO uint16_t *)Address);
}

/// @brief 从Flash指定地址中读取一个字节（8bits）
/// @param Address
/// @return 读取到的数据
uint8_t InterFlash_ReadByte(uint32_t Address)
{
    return (*(__IO uint8_t *)Address);
}

/// @brief 按字读取若干个数据
/// @param StartAddress 起始地址
/// @param DataLen 数据长度
/// @param RecvBuf 接收buffer
/// @param BufLen buffer长度
void InterFlash_ReadBuf_Word(uint32_t StartAddress,
                             uint32_t DataLen,
                             uint32_t *RecvBuf,
                             uint32_t BufLen)
{
    if (RecvBuf == NULL) return;

    uint32_t ReadLen = (BufLen >= DataLen) ? DataLen : BufLen;

    for (uint32_t i = 0; i < ReadLen; i++) {
        RecvBuf[i] = InterFlash_ReadWord(StartAddress + i * sizeof(uint32_t));
    }
}

/// @brief 按页擦除Flash
/// @param  要擦除的页的首地址
/// @return 状态
FLASH_Status InterFlash_ErasePage(uint32_t Address)
{
    FLASH_Unlock();
    FLASH_Status state = FLASH_ErasePage(Address);
    FLASH_Lock();
    return state;
}

/// @brief 向闪存写入半字
/// @param Address 写入地址
/// @param Data 写入数据
/// @return 状态
FLASH_Status InterFlash_WriteHalfWord(uint32_t Address, uint16_t Data)
{
    FLASH_Unlock();
    FLASH_Status state = FLASH_ProgramHalfWord(Address, Data);
    FLASH_Lock();
    return state;
}

/// @brief 向闪存写入字
/// @param Address 写入地址
/// @param Data 写入数据
/// @return 状态
FLASH_Status InterFlash_WriteWord(uint32_t Address, uint32_t Data)
{
    FLASH_Unlock();
    FLASH_Status state = FLASH_ProgramWord(Address, Data);
    FLASH_Lock();
    return state;
}

FLASH_Status InterFlash_WriteBuf_Word(uint32_t StartAddress, uint32_t *DataBuf, uint32_t DataLen)
{
    FLASH_Status state;
    uint32_t addr = 0;

    FLASH_Unlock();
    for (uint32_t i = 0; i < DataLen; i++) {
        addr = StartAddress + i * sizeof(uint32_t);
        if (addr > INTERFLASH_MAX_ADDR) {
            LOG("internal flash over written\r\n");
            while (1);
        }

        state = FLASH_ProgramWord(addr, DataBuf[i]);
        if (state != FLASH_COMPLETE) {
            LOG("internal flash write error\r\n");
            return state;
        }
    }
    FLASH_Lock();
    return state;
}

FLASH_Status InterFlash_WritePage(uint32_t PageAddress, uint32_t *DataBuf)
{
    // 此处写入的是字数据，因此写入长度是（字节数/4）
    return InterFlash_WriteBuf_Word(PageAddress, DataBuf, (INTERFLASH_PAGE_SIZE / 4));
}

static void InterFlash_UpdatePage(void)
{
    uint32_t PageAddr = 0;
    // 1、处理完整的1页数据
    if ((Xmodem_FCB.PacketNum % XMODEM_PACKETNUM_PRE_PAGE) == 0) {
        PageAddr = APP_BASE_ADDR +
                   INTERFLASH_PAGE_SIZE * ((Xmodem_FCB.PacketNum / XMODEM_PACKETNUM_PRE_PAGE) - 1);
    }

    // 2、处理不完整的一页数据
    else {
        PageAddr = APP_BASE_ADDR +
                   INTERFLASH_PAGE_SIZE * ((Xmodem_FCB.PacketNum / XMODEM_PACKETNUM_PRE_PAGE));
    }

    InterFlash_ErasePage(PageAddr);
    InterFlash_WritePage(PageAddr, (uint32_t *)(Xmodem_FCB.Buffer_1k));
}

void InterFlash_RecvBin(void)
{
    uint8_t ret = 0;

    while (1) {
        // 1、按1024字节（Flash的1页）接收数据
        ret = Xmodem_RecvData_1K();
        if (ret == Xmodem_NumWrongOrder_Err) {
            LOG("序号出错，主动结束传输 \r\n");
            return;
        }

        if (ret == Xmodem_EOT_Err && (Xmodem_FCB.PacketNum % XMODEM_PACKETNUM_PRE_PAGE) == 0) {
            LOG("页缓冲区中已无需要写入的数据 \r\n");
            return;
        }

        // 2、接收满（或收到EOT）后将页缓冲区写入flash
        InterFlash_UpdatePage();

        // 3、如果是最后一包，则结束写入
        if (ret == Xmodem_EOT_Err) return;
    }
}
