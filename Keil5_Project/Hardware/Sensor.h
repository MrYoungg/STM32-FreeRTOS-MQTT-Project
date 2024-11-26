#ifndef SENSOR_H
#define SENSOR_H
#include "stm32f10x.h"

#define SENSOR_GPIOx     GPIOA
#define FOOD_SENSOR_PIN  GPIO_Pin_5
#define WATER_SENSOR_PIN GPIO_Pin_6

#define SENSOR_ADCx              ADC1
#define FOOD_SENSOR_ADC_CHANNEL  ADC_Channel_5
#define WATER_SENSOR_ADC_CHANNEL ADC_Channel_6
#define SENSOR_NUMBER            2
#define SENSOR_DMAy_CHANNELx     DMA1_Channel1

enum { FoodSensorIndex = 0, WaterSensorIndex, lightSensorIndex };

typedef struct {
    uint32_t food;
    uint32_t water;
    uint32_t light;
} SensorData_t;

void Sensor_AD_Init(void);

#endif
