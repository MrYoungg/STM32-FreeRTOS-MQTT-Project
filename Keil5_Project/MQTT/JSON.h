#ifndef __JSON_H
#define __JSON_H
#include "stdlib.h"
#include "string.h"
#include "FreeRTOS.h"
#include "Debug_USART.h"
#include "Sensor.h"

#define JSON_STRING_MAX_LENGTH  256
#define JSON_NAME_MAX_LENGTH    32
#define VALUE_STRING_MAX_LENGTH 32
#define CAN_ACCESS_TO_OFFSET(JSONStr)                                                              \
    ((JSONStr) != NULL) && ((JSONStr)->stringContent != NULL) &&                                   \
        ((JSONStr)->offset < (JSONStr)->length)
#define ACCESS_TO_OFFSET(JSONStr) ((JSONStr)->stringContent)[(JSONStr)->offset]
#define CAN_WRITE_STR(JSONStr)                                                                     \
    ((JSONStr) != NULL) && ((JSONStr)->stringContent != NULL) &&                                   \
        ((JSONStr)->offset < (sizeof((JSONStr)->stringContent) - 1))
#define CLEAR_OFFSET(JSONStr)           (JSONStr)->offset = 0
#define ADD_OFFSET(JSONStr)             ((JSONStr)->offset) += 1
#define CAN_WRITE_ITEM(JSONItem, index) ((index) < (sizeof((JSONItem)->nameStr) - 1))
#define VALUE_IS_NUMBER(JSONStr)                                                                   \
    (ACCESS_TO_OFFSET(JSONStr) >= '0' && ACCESS_TO_OFFSET(JSONStr) <= '9') ||                      \
        ACCESS_TO_OFFSET(JSONStr) == '-'
#define VALUE_IS_STRING(JSONStr) ACCESS_TO_OFFSET((JSONStr)) == '"'
#define LIGHT_PARAM_NAME         "LightSwitch"
#define FOOD_PARAM_NAME          "FoodRemain"
#define WATER_PARAM_NAME         "WaterRemain"

typedef int JSONType_t;
enum { JSON_Object = 0, JSON_Number, JSON_String };

typedef struct JSON_t {
    // name of item;
    char nameStr[JSON_NAME_MAX_LENGTH];

    // type of item;
    JSONType_t type;

    // value of item;
    double valueNumber;
    char valueString[VALUE_STRING_MAX_LENGTH];

    struct JSON_t *next; // 在结构体内部引用自身，需加上struct

} JSON_t;

typedef struct JSONString_t {
    char stringContent[JSON_STRING_MAX_LENGTH];
    int length;
    int offset;
} JSONString_t;

int JSON_GetParams(const char *const JSONStr, JSON_t **itemHead);
void JSON_PrintItemList(JSON_t *listHead);
void JSON_Free(JSON_t *itemHead);
JSON_t *isInItemList(JSON_t *listHead, char *itemName);
int getItemValueNumber(JSON_t *item);

#endif
