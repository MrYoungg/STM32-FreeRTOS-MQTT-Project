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
    // 1、修改WiFi模式：AT+CWMODE=3

    // 2、连接路由器（WiFi/热点）：AT+CWJAP="SSID","PassWord"

    // 3、与服务器建立TCP连接：AT+CIPSTART="proto","host",port
    return 0;
}

// 接收网络数据包（收到"+IPD,<len>:<data>"格式的信息）
int platform_net_socket_recv(int fd, void *buf, size_t len, int flags)
{
    // 读AT_DataBuffer的数据，读不到则阻塞
    return 0;
}

int platform_net_socket_recv_timeout(int fd, unsigned char *buf, int len, int timeout)
{
    return 0;
}

// 发送网络数据包（必须在建立TCP连接之后才能发送）
int platform_net_socket_write(int fd, void *buf, size_t len)
{
    // 1、发送数据输出AT命令：AT+CIPSEND=11

    // 2、收到OK，以及尖括号'>'之后，可以发送数据
    return 0;
}

int platform_net_socket_write_timeout(int fd, unsigned char *buf, int len, int timeout)
{
    return 0;
}

// 关闭TCP连接
int platform_net_socket_close(int fd)
{
    return 0;
}

// int platform_net_socket_set_block(int fd)
// {
//     return 0;
// }

// int platform_net_socket_set_nonblock(int fd)
// {
//     return 0;
// }

// int platform_net_socket_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
// {
//     return 0;
// }
