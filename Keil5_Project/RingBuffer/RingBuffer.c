#include "RingBuffer.h"

// 初始化缓冲区
int RingBuffer_Init(RingBuffer_t *ringBuffer, uint32_t bufferSize)
{
    // 1、pvPortMalloc，为buffer动态分配内存
    (ringBuffer->bufferHead) = (uint8_t *)pvPortMalloc(bufferSize);
    if (ringBuffer == NULL) {
        return pdFAIL;
    }

    // 静态分配
    // ringBuffer->buffer = buffer;

    // 2、初始化其他成员变量
    (ringBuffer)->bufferSize = bufferSize;
    (ringBuffer)->dataSize = 0;
    (ringBuffer)->readIndex = 0;
    (ringBuffer)->writeIndex = 0;

    return pdPASS;
}

/// @brief 读环形缓冲区
/// @param ringBuffer 被读取的缓冲区
/// @param recvBuffer 存放读出数据的缓冲区
/// @param len 期望读出的长度
/// @return
int Read_RingBuffer(RingBuffer_t *ringBuffer, uint8_t *recvBuffer, uint32_t readLength, uint32_t recvBufferLength)
{
    if (!ringBuffer || !(ringBuffer->bufferHead) || is_RingBuffer_Empty(ringBuffer)) {
        return pdFAIL;
    }

    // 防止读取长度超出ringBuffer的数据长度
    if (readLength > ringBuffer->dataSize) {
        readLength = ringBuffer->dataSize;
    }

    // 防止读取长度超出recvBuffer的长度
    if (readLength >= recvBufferLength) {
        readLength = recvBufferLength - 1;
    }

    uint32_t index = ringBuffer->readIndex;
    for (int i = 0; i < readLength; i++) {
        recvBuffer[i] = ringBuffer->bufferHead[index];
        index = (index + 1) % ringBuffer->bufferSize;
    }

    recvBuffer[readLength] = '\0';
    ringBuffer->dataSize -= readLength;
    ringBuffer->readIndex = index;
    return pdPASS;
}

/// @brief 写入环形缓冲区
/// @param ringBuffer 想要写入的环形缓冲区
/// @param data 写入的数据
/// @return
int Write_RingBuffer(RingBuffer_t *ringBuffer, const uint8_t *data, uint32_t dataLength)
{
    if (!ringBuffer || !(ringBuffer->bufferHead) || is_RingBuffer_Full(ringBuffer)) {
        return pdFAIL;
    }

    // 缓冲区容量不足,舍弃数据
    if (dataLength > (ringBuffer->bufferSize - ringBuffer->dataSize)) {
        return pdFAIL;
    }

    // 写入数据
    uint32_t index = ringBuffer->writeIndex;
    for (int i = 0; i < dataLength; i++) {
        ringBuffer->bufferHead[index] = data[i];
        index = (index + 1) % (ringBuffer->bufferSize);
    }

    // 更新成员变量
    ringBuffer->dataSize += dataLength;
    ringBuffer->writeIndex = index;
    return pdPASS;
}

// 判断缓冲区是否为空
bool is_RingBuffer_Empty(RingBuffer_t *ringBuffer)
{
    return (ringBuffer->dataSize == 0);
}

// 判断缓冲区是否满
int is_RingBuffer_Full(RingBuffer_t *ringBuffer)
{
    return (ringBuffer->dataSize >= ringBuffer->bufferSize);
}
