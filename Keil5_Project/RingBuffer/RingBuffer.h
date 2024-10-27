#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H
#include <stm32f10x.h>
#include <stdlib.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include <Debug_USART.h>

typedef struct {
    uint8_t *bufferHead;          // 缓冲区开头
    uint32_t bufferSize;          // 缓冲区大小
    volatile uint32_t dataSize;   // 当前已有数据项大小
    volatile uint32_t readIndex;  // 指向上次读出末尾的下一个位置
    volatile uint32_t writeIndex; // 指向上次写入末尾的下一个位置
} RingBuffer_t;

int RingBuffer_Init(RingBuffer_t *ringBuffer, uint32_t bufferSize);
int Read_RingBuffer(RingBuffer_t *ringBuffer, uint8_t *recvBuffer, uint32_t readLength, uint32_t recvBufferLength);
int Write_RingBuffer(RingBuffer_t *ringBuffer, const uint8_t *data, uint32_t dataLength);
bool is_RingBuffer_Empty(RingBuffer_t *ringBuffer);
int is_RingBuffer_Full(RingBuffer_t *ringBuffer);

#endif
