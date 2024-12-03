#include "RingBuffer.h"

// 初始化环形缓冲区
int RingBuffer_Init(RingBuffer_t *ringBuffer)
{
    // 1、初始化成员变量
    ringBuffer->bufferSize = RING_BUFFER_SIZE;
    ringBuffer->dataSize = 0;
    ringBuffer->readIndex = 0;
    ringBuffer->writeIndex = 0;

    // 2、初始化数据帧链表
    ringBuffer->FCBListHead = NULL;
    ringBuffer->FCBListSize = 0;

    return true;
}

/// @brief 读环形缓冲区
/// @param ringBuffer 被读取的缓冲区
/// @param recvBuffer 存放读出数据的缓冲区
/// @param len 期望读出的长度
/// @return 成功返回true，失败返回false
int Read_RingBuffer(RingBuffer_t *ringBuffer,
                    uint8_t *recvBuffer,
                    uint32_t readLength,
                    uint32_t recvBufferLength)
{
    if (!ringBuffer || !(ringBuffer->bufferHead) || is_RingBuffer_Empty(ringBuffer) ||
        !recvBuffer) {
        return false;
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
    for (uint8_t i = 0; i < readLength; i++) {
        recvBuffer[i] = ringBuffer->bufferHead[index];
        index = (index + 1) % ringBuffer->bufferSize;
    }

    recvBuffer[readLength] = '\0';
    ringBuffer->dataSize -= readLength;
    ringBuffer->readIndex = index;
    return true;
}

/// @brief 读出一帧数据
/// @param ringBuffer
/// @param recvBuffer
/// @param recvBufferLength
void Read_DataFrame(RingBuffer_t *ringBuffer, uint8_t *recvBuffer, uint32_t recvBufferLength)
{
    FCB_ListItem_t *listHead = ringBuffer->FCBListHead;
    if (listHead == NULL || ringBuffer->FCBListSize == 0) {
        return;
    }

    // uint16_t frameBegin = listHead->frameBegin;
    uint16_t frameSize = listHead->frameSize;

    Read_RingBuffer(ringBuffer, recvBuffer, frameSize, recvBufferLength);
    ringBuffer->FCBListHead = ringBuffer->FCBListHead->next;
    if (listHead != NULL) {
        free(listHead);
    }

    ringBuffer->FCBListSize--;
}

/// @brief 写入环形缓冲区
/// @param ringBuffer 想要写入的环形缓冲区
/// @param data 写入的数据
/// @param dataLength 写入数据的长度
/// @return
int Write_RingBuffer(RingBuffer_t *ringBuffer, const uint8_t *data, uint32_t dataLength)
{
    if (!ringBuffer || !(ringBuffer->bufferHead) || is_RingBuffer_Full(ringBuffer)) {
        return false;
    }

    // 缓冲区容量不足,舍弃数据
    if (dataLength > (ringBuffer->bufferSize - ringBuffer->dataSize)) {
        return false;
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
    return true;
}

FCB_ListItem_t *Create_FCBListItem(uint16_t frameBegin, uint16_t frameSize)
{
    FCB_ListItem_t *Item = (FCB_ListItem_t *)malloc(sizeof(FCB_ListItem_t));
    if (Item == NULL) {
        return NULL;
    }

    Item->frameBegin = frameBegin;
    Item->frameSize = frameSize;
    Item->next = NULL;
    return Item;
}

// 更新数据帧管理链表
void Update_FCBList(RingBuffer_t *ringBuffer, uint16_t frameSize)
{
    if (ringBuffer->FCBListSize >= MAX_FCB_LIST_SIZE) {
        return;
    }

    // 1、创建新FCB item
    uint16_t frameBegin = ringBuffer->writeIndex - frameSize;
    FCB_ListItem_t *newItem = Create_FCBListItem(frameBegin, frameSize);
    if (newItem == NULL) {
        return;
    }

    // 2、将新FCB item插入链表中
    FCB_ListItem_t *cur = ringBuffer->FCBListHead;
    if (cur == NULL) {
        ringBuffer->FCBListHead = newItem;
    }
    else {
        while (cur->next != NULL) cur = cur->next;
        cur->next = newItem;
    }
    ringBuffer->FCBListSize++;
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
