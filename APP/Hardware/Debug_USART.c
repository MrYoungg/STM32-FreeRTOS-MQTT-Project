#include "Debug_USART.h"
#include "AT_USART.h"
/*Debug串口 - USART3*/
#define APBx_DEBUG_USART_GPIOx_CLOCK_CMD(APBx_GPIOx) RCC_APB2PeriphClockCmd((APBx_GPIOx), ENABLE)
#define APBx_DEBUG_USARTx_CLOCK_CMD(APBx_USARTx)     RCC_APB1PeriphClockCmd((APBx_USARTx), ENABLE)
#define APBx_DEBUG_USART_GPIOx                       RCC_APB2Periph_GPIOA
#define APBx_DEBUG_USARTx                            RCC_APB1Periph_USART2

#define DEBUG_USARTx         USART2
#define DEBUG_USART_GPIOx    GPIOA
#define DEBUG_USART_TX_PIN   GPIO_Pin_2
#define DEBUG_USART_RX_PIN   GPIO_Pin_3
#define DEBUG_USATR_BAUDRATE 115200

void Debug_USART_Init(void)
{
    // 开启GPIO时钟
    APBx_DEBUG_USART_GPIOx_CLOCK_CMD(APBx_DEBUG_USART_GPIOx);
    // 开启USART时钟
    APBx_DEBUG_USARTx_CLOCK_CMD(APBx_DEBUG_USARTx);

    // 初始化GPIO
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DEBUG_USART_GPIOx, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DEBUG_USART_GPIOx, &GPIO_InitStructure);

    // 初始化USART
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = DEBUG_USATR_BAUDRATE;                      // 设置波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 是否开启硬件流控制
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                 // 发送+接收
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 选择校验位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 选择停止位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 选择数据帧长度
    USART_Init(DEBUG_USARTx, &USART_InitStructure);

    // 使能USART
    USART_Cmd(DEBUG_USARTx, ENABLE);
}

/// @brief Debug串口发送一个字节数据
/// @param Byte 待发送数据
static void Debug_USART_SendByte(uint8_t Data)
{
    USART_SendData(DEBUG_USARTx, Data);
    // 死等发送
    while (!USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_TXE)); // 等待发送数据寄存器空
}

/// @brief Debug串口发送一个字符串
/// @param String 待发送字符串
static void Debug_USART_SendString(char *String)
{
    for (uint32_t i = 0; String[i] != '\0'; i++) {
        Debug_USART_SendByte(String[i]);
    }
}

// 定义串口Printf函数
void Debug_USART_Printf(char *format, ...)
{
    char String[MAX_USART_FRAME_SIZE];
    va_list arg;

    va_start(arg, format);
    vsnprintf(String, sizeof(String), format, arg);
    va_end(arg);
    Debug_USART_SendString(String);
}

#if 1
// 改写fputc函数
int fputc(int ch, FILE *f)
{
    // 尽管fputc传入整型变量ch作为参数，但事实上它从printf接收的是char变量；
    // 早期C语言中会用int替代很多类型，因为int是当时最大的数据类型，兼容性更好
    Debug_USART_SendByte(ch); // 通过串口发送一个字节数据
    return ch;
}
#endif
