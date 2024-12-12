
#include "W25Q64.h"
#include "W25Q64_Ins.h"

static void W25Q64_WaitNotBusy(uint32_t timeout);

void W25Q64_Init(void)
{
    HWSPI_Init();
}

void W25Q64_ReadID(uint8_t *MID, uint16_t *DID)
{
    W25Q64_WaitNotBusy(W25Q64_TIMEOUT);
    SPI_Start();
    SPI_SwapByte(W25Q64_JEDEC_ID);       // 读取JEDEC ID
    *MID = SPI_SwapByte(SPI_DUMMY_BYTE); // 读取厂商ID
    *DID = SPI_SwapByte(SPI_DUMMY_BYTE); // 读取器件ID高8位
    *DID <<= 8;
    *DID |= SPI_SwapByte(SPI_DUMMY_BYTE); // 读取器件ID低8位
    SPI_Stop();
}

/// @brief 等待忙状态结束
/// @param  无
static void W25Q64_WaitNotBusy(uint32_t timeout)
{
    SPI_Start();
    SPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);
    while ((SPI_SwapByte(SPI_DUMMY_BYTE) & 0x01) == 0x01) // 等待忙结束
    {
        Delay_ms(1);
        timeout--;
        if (timeout == 0) {
            LOG("wait not busy timeout\r\n");
            break; // 超时退出
        }
    }
    SPI_Stop();
}

/// @brief W25Q64写使能
/// @param 无
static void W25Q64_WriteEnable(void)
{
    SPI_Start();
    SPI_SwapByte(W25Q64_WRITE_ENABLE);
    SPI_Stop();
}

/// @brief 按扇区擦除4KB
/// @param Address 扇区地址
void W25Q64_SectorErase(uint32_t Address)
{
    W25Q64_WaitNotBusy(W25Q64_TIMEOUT);
    W25Q64_WriteEnable(); // 所有的写入操作之前都先进行写使能，下一个时序结束后芯片会自动写失能

    SPI_Start();

    SPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);

    SPI_SwapByte(W25Q64_GET_HIGH_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_MID_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_LOW_ADDR(Address));

    SPI_Stop();
    W25Q64_WaitNotBusy(W25Q64_TIMEOUT); // 擦除操作结束后芯片进入忙状态，等待忙状态结束
}

/// @brief 按块擦除64KB
/// @param Address
void W25Q64_BlockErase_64K(uint32_t Address)
{
    W25Q64_WaitNotBusy(W25Q64_TIMEOUT);
    // 1、写使能
    W25Q64_WriteEnable();
    // 2、起始条件
    SPI_Start();

    // 3、发送指令
    SPI_SwapByte(W25Q64_BLOCK_ERASE_64KB);

    // 4、发送24位块起始地址
    SPI_SwapByte(W25Q64_GET_HIGH_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_MID_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_LOW_ADDR(Address));

    // 5、终止条件，不发终止条件指令不起作用
    SPI_Stop();

    // 6、等待擦除结束
    W25Q64_WaitNotBusy(W25Q64_TIMEOUT);
}

/// @brief 向一页写入数据
/// @param Address 起始地址
/// @param DataBuf 要写入的数据数组
/// @param DataBufLen 要写入的数据量（不能大于256，否则会重新覆盖页首）
void W25Q64_WritePage(uint32_t Address, uint8_t *DataBuf, uint32_t DataBufLen)
{
    if (DataBufLen > 256 || DataBuf == NULL) {
        LOG("write page wrong: buffer is too big\r\n");
        return;
    }

    W25Q64_WaitNotBusy(W25Q64_TIMEOUT);

    W25Q64_WriteEnable(); // 写使能，在完成页编程之后W25Q64会自动写失能
    SPI_Start();          // 起始信号

    SPI_SwapByte(W25Q64_PAGE_PROGRAM); // 发送页编程指令

    // 先传输24bits地址
    SPI_SwapByte(W25Q64_GET_HIGH_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_MID_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_LOW_ADDR(Address));

    // 发送数据
    SPI_Write(DataBuf, DataBufLen);
    // for (uint8_t i = 0; i < DataBufLen; i++) {
    //     SPI_SwapByte(DataBuf[i]);
    // }

    // 终止信号
    SPI_Stop();
    W25Q64_WaitNotBusy(W25Q64_TIMEOUT); // 写入操作结束后芯片进入忙状态，等待忙状态结束
}

/// @brief 读取一定长度的数据
/// @param Address 起始地址
/// @param RecvBuf 接收数据的数组
/// @param RecvBufLen 接收数据数组的数量
/// @param DataLen 接收数据的长度
void W25Q64_ReadData(uint32_t Address, uint8_t *RecvBuf, uint32_t RecvBufLen, uint32_t DataLen)
{
    if (RecvBuf == NULL) {
        LOG("recvBuf is NULL\r\n");
        return;
    }
    uint32_t recvLen = (RecvBufLen >= DataLen) ? DataLen : RecvBufLen;

    W25Q64_WaitNotBusy(W25Q64_TIMEOUT);
    SPI_Start();

    SPI_SwapByte(W25Q64_READ_DATA);

    SPI_SwapByte(W25Q64_GET_HIGH_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_MID_ADDR(Address));
    SPI_SwapByte(W25Q64_GET_LOW_ADDR(Address));

    SPI_Read(RecvBuf, RecvBufLen, recvLen);
    // for (uint32_t i = 0; i < DataNum; i++) {
    //     DataBuf[i] = SPI_SwapByte(W25Q64_DUMMY_BYTE);
    // }

    SPI_Stop();
}

void W25Q64_ReadPages(uint32_t StartPageAddress,
                      uint8_t *RecvBuf,
                      uint32_t RecvBufLen,
                      uint16_t pageNum)
{
    if (RecvBuf == NULL) {
        LOG("recvBuf is NULL\r\n");
        return;
    }
    uint32_t pageAddr = StartPageAddress;

    uint32_t readLen =
        (RecvBufLen >= pageNum * W25Q64_PAGE_SIZE) ? (pageNum * W25Q64_PAGE_SIZE) : RecvBufLen;

    W25Q64_ReadData(pageAddr, RecvBuf, RecvBufLen, readLen);
    pageAddr += W25Q64_PAGE_SIZE;
}
