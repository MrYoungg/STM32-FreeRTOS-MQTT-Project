#include <WiFiConnect.h>

void WiFi_Config(WiFi_t *wifiMsg)
{
    wifiMsg->wifiMode = WIFI_MODE;
    wifiMsg->ssid = SSID;
    wifiMsg->password = WIFI_PASSWORD;
}

int WiFi_Connect(const WiFi_t *wifiMsg)
{
    int ret;
    char command[AT_COMMAND_MAX_SIZE];

    // 1、修改WiFi模式：AT+CWMODE=wifiMode
    snprintf(command, sizeof(command), "AT+CWMODE=%d\r\n", wifiMsg->wifiMode);
    ret = AT_SendCommand(command, AT_SEND_TIMEOUT);
    if (ret == AT_OK) {
        DEBUG_LOG("set wifi mode successfully\r\n");
    }
    else {
        DEBUG_LOG("fail to set wifi mode\r\n");
        return AT_ERROR;
    }

    // 2、连接路由器（WiFi/热点）：AT+CWJAP="ssid","password"
    snprintf(
        command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"\r\n", wifiMsg->ssid, wifiMsg->password);
    ret = AT_SendCommand(command, AT_SEND_TIMEOUT);
    if (ret == AT_OK) {
        DEBUG_LOG("connect wifi successfully\r\n");
    }
    else {
        DEBUG_LOG("fail to connect wifi\r\n");
        ret = AT_ERROR;
    }
    return ret;
}