#include "Xmodem.h"

Xmodem_FCB_t Xmodem_FCB;

static uint16_t Xmodem_GetCRC16_Byte(uint8_t Data)
{
    uint16_t CRC_Val = Data << 8;
    for (uint8_t i = 0; i < 8; i++) {
        if ((CRC_Val & 0x8000) == 0) {
            CRC_Val = CRC_Val << 1;
        }
        else {
            CRC_Val = CRC_Val << 1;
            CRC_Val = CRC_Val ^ XMODEM_CRC16_POLY;
        }
    }
    return CRC_Val;
}

static uint16_t Xmodem_GetCRC16_Buf(uint8_t *Buf, uint16_t BufLen)
{
    uint16_t CRC_Val = 0x0000;
    for (uint32_t i = 0; i < BufLen; i++) {

        CRC_Val ^= (Buf[i] << 8);

        for (uint8_t i = 0; i < 8; i++) {

            if ((CRC_Val & 0x8000) == 0) {
                CRC_Val <<= 1;
            }
            else {
                CRC_Val <<= 1;
                CRC_Val ^= XMODEM_CRC16_POLY;
            }
        }
    }

    return CRC_Val;
}

static uint8_t Xmodem_CheckNum(uint8_t PacketNum, uint8_t Ne_PacketNum)
{
    // 1、注意：使用~取反时，变量会先被提升至int类型再取反，此时比较结果会出错（需要强转类型）
    // if(PacketNum!=(~Ne_PacketNum))，
    // 2、直接通过&判断是否互为反码即可
    if (PacketNum & Ne_PacketNum) {
        return Xmodem_NumUncomplete_Err;
    }

    if (PacketNum > Xmodem_FCB.PacketNum) {
        return Xmodem_NumWrongOrder_Err;
    }

    if (PacketNum < Xmodem_FCB.PacketNum) {
        return Xmodem_RepeatedPacket_Err;
    }

    return Xmodem_Num_Correct;
}

static bool Xmodem_CheckCRC(uint8_t *DataBuf, uint16_t CRC_Val)
{
    return CRC_Val == Xmodem_GetCRC16_Buf(DataBuf, XMODEM_DATA_SIZE);
}

static uint8_t Xmodem_RecvPacket(uint8_t *DataBuf)
{
    uint8_t Packet[XMODEM_PACKET_SIZE] = {0};
    uint8_t ret = 0;

    // 1、读取整个数据帧
    while (is_FCBList_Empty(&CMD_USART_RingBuffer));
    RingBuffer_ReadFrame(&CMD_USART_RingBuffer, Packet, sizeof(Packet));

    if (Packet[0] == XMODEM_EOT) {
        // LOG("接收到结束信号:EOT \r\n");
        Xmodem_SendACK;
        return Xmodem_EOT_Err;
    }

    if (Packet[0] != XMODEM_SOH) {
        // LOG("帧头错误，请求重传 \r\n");
        Xmodem_SendNAK;
        return Xmodem_SOH_Err;
    }
    // 帧头正确，则接收包数量++
    Xmodem_FCB.PacketNum++;

    // 2、截取数据部分
    memcpy(DataBuf, &(Packet[XMODEM_DATA_OFFSET]), XMODEM_DATA_SIZE);

    // 3、检查序号
    uint8_t PacketNum = Packet[XMODEM_PACKETNUM_OFFSET];
    uint8_t Ne_PacketNum = Packet[XMODEM_NE_PACKETNUM_OFFSET];
    ret = Xmodem_CheckNum(PacketNum, Ne_PacketNum);
    if (ret == Xmodem_NumUncomplete_Err) {
        // LOG("Xmodem 数据包序号不完整，请求重传 \r\n");
        Xmodem_SendNAK;
        return Xmodem_NumUncomplete_Err;
    }
    else if (ret == Xmodem_RepeatedPacket_Err) {
        // LOG("Xmodem 数据包重复，忽略即可 \r\n");
        Xmodem_SendACK;
        return Xmodem_RepeatedPacket_Err;
    }
    else if (ret == Xmodem_NumWrongOrder_Err) {
        // LOG("Xmodem 数据包序号乱序，终止传输 \r\n");
        Xmodem_Stop;
        return Xmodem_NumWrongOrder_Err;
    }

    // 4、检查CRC
    uint16_t CRC_Val = 0x0000;
    CRC_Val |= (uint16_t)(Packet[XMODEM_CRC_OFFSET] << 8);
    CRC_Val |= (uint16_t)Packet[XMODEM_CRC_OFFSET + 1];
    ret = Xmodem_CheckCRC(DataBuf, CRC_Val);
    if (ret == false) {
        // LOG("Xmodem 数据包CRC校验错误，请求重传 \r\n");
        Xmodem_SendNAK;
        return Xmodem_CRC_Err;
    }

    return Xmodem_Recv_Success;
}

