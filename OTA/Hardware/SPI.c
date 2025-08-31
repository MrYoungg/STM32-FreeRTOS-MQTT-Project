#include "SPI.h"

static void SPI_W_SS(uint8_t Bitvalue);

void HWSPI_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    // 输出引脚配置
    // SS片选（用软件模拟，因此设置为通用推挽输出）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = SPI_SS_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_GPIOx, &GPIO_InitStructure);

    // CLK时钟（复用推挽输出），MOSI（复用推挽输出）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN | SPI_MOSI_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_GPIOx, &GPIO_InitStructure);

    // MISO（上拉输入）
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI_GPIOx, &GPIO_InitStructure);

    // SPI初始化
    SPI_I2S_DeInit(SPIx);
    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_BaudRatePrescaler =
        SPI_BaudRatePrescaler_2;                      // 选择波特率分频(SPI的SCK时钟频率=APBx时钟频率/分频)
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;      // 选择第几边缘采样
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;        // 选择时钟默认空闲电平
    SPI_InitStructure.SPI_CRCPolynomial = 7;          // 用于CRC参数校验，可以选默认值7
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; // 选择数据长度（常用8bits）
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; // 选择全双工或半双工
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                 // 选择高位先行（常用）、低位先行
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                      // 选择主模式还是从模式
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; // 软件方式给NSS信号，NSS用于实现多主机模型
    SPI_Init(SPIx, &SPI_InitStructure);

    SPI_Cmd(SPIx, ENABLE);
    SPI_W_SS(1); // 初始化结束之后默认给SS高电平，即不选中从机
}

static void SPI_W_SS(uint8_t Bitvalue) // 用软件模拟SS信号
{
    GPIO_WriteBit(SPI_GPIOx, SPI_SS_PIN, (BitAction)Bitvalue);
}

void SPI_Start(void)
{
    SPI_W_SS(0);
}

void SPI_Stop(void)
{
    SPI_W_SS(1);
}

/// @brief SPI模式0数据交换
/// @param ByteSend 要发送的数据
/// @return 接收到的数据
uint8_t SPI_SwapByte(uint8_t ByteSend)
{
    // 等待发送寄存器空（TXE=1），写数据时标志位自动清除，不需要手动清除
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) != SET);

    SPI_I2S_SendData(SPIx, ByteSend); // 写入数据

    // 等待接收寄存器非空（RXNE=1），读数据时标志位自动清除，不需要手动清除
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) != SET);

    return SPI_I2S_ReceiveData(SPIx); // 读取数据寄存器
}

void SPI_Read(uint8_t *recvBuf, uint32_t recvBufLen, uint32_t dataLen)
{
    if (recvBuf == NULL) {
        return;
    }
    uint32_t readLen = (recvBufLen >= dataLen) ? dataLen : recvBufLen;

    for (uint32_t i = 0; i < readLen; i++) {
        recvBuf[i] = SPI_SwapByte(SPI_DUMMY_BYTE);
    }
}

void SPI_Write(uint8_t *sendBuf, uint32_t sendBufLen)
{
    if (sendBuf == NULL) {
        return;
    }

    for (uint32_t i = 0; i < sendBufLen; i++) {
        SPI_SwapByte(sendBuf[i]);
    }
}
