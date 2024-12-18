#ifndef ATCOMMAND_H
#define ATCOMMAND_H
#include "AT_USART.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "Debug_USART.h"
#include "Delay.h"
#include <stdbool.h>
#include <string.h>

#define AT_SEND_TIMEOUT    portMAX_DELAY
#define AT_RECEIVE_TIMEOUT pdMS_TO_TICKS(1000)

#define AT_MAX_COMMAND_SIZE     256
#define AT_RESPONSE_BUFFER_SIZE 256
#define AT_DATA_PACKET_SIZE     256

enum {
    AT_OK = 0,
    AT_ERROR,
    AT_DATA_REQUEST,
    AT_DATA_FROM_HOST,
    AT_RESP_UNCOMPLETE,
    AT_RESPONSE_TIMEOUT,
    AT_COMMAND_UNCOMPLETE
};

void AT_Init(void);
void AT_Reset(void);
int AT_SendData(char *data, TickType_t timeout);
int AT_SendCommand(char *command, TickType_t timeout);
static int AT_Send(char *c, TickType_t timeout);
int AT_ReceiveResponse(void);
void AT_ParseResponse(char *responseBuffer);
static int SaveJSONDataToBuffer(char *buf);
int AT_Read_DataPacketBuffer(char *buf, int bufLen, int timeout);
#endif
