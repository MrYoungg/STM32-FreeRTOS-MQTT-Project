#include "stm32f10x.h" // Device header

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include "Delay.h"
#include "AT_USART.h"
#include "Device.h"
#include "Sensor.h"
#include "Debug_USART.h"
#include "W25Q64.h"
#include "AT24C02.h"

#include "AT.h"
#include "RingBuffer.h"
#include "WiFiConnect.h"
#include "MQTTClient.h"
#include "JSON.h"
#include "OTA.h"

TaskHandle_t Task_Test_Handle;
TaskHandle_t Task_MQTTConnect_Handle;
TaskHandle_t Task_MQTTDataPost_Handle;
TaskHandle_t Task_OnlineDeviceCtrl_Handle;
TaskHandle_t Task_LocalDeviceCtrl_Handle;
TaskHandle_t Task_OTA_Handle;
TaskHandle_t Task_Ping_Handle;
TaskHandle_t Task_Receive_Handle;

QueueHandle_t DataPost_Semphore;

void Task_Test(void *parameter);
void Task_MQTTConnect(void *parameter);
void Task_MQTTDataPost(void *parameter);
void Task_OnlineDeviceCtrl(void *parameter);
void Task_LocalDeviceCtrl(void *parameter);
void Task_OTA(void *parameter);
void Task_Ping(void *parameter);
void Task_Receive(void *parameter);

int main(void)
{
    // 裸机调试时必须手动初始化Systick,否则无法使用Delay函数
    // 同时需要注释FreeRTOS内部的SystickHandler,否则无法正常延时
    // Systick_Init();
    Device_Init();
    Sensor_AD_Init();
    Debug_USART_Init();
    AT_Init();
    W25Q64_Init();
    AT24C02_Init();

    DEBUG_LOG("reset and init...\r\n");

    taskENTER_CRITICAL(); // 进入临界区,关中断

    // xTaskCreate(Task_Test, NULL, 128, NULL, 1, &Task_Test_Handle);
    xTaskCreate(Task_MQTTConnect, NULL, 512, NULL, 1, &Task_MQTTConnect_Handle);
    xTaskCreate(Task_MQTTDataPost, NULL, 512, NULL, 2, &Task_MQTTDataPost_Handle);
    xTaskCreate(Task_OnlineDeviceCtrl, NULL, 512, NULL, 3, &Task_OnlineDeviceCtrl_Handle);
    xTaskCreate(Task_LocalDeviceCtrl, NULL, 32, NULL, 3, &Task_LocalDeviceCtrl_Handle);
    xTaskCreate(Task_OTA, NULL, 1024, NULL, 4, &Task_OTA_Handle);
    // xTaskCreate(Task_Ping, NULL, 128, NULL, 5, &Task_Ping_Handle);
    xTaskCreate(Task_Receive, NULL, 512, NULL, 6, &Task_Receive_Handle);

    taskEXIT_CRITICAL(); // 退出临界区,开中断

    vTaskStartScheduler();

    while (1);
}

