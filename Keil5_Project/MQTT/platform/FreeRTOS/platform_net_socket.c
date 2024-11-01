/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2020-01-10 23:45:59
 * @LastEditTime: 2020-04-25 17:50:58
 * @Description: the code belongs to jiejie, please keep the author information and source code
 * according to the license.
 */
#include "platform_net_socket.h"

// 创建TCP连接
int platform_net_socket_connect(const char *host, const char *port, int proto)
{
    int ret;
    char *SSID = "Xiaomi13";
    char *password = "1234567890";

    // 1、修改WiFi模式：AT+CWMODE=3
    ret = AT_SendCommand("AT+CWMODE=3\r\n", AT_SEND_TIMEOUT);
    if (ret == AT_OK) {
        DEBUG_LOG("set wifi mode successfully\r\n");
    }
    else {
        DEBUG_LOG("fail to set wifi mode\r\n");
        return MQTT_SOCKET_FAILED_ERROR;
    }

    // 2、连接路由器（WiFi/热点）：AT+CWJAP="SSID","PassWord"
    char command[64];
    snprintf(command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, password);
    ret = AT_SendCommand(command, AT_SEND_TIMEOUT);
    if (ret == AT_OK) {
        DEBUG_LOG("connect wifi successfully\r\n");
    }
    else {
        DEBUG_LOG("fail to connect wifi\r\n");
        return MQTT_SOCKET_FAILED_ERROR;
    }

    // 3、与服务器建立TCP连接：AT+CIPSTART="TCP","192.168.16.74",8266
    char *curProto = (proto == PLATFORM_NET_PROTO_TCP) ? "TCP" : "UDP";
    snprintf(command, sizeof(command), "AT+CIPSTART=\"%s\",\"%s\",%s\r\n", curProto, host, port);
    ret = AT_SendCommand(command, AT_SEND_TIMEOUT);

    if (ret == AT_OK) {
        DEBUG_LOG("connect server successfully\r\n");
    }
    else {
        DEBUG_LOG("fail to connect server\r\n");
        return MQTT_SOCKET_FAILED_ERROR;
    }

    return MQTT_SUCCESS_ERROR;
}

int platform_net_socket_recv(int fd, void *buf, size_t len, int flags)
{
    return platform_net_socket_recv_timeout(fd, buf, len, 0);
}

/// @brief 读出网络数据包中的数据
/// @param fd
/// @param buf 存储读出数据的buf
/// @param len 数据长度
/// @param timeout 超时时间
/// @return 成功读取返回读取到的长度，失败返回MQTT_SOCKET_FAILED_ERROR
int platform_net_socket_recv_timeout(int fd, unsigned char *buf, int len, int timeout)
{
    int ret;
    ret = AT_Read_DataPacketBuffer(buf, len, timeout);

    return (ret == pdPASS) ? len : MQTT_SOCKET_FAILED_ERROR;
}

// 发送网络数据包（必须在建立TCP连接之后才能发送）
int platform_net_socket_write(int fd, void *buf, size_t len)
{
    return platform_net_socket_write_timeout(fd, buf, len, 0);
}

int platform_net_socket_write_timeout(int fd, unsigned char *buf, int len, int timeout)
{
    int ret;
    char sendCommand[32];
    // 1、发送数据输出AT命令：AT+CIPSEND=len
    snprintf(sendCommand, sizeof(sendCommand), "AT+CIPSEND=%d\r\n", len);
    ret = AT_SendCommand(sendCommand, timeout);

    // 2、收到OK，以及尖括号'>'之后，可以发送数据
    if (AT_DATA_REQUEST == ret) {
        ret = AT_SendData((char *)buf, timeout);
    }
    else {
        return MQTT_SOCKET_FAILED_ERROR;
    }

    // 3、数据发送成功：返回发送数据的长度
    if (AT_OK == ret) {
        return len;
    }
    else {
        return MQTT_SOCKET_FAILED_ERROR;
    }
}

// 关闭TCP连接:AT+CIPCLOSE
int platform_net_socket_close(int fd)
{
    return AT_SendCommand("AT+CIPCLOSE\r\n", AT_SEND_TIMEOUT);
}

#if 0
int platform_net_socket_set_block(int fd)
{
    return 0;
}

int platform_net_socket_set_nonblock(int fd)
{
    return 0;
}

int platform_net_socket_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
    return 0;
}
#endif
