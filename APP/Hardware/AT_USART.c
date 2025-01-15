#include "AT_USART.h"
#include <string.h>
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "Debug_USART.h"
#include "Delay.h"
#include "RingBuffer.h"
#include "AT.h"

/*AT命令串口 - AT_USARTx*/
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

volatile static RingBuffer_t USART_RingBuffer;
volatile static int USART_RxFlag;
volatile static int ORE_Flag;

// volatile static SemaphoreHandle_t USART_Receive_Mutex;

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
    // 开启USART接收中断 - 串口空闲触发中断
    USART_ITConfig(AT_USARTx, USART_IT_IDLE, ENABLE);
    // 开启串口溢出中断
    // USART_ITConfig(AT_USARTx, USART_IT_ORE, ENABLE);
    // 开启NVIC
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = AT_USARTx_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);

    // 使能串口，会使TX口输出高电平(表示空闲状态)
    USART_Cmd(AT_USARTx, ENABLE);

    // 初始化串口环形缓冲区
    RingBuffer_Init((RingBuffer_t *)&USART_RingBuffer);

    // 初始化串口接收mutex
    // platform_mutex_init(&USART_Receive_Mutex);
    // USART_Receive_Mutex = xSemaphoreCreateMutex();
    // xSemaphoreTake(USART_Receive_Mutex, 0);
}

/// @brief AT命令串口发送一个字节数据
/// @param Byte 待发送数据
static void AT_USART_SendByte(uint8_t Data)
{
    USART_SendData(AT_USARTx, Data);
    // 死等发送
    while (!USART_GetFlagStatus(AT_USARTx, USART_FLAG_TXE)); // 等待发送数据寄存器空
    while (!USART_GetFlagStatus(AT_USARTx, USART_FLAG_TC));  // 等待发送完成
}

/// @brief AT命令串口发送数据包
/// @param Buf 数据包
/// @param BufLen 数据包长度
void AT_USART_SendDataBuf(char *Buf, uint16_t BufLen)
{
    for (uint32_t i = 0; i < BufLen; i++) {
        AT_USART_SendByte(Buf[i]);
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

/// @brief 等待AT串口接收信息
/// @param buf 存放接收到的数据
/// @param bufLen buf长度
void AT_USART_WaitForInput(char *buf, size_t bufLen)
{
    while (is_FCBList_Empty((RingBuffer_t *)&USART_RingBuffer));
    memset(buf, 0, bufLen);
    RingBuffer_ReadFrame((RingBuffer_t *)&USART_RingBuffer, (uint8_t *)buf, bufLen);
}

/// @brief 读取串口环形缓冲区的数据
/// @param timeout 超时时间
/// @return 数据接收状态，RX_BUFFER_OVERFLOW-缓冲区溢出，RX_DATA_RECEIVED-成功接收到数据
int USART_Read_Buffer(TickType_t timeout)
{
    // 1、数据帧链表空,则等待在接收事件标志位RECV_EVENTBIT上
    while (is_RingBuffer_Empty((RingBuffer_t *)&USART_RingBuffer)) {
        DEBUG_LOG("Wait for RECV_EVENTBIT\r\n");
        xEventGroupWaitBits(AT_EventGroup, RECV_EVENTBIT, pdTRUE, pdFALSE, timeout);
        DEBUG_LOG("RECV_EVENTBIT being set\r\n");
    }

    // 2、缓冲区发生接收溢出,数据不完整,直接放弃本次接收的数据
    if (USART_RxFlag == RX_BUFFER_OVERFLOW) {
        DEBUG_LOG("USART_RingBuffer overflow, clear the data received this time\r\n");
    }
    // 3、缓冲区没有溢出,其中有完整数据,则读取到responseBuffer中
    else if (USART_RxFlag == RX_RECV_SUCCESE) {
        // 读一帧数据到responseBuffer
        // DEBUG_LOG("Save a frame to AT_ResponseBuffer\r\n");
        memset(AT_ResponseBuffer, 0, sizeof(AT_ResponseBuffer));
        RingBuffer_ReadFrame(
            (RingBuffer_t *)&USART_RingBuffer, (uint8_t *)AT_ResponseBuffer, sizeof(AT_ResponseBuffer));
    }
    else {
        DEBUG_LOG("USART_RxFlag else\r\n");
    }

    return USART_RxFlag;
}

/// @brief 接收中断处理,将接收到的数据写入环形缓冲区
/// @param 无
void USART_Write_Buffer(void)
{
#if 0
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
#endif

    static uint16_t frameSize = 0;
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 1、RXNE中断：将数据逐个写入缓冲区（频繁进中断，考虑用DMA代替）
    // 在IDLE中断中，通过 DMA_GetCurrDataCounter() 计算接收到的串口数据帧长度
    if (USART_GetITStatus(AT_USARTx, USART_IT_RXNE) == SET) {
        // 1、读DR，清除RXNE标志位
        uint8_t recvData = (uint8_t)USART_ReceiveData(AT_USARTx);
        // 2、将单个数据写入环形缓冲区
        if ((frameSize < MAX_USART_FRAME_SIZE) && !(is_RingBuffer_Full((RingBuffer_t *)&USART_RingBuffer))) {
            Write_RingBuffer((RingBuffer_t *)&USART_RingBuffer, &recvData, 1);
            frameSize++;
        }
        else {
            USART_RxFlag = RX_BUFFER_OVERFLOW;
        }
    }

    // 2、IDLE中断：将一帧数据的属性添加到FCB链表中，唤醒串口接收线程
    if (USART_GetITStatus(AT_USARTx, USART_IT_IDLE) == SET) {
        // 1、先读SR，后读DR，清除IDLE标志位
        USART_ReceiveData(AT_USARTx);
        // 2、如果数据没有丢失，则更新FCB链表(插入新链表项)
        if (USART_RxFlag != RX_BUFFER_OVERFLOW) {
            Update_FCBList((RingBuffer_t *)&USART_RingBuffer, frameSize);
            USART_RxFlag = RX_RECV_SUCCESE;
        }
        frameSize = 0;

        // 3、唤醒串口接收线程
        xResult = xEventGroupSetBitsFromISR(AT_EventGroup, RECV_EVENTBIT, &xHigherPriorityTaskWoken);
        if (xResult != pdFAIL) portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
