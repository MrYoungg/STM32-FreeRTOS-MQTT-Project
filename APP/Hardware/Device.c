#include "Device.h"
#include "Delay.h"
#include "Sensor.h"
#include "event_groups.h"
#include "AT.h"

volatile uint8_t DeviceState;
volatile static DeviceCB_t DeviceCB;
volatile DeviceStatus_t DeviceStatus;

void Device_Init(void)
{
    RCC_APB2PeriphClockCmd(DEVICE_APBx_GPIOx, ENABLE);
    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = LIGHT_PINx | FOOD_PINx | WATER_PINx;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(DEVICE_GPIOx, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = LIGHT_KEY_PINx;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(LIGHT_KEY_GPIOx, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = FOOD_KEY_PINx;
    GPIO_Init(FOOD_KEY_GPIOx, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = WATER_KEY_PINx;
    GPIO_Init(WATER_KEY_GPIOx, &GPIO_InitStructure);

    GPIO_EXTILineConfig(LIGHT_KEY_EXTI_SourceGPIOx, LIGHT_KEY_EXTI_SourcePinx);
    GPIO_EXTILineConfig(FOOD_KEY_EXTI_SourceGPIOx, FOOD_KEY_EXTI_SourcePinx);
    GPIO_EXTILineConfig(WATER_KEY_EXTI_SourceGPIOx, WATER_KEY_EXTI_SourcePinx);

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = LIGHT_KEY_EXTI_Linex | FOOD_KEY_EXTI_Linex | WATER_KEY_EXTI_Linex;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = LIGHT_KEY_EXTI_IRQChannel;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = DeviceCtrl_Priority;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = FOOD_KEY_EXTI_IRQChannel;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = WATER_KEY_EXTI_IRQChannel;
    NVIC_Init(&NVIC_InitStructure);
}

void Light_Blink(void)
{
    uint8_t times = 3;
    while (times--) {
        LIGHT_TURN_ON;
        Delay_ms(500);
        LIGHT_TURN_OFF;
        Delay_ms(500);
    }
}

void Device_SetKeySignal(DeviceNum_t DeviceNum, uint8_t Signal)
{
    if (DeviceNum == LIGHT_NUM) DeviceCB.LightKeySignal = SET;
    if (DeviceNum == WATER_NUM) DeviceCB.WaterKeySignal = SET;
    if (DeviceNum == FOOD_NUM) DeviceCB.FoodKeySignal = SET;
}

uint8_t Device_GetKeySignal(DeviceNum_t DeviceNum)
{
    uint8_t RetSignal = 0;

    if (DeviceNum == LIGHT_NUM) {
        RetSignal = DeviceCB.LightKeySignal;
        DeviceCB.LightKeySignal = RESET;
    }
    if (DeviceNum == WATER_NUM) {
        RetSignal = DeviceCB.WaterKeySignal;
        DeviceCB.WaterKeySignal = RESET;
    }
    if (DeviceNum == FOOD_NUM) {
        RetSignal = DeviceCB.FoodKeySignal;
        DeviceCB.FoodKeySignal = RESET;
    }

    return RetSignal;
}

#if 0
void AppointmentFeed(uint8_t AppointmentFlag)
{

    switch (AppointmentFlag) {
        case NoPlan: {
            break;
        }

        case Countdown: {
            Feed_AfterTime(time);
            break;
        }

        case Everyday: {
            Feed_Everday(time);
        }

        case SingleFeed: {
            Feed_SpecificDate(time);
        }

        case Clear_AferTime: {
            Clear_AferTime_Func();
        }

        case Clear_Everyday: {
            Clear_Everyday_Func();
        }

        case Clear_SingleFeed: {
            Clear_SpecificDate_Func();
        }

        default:
            break;
    }
}
#endif

void LIGHT_KEY_IRQHandler(void)
{
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = 0;

    if (EXTI_GetITStatus(LIGHT_KEY_EXTI_Linex) == SET) {

        Delay_ms(20); // 消抖
        while (isLIGHT_KEY_PRESS);

        if (isLIGHT_KEY_RELEASE) {
            // 1、设置标志位
            Device_SetKeySignal(LIGHT_NUM, SET);

            // 2、唤醒本地设备控制线程
            xResult = xEventGroupSetBitsFromISR(
                AT_EventGroup, LOCAL_DEVICE_CTRL_EVENTBIT, &pxHigherPriorityTaskWoken);
            if (xResult == pdPASS) {
                portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
            }
        }
    }

    EXTI_ClearITPendingBit(LIGHT_KEY_EXTI_Linex);
}

void FOOD_KEY_IRQHandler(void)
{
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = 0;

    if (EXTI_GetITStatus(FOOD_KEY_EXTI_Linex) == SET) {

        Delay_ms(20); // 消抖
        while (isFOOD_KEY_PRESS);

        if (isFOOD_KEY_RELEASE) {
            // 1、设置标志位
            Device_SetKeySignal(FOOD_NUM, SET);
            // 2、唤醒本地设备控制线程
            xResult = xEventGroupSetBitsFromISR(
                AT_EventGroup, LOCAL_DEVICE_CTRL_EVENTBIT, &pxHigherPriorityTaskWoken);
            if (xResult == pdPASS) {
                portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
            }
        }
    }
    EXTI_ClearITPendingBit(FOOD_KEY_EXTI_Linex);
}

void WATER_KEY_IRQHandler(void)
{
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = 0;

    if (EXTI_GetITStatus(WATER_KEY_EXTI_Linex) == SET) {

        Delay_ms(20);
        while (isWATER_KEY_PRESS);

        if (isWATER_KEY_RELEASE) {
            // 1、设置标志位
            Device_SetKeySignal(WATER_NUM, SET);

            // 2、唤醒本地设备控制线程
            xResult = xEventGroupSetBitsFromISR(
                AT_EventGroup, LOCAL_DEVICE_CTRL_EVENTBIT, &pxHigherPriorityTaskWoken);
            if (xResult == pdPASS) {
                portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
            }
        }
    }
    EXTI_ClearITPendingBit(WATER_KEY_EXTI_Linex);
}
