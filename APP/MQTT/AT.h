#ifndef AT_H
#define AT_H
#include "FreeRTOS.h"
#include "semphr.h"
#include "event_groups.h"
#include "AT_USART.h"

// #define AT_SEND_TIMEOUT    portMAX_DELAY
#define AT_SEND_TIMEOUT    pdMS_TO_TICKS(10000)
#define AT_RECEIVE_TIMEOUT pdMS_TO_TICKS(10000)

#define AT_COMMAND_MAX_SIZE     256
#define AT_RESPONSE_BUFFER_SIZE MAX_USART_FRAME_SIZE

#define RECV_EVENTBIT               ((EventBits_t)1 << 0)
#define SEND_RESP_EVENTBIT          ((EventBits_t)1 << 1)
#define ONLINE_DEVICE_CTRL_EVENTBIT ((EventBits_t)1 << 2)
#define LOCAL_DEVICE_CTRL_EVENTBIT  ((EventBits_t)1 << 3)
#define OTA_EVENTBIT                ((EventBits_t)1 << 4)
#define DISCONNECT_EVENTBIT         ((EventBits_t)1 << 5)

enum {
    AT_OK = 1,
    AT_ERROR,
    AT_DATA_REQUEST,
    AT_GET_SUB_NEWS,
    AT_RESP_UNCOMPLETE,
    AT_RESPONSE_TIMEOUT,
    AT_GET_OTA_PACKET,
    AT_COMMAND_UNCOMPLETE
};

extern char AT_ResponseBuffer[AT_RESPONSE_BUFFER_SIZE];
extern volatile SemaphoreHandle_t AT_Send_Mutex;
extern volatile EventGroupHandle_t AT_EventGroup;

void AT_Init(void);
void AT_Reset(void);
void AT_ExitPassthroughMode(void);
void Set_AT_State(int status);
int Get_AT_Status(void);
int AT_SendData_NormalMode(char *data, TickType_t timeout);
void AT_SendData_PassthroughMode(char *dataBuf, uint16_t BufLen);
int AT_SendCommand(char *command, TickType_t timeout);
int AT_ReceiveResponse(void);
#endif
