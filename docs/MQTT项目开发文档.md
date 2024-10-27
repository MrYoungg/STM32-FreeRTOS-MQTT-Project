# MQTT项目手册

## 1. 项目前置知识

### 1.1 网络连接模型

发布端-服务器-订阅端

### 1.1 MQTT协议源码分析

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

### 1.2 MQTT移植

#### 1.1.1 将MQTT库移植到Keil工程中

##### （1）过程

1. 将`mqttclient\platform\FreeRTOS`文件夹中的.c文件移植到工程中；
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409221634169.png" alt="image-20240922163318680" style="zoom: 50%;" />

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
        ![image-20240922231757547](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409222317753.png)

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
        ![image-20240922174719713](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409221747761.png)

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

<img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409221555727.png" alt="img" style="zoom: 67%;" />

- **RTOS：**FreeRTOS为LwIP提供的操作系统级API接口，如创建线程、提供锁等；
    - 主要通过freertos/sys_arch.c实现；
- **Network System Config：**对LwIP协议栈的系统设置。
    - 主要通过opt.h（系统默认设置）和lwipopts.h（用户设置）实现；
- **LwIP Stack：**LwIP的TCP/IP协议栈。
    - 主要文件有：`LwIP/src/api/*.c`、`LwIP/src/core/*.c`、`LwIP/src/core/ipv4/*.c`、
- **Hardware Driver：**主要是STM32平台以太网接口的驱动层；
    - 主要通过`ethernet.c`实现以太网数据的收发；

## 2. 项目实现

### 2.0 项目常用AT命令



### 2.1 网络框架层次

![image-20240929153217800](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291532065.png)

**应用层**

- MQTT协议栈，调用平台网络层实现的函数，包括：创建连接、收发信息、关闭连接等；实现MQTT工作逻辑；

**平台网络层**

- 调用AT命令发送函数，封装MQTT协议栈需要的函数；向ESP8266/ESP32等发送各类AT命令，操作其通过路由器连接网络；

**传输层**

- 调用串口收发函数，封装AT命令发送函数，发送AT命令；

**数据链路层**

- 调用底层的寄存器操作命令，向上封装串口收发函数，通过串口收发字节流；

### 2.2 AT固件烧录+测试

#### 2.2.1 AT固件+烧录软件下载

##### （1）AT固件下载

