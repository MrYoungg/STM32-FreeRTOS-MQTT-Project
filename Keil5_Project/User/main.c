#include "stm32f10x.h" // Device header

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include "Delay.h"
#include "AT_USART.h"
#include "Key.h"
#include "ATCommand.h"
#include "Debug_USART.h"
#include "RingBuffer.h"

#include "platform_net_socket.h"

TaskHandle_t Task_Test_Handle;
TaskHandle_t Task_ATSend_Handle;
TaskHandle_t Task_ATReceive_Handle;
TaskHandle_t Task_ATDataProcess_Handle;

void Task_Test(void *parameter);
void Task_ATSend(void *parameter);
void Task_ATReceive(void *parameter);
void Task_ATDataProcess(void *parameter);

int main(void)
{
    // 裸机调试时必须手动初始化Systick,否则无法使用Delay函数
    // 同时需要注释FreeRTOS内部的SystickHandler,否则无法正常延时
    // Systick_Init();
    Key_Init();
    Debug_USART_Init();
    AT_Init();

    DEBUG_LOG("reset and init...\r\n");

    taskENTER_CRITICAL(); // 进入临界区,关中断
    // xTaskCreate(Task_Test, "Task_Test", 128, NULL, 1, &Task_Test_Handle);
    xTaskCreate(Task_ATSend, "Task_ATSend", 128, NULL, 1, &Task_ATSend_Handle);
    xTaskCreate(Task_ATReceive, "Task_ATReceive", 256, NULL, 2, &Task_ATReceive_Handle);
    // xTaskCreate(Task_ATDataProcess, "Task_ATDataProcess", 512, NULL, 2, &Task_ATDataProcess_Handle);

    taskEXIT_CRITICAL(); // 退出临界区,开中断

    vTaskStartScheduler();

    while (1) {
    }
}

void Task_ATSend(void *parameter)
{
    while (1) {
        int ret;
        // DEBUG_LOG("send task\r\n");
        char *proto = "TCP";
        char *host = "192.168.16.74";
        int port = 8266;

        ret = platform_net_socket_connect(proto, host, port);
        if (!ret) {
            DEBUG_LOG("connect failed\r\n");
            while (1);
        }

        // platform_net_socket_write(NULL, NULL, NULL);

        vTaskDelay(portMAX_DELAY);
    }
}

void Task_ATReceive(void *parameter)
{
    while (1) {
        // DEBUG_LOG("receive task\r\n");
        AT_ReceiveResponse();
    }
}

void Task_ATDataProcess(void *parameter)
{
    while (1) {
        AT_ProcessData();
    }
}

void Task_Test(void *parameter)
{
    while (1) {
        DEBUG_LOG("test task:start\r\n");
        int err;

        // RingBuffer_t testBufferOld;
        // err = RingBuffer_Init_Old(&testBufferOld, 10, NULL);

        RingBuffer_t testBuffer;
        err = RingBuffer_Init(&testBuffer, 10);
#if 1
        if (!err) {
            DEBUG_LOG("test task:failed to init ring buffer\r\n");
        }

        DEBUG_LOG("test task:init ring buffer successfully\r\n");
        if (is_RingBuffer_Empty(&testBuffer)) {
            DEBUG_LOG("test task:ring buffer empty\r\n");
        }

        int size = testBuffer.bufferSize;
        DEBUG_LOG("testBuffer size:%d\r\n", size);

        uint8_t data[] = {"123456789"};
        DEBUG_LOG("sizeof(data):%d\r\n", sizeof(data));
        Write_RingBuffer(&testBuffer, data, sizeof(data));

        if (is_RingBuffer_Full(&testBuffer)) {
            DEBUG_LOG("test task:ring buffer full\r\n");
        }

        uint8_t recvBuffer1[10];
        err = Read_RingBuffer(&testBuffer, recvBuffer1, 10, 10);
        DEBUG_LOG("recvBuffer1:%s\r\n", recvBuffer1);

        uint8_t recvBuffer2[10];
        err = Read_RingBuffer(&testBuffer, recvBuffer2, 5, 10);
        DEBUG_LOG("recvBuffer2:%s\r\n", recvBuffer2);
#endif
        vTaskDelay(portMAX_DELAY);
    }
}