// 线程1：创建MQTT连接
void Task_MQTTConnect(void *parameter)
{
    int ret;
    WiFi_t wifiMsg;
    extern MQTTClient_t mqttClient;

    // 先读取AT24C02中的数据
    Read_SavingOTAInfo();
    Light_Blink();

    while (1) {
        DEBUG_LOG("MQTT连接线程 \r\n");

        // 等待断开连接事件标志位
        while (mqttClient.connectState != MQTT_DISCONNECTED) {
            xEventGroupWaitBits(AT_EventGroup, DISCONNECT_EVENTBIT, pdTRUE, pdFALSE, portMAX_DELAY);
        }

        AT_Reset();
        Delay_ms(200);

        // 1、连接WiFi
    wifiReconnect:
        DEBUG_LOG("配置WiFi信息并连接WiFi \r\n");
        // 加入用户配置功能
        WiFi_Config(&wifiMsg);
        ret = WiFi_Connect(&wifiMsg);
        if (ret != AT_OK) {
            DEBUG_LOG("WiFi连接失败,尝试重连 \r\n");
            // 尝试重连
            Delay_s(3);
            goto wifiReconnect;
        }

        // 2、连接MQTT服务器
    mqttReconnect:
        DEBUG_LOG("配置信息并连接MQTT服务器 \r\n");
        MQTT_ClientConfig(&mqttClient);
        ret = MQTT_ConnectServer(&mqttClient);

        if (ret != AT_OK) {
            DEBUG_LOG("MQTT服务器连接失败,尝试重连 \r\n");
            // 尝试重连
            Delay_s(3);
            goto mqttReconnect;
        }

        // (2)连接成功,设置信息,将挂起的任务(DataPost/OTA)唤醒
        DEBUG_LOG("唤醒OTA线程和DataPost线程 \r\n");
        mqttClient.connectState = MQTT_CONNECTED;
        vTaskResume(Task_MQTTDataPost_Handle);
        vTaskResume(Task_OnlineDeviceCtrl_Handle);
        vTaskResume(Task_OTA_Handle);

        // 3、订阅属性设置主题和OTA相关主题
        DEBUG_LOG("订阅相关主题 \r\n");
        MQTT_Subscribe(PARAM_SET_TOPIC);
        MQTT_Subscribe(RECV_OTA_INFO_TOPIC);
        MQTT_Subscribe(RECV_DOWNLOAD_TOPIC);
        MQTT_Subscribe(RECV_OTA_PUSH_TOPIC);

        DEBUG_LOG("上报当前程序版本号: \r\n");
        DEBUG_LOG("%s \r\n", OTA_Info.CurAPP_Version);
        OTA_PostVersion();
    }
}

// 线程2：MQTT数据上报+PING线程, 检查连接(AT+MQTTCONN?)
void Task_MQTTDataPost(void *parameter)
{
    SensorData_t SensorData_Normal;
    memset(&SensorData_Normal, 0, sizeof(SensorData_Normal));
    DataPost_Semphore = xSemaphoreCreateBinary();

    while (1) {
        DEBUG_LOG("数据上报线程 \r\n");

        // MQTT连接成功才上报，否则挂起
        while (mqttClient.connectState != MQTT_CONNECTED) {
            DEBUG_LOG("MQTT连接未建立，挂起DataPost线程\r\n");
            vTaskSuspend(NULL);
        }

        // 传感器数据归一化为百分比数据，并获取设备状态
        Sensor_DataNormalize(&SensorData_Normal);
        DeviceStatus.Light_Status = LIGHT_STATUS;
        DeviceStatus.Water_Status = WATER_STATUS;
        DeviceStatus.Food_Status = FOOD_STATUS;

        DEBUG_LOG("foodRemain sensor: %d\r\n", SensorData_Normal.foodRemain);
        DEBUG_LOG("waterRemain sensor: %d\r\n", SensorData_Normal.waterRemain);
        DEBUG_LOG("lightSwitch: %d\r\n", DeviceStatus.Light_Status);
        DEBUG_LOG("foodSwitch: %d\r\n", DeviceStatus.Food_Status);
        DEBUG_LOG("waterSwitch: %d\r\n", DeviceStatus.Water_Status);

        DEBUG_LOG("上报传感器数据和开关状态 \r\n");
        MQTT_PostData(SensorData_Normal, DeviceStatus);

        xSemaphoreTake(DataPost_Semphore, pdMS_TO_TICKS(60 * 1000));
    }
}

