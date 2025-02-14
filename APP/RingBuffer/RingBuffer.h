#ifndef RINGBUFFER_H
#define RINGBUFFER_H
#include <stm32f10x.h>
#include <stdbool.h>

#define isFrameOverWritten (ringBuffer->FCBListHead->frameBegin) <= (ringBuffer->writeIndex)
#define RING_BUFFER_SIZE   1024
#define MAX_FCB_LIST_SIZE  30
#define FCB_ARR_SIZE       16

#if 0 
typedef struct FCB_ArrItem {
    uint16_t start;
    uint16_t end;
    uint16_t len;
} FCB_ArrItem_t;

typedef struct FCB_Arr {
    FCB_ArrItem_t arr[FCB_ARR_SIZE];
    uint16_t rIndex;
    uint16_t wIndex;
    uint16_t itemNum;
} FCB_Arr_t;
#endif

typedef struct FCB_ListItem {
    uint16_t frameBegin;
    uint16_t frameSize;
    struct FCB_ListItem *next;
} FCB_ListItem_t;

typedef struct {
    uint8_t bufferHead[RING_BUFFER_SIZE]; // 缓冲区
    uint32_t bufferSize;                  // 缓冲区大小
    volatile uint16_t dataSize;           // 当前已有数据项大小
    volatile uint16_t readIndex;          // 指向下一次要读出的位置
    volatile uint16_t writeIndex;         // 指向下一次要写入的位置

    FCB_ListItem_t *FCBListHead; // 数据帧管理链表头节点
    uint8_t FCBListSize;         // FCB链表项的项数

} RingBuffer_t;

int RingBuffer_Init(RingBuffer_t *ringBuffer);
int Read_RingBuffer(RingBuffer_t *ringBuffer,
                    uint8_t *recvBuffer,
                    uint32_t readLength,
                    uint32_t recvBufferLength);
void RingBuffer_ReadFrame(RingBuffer_t *ringBuffer, uint8_t *recvBuffer, uint32_t recvBufferLength);
int Write_RingBuffer(RingBuffer_t *ringBuffer, const uint8_t *data, uint32_t dataLength);
FCB_ListItem_t *Create_FCBListItem(uint16_t frameBegin, uint16_t frameSize);
void Update_FCBList(RingBuffer_t *ringBuffer, uint16_t frameSize);
bool is_RingBuffer_Empty(RingBuffer_t *ringBuffer);
bool is_FCBList_Empty(RingBuffer_t *ringBuffer);
bool is_RingBuffer_Full(RingBuffer_t *ringBuffer);

#endif
