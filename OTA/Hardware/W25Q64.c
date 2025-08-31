
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

void W25Q64_ReadPages(uint32_t StartPageAddress, uint8_t *RecvBuf, uint32_t RecvBufLen, uint16_t pageNum)
{
    if (RecvBuf == NULL) {
        LOG("recvBuf is NULL\r\n");
        return;
    }
    uint32_t pageAddr = StartPageAddress;

    uint32_t readLen = (RecvBufLen >= pageNum * W25Q64_PAGE_SIZE) ? (pageNum * W25Q64_PAGE_SIZE) : RecvBufLen;

    W25Q64_ReadData(pageAddr, RecvBuf, RecvBufLen, readLen);
    pageAddr += W25Q64_PAGE_SIZE;
}

static void ExFlash_Menu(void)
{
    ExFlash_Info_t *info = OTA_Info.ExFlash_InfoArr;

    char *Area = "Area";
    char *Version = "Version";
    char *ProgSize = "Size";
    char *Remark = "Remark";

    LOG("当前外存存储情况如下： \r\n");
    LOG("|%6s|%28s|%10s|%16s|\r\n", Area, Version, ProgSize, Remark);
    for (uint8_t i = 0; i < EXFLASH_APP_NUM; i++) {

        char curVersion[VERSION_LEN] = {0};
        uint8_t StrLen = strlen(info[i].version);

        // 如果不恰好拷贝（StrLen - 1）长度的字符串，串口显示就无法对齐，暂未找出原因
        if (StrLen > 0) {
            // memcpy(curVersion, (info[i].version), StrLen - 1);
            memcpy(curVersion, (info[i].version), StrLen);
        }

        if (i == 0) {
            LOG("|%6d|%28s|%10d|%16s|\r\n", i, curVersion, info[i].size, "Only for OTA");
            continue;
        }
        LOG("|%6d|%28s|%10d|%16s|\r\n", i, curVersion, info[i].size, "none");
    }
}

static uint8_t ExFlash_SelectBlock(void)
{
    uint8_t ExFlash_Num = 0;

select:
    WaitForInput((char *)&ExFlash_Num, sizeof(ExFlash_Num));
    ExFlash_Num -= '0';

    if (ExFlash_Num <= 0 || ExFlash_Num >= EXFLASH_APP_NUM) {
        if (ExFlash_Num == 0) {
            LOG("0号存储块仅供OTA使用，请重新输入（1~%d） \r\n", EXFLASH_APP_NUM - 1);
            goto select;
        }
        LOG("请输入正确的外存编号（1~%d） \r\n", EXFLASH_APP_NUM - 1);
        goto select;
    }

    return ExFlash_Num;
}

static uint8_t ExFlash_SetVersion(uint8_t BlockNum)
{
    uint8_t versionStr[VERSION_LEN] = {0};
    uint8_t matched = 0;
    int temp;

    WaitForInput((char *)versionStr, sizeof(versionStr));
    LOG("%s\r\n", versionStr);

    matched = sscanf((const char *)versionStr, VERSION_PATTERN, &temp);
    if (matched == 0) {
        LOG("版本信息格式有误 \r\n");
        return false;
    }

    memcpy(OTA_Info.ExFlash_InfoArr[BlockNum].version, versionStr, sizeof(versionStr));
    Saving_OTAInfo();
    LOG("版本设置成功 \r\n");
    return true;
}

