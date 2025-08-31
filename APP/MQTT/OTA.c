#include "OTA.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
#include "stm32f10x.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "Debug_USART.h"
#include "AT.h"
#include "W25Q64.h"
#include "AT24C02.h"
#include "JSON.h"
#include "MQTTClient.h"

volatile SemaphoreHandle_t OTA_Mutex;
OTA_Info_t OTA_Info;
OTA_Download_t OTA_Download_CB;
char OTA_DataPacketBuffer[OTA_DATA_PACKET_SIZE];

static OTA_State_t OTA_State;

void OTA_Init(void)
{
    OTA_Mutex = xSemaphoreCreateMutex();
    xSemaphoreTake(OTA_Mutex, 0);

    Read_SavingOTAInfo();
}

void Read_SavingOTAInfo(void)
{
    memset(&OTA_Info, 0, OTA_INFO_SIZE);
    AT24C02_Read(OTA_INFO_BASE_ADDR, OTA_INFO_SIZE, (uint8_t *)&OTA_Info, OTA_INFO_SIZE);
}

void Save_OTAInfo(void)
{
    AT24C02_Write(OTA_INFO_BASE_ADDR, (uint8_t *)&OTA_Info, OTA_INFO_SIZE);
}

void OTA_ConfigInfo(void)

{
    char *Version = OTA_Download_CB.Version;
    char *OTA_Version = (char *)OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].version;

    // version本身是一个指向结构体对象OTA_Info的version数组成员的指针，不能被直接赋值，用memcpy或strcpy
    // OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].version = "Online OTA Version";
    strncpy(OTA_Version, Version, VERSION_LEN - 1);
    OTA_Version[VERSION_LEN - 1] = '\0'; // 手动加上终止符
    DEBUG_LOG("%s\r\n", OTA_Version);

    OTA_Info.OTA_Flag = OTA_FLAG_SET;
    OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].size = OTA_Download_CB.FileSize;

    Save_OTAInfo();
}

void OTA_Reset(void)
{
    DEBUG_LOG("Device is ready to reset... \r\n");
    for (uint8_t i = 0; i < 3; i++) {
        DEBUG_LOG("%d\r\n", 3 - i);
        Delay_s(1);
    }
    NVIC_SystemReset();
}

OTA_State_t Get_OTA_State(void)
{
    OTA_State_t state = OTA_State;
    OTA_State = OTA_STATE_NONEWS;
    return state;
}

void Set_OTA_State(OTA_State_t state)
{
    OTA_State = state;
}

uint8_t OTA_PostVersion(void)
{
    char JSONStr[JSON_STRING_MAX_LENGTH] = {0};
    uint16_t JSONStrLen = 0;
    uint8_t ret = 0;

    JSONStrLen = snprintf(
        JSONStr, sizeof(JSONStr), "{\"id\":\"1\",\"params\":{\"version\":\"%s\"}}", OTA_Info.CurAPP_Version);

    ret = MQTT_Pubish(POST_VERSION_TOPIC, JSONStr, JSONStrLen);
    if (ret != MQTT_SUCCESS) {
        DEBUG_LOG("版本号上报失败 \r\n");
        return OTA_POST_VERSION_ERR;
    }

    DEBUG_LOG("版本号上报成功 \r\n");
    return ret;
}

uint8_t OTA_RequestDownloadInfo(void)
{
    uint8_t ret = 0;
    char JSONStr[] = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{},\"method\":\"thing.ota.firmware.get\"}";

    // 传输字节数要减去最后的'\0'
    ret = MQTT_Pubish(REQUEST_OTA_INFO_TOPIC, JSONStr, sizeof(JSONStr) - 1);
    if (ret != MQTT_SUCCESS) {
        DEBUG_LOG("请求升级包信息失败 \r\n");
        return OTA_POST_VERSION_ERR;
    }

    DEBUG_LOG("请求升级包信息成功 \r\n");
    return ret;
}

void OTA_GetDownloadInfo()
{
    char transBuf[10] = {0};

    // 1、获取文件大小
    if (MQTT_GetJSONValue_Str(MQTT_SubNewsBuffer, "size", transBuf, sizeof(transBuf)) == true) {
        OTA_Download_CB.FileSize = strtoul(transBuf, NULL, 10);
        DEBUG_LOG("云端文件大小：%d\r\n", OTA_Download_CB.FileSize);
    }

    // 2、获取文件版本号
    if (MQTT_GetJSONValue_Str(
            MQTT_SubNewsBuffer, "version", OTA_Download_CB.Version, sizeof(OTA_Download_CB.Version)) ==
        true) {
        DEBUG_LOG("云端文件版本号：%s\r\n", OTA_Download_CB.Version);
    }

    // 3、获取StreamID
    if (MQTT_GetJSONValue_Str(MQTT_SubNewsBuffer, "streamId", transBuf, sizeof(transBuf)) == true) {
        OTA_Download_CB.StreamID = strtoul(transBuf, NULL, 10);
        DEBUG_LOG("云端文件StreamID：%d\r\n", OTA_Download_CB.StreamID);
    }

    // 4、获取FileID
    if (MQTT_GetJSONValue_Str(MQTT_SubNewsBuffer, "streamFileId", transBuf, sizeof(transBuf)) == true) {
        OTA_Download_CB.FileID = strtoul(transBuf, NULL, 10);
        DEBUG_LOG("云端文件FileID：%d\r\n", OTA_Download_CB.FileID);
    }
}