1. **ESP32系列：**[发布的固件 - ESP32 - — ESP-AT 用户指南 latest 文档 (espressif.com)](https://docs.espressif.com/projects/esp-at/zh_CN/latest/esp32/AT_Binary_Lists/esp_at_binaries.html)、
2. **ESP8266系列：**[发布的固件 — ESP-AT 用户指南 文档 (espressif.com)](https://docs.espressif.com/projects/esp-at/zh-cn/release-v2.2.0.0_esp8266/AT_Binary_Lists/ESP8266_AT_binaries.html)
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291615280.png" alt="image-20240929161553175" style="zoom: 67%;" />

##### （2）烧录软件下载

1. 下载地址：[工具｜乐鑫科技 (espressif.com)](https://www.espressif.com/zh-hans/support/download/other-tools)
    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291618043.png" alt="image-20240929161700983" style="zoom: 67%;" />

#### 2.2.2 固件烧录过程

##### （1）硬件连接

1. **通过Type-c烧录（简单稳定）：**通过USB-TypeC数据线连接电脑USB口和ESP8266Type-c口；
2. **通过串口烧录：**通过USB转TTL接口连接电脑USB口和ESP8266串口0（ESP8266的UART0用于固件烧录，GPIO3-RX和GPIO1-TX）;

##### （2）烧录软件操作

1. 选择**ChipType:ESP8266，WorkMode:Develop；**
    ![image-20240929162851523](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291629517.png)
2. 选择**AT固件库-factory文件夹**中的**factory_xxx.bin**文件，烧录进ESP8266即可（记得勾选文件！）
    ![image-20240929163858297](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291638397.png)

#### 2.2.3 烧录后测试

##### （1）硬件连接

1. ESP8266上烧录固件、收发AT命令和输出日志的端口是不同的，如图所示；ESP8266的UART0被映射到两组不同的端口，分别用于烧录固件和收发AT指令；
    ![image-20240929162552106](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291642595.png)
2. 在测试AT命令时需要将串口连接到ESP8266的**D7(GPIO13-RX)**和**D8(GPIO15-TX)**；
    ![image-20240929165041589](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409291650763.png)

##### （2）AT指令

发送以下AT命令；

```
AT+GMR

```

收到回复：AT固件的版本信息；
<img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409292200151.png" style="zoom: 80%;" />

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
​    <img src="https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409292226009.png" alt="image-20240929222436873" style="zoom:67%;" />

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

    ![image-20240929223341072](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409292233128.png)

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
    
        ![image-20240929154047153](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202409292238291.png)

### 2.4 数据链路层：串口收发数据

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

### 2.5 传输层：AT命令发送/响应接收

#### 2.5.0 传输层的多线程框架

![image-20241025175245287](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410251752523.png)

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

### 2.6 网络层：实现网络连接和数据传输

#### 2.6.1 建立网络连接 - platform_net_socket_connect()

（1）概述

1. 配置ESP8266的WiFi模式：`AT+CWMODE=3`；
2. 连接路由器节点（WiFi/热点）：`AT+CWJAP="SSID","PassWord"`；
3. 与服务器建立TCP连接：A`T+CIPSTART="proto","host",port`

（2）代码

```c

```

#### 2.6.2 断开网络连接 - platform_net_socket_close()

1. 关闭TCP连接：`AT+CIPCLOSE`；

#### 2.6.3 通过网络连接发送数据包 - platform_net_socket_write()

1. 必须与服务器建立TCP连接之后，才能发送数据包；
2. 发送命令：`AT+CIPSEND=<length>`；
3. 收到相应OK以及“>”之后，向服务器发送数据；

#### 2.6.4 接收网络数据包 - platform_net_socket_recv()

1. 

### 2.7 应用层：实现MQTT连接服务器

### ⭐2.x OTA远程升级

## 3. 项目总结的

### 3.1 debug记录

#### （1）STM32基于串口初始化发送0xFF

1. **可能原因：**GPIO口和串口初始化顺序有问题，GPIO时钟 - GPIO - USART时钟 - USART
2. **现象：**
    1. 按照上述顺序初始化时，发现在执行到开启USART时钟时，串口会输出一个0xFF；
        ![image-20241018141745123](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410181417226.png)
    2. 怀疑串口将电平误判为数据，在初始化USART时钟前**加一个延时**：GPIO时钟 - GPIO - **延时** - USART时钟 - USART，发现串口输出数据变为0x00；
    3. 通过单步调试发现，仅当延时作用于**初始化TX的GPIO口**和**开启USART时钟**之间时，会改为输出0x00；
        ![image-20241018145710544](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410181457624.png)
    4. 将初始化顺序改成：GPIO时钟 - USART时钟 - GPIO - USART；串口在初始化过程中不再输出任何数据；
        ![image-20241018142921367](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410181429455.png)
3. **具体原因分析**
    1. 只要供电正常，ESP8266模块的RX口就处于时刻准备接收的状态；
    2. 如果在**初始化GPIO为复用推挽输出模式**（将GPIO_Pin的控制权交给外设）时**还未初始化USART时钟**，则会导致GPIO_Pin的电平处于**不确定状态**；
    3. 而**GPIO初始化**（GPIO_Init()）和**USART时钟初始化**（AT_USARTx_APBxCLOCKCMD()）的过程中，都会产生电平波动；此时TX引脚上的电平波动就有可能让ESP8266误认为有数据传输；电平时序如图所示
        ![image-20241018162435209](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410181624423.png)
    4. 如果**先初始化USART时钟再初始化GPIO外设为复用推挽输出模式**，则GPIO_Pin的电平由USART确定（高/低），不会处于不确定的波动状态；

#### （2）在task中添加串口打印日志时，进入HardFault

1. **原因**：task的任务栈大小不足，导致栈溢出（内存访问错误），因此进入HardFault；
2. **解决办法**：创建任务时申请更大的栈；
3. **注意点**：
    - 栈溢出与否与打印的字符串长度无关，因为**字符串常量**本身存储在整个程序代码段的**只读数据段（.rodata）**中，通过指针传给打印函数，该指针存储在任务栈中
        ![image-20241021181124121](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410211811193.png)
    - 对于字符数组会有点不一样，如果在任务中定义了一个字符数组，那么**整个字符数组都存储在任务栈中**；
        ![image-20241021182227843](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410211822894.png)

#### （3）初始化环形缓冲区（RingBuffer）出错

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
        ![image-20241027154434672](https://gitee.com/yyangyyyy/typora-image/raw/master/Typora_Image/202410271544740.png)

2. **原因：**

    - 在上述函数定义中，传入的是指针变量`RingBuffer_t *testBuffer`；而在函数中将指针值拷贝到了`RingBuffer_t *ringBuffer`中；
    - `(ringBuffer) = (RingBuffer_t *)pvPortMalloc(sizeof(RingBuffer_t) + bufferSize);`这句调用想要改变实参`testBuffer`的指向，但实际上仅改变了形参`ringBuffer`的指向；

3. **解决办法：**

    1.  传入`testBuffer`的**二级指针** - 相对复杂

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

            

    2.  在main函数中直接定义`RingBuffer_t` 结构体，在调用初始化函数时**传入结构体的地址**即可；

        - main函数中定义结构体时，就已经为结构体分配了内存，因此只需要为数据缓冲区`ringBuffer->bufferHead`分配内存即可；

            ```c
            ringBuffer->bufferHead= (uint8_t *)pvPortMalloc(bufferSize);
            ```