/// @brief 从串口IAP下载程序
/// @param 无
void ExFlash_DownloadFromUSART(void)
{
    uint8_t ExFlash_Num = 0;
    uint8_t PageNum_1k = 0;
    uint32_t BlockAddr = 0;
    uint32_t PageBaseAddr = 0;
    uint32_t PageAddr = 0;
    uint8_t ret = 0;
    ExFlash_Info_t *info = NULL;

    // 1、展示当前外存的存储情况
    ExFlash_Menu();

    // 2、选择要写入的外存块
    LOG("请选择要写入的外存块：1~%d \r\n", EXFLASH_APP_NUM - 1);
    ExFlash_Num = ExFlash_SelectBlock();
    info = &(OTA_Info.ExFlash_InfoArr[ExFlash_Num]);

    if (info[ExFlash_Num].size != 0) {
        uint8_t ans = 0;
        LOG("当前区域已有程序，确定要覆写吗？（y/n） \r\n");
    wait:
        WaitForInput((char *)&ans, sizeof(ans));

        if (ans == 'n' || ans == 'N') {
            LOG("取消写入 \r\n");
            return;
        }
        else if (ans != 'y' && ans != 'Y') {
            goto wait;
        }
    }

    // 设置版本信息
    LOG("请输入版本信息: \r\n");
    if (ExFlash_SetVersion(ExFlash_Num) == false) {
        return;
    }

    // 擦除对应外存块
    LOG("准备写入%d号外存块 \r\n", ExFlash_Num);
    BlockAddr = ExFlash_Num * W25Q64_BLOCK_SIZE;
    PageBaseAddr = BlockAddr;
    memset(&Xmodem_FCB, 0, sizeof(Xmodem_FCB_t));
    W25Q64_BlockErase_64K(BlockAddr);

    LOG("请上传bin文件 \r\n");

    while (1) {
        // 3、读取1k数据到PageBuffer
        ret = Xmodem_RecvData_1K();
        if (ret == Xmodem_NumWrongOrder_Err) {
            LOG("序号出错，主动结束传输 \r\n");
            return;
        }
        // 如果写满1k后恰好收到EOT，则跳出；否则还需将缓冲区中不足1k的数据写入外存
        if (ret == Xmodem_EOT_Err && Xmodem_FCB.PacketNum % 8 == 0) {
            OTA_Info.ExFlash_InfoArr[ExFlash_Num].size = Xmodem_FCB.PacketNum * XMODEM_DATA_SIZE;
            Saving_OTAInfo();
            break;
        }

        // 4、写入1k数据到外存块
        PageNum_1k = INTERFLASH_PAGE_SIZE / W25Q64_PAGE_SIZE;
        for (uint8_t i = 0; i < PageNum_1k; i++) {
            PageAddr = PageBaseAddr + i * W25Q64_PAGE_SIZE;
            W25Q64_WritePage(PageAddr, &(Xmodem_FCB.Buffer_1k[i * W25Q64_PAGE_SIZE]), W25Q64_PAGE_SIZE);
        }

        PageBaseAddr += PageNum_1k * W25Q64_PAGE_SIZE;

        // 5、写入最后不足1k的数据到外存块（如果有）
        if (ret == Xmodem_EOT_Err) {
            OTA_Info.ExFlash_InfoArr[ExFlash_Num].size = Xmodem_FCB.PacketNum * XMODEM_DATA_SIZE;
            Saving_OTAInfo();
            return;
        }
    }
}

/// @brief 将外部Flash中的程序写入内部Flash
/// @param 无
void ExFlash_MoveInAPP(void)
{
    uint8_t ExFlash_Num = 0;
    uint32_t Move_KBs = 0;
    ExFlash_Info_t *info = NULL;
    uint8_t buf[1024] = {0};
    uint32_t BlockAddr = 0;
    uint32_t ExPageAddr = 0;
    uint32_t InPageAddr = 0;

    // 1、展示外存的存储情况
    ExFlash_Menu();

    // 2、选择要读出的外存块
    LOG("请选择要读取的外存块：1~%d \r\n", EXFLASH_APP_NUM - 1);
reselect:
    ExFlash_Num = ExFlash_SelectBlock();
    info = &(OTA_Info.ExFlash_InfoArr[ExFlash_Num]);

    if (info->size == 0) {
        LOG("当前存储块没有程序，请重新输入 \r\n");
        goto reselect;
    }
    else if (info->size > APP_MAX_SIZE) {
        LOG("当前存储块程序过大，请重新选择 \r\n");
        goto reselect;
    }

    LOG("准备将%d号外存块写入内部Flash \r\n", ExFlash_Num);

    BlockAddr = ExFlash_Num * W25Q64_BLOCK_SIZE;
    ExPageAddr = BlockAddr;
    InPageAddr = APP_BASE_ADDR;

    // 3、写入内部Flash中（读1k写1k）
    info = &(OTA_Info.ExFlash_InfoArr[ExFlash_Num]);
    Move_KBs = ((info->size) / 1024) + 1;
    for (uint8_t i = 0; i < Move_KBs; i++) {

        memset(buf, 0, sizeof(buf));
        W25Q64_ReadPages(ExPageAddr, buf, sizeof(buf), INTERFLASH_PAGE_SIZE / W25Q64_PAGE_SIZE);
        InterFlash_ErasePage(InPageAddr);
        InterFlash_WritePage(InPageAddr, (uint32_t *)buf);

        ExPageAddr += INTERFLASH_PAGE_SIZE;
        InPageAddr += INTERFLASH_PAGE_SIZE;
    }
}

/// @brief 删除外部flash中的程序
/// @param 无
void ExFlash_DeleteProg(void)
{
    uint8_t ExFlash_Num = 0;
    uint32_t BlockAddr = 0;
    ExFlash_Info_t *info = NULL;

    // 1、展示外存的存储情况
    LOG("请选择要删除的外存块：1~%d \r\n", EXFLASH_APP_NUM - 1);
    ExFlash_Menu();

    // 2、选择要删除的外存块
    ExFlash_Num = ExFlash_SelectBlock();
    info = &(OTA_Info.ExFlash_InfoArr[ExFlash_Num]);
    if (info->size == 0) {
        LOG("所选外存块为空 \r\n");
        return;
    }

    BlockAddr = ExFlash_Num * W25Q64_BLOCK_SIZE;
    W25Q64_BlockErase_64K(BlockAddr);

    info = &(OTA_Info.ExFlash_InfoArr[ExFlash_Num]);
    info->size = 0;
    memset(info->version, 0, sizeof(info->version));
    Saving_OTAInfo();

    LOG("%d号外存块擦除完毕 \r\n", ExFlash_Num);
}
