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
    uint32_t addr = StartAddress;

    FLASH_Unlock();
    for (uint32_t i = 0; i < DataLen; i++) {
        addr = StartAddress + i * sizeof(uint32_t);
        if (addr > INTERFLASH_MAX_ADDR) {
            DEBUG_LOG("internal flash over written\r\n");
            while (1);
        }

        state = FLASH_ProgramWord(addr, DataBuf[i]);
        if (state != FLASH_COMPLETE) {
            DEBUG_LOG("internal flash write error\r\n");
            return state;
        }
    }
    FLASH_Lock();
    return state;
}