// 线程3：设备控制线程（云端控制+本地控制）
void Task_OnlineDeviceCtrl(void *parameter)
{
    while (1) {
        int8_t LightSwitch = -1;
        int8_t FoodSwitch = -1;
        int8_t WaterSwitch = -1;
        uint8_t AppointmentFlag = 0;

        DEBUG_LOG("云端控制线程 \r\n");
        while (mqttClient.connectState != MQTT_CONNECTED) {
            DEBUG_LOG("MQTT连接未建立，挂起OnlineDeviceCtrl线程 \r\n");
            vTaskSuspend(NULL);
        }

        DEBUG_LOG("云端控制线程等待 \r\n");
        xEventGroupWaitBits(AT_EventGroup, ONLINE_DEVICE_CTRL_EVENTBIT, pdTRUE, pdFALSE, portMAX_DELAY);

        if (MQTT_GetJSONValue_Str(
                MQTT_SubNewsBuffer, LIGHT_FUNC_NAME, (char *)&LightSwitch, sizeof(LightSwitch))) {
            SINGLE_STRING_TO_NUM(LightSwitch);
        }
        if (MQTT_GetJSONValue_Str(
                MQTT_SubNewsBuffer, FOOD_FUNC_NAME, (char *)&FoodSwitch, sizeof(FoodSwitch))) {
            SINGLE_STRING_TO_NUM(FoodSwitch);
        }
        if (MQTT_GetJSONValue_Str(
                MQTT_SubNewsBuffer, WATER_FUNC_NAME, (char *)&WaterSwitch, sizeof(WaterSwitch))) {
            SINGLE_STRING_TO_NUM(WaterSwitch);
        }
        if (MQTT_GetJSONValue_Str(
                MQTT_SubNewsBuffer, FEEDING_PLAN_NAME, (char *)&AppointmentFlag, sizeof(AppointmentFlag))) {
            SINGLE_STRING_TO_NUM(AppointmentFlag);
        }

        // 1、设置预约喂食
        // AppointmentFeed(AppointmentFlag);

        // 2、云端主动控制
        if (LightSwitch == 1)
            LIGHT_TURN_ON;
        else if (LightSwitch == 0) {
            LIGHT_TURN_OFF;
        }

        if (FoodSwitch == 1) {
            FOOD_TURN_ON;
        }
        else if (FoodSwitch == 0) {
            FOOD_TURN_OFF;
        }

        if (WaterSwitch == 1) {
            WATER_TURN_ON;
        }
        else if (WaterSwitch == 0) {
            WATER_TURN_OFF;
        }

        Delay_s(3);
        if (FoodSwitch == 1) FOOD_TURN_OFF;
        if (WaterSwitch == 1) WATER_TURN_OFF;

        // 3、完成动作后，唤醒数据上报线程，上报一次当前数据
        xSemaphoreGive(DataPost_Semphore);
    }
}

void Task_LocalDeviceCtrl(void *parameter)
{
    while (1) {
        DEBUG_LOG("本地控制线程 \r\n");
        xEventGroupWaitBits(AT_EventGroup, LOCAL_DEVICE_CTRL_EVENTBIT, pdTRUE, pdFALSE, portMAX_DELAY);

        if (Device_GetKeySignal(LIGHT_NUM) == SET) {
            DEVICE_FILP(LIGHT);
        }

        if (Device_GetKeySignal(FOOD_NUM) == SET) {
            DEVICE_FILP(FOOD);
        }

        if (Device_GetKeySignal(WATER_NUM) == SET) {
            DEVICE_FILP(WATER);
        }

        // 完成动作后，唤醒数据上报线程，上报一次当前数据
        xSemaphoreGive(DataPost_Semphore);
    }
}

// 线程4：OTA升级
void Task_OTA(void *parameter)
{
    OTA_Init();
    while (1) {
        DEBUG_LOG("OTA线程 \r\n");

        // MQTT连接成功才执行，否则挂起
        while (mqttClient.connectState != MQTT_CONNECTED) {
            DEBUG_LOG("MQTT连接未建立，挂起OTA线程\r\n");
            vTaskSuspend(NULL);
        }

        xEventGroupWaitBits(AT_EventGroup, OTA_EVENTBIT, pdTRUE, pdFALSE, portMAX_DELAY);

        OTA_State_t state = Get_OTA_State();

        // 等待OTA事件标志位
        while (state == OTA_STATE_NONEWS) {
            DEBUG_LOG("Waiting for OTA_EVENTBIT\r\n");
            state = Get_OTA_State();
            DEBUG_LOG("OTA_EVENTBIT being set\r\n");
        }

        switch (state) {
            // （1）收到升级包信息
            case OTA_STATE_GET_INFO: {
                DEBUG_LOG("处理升级包信息，继续申请bin数据\r\n");
                OTA_DealingInfo();
                break;
            }

            // （2）收到分片升级包
            case OTA_STATE_GET_BIN: {
                DEBUG_LOG("收到bin数据包，准备开始OTA升级\r\n");
                OTA_DealingBinData();
                break;
            }
            default:
                break;
        }
    }
}

// 线程5：MQTT心跳包线程
void Task_Ping(void *parameter) {}

// 线程6：AT响应处理
void Task_Receive(void *parameter)
{
    while (1) {
        DEBUG_LOG("接收线程 \r\n");
        AT_ReceiveResponse();
    }
}

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
