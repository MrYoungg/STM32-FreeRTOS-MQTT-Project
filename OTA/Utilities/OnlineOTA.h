#ifndef ONLINEOTA_H
#define ONLINEOTA_H
#include "stm32f10x.h"

#define OTA_BLOCK_NUM 0

uint8_t OnlineOTA_UpdateAPP(void);
void OnlineOTA_ConfigInfo(void);

#endif
