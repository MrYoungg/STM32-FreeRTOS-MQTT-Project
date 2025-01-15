#include "AT24C02.h"
// #define IIC_ACK_TIMEOUT 10
#define IIC_ACK_TIMEOUT

void AT24C02_Init(void)
{
    IIC_Init();
}

uint8_t AT24C02_WriteByte(uint8_t byteAddr, uint8_t data)
{
    uint8_t ret = 0;
    // 1、起始条件
    IIC_Start();

    // 2、发送器件寻址字节（写）
    IIC_SendByte(AT24C02_WRITE_ADDR);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 3、发送字节地址
    IIC_SendByte(byteAddr);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 4、发送数据
    IIC_SendByte(data);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 5、停止条件
    IIC_Stop();
    return ret;
}

uint8_t AT24C02_WritePage(uint8_t pageAddr, uint8_t *data)
{
    uint8_t ret = 0;
    // 1、起始条件
    IIC_Start();

    // 2、发送器件寻址字节（写） + 等待ACK
    IIC_SendByte(AT24C02_WRITE_ADDR);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 3、发送页地址（最好是起始地址） + 等待ACK
    IIC_SendByte(pageAddr);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 4、发送数据 + 等待ACK
    for (uint8_t i = 0; i < AT24C02_PAGE_SIZE; i++) {
        IIC_SendByte(data[i]);
        ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
        if (ret == IIC_NOACK) return ret;
    }

    // 5、停止条件
    IIC_Stop();
    return ret;
}

uint8_t AT24C02_ReadByte(uint8_t byteAddr)
{
    uint8_t data = 0;
    uint8_t ret = 0;
    // 1、起始条件
    IIC_Start();

    // 2、器件寻址字节（写）+ 等待ACK
    IIC_SendByte(AT24C02_WRITE_ADDR);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 3、字节地址 + 等待ACK
    IIC_SendByte(byteAddr);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 4、重复起始条件
    IIC_Start();

    // 5、器件寻址字节（读）+ 等待ACK
    IIC_SendByte(AT24C02_READ_ADDR);
    ret = IIC_ReceiveAck(IIC_ACK_TIMEOUT);
    if (ret == IIC_NOACK) return ret;

    // 6、读取数据
    data = IIC_ReceiveByte();

    // 7、发送无应答
    IIC_SendAck(IIC_NOACK);

    // 8、停止条件
    IIC_Stop();
    return data;
}

void AT24C02_Read(uint8_t startAddr, uint16_t dataLen, uint8_t *recvBuf, uint16_t recvBufLen)
{
    if (dataLen > AT24C02_BYTE_NUM || recvBuf == NULL) {
        return;
    }

    uint16_t readLen = (dataLen <= recvBufLen) ? dataLen : recvBufLen;

    for (uint16_t i = 0; i < readLen; i++) {
        recvBuf[i] = AT24C02_ReadByte(startAddr + i);
    }
}

void AT24C02_Write(uint8_t startAddr, uint8_t *dataBuf, uint16_t dataBufLen)
{
    if ((dataBufLen > AT24C02_BYTE_NUM) || dataBuf == NULL) {
        return;
    }

    for (uint16_t i = 0; i < dataBufLen; i++) {
        AT24C02_WriteByte(startAddr + i, dataBuf[i]);
        AT24C02_WAIT_FOR_WRITE;
    }
}
