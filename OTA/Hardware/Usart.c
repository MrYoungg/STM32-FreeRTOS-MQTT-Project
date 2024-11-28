#include "Usart.h"

#define CMD_USARTx            USART2
#define CMD_USART_GPIOx       GPIOA
#define CMD_USART_TX_PIN      GPIO_Pin_2
#define CMD_USART_RX_PIN      GPIO_Pin_3
#define CMD_USART_BAUD_RATE   115200
#define CMD_USARTx_IRQn       USART2_IRQn
#define CMD_USARTx_IRQHandler USART2_IRQHandler

static void CMD_USART_Init(void);
RingBuffer_t CMD_USART_Buffer;

void MyUSART_Init(void)
{
    CMD_USART_Init();
    RingBuffer_Init(&CMD_USART_Buffer);
}

static void CMD_USART_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = CMD_USART_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CMD_USART_GPIOx, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = CMD_USART_RX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CMD_USART_GPIOx, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = CMD_USART_BAUD_RATE;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(CMD_USARTx, &USART_InitStructure);

    USART_ITConfig(CMD_USARTx, USART_IT_IDLE, ENABLE);
    USART_ITConfig(CMD_USARTx, USART_IT_RXNE, ENABLE);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = CMD_USARTx_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(CMD_USARTx, ENABLE);
}

/// @brief CMD串口发送一个字节数据
/// @param Byte 待发送数据
static void CMD_USART_SendByte(uint8_t Data)
{
    USART_SendData(CMD_USARTx, Data);
    // 死等发送
    while (!USART_GetFlagStatus(CMD_USARTx, USART_FLAG_TXE)) // 等待发送数据寄存器空
    {
    }
}

/// @brief CMD串口发送一个字符串
/// @param String 待发送字符串
static void CMD_USART_SendString(char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++) {
        CMD_USART_SendByte(String[i]);
    }
}

// 定义串口Printf函数
void CMD_USART_Printf(char *format, ...)
{
    char String[256];
    va_list arg;

    va_start(arg, format);
    vsnprintf(String, sizeof(String), format, arg);
    va_end(arg);
    CMD_USART_SendString(String);
}

void CMD_USARTx_IRQHandler(void)
{
    static uint16_t frameSize = 0;
    static int ret = 0;
    // 1、RXNE中断：将数据逐个写入缓冲区
    if (USART_GetITStatus(CMD_USARTx, USART_IT_RXNE) == SET) {
        // 1、读DR，清除RXNE标志位
        uint8_t recvData = (uint8_t)USART_ReceiveData(CMD_USARTx);
        // 2、将数据写入环形缓冲区
        if (frameSize <= MAX_USART_FRAME_SIZE) {
            ret = Write_RingBuffer(&CMD_USART_Buffer, &recvData, 1);
            frameSize++;
        }
    }

    // 2、IDLE中断：将这帧数据的属性添加到FCB链表中
    if (USART_GetITStatus(CMD_USARTx, USART_IT_IDLE) == SET) {
        // 1、先读SR，后读DR，清除IDLE标志位
        USART_ReceiveData(CMD_USARTx);
        // 2、如果数据没有丢失，则更新FCB链表(插入新链表项)
        if (ret != 0) {
            Update_FCBList(&CMD_USART_Buffer, frameSize);
            ret = 0;
        }
        frameSize = 0;
    }
}
