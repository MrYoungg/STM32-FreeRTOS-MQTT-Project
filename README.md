# STM32-FreeRTOS-MQTT-Project 项目手册

## chap 1 项目前置知识

### 1.0 MQTT协议

[MQTT协议中文版 (gitbooks.io)](https://mcxiaoke.gitbooks.io/mqtt-cn/content/mqtt/01-Introduction.html)

#### 1.0.1 协议模型

- 发布端（发布topic）- 服务器（将收到的topic下发给订阅端） - 订阅端（订阅topic）

#### 1.0.2 MQTT服务质量 - QoS

[MQTT QoS 0、1、2 解析：快速入门指南 | EMQ (emqx.com)](https://www.emqx.com/zh/blog/introduction-to-mqtt-qos)

##### （1）QoS 0 - 可能丢失

1. 仅发送一次，不需要响应；

##### （2）QoS 1 - 不会丢失，可能重复

1. 发送后会等待接收方的响应；
2. 如果一定时间内未响应则重传；
3. 缺点：由于响应没这么快到达发送方，接收方在接收成功的情况下，仍有可能收到重复的多个数据包；

##### （3）QoS 2 - 不会丢失，不会重复

1. 发送后等待响应；
2. 一定时间内无响应则重传；
3. 如果收到响应，则进入 packet ID 释放阶段，通信双方释放当前数据包的 packet ID ，使得该ID能够代表新的数据包；

#### 1.0.3 MQTT控制报文格式

| 字段类型 |                           字段内容                           |
| :------: | :----------------------------------------------------------: |
| 固定报头 | 报文类型（4 bits）+报文标志位（4 bits）+剩余长度（1~4 bytes） |
| 可变报头 |                    描述特定报文的某些特征                    |
| 有效载荷 |                            数据包                            |

##### （1）固定报头

![image-20241107134900354](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411071349410.png)

1. ##### MQTT控制报文类型（ byte 1 [4:7] ）
   
    ![image-20241107135031712](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411071350768.png)
    
2. ##### 报文标志位（byte 2 [0:3]）

    ![image-20241107140129075](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411071401127.png)

3. ##### 剩余长度（byte 2 ~ ...）

    - 最多占据固定报头中的4个字节；
    - 每个字节的 [0 : 6] 表示数值（0 ~ 127），是拓展位，代表是否还有下一字节；
    - 完整的“剩余长度”数值以**128进制**表示，即：
        `Remaining_Length = 1*a + 128 * b + 128^2 * c + 128^3 * d` 
    - 其中 a、b、c、d 分别是4个字节（除去最高位 [ 7 ] ）所代表的数值（0 ~ 127）
        ![image-20241107141418526](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411071414577.png)

##### （2）可变报头 - 以 CONNECT 报文为例

![image-20241107140448309](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411071404361.png)

##### （3）有效载荷 - 以 CONNECT 报文为例

- 客户端标识符（Client Identifier）
- 遗嘱主题 Will Topic
- 用户名 User Name
- 密码 Password

#### 1.0.4 常用MQTT控制报文

##### （1）CONNECT - 与服务端建立MQTT会话

![image-20241107182455250](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411071824370.png)

1. ##### 固定报头

    1. 具体数据为：

    ![image-20241107201033046](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411072010100.png)

2. ##### 可变报头

    - 协议名称 - Protocol Name
        - 长度 4（2 bytes） + 字符串”MQTT“ （4 bytes）
    - 协议等级（基于版本）-  Protocol Level
        - 在MQTT 3.1.1版本中，协议等级为4
    - 连接标志位 - Connect Flags
        ![image-20241107201427102](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411072014148.png)
    - 超时时间 - Keep Alive（2 bytes）
        - 以秒为单位，如果Server在1.5倍超时时间中没有收到Client的控制报文，则断开连接；
            ![image-20241107201706555](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411072017600.png)

3. ##### 有效载荷

    > 有效载荷中的每个字段都必须以**UTF-8字符串编码格式**编码，即：最前面的 2 bytes 是后续字符串的长度，单个字符的编码值与ASCII相同；

    1. 客户端标识符 - Client Identifier

    2. 用户名 - User Name
    3. 密码 - Password

##### （2）CONNACK – 连接请求确认

1. 固定报头：0x20 + 剩余长度；
2. 可变报头：
    ![image-20241108005654965](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411080056187.png)
    - SP=0，没有已存储的对话状态，开始新对话即可；SP = 1，存储有会话状态；
    - 返回码
        ![image-20241108010215891](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411080102949.png)
3. 有效载荷：无

##### （3）PUBLISH – 发布消息

1. **固定报头**
    ![image-20241108011817232](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411080118287.png)
    - DUP = 0，该PUBLISH报文第一次发送；DUP = 1，早前报文的重发；
    - QoS等级：00、01、10
        ![image-20241108011953503](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411080119553.png)
    - RETAIN = 1
        - 服务器收到时，代表保存该消息和QoS，如果QoS=0，则还要丢弃该主题之前保留的所有消息；
        - 客户端收到时，代表这是个新订阅主题的发布；
    - RETAIN = 0
        - 服务器收到时，代表不存储该消息，也不能移除或替换任何现存的保留消息；
        - 客户端收到时，代表这是一个已建立订阅的匹配消息；
2. **可变报头**：主题长度 + 主题名 + 报文标识符（QoS>=1）
3. **有效载荷**：特定格式的数据（阿里云 - Alink JSON）
   
    - [物模型属性、事件、服务的Alink JSON数据格式和Topic_物联网平台(IoT)-阿里云帮助中心 (aliyun.com)](https://help.aliyun.com/zh/iot/user-guide/device-properties-events-and-services?spm=a2c4g.11186623.0.0.45a2161flk6UOe#section-g4j-5zg-12b)
    
    - ```json
        {
            "method":"thing.event.property.post",
            "id":"0000000001",
            "params":{"LightSwitch":0,"CurrentTemperature":25,"Brightness":66},	
            "version":"1.0.0"
        }
        ```

##### （4）PUBACK –发布确认

1. 固定报头
2. 可变报头
3. 有效载荷

##### （5）SUBSCRIBE - 订阅主题

![image-20241107210549076](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411072105183.png)

1. 固定报头：0x82 + 剩余长度；
2. 可变报头：报文标识符 Packet ID，用于标识本轮对话（订阅+订阅响应）；
3. 有效载荷：主题长度 + 主题（过滤器） + QoS （有效载荷是N个上述序列的组合，N>=0）；

##### （6）SUBACK - 订阅确认

1. 固定报头：0x90 + 剩余长度
2. 可变报头：等待确认的 SUBSCRIBE 报文的报文标识符 Packet ID；
3. 有效载荷：无；

##### （7）PINGREQ - 心跳请求

1. 固定报头：`0xC0 0X00`；
2. 可变报头：无；
3. 有效载荷：无；

##### （8）PINGRESP – 心跳响应

1. 固定报头：`0xD0 0x00`
2. 可变报头：无；
3. 有效载荷：无；

##### （9）DISCONNECT –断开连接

### 1.1 MQTT协议 - 源码分析

#### 1.1.1 连接服务器并等待包 - 核心线程

1. **核心逻辑：**等待服务器发布消息(publish) → 收到消息后解析消息，获取主题(topic) → 如果我已订阅的主题（链表）中有该主题 → 调用对应的handler函数；

2. **主要函数调用流程**

    ```c
    mqtt_yield_thread(c, c->mqtt_cmd_timeout) // 核心线程
    	while(1)
            // 核心函数，等待服务器发来的MQTT包
    		mqtt_yield(c, c->mqtt_cmd_timeout) 
    		    // MQTT包处理函数
    			mqtt_packet_handle(c, &timer) 
    			    // 读取接收到的包
    				rc = mqtt_read_packet(c, &packet_type, timer) 
            		...
    				// 如果读取到的是服务器发布的包，调用该函数处理
    				mqtt_publish_packet_handle(c, timer) 
    				    // 投递信息
    				    mqtt_deliver_message(c, &topic_name, &msg) 
    				       	if(NULL != msg_handler){
                                // 调用对应的主题处理函数
    				               msg_handler->handler(c, &md); 
    				           }
    ```

##### （1）CONNECT报文实现

1. **固定报头**

    - Byte 1: 报文类型 + 报文标志位(0000)

        ```c
        // 通过 union + struct 来控制每个位字段
        typedef union
        {
        	unsigned char byte;	                /**< the whole byte */
        	struct
        	{
        		unsigned int retain : 1;	/**< retained flag bit, 1 bit */
        		unsigned int qos : 2;		/**< QoS value, 0, 1 or 2, 2 bits */
        		unsigned int dup : 1;		/**< DUP flag bit, 1 bit */
        		unsigned int type : 4;		/**< 报文类型, 4 bits */
        	} bits;
        } MQTTHeader;
        ```

    - 剩余长度

        ```c
        
        ```

        

2. **可变报头**
    - 

#### 1.1.2 发布消息

#### 1.1.3 订阅主题

```c
mqtt_subscribe(client, "topic1", QOS0, topic1_handler);
```

1. **实现逻辑：**序列化订阅信息的数据包 → 发送数据包 → 成功发送后创建一个消息结构体`msg_handler`，记录订阅的主题、服务质量、回调函数等信息 → 将该结构体放入链表中管理；

2. **主要函数调用流程**

    ```c
    // 订阅主题topic1
    mqtt_subscribe(client, "topic1", QOS0, topic1_handler){
        MQTTSerialize_subscribe(); // 序列化订阅信息的数据包
        mqtt_send_packet();{ // 发送数据包
            network_write();
        }
        // 创建一个消息结构体`msg_handler`，记录订阅的主题、服务质量、回调函数等信息
        msg_handler = mqtt_msg_handler_create(); 
        mqtt_ack_list_record(); // 将该结构体放入链表中管理
    }
    ```

### 1.2 MQTT移植（非必须，利用AT-MQTT命令也可以）

#### 1.1.1 将MQTT库移植到Keil工程中

##### （1）过程

1. 将`mqttclient\platform\FreeRTOS`文件夹中的.c文件移植到工程中；
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137350.png" alt="image-20240922163318680" style="zoom: 50%;" />

##### （2）问题

1. `platform_net_socket.h`中找不到`lwip/api.h`

    ```c
    MQTT\platform\FreeRTOS\platform_net_socket.h(15): error:  #5: cannot open source input file "lwip/api.h": No such file or directory
      #include "lwip/api.h"
    ```

    - 原因：未移植LwIP库；

#### 1.2.2 LwIP库移植

##### （1）问题1

- 将LwIP库加入工程文件夹中，并在Keil路径中包含`.\LwIP\src\include`（其中有`lwip/api.h`）；

- **报错：**`lwip/arch.h`中找不到`arch/cc.h`头文件；

- **解决办法：**编写适配于Keil中ARMCC编译器的cc.h文件（定义与处理器相关的数据类型）；

    ```c
    #ifndef __CC_H__
    #define __CC_H__
    
    #include "stdio.h"
    // #include <errno.h>
    
    #define U16_F "hu"
    #define S16_F "d"
    #define X16_F "hx"
    #define U32_F "u"
    #define S32_F "d"
    #define X32_F "x"
    #define SZT_F "uz"
    
    /* define compiler specific symbols */
    #if defined(__ICCARM__)
    
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_STRUCT
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(x) x
    #define PACK_STRUCT_USE_INCLUDES
    
    #elif defined(__CC_ARM)
    
    #define PACK_STRUCT_BEGIN __packed
    #define PACK_STRUCT_STRUCT
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(x) x
    
    #elif defined(__GNUC__)
    
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_STRUCT __attribute__((__packed__))
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(x) x
    
    #elif defined(__TASKING__)
    
    #define PACK_STRUCT_BEGIN
    #define PACK_STRUCT_STRUCT
    #define PACK_STRUCT_END
    #define PACK_STRUCT_FIELD(x) x
    
    #endif
    
    #define LWIP_PLATFORM_ASSERT(x)                                                                    \
        do {                                                                                           \
            printf(x);                                                                                 \
        } while (0)
    
    #endif /* __CC_H__ */
    
    ```

##### （2）问题2

- 找不到宏：`EINVAL`

    ```c
    LwIP\src\api\netdb.c(182): error:  #20: identifier "EINVAL" is undefined
          *h_errnop = EINVAL;
    ```

- 解决办法：

    1. 在lwipopts.h中定义宏：`#define LWIP_PROVIDE_ERRNO 1`；
    2. 使得errno.h中对应的宏得到定义：
        ![image-20240922231757547](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137440.png)

##### （3）问题3

- 找不到路径`"path/to/my/lwip_hooks.h"`

    ```c
    LwIP\src\api\sockets.c(68): error:  #5: cannot open source input file "path/to/my/lwip_hooks.h": No such file or directory
      #include 
    WIP_HOOK_FILENAME
    ```

- 解决办法

    1. opt.h中有如下代码，用于启用自定义的勾子函数；

        ```c
        /**
         * LWIP_HOOK_FILENAME: Custom filename to \#include in files that provide hooks.
         * Declare your hook function prototypes in there, you may also \#include all headers
         * providing data types that are need in this file.
         */
        #ifdef __DOXYGEN__
        #define LWIP_HOOK_FILENAME "path/to/my/lwip_hooks.h"
        #endif
        ```

    2. 先将该路径注释；
        ![image-20240922174719713](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137949.png)

##### （4）问题4

- 找不到errno的定义

    ```c
    .\Objects\Project.axf: Error: L6218E: Undefined symbol errno (referred from sockets.o).
    ```

- 解决办法：这是个全局变量，在errno.h中通过extern声明在外部，但外部并未找到定义，因此在err.c中定义即可

    ```c
    ./lwip/errno.h
    #ifndef errno
    extern int errno;
    #endif
    
    ./src/api/err.c
    int errno;
    ```

##### （5）LwIP系统框图

<img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137202.png" alt="img" style="zoom: 67%;" />

- **RTOS：**FreeRTOS为LwIP提供的操作系统级API接口，如创建线程、提供锁等；
    - 主要通过freertos/sys_arch.c实现；
- **Network System Config：**对LwIP协议栈的系统设置。
    - 主要通过opt.h（系统默认设置）和lwipopts.h（用户设置）实现；
- **LwIP Stack：**LwIP的TCP/IP协议栈。
    - 主要文件有：`LwIP/src/api/*.c`、`LwIP/src/core/*.c`、`LwIP/src/core/ipv4/*.c`、
- **Hardware Driver：**主要是STM32平台以太网接口的驱动层；
    - 主要通过`ethernet.c`实现以太网数据的收发；

## chap 2 项目实现

### 2.0 项目常用AT命令

> AT指令的分类：
> ![image-20241105110857279](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202411051108352.png)

#### 2.0.1 查询/设置WiFi模式：AT+CWMODE

```shell
#命令：设置WiFi模式为3（Station+AP）
AT+CWMODE=3

#响应
AT+CWMODE=3

OK
```

##### （1）查询命令

查询 ESP 设备的 Wi-Fi 模式

```shell
#命令
AT+CWMODE?

#响应
AT+CWMODE:<mode>
OK
```

##### （2）设置命令

设置 ESP 设备的 Wi-Fi 模式

```shell
#命令
AT+CWMODE=<mode>[,<auto_connect>]

#响应
AT+CWMODE=<mode>[,<auto_connect>]

OK
```

##### （3）参数

- **<mode\>**：模式
    - 0: 无 Wi-Fi 模式，并且关闭 Wi-Fi RF
    - 1: Station 模式（作为连接端，类似没有热点的手机）
    - 2: SoftAP 模式（作为网络接入点，类似路由器）
    - 3: SoftAP+Station 模式
- **<auto_connect>**：切换 ESP 设备的 Wi-Fi 模式时（例如，从 SoftAP 或无 Wi-Fi 模式切换为 Station 模式或 SoftAP+Station 模式），是否启用自动连接 AP 的功能，默认值：1。参数缺省时，使用默认值，也就是能自动连接。
    - 0: 禁用自动连接 AP 的功能
    - 1: 启用自动连接 AP 的功能，若之前已经将自动连接 AP 的配置保存到 flash 中，则 ESP 设备将自动连接 AP

#### 2.0.2  连接 AP：AT+CWJAP

```shell
// 如果目标 AP 的 SSID 是 "abc"，密码是 "0123456789"，则命令是：
AT+CWJAP="abc","0123456789"

// 如果目标 AP 的 SSID 是 "ab\,c"，密码是 "0123456789"\"，则命令是：
AT+CWJAP="ab\\\,c","0123456789\"\\"

// 如果多个 AP 有相同的 SSID "abc"，可通过 BSSID （MAC地址）找到目标 AP：
AT+CWJAP="abc","0123456789","ca:d7:19:d8:a6:44"

// 如果 ESP-AT 要求通过 PMF 连接 AP，则命令是：
AT+CWJAP="abc","0123456789",,,,,,,3
```

##### （1）查询命令

查询已连接的AP信息

```shell
#命令
AT+CWJAP?

#响应
AT+CWJAP:<ssid>,<bssid>,<channel>,<rssi>,<pci_en>,<reconn_interval>,<listen_interval>,<scan_mode>,<pmf>
OK
```

##### （2）设置命令

设置 ESP station 想要连接的AP

```shell
#命令
AT+CWJAP=[<ssid>],[<pwd>][,<bssid>][,<pci_en>][,<reconn_interval>][,<listen_interval>][,<scan_mode>][,<jap_timeout>][,<pmf>]

#响应
WIFI CONNECTED
WIFI GOT IP

OK
[WIFI GOT IPv6 LL]
[WIFI GOT IPv6 GL]
```

##### （3）执行命令

将 ESP station 连接至上次 Wi-Fi 配置中的 AP

```shell
#命令
AT+CWJAP

#响应
WIFI CONNECTED
WIFI GOT IP

OK
[WIFI GOT IPv6 LL]
[WIFI GOT IPv6 GL]
```

##### （4）参数

参考：[Wi-Fi AT 命令集 — ESP-AT 用户指南 文档 (espressif.com)](https://docs.espressif.com/projects/esp-at/zh-cn/release-v2.2.0.0_esp8266/AT_Command_Set/Wi-Fi_AT_Commands.html#cmd-jap)

#### 2.0.3 单连接/多连接模式：AT+CIPMUX

#### 2.0.4 设置传输模式为 - 普通/透传模式：AT+CIPMODE

#### 2.0.5 与远端建立TCP连接：AT+CIPSTART

#### 2.0.6 在普通/透传模式下发送数据：AT+CIPSEND

#### 2.0.7 MQTT - AT：



### 2.1 网络框架层次

![image-20240929153217800](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137382.png)

#### 2.1.1 **应用层**

- MQTT协议栈，调用平台网络层实现的函数，包括：创建连接、收发信息、关闭连接等；实现MQTT工作逻辑；

#### 2.1.2 **平台网络层**

- 调用AT命令发送函数，封装MQTT协议栈需要的函数；向ESP8266/ESP32等发送各类AT命令，操作其通过路由器连接网络；

#### **2.1.3 传输层**

- 调用串口收发函数，封装AT命令发送函数，发送AT命令；

#### 2.1.4 **数据链路层**

- 调用底层的寄存器操作命令，向上封装串口收发函数，通过串口收发字节流；

### 2.2 AT固件烧录+测试

#### 2.2.1 AT固件+烧录软件下载

##### （1）AT固件下载

1. **ESP32系列：**[发布的固件 - ESP32 - — ESP-AT 用户指南 latest 文档 (espressif.com)](https://docs.espressif.com/projects/esp-at/zh_CN/latest/esp32/AT_Binary_Lists/esp_at_binaries.html)、
2. **ESP8266系列：**[发布的固件 — ESP-AT 用户指南 文档 (espressif.com)](https://docs.espressif.com/projects/esp-at/zh-cn/release-v2.2.0.0_esp8266/AT_Binary_Lists/ESP8266_AT_binaries.html)
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137366.png" alt="image-20240929161553175" style="zoom: 67%;" />

##### （2）烧录软件下载

1. 下载地址：[工具｜乐鑫科技 (espressif.com)](https://www.espressif.com/zh-hans/support/download/other-tools)
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137586.png" alt="image-20240929161700983" style="zoom: 67%;" />

#### 2.2.2 固件烧录过程

##### （1）硬件连接

1. **通过Type-c烧录（简单稳定）：**通过USB-TypeC数据线连接电脑USB口和ESP8266Type-c口；
2. **通过串口烧录：**通过USB转TTL接口连接电脑USB口和ESP8266串口0（ESP8266的UART0用于固件烧录，GPIO3-RX和GPIO1-TX）;

##### （2）烧录软件操作

1. 选择**ChipType:ESP8266，WorkMode:Develop；**
    ![image-20240929162851523](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137227.png)
2. 选择**AT固件库-factory文件夹**中的**factory_xxx.bin**文件，烧录进ESP8266即可（记得勾选文件！）
    ![image-20240929163858297](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137044.png)

#### 2.2.3 烧录后测试

##### （1）硬件连接

1. ESP8266上烧录固件、收发AT命令和输出日志的端口是不同的，如图所示；ESP8266的UART0被映射到两组不同的端口，分别用于烧录固件和收发AT指令；
    ![image-20240929162552106](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137661.png)
2. 在测试AT命令时需要将串口连接到ESP8266的**D7(GPIO13-RX)**和**D8(GPIO15-TX)**；
    ![image-20240929165041589](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137379.png)

##### （2）AT指令

发送以下AT命令；

```
AT+GMR

```

收到回复：AT固件的版本信息；
<img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137183.png" style="zoom: 80%;" />

### 2.3 TCP连接测试

#### 2.3.1 连接WiFi

1. 设置手机热点为**2.4GHz**频段，ESP8266仅支持连接2.4GHz网络；
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291225033.jpg" alt="97e288bb3f05ab69b391ca1091b7fe2" style="zoom:25%;" />

2. 使用AT命令连接到WiFi热点

    ```c
    // 查询当前WiFi模式；
    // 0：无WiFi并关闭WiFi无线电频段
    // 1：Station模式，作为客户端，连接到现有的无线网络中；
    // 2：SoftAP模式，创建一个无线热点，作为网络接入点；
    // 3：Station+SoftAP模式；
    AT+CWMODE?
    
    // 设置当前WiFi模式为3
    AT+CWMODE=3
    
    // 连接到AP（Access Point，接入点）
    AT+CWJAP="SSID","PassWord"
    ```


​    
​    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137915.png" alt="image-20240929222436873" style="zoom:67%;" />

#### 2.3.2 与本地服务器创建TCP连接

##### （1）**新建win10防火墙入站规则**

1. 控制面板 - 系统和安全 - Windows Defender 防火墙 - 高级设置 - 入站规则 - 新建规则;
2. 选择ESP8266想要连接的端口，选择允许接入即可；
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291449743.png" alt="image-20240929144932691" style="zoom:50%;" />

##### （2）创建TCP连接

1. **AT指令**

    ```c
    // 查询当前TCP连接状态
    // 0: ESP station 为未初始化状态
    // 1: ESP station 为已初始化状态，但还未开始 Wi-Fi 连接
    // 2: ESP station 已连接 AP，获得 IP 地址
    // 3: ESP station 已建立 TCP、UDP 或 SSL 传输
    // 4: ESP 设备所有的 TCP、UDP 和 SSL 均断开
    // 5: ESP station 开始过 Wi-Fi 连接，但尚未连接上 AP 或从 AP 断开
    AT+CIPSTATUS
    
    // 建立TCP连接
    AT+CIPSTART="TCP","192.168.225.74",8266
    
    ```

    ![image-20240929223341072](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137668.png)

2. **ESP8266 - Server 通信**

    - 客户端在普通模式下向服务器发送信息时，必须通过以下方式

        ```shell
        // 发送指定长度(11 Bytes)的数据
        AT+CIPSEND=11
        
        // 响应
        AT+CIPSEND=11
        
        OK
        
        >
        // 收到尖括号反馈后,再向服务器发相应长度(11 Bytes)的信息
        ```

    - 当客户端接收到服务器数据时，会通过串口回发以下格式的信息；**逗号**后是接收到的数据长度，**冒号**后面是接收到的数据本身；

        ```shell
        +IPD,13:from server\r\n
        ```

        ![image-20240929154047153](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137178.png)

### 2.4 数据链路层实现：串口收发数据

#### 2.4.1 串口发送 - AT_USART_SendString()

##### 1、概述

- 循环调用USART的字节数据发送函数AT_USART_SendByte()，实现字符串发送；

##### 2、代码

```c
/// @brief AT命令串口发送一个字节数据
/// @param Byte 待发送数据
static void AT_USART_SendByte(uint8_t Data)
{
    USART_SendData(AT_USARTx, Data);
    // 死等发送
    // 等待发送数据寄存器空
    while (!USART_GetFlagStatus(AT_USARTx, USART_FLAG_TXE)) 
    {
    }
}

/// @brief AT命令串口发送一个字符串
/// @param String 待发送字符串
void AT_USART_SendString(char *String)
{
    for (uint8_t i = 0; String[i] != 0; i++) {
        AT_USART_SendByte(String[i]);
    }
}
```

#### 2.4.2 串口接收(基于USARTx_IRQHandler)

##### 1、接收数据到环形缓冲区 - USART_Write_Buffer()

> 拓展到串口接收中断USART3_IRQHandler;

1. **概述**：串口接收到数据后，触发RXNE（接收寄存器非空）中断，将数据写入**环形缓冲区**`USART_Buffer.buffer[]`；在某个缓冲区**兼有写入和读出功能**的时候，适合于采用环形缓冲区；

2. **代码**

    ```c
    #define USART_Write_Buffer USART3_IRQHandler
    
    /// @brief 接收中断处理,将接收到的数据写入环形缓冲区
    /// @param 无
    void USART_Write_Buffer(void)
    {
        // 清理：ORE - 溢出错误中断
        // 串口数据寄存器数据未取走时，如果又收到一个数据，则会触发溢出错误中断
        if (USART_GetITStatus(AT_USARTx, USART_IT_ORE)) {
            // 读状态寄存器（在if中） + 读数据寄存器：清除ORE_Flag
            USART_ReceiveData(AT_USARTx); 
        }
    
        if (USART_GetITStatus(AT_USARTx, USART_IT_RXNE)) {
            // 串口收到数据,唤醒USART_Receive_Mutex
            // platform_mutex_unlock(&USART_Receive_Mutex);
            platform_mutex_unlock_from_isr(&USART_Receive_Mutex);
    
            volatile uint8_t receiveData = USART_ReceiveData(AT_USARTx);
    
            if (USART_Buffer.DataSize < USART_BUFFER_SIZE) {
                // 通过取余实现环形缓冲区
                USART_Buffer.writeIndex = (USART_Buffer.writeIndex + 1) % USART_BUFFER_SIZE;
                USART_Buffer.buffer[USART_Buffer.writeIndex] = receiveData;
                USART_Buffer.DataSize++;
                Serial_rxFlag = RX_DATA_RECEIVED;
            }
            else {
                Serial_rxFlag = RX_BUFFER_OVERFLOW;
            }
        }
    }
    ```

##### 2、读出环形缓冲区的数据 - USART_Read_Buffer()

1. **概述**：将环形缓冲区中收到的数据读出到字符串中，如果环形缓冲区溢出，则丢弃本次数据，并清空缓冲区；

2. **代码**

    ```c
    int USART_Read_Buffer(char *responseBuffer,TickType_t timeout)
    {
    
        while (USART_Buffer.DataSize == 0) {
            // 缓冲区空,则在USART_Receive_Mutex上阻塞,直到接收到数据
            DEBUG_LOG("receive mutex lock\r\n");
            platform_mutex_lock_timeout(&USART_Receive_Mutex, timeout);
            DEBUG_LOG("receive mutex unlock\r\n");
        }
        DEBUG_LOG("DataSize = %d\r\n", USART_Buffer.DataSize);
    
        // 缓冲区溢出,数据不完整,直接放弃本次写入的数据,并清空缓冲区
        if (Serial_rxFlag == RX_BUFFER_OVERFLOW) {
            DEBUG_LOG("USART_Buffer overflow, clear the data receive this time\r\n");
            USART_Buffer.DataSize = 0;
            USART_Buffer.readIndex = USART_Buffer.writeIndex;
        }
        // 缓冲区没有溢出,其中有完整数据,则读取到responseBuffer中
        else if (Serial_rxFlag == RX_DATA_RECEIVED) {
            DEBUG_LOG("USART_Buffer->responseBuffer\r\n");
    
            for (int i = 0; i < USART_Buffer.DataSize; i++) {
                // 环形缓冲区
                USART_Buffer.readIndex = (USART_Buffer.readIndex + 1) % USART_BUFFER_SIZE;
                responseBuffer[i] = USART_Buffer.buffer[USART_Buffer.readIndex];
            }
    
            responseBuffer[USART_Buffer.DataSize] = '\0';
            USART_Buffer.DataSize = 0;
        }
        else {
            DEBUG_LOG("Serial_rxFlag else\r\n");
        }
    
        return Serial_rxFlag;
    }
    ```

### 2.5 传输层实现：AT命令发送/响应接收

#### 2.5.0 传输层的多线程框架

![image-20241025175245287](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137594.png)

##### （1）线程1 - AT命令发送

1. 发送AT命令，并且休眠等待AT响应；
2. **①收到AT响应 ②超时**，则会被唤醒；
3. 检测当前状态：超时/接收到响应；
4. 继续执行其他任务；

##### （2）线程2 - AT响应接收

1. 检测串口环形缓冲区，当环形缓冲区空时休眠等待；
2. **当环形缓冲区中存在数据时，将会被唤醒；**
3. 读取唤醒缓冲区的数据，并对其进行解析，判断是什么类型的数据（OK/ERROR/+IPD等）；
4. 如果是正常的响应(OK/ERROR)，则**唤醒线程1 - AT命令发送线程；**
5. 如果是数据包(+IPD)，则将数据写入AT环形缓冲区，并**唤醒线程3 - AT数据包接收/处理**

##### （3）线程3 - AT数据包接收/处理

1. 如果AT环形缓冲区没有数据，则休眠；
2. 当**线程2**接收到server发来的数据包时，会将本线程唤醒；
3. 读取AT环形缓冲区中的数据，根据数据执行不同的操作；

##### （4）串口接收中断

1. 串口收到数据时触发中断，将数据写入环形缓冲区；
2. **唤醒**AT响应接收线程；

#### 2.5.1 AT命令发送 - AT_SendCommand()

##### （1）概述

1. 发送AT命令，并休眠等待响应；
2. 唤醒后根据AT_Status继续执行相应的工作；

##### （2）代码

```c

```

#### 2.5.2 AT响应接收 - AT_ReceiveResponse()

#### 2.5.3 AT响应解析 - AT_ParseResponse()

#### 2.5.4 AT数据包处理 - AT_ProcessData()

### 2.6 网络层实现：网络连接和数据传输

#### 2.6.1 建立网络连接 - platform_net_socket_connect()

##### （1）流程概述

1. 配置ESP8266的WiFi模式：`AT+CWMODE=3`；
2. 连接路由器节点（WiFi/热点）：`AT+CWJAP="SSID","PassWord"`；
3. 与MQTT服务器建立TCP连接：`AT+MQTTCONN=<LinkID>,<"host">,<port>,<reconnect>`；

##### （2）代码

```c

```

##### （3）成功与服务器连接 - 从服务器接收数据

![image-20241028154542623](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137290.png)

#### 2.6.4 断开网络连接 - platform_net_socket_close()

1. 关闭TCP连接：`AT+CIPCLOSE`；

### 2.7 应用层：MQTT 应用

[MQTT AT Commands — ESP-AT 用户指南 文档 (readthedocs-hosted.com)](https://espressif-docs.readthedocs-hosted.com/projects/esp-at/zh-cn/release-v2.2.0.0_esp32/AT_Command_Set/MQTT_AT_Commands.html#)

#### 2.7.0 JSON 格式

##### 2.7.0.0 JSON 格式概述

[JSON - 维基百科(wikipedia.org)](https://zh.wikipedia.org/wiki/JSON)

1. **JSON对象基本结构**：（JSON根对象）{ 键:值对 (Key:Value) 序列}；

2. **键的数据类型**：只能是`“”`括起来的字符串；

3. **值的数据类型**：可以是所以基本数据类型，包括

    - **数值 double**：十进制数，不能有前导0，可以为负数，可以有小数部分。还可以用`e`或者`E`表示指数部分。不能包含非数，如NaN。不区分整数与浮点数；
    - **字符串 string**：可以为空字符；
    - **布尔值**：true/false；
    - **数组**：用`[]`括起，数组元素之间用逗号`,`分割，每个值都可以为任意类型；
    - **JSON 对象**：JSON对象中的某个键值对的值可以是另一个JSON子对象；

4. **实例**

    ```json
    {
        "name":"John",
        "age":30,
        "isStudent":false,
        "hobbies":["Reading", "Programming"],
        "address":
        {
             "streetAddress": "21 2nd Street",
             "city": "New York",
             "state": "NY",
             "postalCode": "10021" 
        }
    }
    ```

##### 2.7.0.1 JSON - 对象管理链表

##### 2.7.0.2 JSON - 生成

##### 2.7.0.3 JSON - 解析

#### 2.7.1 订阅主题

#### 2.7.2 发布消息

##### 

#### 2.7.3 处理消息



#### 2.7.4 心跳包

##### （1）客户端

- ESP32在设置了keepalive之后会自动向服务器发送PINGREQ；

##### （2）服务端

- 阿里云物联网平台要求keepalive的取值范围是[30,1200]；

### ⭐2.x OTA远程升级

## chap 3 项目总结的

### 3.1 debug记录

#### 3.1.1 STM32基于串口初始化发送0xFF

1. **可能原因：**GPIO口和串口初始化顺序有问题，GPIO时钟 - GPIO - USART时钟 - USART
2. **现象：**
    1. 按照上述顺序初始化时，发现在执行到开启USART时钟时，串口会输出一个0xFF；
        ![image-20241018141745123](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137188.png)
    2. 怀疑串口将电平误判为数据，在初始化USART时钟前**加一个延时**：GPIO时钟 - GPIO - **延时** - USART时钟 - USART，发现串口输出数据变为0x00；
    3. 通过单步调试发现，仅当延时作用于**初始化TX的GPIO口**和**开启USART时钟**之间时，会改为输出0x00；
        ![image-20241018145710544](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137914.png)
    4. 将初始化顺序改成：GPIO时钟 - USART时钟 - GPIO - USART；串口在初始化过程中不再输出任何数据；
        ![image-20241018142921367](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137006.png)
3. **具体原因分析**
    1. 只要供电正常，ESP8266模块的RX口就处于时刻准备接收的状态；
    2. 如果在**初始化GPIO为复用推挽输出模式**（将GPIO_Pin的控制权交给外设）时**还未初始化USART时钟**，则会导致GPIO_Pin的电平处于**不确定状态**；
    3. 而**GPIO初始化**（GPIO_Init()）和**USART时钟初始化**（AT_USARTx_APBxCLOCKCMD()）的过程中，都会产生电平波动；此时TX引脚上的电平波动就有可能让ESP8266误认为有数据传输；电平时序如图所示
        ![image-20241018162435209](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137503.png)
    4. 如果**先初始化USART时钟再初始化GPIO外设为复用推挽输出模式**，则GPIO_Pin的电平由USART确定（高/低），不会处于不确定的波动状态；

#### 3.1.2 在task中添加串口打印日志时，进入HardFault

1. **原因**：task的任务栈大小不足，导致栈溢出（内存访问错误），因此进入HardFault；
2. **解决办法**：创建任务时申请更大的栈；
3. **注意点**：
    - 栈溢出与否与打印的字符串长度无关，因为**字符串常量**本身存储在整个程序代码段的**只读数据段（.rodata）**中，通过指针传给打印函数，该指针存储在任务栈中
        ![image-20241021181124121](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137160.png)
    - 对于字符数组会有点不一样，如果在任务中定义了一个字符数组，那么**整个字符数组都存储在任务栈中**；
        ![image-20241021182227843](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137002.png)

#### 3.1.3 初始化环形缓冲区（RingBuffer）出错

1. **现象：**

    - RingBuffer结构体定义如下

        ```c
        typedef struct {
            uint8_t *bufferHead;          // 缓冲区开头
            uint32_t bufferSize;          // 缓冲区大小
            volatile uint32_t dataSize;   // 当前已有数据项大小
            volatile uint32_t readIndex;  // 指向上次读出末尾的下一个位置
            volatile uint32_t writeIndex; // 指向上次写入末尾的下一个位置
        } RingBuffer_t;
        ```

        

    - RingBuffer初始化函数的定义、调用如下

        ```c
        // main.c中调用：
        RingBuffer_t *testBuffer;
        RingBuffer_Init(&testBuffer, 10);
        
        //RingBuffer.c中定义：
        int RingBuffer_Init(RingBuffer_t *ringBuffer, uint32_t bufferSize){
            // 1、malloc动态分配内存
            DEBUG_LOG("sizeof(RingBuffer_t):%d\r\n",sizeof(RingBuffer_t));
            (ringBuffer) = (RingBuffer_t *)pvPortMalloc(sizeof(RingBuffer_t) + bufferSize);
            if (ringBuffer == NULL) {
                return pdFAIL;
            }
        
            // 静态分配
            // ringBuffer->buffer = buffer;
        
            // 2、重新定位bufferHead的位置
            (ringBuffer)->bufferHead += sizeof(RingBuffer_t);
        
            // 3、初始化其他成员变量
            (ringBuffer)->bufferSize = bufferSize;
            (ringBuffer)->dataSize = 0;
            (ringBuffer)->readIndex = 0;
            (ringBuffer)->writeIndex = 0;
        
            return pdPASS;
        }
        ```

        

    - 发现传入testBuffer之后无法初始化，testBuffer内部仍然是未定义数据，但函数内部的形参已经正确初始化；
        ![image-20241027154434672](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410291137218.png)

2. **原因：**

    - 在上述函数定义中，传入的是指针变量`RingBuffer_t *testBuffer`；而在函数中将指针值拷贝到了`RingBuffer_t *ringBuffer`中；
    - `(ringBuffer) = (RingBuffer_t *)pvPortMalloc(sizeof(RingBuffer_t) + bufferSize);`这句调用想要改变实参`testBuffer`的指向，但实际上仅改变了形参`ringBuffer`的指向；

3. **解决办法：**

    1. 传入`testBuffer`的**二级指针** - 相对复杂

        - 为整个结构体分配内存，大小为`sizeof(RingBuffer_t) + bufferSize`；

            ```c
            (*ringBuffer) = (RingBuffer_t *)pvPortMalloc(sizeof(RingBuffer_t) + bufferSize);
            ```

        - 重新定位`bufferHead`的位置，让其指向存储缓冲区的开头；

            ```c
            (*ringBuffer)->bufferHead += sizeof(RingBuffer_t);
            ```

        - 初始化其他变量

            ```c
            (*ringBuffer)->bufferSize = bufferSize;
            (*ringBuffer)->dataSize = 0;
            (*ringBuffer)->readIndex = 0;
            (*ringBuffer)->writeIndex = 0;
            ```

            

    2. 在main函数中直接定义`RingBuffer_t` 结构体，在调用初始化函数时**传入结构体的地址**即可；

        - main函数中定义结构体时，就已经为结构体分配了内存，因此只需要为数据缓冲区`ringBuffer->bufferHead`分配内存即可；

            ```c
            ringBuffer->bufferHead= (uint8_t *)pvPortMalloc(bufferSize);
            ```

#### 3.1.4 ESP32 无法识别MQTT系列AT命令

1. **现象**

    ```c
    AT+MQTTUSERCFG=0,1,"","","",0,0,""
    ERR CODE:0x01090000
    
    ERROR
    ```

2. **原因**：AT固件版本不支持，自 `commit: 8ebdee924` 之后, `ESP-AT` 才支持 `MQTT` 系列 `AT` 指令；
3. **解决办法**：更新AT固件即可；