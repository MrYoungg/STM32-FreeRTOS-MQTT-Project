#include "stm32f10x.h" // Device header

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include "Delay.h"
#include "AT_USART.h"
#include "Key.h"
#include "LED.h"
#include "Sensor.h"

#include "ATCommand.h"
#include "Debug_USART.h"
#include "RingBuffer.h"
#include "WiFiConnect.h"
#include "MQTTClient.h"
#include "JSON.h"

TaskHandle_t Task_Test_Handle;
TaskHandle_t Task_ATSend_Handle;
TaskHandle_t Task_ATReceive_Handle;
TaskHandle_t Task_MQTTDataRcv_Handle;
TaskHandle_t Task_MQTTDataSend_Handle;

void Task_Test(void *parameter);
void Task_ATConnect(void *parameter);
void Task_ATResponseRcv(void *parameter);
void Task_MQTTDataRcv(void *parameter);
void Task_MQTTDataSend(void *parameter);

int main(void)
{
    // 裸机调试时必须手动初始化Systick,否则无法使用Delay函数
    // 同时需要注释FreeRTOS内部的SystickHandler,否则无法正常延时
    // Systick_Init();
    Key_Init();
    LED_Init();
    Sensor_AD_Init();
    Debug_USART_Init();
    AT_Init();
    DEBUG_LOG("reset and init...\r\n");

    taskENTER_CRITICAL(); // 进入临界区,关中断
    // xTaskCreate(Task_Test, "Task_Test", 128, NULL, 1, &Task_Test_Handle);
    xTaskCreate(Task_ATConnect, "Task_ATConnect", 256, NULL, 1, &Task_ATSend_Handle);
    xTaskCreate(Task_ATResponseRcv, "Task_ATResponseRcv", 512, NULL, 3, &Task_ATReceive_Handle);
    xTaskCreate(Task_MQTTDataRcv, "Task_MQTTDataRcv", 1024, NULL, 2, &Task_MQTTDataRcv_Handle);

    taskEXIT_CRITICAL(); // 退出临界区,开中断

    vTaskStartScheduler();

    while (1) {
    }
}

// AT连接线程
void Task_ATConnect(void *parameter)
{
    int ret;
    WiFi_t wifiMsg;
    extern MQTTClient_t mqttClient;
    LED_Blink();
    while (1) {
        DEBUG_LOG("connect task\r\n");
        AT_Reset();
        Delay_ms(500);

        // 1、连接WiFi
    wifiReconnect:
        WiFi_Config(&wifiMsg);
        ret = WiFi_Connect(&wifiMsg);
        if (ret != AT_OK) {
            DEBUG_LOG("wifi connect failed\r\n");
            // 尝试重连
            Delay_ms(500);
            // goto wifiReconnect;
        }

        // 2、连接服务器
    mqttReconnect:
        MQTT_ClientConfig(&mqttClient);
        ret = MQTT_Connect(&mqttClient);

        if (ret != AT_OK) {
            DEBUG_LOG("mqtt connect failed\r\n");
            // 尝试重连
            Delay_ms(500);
            // goto mqttReconnect;
            while (1);
        }

        taskENTER_CRITICAL();
        xTaskCreate(
            Task_MQTTDataSend, "Task_MQTTDataSend", 512, NULL, 1, &Task_MQTTDataSend_Handle);
        taskEXIT_CRITICAL();

        vTaskSuspend(NULL);
    }
}

// AT响应读取线程
void Task_ATResponseRcv(void *parameter)
{
    while (1) {
        DEBUG_LOG("receive task\r\n");
        AT_ReceiveResponse();
    }
}

// MQTT数据读取线程
void Task_MQTTDataRcv(void *parameter)
{
    while (1) {
        DEBUG_LOG("data read task\r\n");

        char jsonBuf[AT_DATA_PACKET_SIZE];
        int len = sizeof(jsonBuf);

        // 1、读取服务器下发的JSON数据
        AT_Read_DataPacketBuffer(jsonBuf, (int)len, (int)portMAX_DELAY);
        DEBUG_LOG("data from server:%s\r\n", jsonBuf);

        // 2、解析JSON数据（获取阿里云下发的param对象）
        JSON_t *itemHead = NULL;
        JSON_GetParams(jsonBuf, &itemHead);
        // 打印链表数据
        JSON_PrintItemList(itemHead);

        // 3、根据对象参数执行相应的操作

        // 3.1 开灯（喂水、喂粮等操作本质一样）
        JSON_t *item = isInItemList(itemHead, "LightSwitch");
        if (item != NULL) {
            int lightSwitch = getItemValueNumber(item);
            if (lightSwitch)
                LED_ON;
            else
                LED_OFF;
        }

        // 3.2 设定时间

        // 4、释放链表内存
        // JSON_Free(itemHead);
    }
}

// MQTT数据上报+PING线程, 检查连接(AT+MQTTCONN?)
void Task_MQTTDataSend(void *parameter)
{
    extern SensorData_t sensorData;
    while (1) {
        sensorData.light = LED_STATUS;
        DEBUG_LOG("food sensor: %d\r\n", sensorData.food);
        DEBUG_LOG("water sensor: %d\r\n", sensorData.water);
        DEBUG_LOG("light sensor: %d\r\n", sensorData.light);
        // Sensor_DataNormalize(sensorData);
        MQTT_Publish(sensorData);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void Task_OTA(void *parameter) {}

#if 0
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
#endif