void OTA_RequestDataFrame(void)
{
    char JSONStr[JSON_STRING_MAX_LENGTH] = {0};
    uint16_t JSONStrLen = 0;
    uint8_t ret = 0;
    uint16_t size = 0;
    uint16_t offset = 0;

    offset = OTA_Download_CB.RecvFileSize;
    size = OTA_Download_CB.FileSize - OTA_Download_CB.RecvFileSize;
    size = size < W25Q64_PAGE_SIZE ? size : W25Q64_PAGE_SIZE;

    JSONStrLen = snprintf(JSONStr,
                          sizeof(JSONStr),
                          "{\"id\":\"1\",\"version\":\"%s\",\"params\":{\"fileInfo\":{\"streamId\":"
                          "%d,\"fileId\":%d},\"fileBlock\":{\"size\":%d,\"offset\":%d}}}",
                          OTA_Download_CB.Version,
                          OTA_Download_CB.StreamID,
                          OTA_Download_CB.FileID,
                          size,
                          offset);

    ret = MQTT_Pubish(REQUEST_DOWNLOAD_TOPIC, JSONStr, JSONStrLen);
    if (ret != MQTT_SUCCESS) {
        DEBUG_LOG("请求升级包数据出错 \r\n");
        return;
    }
}

/// @brief 计算字节流数据的CRC校验码
/// @param Buf 字节流数据缓冲区
/// @param BufLen 字节流数据的长度（单位：字节）
/// @return
uint16_t OTA_GetBuf_CRC16IBM_old(uint8_t *Buf, uint16_t BufLen)
{
    uint16_t CRC_Val = 0x0000;
    for (uint32_t i = 0; i < BufLen; i++) {

        CRC_Val ^= (Buf[i] << 8);

        for (uint8_t i = 0; i < 8; i++) {

            if ((CRC_Val & 0x8000) == 0) {
                CRC_Val <<= 1;
            }
            else {
                CRC_Val <<= 1;
                CRC_Val ^= IBM_CRC16_POLY;
            }
        }
    }

    return CRC_Val;
}

static uint8_t Byte_ReverseBits(uint8_t Byte)
{
    Byte = ((Byte & 0xF0) >> 4) | ((Byte & 0x0F) << 4);
    Byte = ((Byte & 0xCC) >> 2) | ((Byte & 0x33) << 2);
    Byte = ((Byte & 0xAA) >> 1) | ((Byte & 0x55) << 1);
    return Byte;
}

static uint16_t HalfWord_ReverseBits(uint16_t HalfWord)
{
    uint8_t RetMSB = 0;
    uint8_t RetLSB = 0;

    RetMSB = Byte_ReverseBits((uint8_t)(HalfWord >> 0));
    RetLSB = Byte_ReverseBits((uint8_t)(HalfWord >> 8));

    return (uint16_t)((RetMSB << 8) | (RetLSB << 0));
}

uint16_t OTA_GetBuf_CRC16IBM(uint8_t *Buf, uint16_t BufLen)
{
    uint16_t CRC_Val = 0x0000;
    uint8_t ReversedByte = 0;

    for (uint32_t i = 0; i < BufLen; i++) {

        // 1、CRC16/IBM:需要将每个字节按位反转
        ReversedByte = Byte_ReverseBits(Buf[i]);
        CRC_Val ^= (ReversedByte << 8);

        for (uint8_t i = 0; i < 8; i++) {

            if ((CRC_Val & 0x8000) == 0) {
                CRC_Val <<= 1;
            }
            else {
                CRC_Val <<= 1;
                CRC_Val ^= IBM_CRC16_POLY;
            }
        }
    }

    // 2、CRC16/IBM:计算完后，需要将CRC校验值按位反转
    CRC_Val = HalfWord_ReverseBits(CRC_Val);

    return CRC_Val;
}

