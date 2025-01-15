#include "JSON.h"
#include "stdbool.h"

uint8_t MQTT_GetJSONValue_Str(char *JSONStr, char *ItemName, char *ItemValueStr, uint32_t ValueStrLen)
{
    char *start = NULL;
    char *end = NULL;

    start = strstr(JSONStr, ItemName);
    if (start == NULL) {
        DEBUG_LOG("JSON 字符串中没有%s对象 \r\n", ItemName);
        return false;
    }

    while (*start != ':') start++;
    while (*start == ':' || *start == '"') start++;
    for (end = start; *end != ',' && *end != '"' && *end != '}'; end++);

    if ((uint8_t)(end - start) > ValueStrLen) {
        DEBUG_LOG("%s对象的值长度过大，无法获取 \r\n", ItemName);
        return false;
    }

    if (!memset(ItemValueStr, 0, ValueStrLen)) return false;
    if (!memcpy(ItemValueStr, start, (size_t)(end - start))) return false;

    return true;
}
