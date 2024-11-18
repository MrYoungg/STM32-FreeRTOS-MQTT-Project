#include "JSONParse.h"

static int JSON_Item2JSONString(const char *const JSONStr,
                                JSONString_t *JSONParamsStr,
                                const char *JSONItemName);
static int JSON_GetItem(JSONString_t *JSONParamsStr, JSON_t **item);
static int JSON_GetItemName(JSONString_t *JSONParamsStr, JSON_t *item);
static int JSON_GetItemValue(JSONString_t *JSONParamsStr, JSON_t *item);
static void JSON_PrintItemList(JSON_t *listHead);
void JSON_Free(JSON_t *itemHead);

int JSON_GetParams(const char *const JSONStr, JSON_t *itemHead)
{
    // 有写入属性的局部变量必须初始化
    JSONString_t JSONParamsStr = {"", 0, 0};
    JSON_t *curItem = NULL;
    char *itemName = "params";
    int ret;

    // 1、读取JSON根对象中的params对象（一个JSON子对象），并将其转换为JSON字符串
    ret = JSON_Item2JSONString(JSONStr, &JSONParamsStr, itemName);
    DEBUG_LOG("param string:%s\r\n", JSONParamsStr.stringContent);
    DEBUG_LOG("param length:%d\r\n", JSONParamsStr.length);
    DEBUG_LOG("param offset:%d\r\n", JSONParamsStr.offset);
    ADD_OFFSET(&JSONParamsStr); // 跳过开头的'{'

    // 2、读取其中的每个键值项，构成键值项链表
    while (ACCESS_TO_OFFSET(&JSONParamsStr) != '}') {

        // curItem为空，代表这是第一个键值项，将其作为头节点
        if (curItem == NULL) {
            DEBUG_LOG("get first item\r\n");
            ret = JSON_GetItem(&JSONParamsStr, &itemHead);
            curItem = itemHead;
            continue;
        }

        // item已被赋值，继续获取后续键值对，并连接在链表上
        ret = JSON_GetItem(&JSONParamsStr, &(curItem->next));
        curItem = curItem->next;
    }

    // 3、处理链表数据
    JSON_PrintItemList(itemHead);

    // 4、释放链表内存
    JSON_Free(itemHead);

    return pdPASS;
}

static int JSON_Item2JSONString(const char *const JSONStr,
                                JSONString_t *JSONParamsStr,
                                const char *JSONItemName)
{
    int index;
    char *begin;

    begin = strstr(JSONStr, JSONItemName);
    if (begin == NULL) {
        DEBUG_LOG("JSON item doesn't exist\r\n");
        return pdFAIL;
    }

    index = (int)(begin - JSONStr);
    while (JSONStr[++index] != ':');
    while (CAN_WRITE_STR(JSONParamsStr) && JSONStr[index++] != '}') {

        ACCESS_TO_OFFSET(JSONParamsStr) = JSONStr[index];
        ADD_OFFSET(JSONParamsStr);
        JSONParamsStr->length++;
    }

    if (!CAN_WRITE_STR(JSONParamsStr)) {
        return pdFAIL;
    }

    ACCESS_TO_OFFSET(JSONParamsStr) = '\0';
    CLEAR_OFFSET(JSONParamsStr);
    return pdPASS;
}

static int JSON_GetItem(JSONString_t *JSONParamsStr, JSON_t **item)
{
    int ret;
    JSON_t *tmpItem = NULL;
    tmpItem = (JSON_t *)pvPortMalloc(sizeof(JSON_t));
    if (tmpItem == NULL) {
        DEBUG_LOG("malloc failed\r\n");
        return pdFAIL;
    }
    memset(tmpItem, 0, sizeof(JSON_t));

    ret = JSON_GetItemName(JSONParamsStr, tmpItem);
    DEBUG_LOG("item name: %s\r\n", tmpItem->nameStr);

    ret = JSON_GetItemValue(JSONParamsStr, tmpItem);
    DEBUG_LOG("item valueNumber:%f\r\n", tmpItem->valueNumber);
    DEBUG_LOG("item valueString:%s\r\n", tmpItem->valueString);

    // 1. 指向','代表还有下一个键值item，则跳过','指向下一个键值item的起始双引号'"'
    // 2. 否则即指向'}', 代表已经读取完所有的键值item, 返回即可
    if (ACCESS_TO_OFFSET(JSONParamsStr) == ',') ADD_OFFSET(JSONParamsStr);

    *item = tmpItem;
    return ret;
}

