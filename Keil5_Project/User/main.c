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
TaskHandle_t Task_ATDataRead_Handle;

void Task_Test(void *parameter);
void Task_ATConnect(void *parameter);
void Task_ATReceive(void *parameter);
void Task_ATDataRead(void *parameter);

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
    xTaskCreate(Task_ATConnect, "Task_ATConnect", 128, NULL, 1, &Task_ATSend_Handle);
    xTaskCreate(Task_ATReceive, "Task_ATReceive", 256, NULL, 2, &Task_ATReceive_Handle);
    xTaskCreate(Task_ATDataRead, "Task_ATDataRead", 256, NULL, 2, &Task_ATDataRead_Handle);

    taskEXIT_CRITICAL(); // 退出临界区,开中断

    vTaskStartScheduler();

    while (1) {
    }
}

// AT连接线程，每隔一段时间检查连接
void Task_ATConnect(void *parameter)
{
    while (1) {
        int ret;
        DEBUG_LOG("connect task\r\n");
        
        char *host = "192.168.16.74";
        char *port = "8266";
        int proto = PLATFORM_NET_PROTO_TCP;

        // 1、连接服务器
        ret = platform_net_socket_connect(host, port, proto);
        if (ret != MQTT_SUCCESS_ERROR) {
            DEBUG_LOG("connect failed\r\n");
            while (1);
        }

        // 2、每隔一段时间检查连接（心跳包）
        // vTaskDelay(pdMS_TO_TICKS(4000));

        vTaskDelay(portMAX_DELAY);
    }
}

void Task_ATReceive(void *parameter)
{
    while (1) {
        DEBUG_LOG("receive task\r\n");
        AT_Receive();
    }
}

void Task_ATDataRead(void *parameter)
{
    while (1) {
        DEBUG_LOG("data read task\r\n");

        char buf[32];
        memset(buf, 0, sizeof(buf));

        int len = sizeof(buf);

        AT_Read_DataPacketBuffer(buf, len, portMAX_DELAY);
        DEBUG_LOG("data from server:%s\r\n", buf);
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
