#include "IIC.h"

void IIC_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // 开漏输出
    GPIO_InitStructure.GPIO_Pin = IIC_SCL_PIN | IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_GPIOx, &GPIO_InitStructure);
    // 置高电平，即释放总线
    GPIO_SetBits(IIC_GPIOx, IIC_SCL_PIN | IIC_SDA_PIN);
}

/// @brief 写SCL
/// @param Bitvalue 写入的数据（0或1）
static void IIC_W_SCL(uint8_t Bitvalue)
{
    GPIO_WriteBit(IIC_GPIOx, IIC_SCL_PIN, (BitAction)Bitvalue);
    Delay_us(IIC_STDMODE_DELAY);
}

/// @brief 写SDA
/// @param Bitvalue 写入的数据（0或1）
static void IIC_W_SDA(uint8_t Bitvalue)
{
    GPIO_WriteBit(IIC_GPIOx, IIC_SDA_PIN, (BitAction)Bitvalue);
    Delay_us(IIC_STDMODE_DELAY);
}

/// @brief 读SDA
/// @param  无
/// @return 读出的数据
static uint8_t IIC_RD_SDA(void)
{
    uint8_t Bitvalue;
    // Delay_us(IIC_STDMODE_DELAY);
    Bitvalue = GPIO_ReadInputDataBit(IIC_GPIOx, IIC_SDA_PIN);
    Delay_us(IIC_STDMODE_DELAY);
    return Bitvalue;
}

// 起始条件
void IIC_Start(void)
{
    // 1、起始条件要求SDA由高拉低，但上一个SCL周期结束后SDA很可能是低（因为ACK为低）
    // 2、因此，为了实现"重复起始条件"的兼容，在SCL低时（除了结束条件，所有单元结束时SCL均为低），
    //    将SDA先拉高，此后再将SCL拉高 + SDA拉低
    IIC_W_SDA(IIC_HIGH);
    IIC_W_SCL(IIC_HIGH);
    IIC_W_SDA(IIC_LOW);
    IIC_W_SCL(IIC_LOW);
}

// 终止条件
void IIC_Stop(void)
{
    IIC_W_SCL(IIC_LOW); // 保证SCL为低
    IIC_W_SDA(IIC_LOW); // 保证SDA为低
    IIC_W_SCL(IIC_HIGH);
    IIC_W_SDA(IIC_HIGH);
}

/// @brief 发送一个字节
/// @param Byte 要发送的字节数据
void IIC_SendByte(uint8_t Byte)
{
    for (int i = 0; i < 8; i++) {
        IIC_W_SCL(IIC_LOW);                     // 拉低SCL
        IIC_W_SDA(READ_BIT(Byte, (0x80 >> i))); // 高位先行
        // IIC_W_SDA(Byte & (0x80 >> i)); // 高位先行
        IIC_W_SCL(IIC_HIGH);
        IIC_W_SCL(IIC_LOW);
    }
    IIC_W_SDA(IIC_HIGH); // 释放SDA
}

/// @brief 接收一个字节
/// @param  无
/// @return 接收到的字节数据
uint8_t IIC_ReceiveByte(void)
{
    uint8_t Byte = 0x00;
    IIC_W_SCL(IIC_LOW);  // 拉低SCL
    IIC_W_SDA(IIC_HIGH); // 释放SDA
    for (int i = 0; i < 8; i++) {
        IIC_W_SCL(IIC_HIGH);
        if (IIC_RD_SDA() == IIC_HIGH) {
            SET_BIT(Byte, (0x80 >> i));
            // Byte |= (0x80 >> i);
        }
        IIC_W_SCL(IIC_LOW);
    }
    return Byte;
}

/// @brief 发送应答
/// @param AckBit 要发送的应答
void IIC_SendAck(uint8_t AckBit)
{
    IIC_W_SDA(AckBit);
    IIC_W_SCL(IIC_HIGH);
    IIC_W_SCL(IIC_LOW);
    IIC_W_SDA(IIC_HIGH); // 应答之后接收器必须释放SDA，否则当发送器想发送1时无法拉高SDA
}

/// @brief 接收应答
/// @param  无
/// @return 接收到的应答
uint8_t IIC_ReceiveAck(int16_t timeout)
{
    // uint8_t AckBit = 0x00;
    IIC_W_SDA(IIC_HIGH); // 主机释放SDA

    while ((timeout > 0) && (IIC_RD_SDA()) != IIC_LOW) {
        timeout--;
        Delay_us(10);
    }
    IIC_W_SCL(IIC_HIGH);

    // 超时，或在SCL高电平期间SDA上的应答信号不稳定，则返回非应答
    if ((timeout <= 0) || (IIC_RD_SDA() != IIC_LOW)) {
        IIC_W_SCL(IIC_LOW);
        // LOG("IIC ACK timeout\r\n");
        return IIC_NOACK;
    }

    IIC_W_SCL(IIC_LOW);
    return IIC_ACK;
}
