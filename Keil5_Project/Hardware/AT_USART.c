#include "AT_USART.h"

/*AT命令串口 - USARTx*/
#define AT_USART_GPIOx_APBxCLOCKCMD(APBx_GPIOx) RCC_APB2PeriphClockCmd((APBx_GPIOx), ENABLE)
#define AT_USART_APBx_GPIOx                     RCC_APB2Periph_GPIOB
#define AT_USART_GPIOx                          GPIOB
#define AT_USART_TX_PIN                         GPIO_Pin_10
#define AT_USART_RX_PIN                         GPIO_Pin_11

#define AT_USARTx_APBxCLOCKCMD(APBx_USARTx) RCC_APB1PeriphClockCmd((APBx_USARTx), ENABLE)
#define AT_APBx_USARTx                      RCC_APB1Periph_USART3
#define AT_USATRx_BAUDRATE                  115200
#define AT_USARTx                           USART3
#define AT_USARTx_IRQn                      USART3_IRQn
#define USART_Write_Buffer                  USART3_IRQHandler

volatile static RingBuffer_t USART_Buffer;
volatile static int Serial_rxFlag;
volatile static int ORE_Flag;

static platform_mutex_t USART_Receive_Mutex;

/// @brief 初始化AT串口
/// @param 无
void AT_USART_Init(void)
{
    // 开启GPIO口时钟
    AT_USART_GPIOx_APBxCLOCKCMD(AT_USART_APBx_GPIOx);
    // 开启USART时钟,在此处先开启时钟,则串口初始化时不会发送0xFF或0x00
    AT_USARTx_APBxCLOCKCMD(AT_APBx_USARTx);

    // 开启GPIO口
    GPIO_InitTypeDef GPIO_InitStructure;
    // TX配置为复用推挽输出模式,将GPIO的控制权交给复用外设
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = AT_USART_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    // init前,该端子由8266RX的上拉电阻提供一点电压,init后被STM32的TX下拉为低
    GPIO_Init(AT_USART_GPIOx, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = AT_USART_RX_PIN; // RX配置为上拉输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(AT_USART_GPIOx, &GPIO_InitStructure);

    // 开启AT命令串口
    // 在初始化GPIO_TX和初始化USART时钟之间加延时,串口改为发送0x00
    // for(int i = 0; i < 1000; i++);
    // 在此处开启时钟会让串口发送0xFF/0x00
    // AT_USARTx_APBxCLOCKCMD(AT_APBx_USARTx);
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = AT_USATRx_BAUDRATE;                        // 设置波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 是否开启硬件流控制
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                 // 选择发送/接收
    USART_InitStructure.USART_Parity = USART_Parity_No;                             // 选择校验位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;                          // 选择停止位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;                     // 选择数据帧长度
    USART_Init(AT_USARTx, &USART_InitStructure);

    // 开启USART接收中断 - 接收寄存器非空触发中断
    USART_ITConfig(AT_USARTx, USART_IT_RXNE, ENABLE);
    // 开启串口溢出中断
    USART_ITConfig(AT_USARTx, USART_IT_ORE, ENABLE);
    // 开启NVIC
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = AT_USARTx_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);

    // 使能串口，会使TX口输出高电平(表示空闲状态)
    USART_Cmd(AT_USARTx, ENABLE);

    // 初始化串口环形缓冲区
    RingBuffer_Init((RingBuffer_t *)&USART_Buffer, USART_BUFFER_SIZE);

    // 初始化串口接收mutex
    platform_mutex_init(&USART_Receive_Mutex);
    platform_mutex_lock_timeout(&USART_Receive_Mutex, 0);
}

/// @brief AT命令串口发送一个字节数据
/// @param Byte 待发送数据
static void AT_USART_SendByte(uint8_t Data)
{
    USART_SendData(AT_USARTx, Data);
    // 死等发送
    while (!USART_GetFlagStatus(AT_USARTx, USART_FLAG_TXE)) // 等待发送数据寄存器空
    {
    }
}