static int JSON_GetItemName(JSONString_t *JSONParamsStr, JSON_t *item)
{
    int ret;
    int index = 0;

    if (ACCESS_TO_OFFSET(JSONParamsStr) != '"') {
        DEBUG_LOG("incompatible name of json item\r\n");
        return pdFAIL;
    }
    ADD_OFFSET(JSONParamsStr);

    while (CAN_WRITE_ITEM(item, index) && ACCESS_TO_OFFSET(JSONParamsStr) != '"') {

        item->nameStr[index++] = ACCESS_TO_OFFSET(JSONParamsStr);
        ADD_OFFSET(JSONParamsStr);
    }

    if (!CAN_WRITE_ITEM(item, index)) {

        DEBUG_LOG("fail to write item:name overflow\r\n");
        return pdFAIL;
    }

    item->nameStr[index] = '\0';
    return pdPASS;
}

static int JSON_GetItemValue(JSONString_t *JSONParamsStr, JSON_t *item)
{
    while (ACCESS_TO_OFFSET(JSONParamsStr) != ':') {
        ADD_OFFSET(JSONParamsStr);
    }
    // 跳过':'
    ADD_OFFSET(JSONParamsStr);

    if (VALUE_IS_NUMBER(JSONParamsStr)) {
        JSON_ParseNumber(JSONParamsStr, item);
    }
    else if (VALUE_IS_STRING(JSONParamsStr)) {
        JSON_ParseString(JSONParamsStr, item);
    }
}

int JSON_ParseNumber(JSONString_t *JSONParamsStr, JSON_t *item)
{
    double num = 0.0;
    char numBuf[VALUE_STRING_MAX_LENGTH] = {0};
    int index = 0;

    while (CAN_ACCESS_TO_OFFSET(JSONParamsStr) && ACCESS_TO_OFFSET(JSONParamsStr) != ',' &&
           ACCESS_TO_OFFSET(JSONParamsStr) != '}') {
        numBuf[index++] = ACCESS_TO_OFFSET(JSONParamsStr);
        ADD_OFFSET(JSONParamsStr);
    }

    if (!CAN_ACCESS_TO_OFFSET(JSONParamsStr)) {
        DEBUG_LOG("fail to parse json value\r\n");
        return pdFAIL;
    }

    numBuf[index] = '\0';
    num = strtod(numBuf, NULL);
    if (num == 0.0) {
        DEBUG_LOG("strtod falied\r\n");
        return pdFAIL;
    }

    item->type = JSON_Number;
    item->valueNumber = num;

    return pdPASS;
}

int JSON_ParseString(JSONString_t *JSONParamsStr, JSON_t *item)
{
    int index = 0;

    // 跳过起始双引号
    ADD_OFFSET(JSONParamsStr);
    while (CAN_ACCESS_TO_OFFSET(JSONParamsStr) && ACCESS_TO_OFFSET(JSONParamsStr) != '"') {
        item->valueString[index++] = ACCESS_TO_OFFSET(JSONParamsStr);
        ADD_OFFSET(JSONParamsStr);
    }
    // 跳过末尾双引号
    ADD_OFFSET(JSONParamsStr);

    item->valueString[index] = '\0';
    item->type = JSON_String;

    return pdPASS;
}

static void JSON_PrintItemList(JSON_t *listHead)
{
    if (listHead == NULL) {
        return;
    }

    JSON_t *curItem = listHead;

    while (curItem != NULL) {
        if (curItem->type == JSON_Number)
            DEBUG_LOG("%s:%f -> ", curItem->nameStr, curItem->valueNumber);
        else if (curItem->type == JSON_String)
            DEBUG_LOG("%s:%s -> ", curItem->nameStr, curItem->valueString);
        else {
            DEBUG_LOG("wroung item type\r\n");
            return;
        }

        curItem = curItem->next;
    }
}

void JSON_Free(JSON_t *itemHead)
{
    if (itemHead == NULL) {
        return;
    }

    JSON_Free(itemHead->next);
    vPortFree(itemHead);
    return;
}
