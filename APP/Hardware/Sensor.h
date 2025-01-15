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

#define FOOD_PARAM_NAME  "FoodRemain"
#define WATER_PARAM_NAME "WaterRemain"

enum {
    FoodSensorIndex = 0,
    WaterSensorIndex,
    lightSensorIndex,
};

typedef struct {
    uint32_t foodRemain;
    uint32_t waterRemain;
} SensorData_t;

extern SensorData_t SensorData;

void Sensor_AD_Init(void);
void Sensor_DataNormalize(SensorData_t *SensorData_Normal);

#endif