/// @brief AT命令串口发送一个字符串
/// @param String 待发送字符串
void AT_USART_SendString(char *String)
{
    for (uint8_t i = 0; String[i] != 0; i++) {
        AT_USART_SendByte(String[i]);
    }
}

/// @brief 读取串口环形缓冲区的数据
/// @param responseBuffer 想要读取的环形缓冲区
/// @param timeout 超时时间
/// @return 数据接收状态，RX_BUFFER_OVERFLOW-缓冲区溢出，RX_DATA_RECEIVED-成功接收到数据
int USART_Read_Buffer(uint8_t *responseBuffer, uint32_t responseBufferLen, TickType_t timeout)
{
    while (USART_Buffer.dataSize == 0) {
        // 缓冲区空,则在USART_Receive_Mutex上阻塞,直到接收到数据
        // DEBUG_LOG("receive mutex lock\r\n");
        platform_mutex_lock_timeout(&USART_Receive_Mutex, timeout);
        vTaskDelay(pdMS_TO_TICKS(100));
        // vTaskDelay(pdMS_TO_TICKS(20));
        // DEBUG_LOG("receive mutex unlock\r\n");
    }
    // DEBUG_LOG("dataSize = %d\r\n", USART_Buffer.dataSize);

    // 缓冲区溢出,数据不完整,直接放弃本次写入的数据,并清空缓冲区
    if (Serial_rxFlag == RX_BUFFER_OVERFLOW) {
        DEBUG_LOG("USART_Buffer overflow, clear the data receive this time\r\n");
        USART_Buffer.dataSize = 0;
        USART_Buffer.readIndex = USART_Buffer.writeIndex;
    }
    // 缓冲区没有溢出,其中有完整数据,则读取到responseBuffer中
    else if (Serial_rxFlag == RX_DATA_RECEIVED) {
        // DEBUG_LOG("USART_Buffer->responseBuffer\r\n");

        // 读环形缓冲区
        Read_RingBuffer((RingBuffer_t *)&USART_Buffer, responseBuffer, USART_Buffer.dataSize, responseBufferLen);
        // for (int i = 0; i < USART_Buffer.dataSize; i++) {
        //     USART_Buffer.readIndex = (USART_Buffer.readIndex + 1) % USART_BUFFER_SIZE;
        //     responseBuffer[i] = USART_Buffer.bufferHead[USART_Buffer.readIndex];
        // }

        // // responseBuffer[USART_Buffer.dataSize] = '\0';
        // USART_Buffer.dataSize = 0;
    }
    else {
        DEBUG_LOG("Serial_rxFlag else\r\n");
    }

    return Serial_rxFlag;
}

/// @brief 接收中断处理,将接收到的数据写入环形缓冲区
/// @param 无
void USART_Write_Buffer(void)
{
    // 清理：ORE - 溢出错误
    if (USART_GetITStatus(AT_USARTx, USART_IT_ORE)) {
        DEBUG_LOG("ORE IT\r\n");
        ORE_Flag = 1;
        USART_ReceiveData(AT_USARTx); // 清除ORE_Flag
        if (!USART_GetITStatus(AT_USARTx, USART_IT_ORE)) {
            DEBUG_LOG("ORE IT cleared\r\n");
            ORE_Flag = 0;
        }
    }

    if (USART_GetITStatus(AT_USARTx, USART_IT_RXNE)) {
        // DEBUG_LOG("RXNE Interrupt\r\n");
        volatile uint8_t ret;
        volatile uint8_t receiveData;

        // 串口收到数据,唤醒USART_Receive_Mutex
        // platform_mutex_unlock(&USART_Receive_Mutex);
        platform_mutex_unlock_from_isr(&USART_Receive_Mutex);

        receiveData = USART_ReceiveData(AT_USARTx);

        // 写环形缓冲区
        ret = Write_RingBuffer((RingBuffer_t *)&USART_Buffer, (uint8_t *)&receiveData, 1);

        if (ret == pdPASS)
            Serial_rxFlag = RX_DATA_RECEIVED;
        else
            Serial_rxFlag = RX_BUFFER_OVERFLOW;
    }
}
