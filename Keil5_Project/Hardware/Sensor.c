#include "Sensor.h"

SensorData_t sensorData;

void Sensor_AD_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = FOOD_SENSOR_PIN | WATER_SENSOR_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SENSOR_GPIOx, &GPIO_InitStructure);

    ADC_RegularChannelConfig(SENSOR_ADCx, FOOD_SENSOR_ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);
    ADC_RegularChannelConfig(SENSOR_ADCx, WATER_SENSOR_ADC_CHANNEL, 2, ADC_SampleTime_55Cycles5);

    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_NbrOfChannel = SENSOR_NUMBER;
    ADC_Init(SENSOR_ADCx, &ADC_InitStructure);

    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SENSOR_ADCx->DR);
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&sensorData;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = SENSOR_NUMBER;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    DMA_Init(SENSOR_DMAy_CHANNELx, &DMA_InitStructure);

    DMA_Cmd(SENSOR_DMAy_CHANNELx, ENABLE);
    ADC_DMACmd(SENSOR_ADCx, ENABLE);
    ADC_Cmd(SENSOR_ADCx, ENABLE);

    ADC_ResetCalibration(SENSOR_ADCx);
    while (ADC_GetResetCalibrationStatus(SENSOR_ADCx) == SET);
    ADC_StartCalibration(SENSOR_ADCx);
    while (ADC_GetCalibrationStatus(SENSOR_ADCx) == SET);

    ADC_SoftwareStartConvCmd(SENSOR_ADCx, ENABLE);
}
