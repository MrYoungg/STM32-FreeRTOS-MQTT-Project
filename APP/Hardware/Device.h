#ifndef DEVICE_H
#define DEVICE_H
#include "stm32f10x.h"
#include "Debug_USART.h"

#define DEVICE_APBx_GPIOx RCC_APB2Periph_GPIOA
#define DEVICE_GPIOx      GPIOA
#define LIGHT_PINx        GPIO_Pin_10
#define FOOD_PINx         GPIO_Pin_9
#define WATER_PINx        GPIO_Pin_8

#define LIGHT_TURN_ON    GPIO_SetBits(DEVICE_GPIOx, LIGHT_PINx)
#define LIGHT_TURN_OFF   GPIO_ResetBits(DEVICE_GPIOx, LIGHT_PINx)
#define LIGHT_STATUS     GPIO_ReadOutputDataBit(DEVICE_GPIOx, LIGHT_PINx)
#define LIGHT_STATUS_ON  1
#define LIGHT_STATUS_OFF 0

#define FOOD_TURN_ON    GPIO_SetBits(DEVICE_GPIOx, FOOD_PINx)
#define FOOD_TURN_OFF   GPIO_ResetBits(DEVICE_GPIOx, FOOD_PINx)
#define FOOD_STATUS     GPIO_ReadOutputDataBit(DEVICE_GPIOx, FOOD_PINx)
#define FOOD_STATUS_ON  1
#define FOOD_STATUS_OFF 0

#define WATER_TURN_ON    GPIO_SetBits(DEVICE_GPIOx, WATER_PINx)
#define WATER_TURN_OFF   GPIO_ResetBits(DEVICE_GPIOx, WATER_PINx)
#define WATER_STATUS     GPIO_ReadOutputDataBit(DEVICE_GPIOx, WATER_PINx)
#define WATER_STATUS_ON  1
#define WATER_STATUS_OFF 0

#define LIGHT_KEY_GPIOx GPIOA
#define LIGHT_KEY_PINx  GPIO_Pin_0
#define FOOD_KEY_GPIOx  GPIOA
#define FOOD_KEY_PINx   GPIO_Pin_4
#define WATER_KEY_GPIOx GPIOA
#define WATER_KEY_PINx  GPIO_Pin_7

#define LIGHT_KEY_EXTI_SourceGPIOx GPIO_PortSourceGPIOA
#define FOOD_KEY_EXTI_SourceGPIOx  GPIO_PortSourceGPIOA
#define WATER_KEY_EXTI_SourceGPIOx GPIO_PortSourceGPIOA

#define LIGHT_KEY_EXTI_SourcePinx GPIO_PinSource0
#define FOOD_KEY_EXTI_SourcePinx  GPIO_PinSource4
#define WATER_KEY_EXTI_SourcePinx GPIO_PinSource7

#define LIGHT_KEY_EXTI_Linex EXTI_Line0
#define FOOD_KEY_EXTI_Linex  EXTI_Line4
#define WATER_KEY_EXTI_Linex EXTI_Line7

#define DeviceCtrl_Priority       14
#define LIGHT_KEY_EXTI_IRQChannel EXTI0_IRQn
#define FOOD_KEY_EXTI_IRQChannel  EXTI4_IRQn
#define WATER_KEY_EXTI_IRQChannel EXTI9_5_IRQn

#define LIGHT_KEY_IRQHandler EXTI0_IRQHandler
#define FOOD_KEY_IRQHandler  EXTI4_IRQHandler
#define WATER_KEY_IRQHandler EXTI9_5_IRQHandler

#define isLIGHT_KEY_PRESS (GPIO_ReadInputDataBit(LIGHT_KEY_GPIOx, LIGHT_KEY_PINx) == RESET)
#define isFOOD_KEY_PRESS  (GPIO_ReadInputDataBit(FOOD_KEY_GPIOx, FOOD_KEY_PINx) == RESET)
#define isWATER_KEY_PRESS (GPIO_ReadInputDataBit(WATER_KEY_GPIOx, WATER_KEY_PINx) == RESET)

#define isLIGHT_KEY_RELEASE (GPIO_ReadInputDataBit(LIGHT_KEY_GPIOx, LIGHT_KEY_PINx) == SET)
#define isFOOD_KEY_RELEASE  (GPIO_ReadInputDataBit(FOOD_KEY_GPIOx, FOOD_KEY_PINx) == SET)
#define isWATER_KEY_RELEASE (GPIO_ReadInputDataBit(WATER_KEY_GPIOx, WATER_KEY_PINx) == SET)

#define LIGHT_FUNC_NAME              "LightSwitch"
#define WATER_FUNC_NAME              "WaterSwitch"
#define FOOD_FUNC_NAME               "FoodSwitch"
#define FEEDING_PLAN_NAME            "FeedingPlan"
#define COUNTDOWN_FEEDING_PARAM_NAME "Countdown"
#define EVERYDAY_FEEDING_PARAM_NAME  "Everyday"
#define SINGLE_FEEDING_PARAM_NAME    "SingleFeed"

#define DEVICE_FILP(Device)                                                                                  \
    do {                                                                                                     \
        if (Device##_STATUS == Device##_STATUS_ON) {                                                         \
            Device##_TURN_OFF;                                                                               \
        }                                                                                                    \
        else {                                                                                               \
            Device##_TURN_ON;                                                                                \
        }                                                                                                    \
    } while (0)

#define SINGLE_STRING_TO_NUM(str)                                                                            \
    do {                                                                                                     \
        if ((str) > '9' || (str) < '0') {                                                                    \
            DEBUG_LOG("String to Number failed\r\n");                                                        \
        }                                                                                                    \
        else {                                                                                               \
            (str) -= '0';                                                                                    \
        }                                                                                                    \
    } while (0)

typedef enum {
    LIGHT_NUM = 0,
    FOOD_NUM,
    WATER_NUM,
} DeviceNum_t;

typedef struct {
    uint8_t LightKeySignal;
    uint8_t FoodKeySignal;
    uint8_t WaterKeySignal;
} DeviceCB_t;

typedef struct {
    uint8_t Light_Status;
    uint8_t Food_Status;
    uint8_t Water_Status;
} DeviceStatus_t;

typedef enum {
    NoPlan = 0,
    Countdown,
    Everyday,
    SingleFeed,
    Clear_AferTime,
    Clear_Everyday,
    Clear_SingleFeed,
} FeedingPlan_t;

extern volatile DeviceStatus_t DeviceStatus;

void Device_Init(void);
void Light_Blink(void);
void Device_SetKeySignal(DeviceNum_t DeviceNum, uint8_t Signal);
uint8_t Device_GetKeySignal(DeviceNum_t DeviceNum);

#endif
