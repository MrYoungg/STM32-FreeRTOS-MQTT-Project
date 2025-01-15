#ifndef __WIFICONNECT_H
#define __WIFICONNECT_H

#define WIFI_MODE     1;
#define SSID          "Xiaomi13";
#define WIFI_PASSWORD "1234567890";

typedef struct {
    int wifiMode;
    char *ssid;
    char *password;
} WiFi_t;

void WiFi_Config(WiFi_t *wifiMsg);
int WiFi_Connect(const WiFi_t *wifiMsg);

#endif