static uint8_t Xmodem_RecvPage(void)
{
    uint8_t DataBuf[XMODEM_DATA_SIZE] = {0};
    uint8_t ret = 0;

    memset(Xmodem_FCB.PageBuffer, 0, INTERFLASH_PAGE_SIZE);
    uint8_t PacketPrePage = XMODEM_PACKETNUM_PRE_PAGE;
    for (uint8_t i = 0; i < PacketPrePage; i++) {
        // 1、读取单个包
        memset(DataBuf, 0, sizeof(DataBuf));
        ret = Xmodem_RecvPacket(DataBuf);
        bool isNeedRetransmit = (ret == Xmodem_SOH_Err) || (ret == Xmodem_NumUncomplete_Err) ||
                                (ret == Xmodem_RepeatedPacket_Err) || (ret == Xmodem_CRC_Err);
        if (isNeedRetransmit) {
            // LOG("读取重传的（下一个）数据包 \r\n");
            i--;
            continue;
        }
        else if (ret == Xmodem_EOT_Err) {
            // LOG("接收结束 \r\n");
            return ret;
        }
        else if (ret == Xmodem_NumWrongOrder_Err) {
            // LOG("主动结束传输 \r\n");
            return ret;
        }

        // 2、将数据部分写入页缓冲区相应位置
        if (ret == Xmodem_Recv_Success) {
            uint8_t *WriteAddr = &(Xmodem_FCB.PageBuffer[i * XMODEM_DATA_SIZE]);
            memcpy(WriteAddr, DataBuf, XMODEM_DATA_SIZE);
            // 5、发送ACK
            Xmodem_SendACK;
        }
    }

    return Xmodem_Recv_Success;
}

static void Xmodem_UpdateFLashPage(void)
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
    InterFlash_WritePage(PageAddr, (uint32_t *)(Xmodem_FCB.PageBuffer));
}

static void Xmodem_RecvBin(void)
{
    LOG("请上传bin文件 \r\n");
    uint8_t ret = 0;

    while (is_FCBList_Empty(&CMD_USART_RingBuffer)) {
        Xmodem_Start_CRC;
        Delay_ms(500);
    }

    while (1) {
        // 1、按1024字节（Flash的1页）接收数据
        ret = Xmodem_RecvPage();
        if (ret == Xmodem_NumWrongOrder_Err) {
            LOG("序号出错，主动结束传输 \r\n");
            return;
        }

        if (ret == Xmodem_EOT_Err && (Xmodem_FCB.PacketNum % XMODEM_PACKETNUM_PRE_PAGE) == 0) {
            LOG("页缓冲区中已无需要写入的数据 \r\n");
            return;
        }

        // 2、接收满（或收到EOT）后将页缓冲区写入flash
        Xmodem_UpdateFLashPage();

        // 3、如果是最后一包，则结束写入
        if (ret == Xmodem_EOT_Err) return;
    }
}

void USART_IAP(void)
{
    Xmodem_RecvBin();
}