uint8_t OTA_GetBinDataFrame(void)
{
    char bSizeStr[6] = {0};
    char bOffsetStr[10] = {0};
    char *JSONStart = NULL;
    char *binStart = NULL;
    uint32_t bSize = 0;
    uint32_t bOffset = 0;
    uint16_t JSONStrLen = 0;
    uint16_t CRC_Index = NULL;
    uint16_t CRC16_CalVal = 0x0000;

    // 1、找到bin数据的起始位置
    JSONStrLen = (uint16_t)(MQTT_SubNewsBuffer[0] << 8) + (uint16_t)MQTT_SubNewsBuffer[1];
    DEBUG_LOG("升级包JSON数据长度为：%d\r\n", JSONStrLen);
    binStart = MQTT_SubNewsBuffer + ALIYUN_JSON_BYTES_LENGTH + JSONStrLen;

    // 2、读取bSize参数（bin数据包的长度）
    JSONStart = MQTT_SubNewsBuffer + ALIYUN_JSON_BYTES_LENGTH;
    if (MQTT_GetJSONValue_Str(JSONStart, "bSize", bSizeStr, sizeof(bSizeStr)) == false) {
        return false;
    }
    bSize = strtoul(bSizeStr, NULL, 10);
    DEBUG_LOG("bSize:%d\r\n", bSize);

    // 3、读取bOffset（bin数据包偏移）
    if (MQTT_GetJSONValue_Str(JSONStart, "bOffset", bOffsetStr, sizeof(bOffsetStr)) == false) {
        return false;
    }
    bOffset = strtoul(bOffsetStr, NULL, 10);
    DEBUG_LOG("bOffset:%d\r\n", bOffset);
    if (bOffset != OTA_Download_CB.RecvFileSize) {
        DEBUG_LOG("bin文件包偏移不符合要求 \r\n");
        return false;
    }

    // 4、提取bin数据
    memset(OTA_Download_CB.BinFrameBuf, 0, sizeof(OTA_Download_CB.BinFrameBuf));
    memcpy(OTA_Download_CB.BinFrameBuf, binStart, bSize);

    // 5、提取校验码
    CRC_Index = ALIYUN_JSON_BYTES_LENGTH + JSONStrLen + bSize;
    OTA_Download_CB.CRC16_Value =
        (uint16_t)(MQTT_SubNewsBuffer[CRC_Index]) + (uint16_t)(MQTT_SubNewsBuffer[CRC_Index + 1] << 8);

    // 6、校验
    CRC16_CalVal = OTA_GetBuf_CRC16IBM(OTA_Download_CB.BinFrameBuf, bSize);
    if (CRC16_CalVal != OTA_Download_CB.CRC16_Value) {
        DEBUG_LOG("校验失败 \r\n");
        DEBUG_LOG("计算得到的校验值为：%.4x\r\n", CRC16_CalVal);
        return false;
    }

    DEBUG_LOG("偏移正确！校验码正确！ \r\n");
    OTA_Download_CB.RecvFileSize += bSize;
    return true;
}

void OTA_SaveBinDataFrame(void)
{
    uint32_t WriteAddr = 0;
    uint8_t RecvPageNum =
        (OTA_Download_CB.RecvFileSize + W25Q64_PAGE_SIZE - 1) / W25Q64_PAGE_SIZE; // 向上取整

    // 1、接收并校验完1帧（256字节），写入W25Q64
    WriteAddr = OTA_BASE_ADDR + (RecvPageNum - 1) * W25Q64_PAGE_SIZE;
    DEBUG_LOG("Save 256 bytes into W25Q64 0x%.8x\r\n", WriteAddr);
    W25Q64_WritePage(WriteAddr, OTA_Download_CB.BinFrameBuf, OTA_DATA_FRAME_SIZE);
}

void OTA_DealingInfo(void)
{
    // 1、擦除W25Q64第0块，并清理第0块的属性
    W25Q64_BlockErase_64K(OTA_BASE_ADDR);
    memset(OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].version, 0, VERSION_LEN);
    OTA_Info.ExFlash_InfoArr[OTA_BLOCK_NUM].size = 0;
    Save_OTAInfo();

    // 2、提取升级包信息
    DEBUG_LOG("提取升级包信息 \r\n");
    OTA_GetDownloadInfo();

    // 3、向云端请求升级包
    DEBUG_LOG("向云端请求bin数据 \r\n");
    OTA_RequestDataFrame();
}

void OTA_DealingBinData(void)
{
    uint8_t Repeatitions = 0;
    // 1、接收这一帧数据
    // （1）提取并校验分片数据（256字节数据+2字节校验码）
    DEBUG_LOG("提取并校验bin数据 \r\n");
    if (OTA_GetBinDataFrame() == false) {
        // （2）接收失败重传（文件偏移正确或校验不正确）
        Repeatitions++;
        if (Repeatitions > MAX_OTA_REPETITIONS) {
            DEBUG_LOG("重传失败，退出OTA \r\n");
            return;
        }
    }
    // （3）接收成功，保存到外存（W25Q64）中
    DEBUG_LOG("保存一帧bin数据到W25Q64 \r\n");
    OTA_SaveBinDataFrame();

    // 2、还未接收完成，继续申请数据（本帧或下一帧）
    if (OTA_Download_CB.RecvFileSize < OTA_Download_CB.FileSize) {
        DEBUG_LOG("还未接收完成，继续申请数据，偏移：%d \r\n", OTA_Download_CB.RecvFileSize);
        OTA_RequestDataFrame();
    }
    else {
        // 3、程序保存完成，设置OTA_Flag、程序版本等信息，并存储到AT24C02
        DEBUG_LOG("程序保存完成 \r\n");
        OTA_ConfigInfo();
        DEBUG_LOG("设置OTA_Info完成 \r\n");

        // 4、复位，跳转到BootLoader
        OTA_Reset();
    }
}
