
   # MQTT
MQTT（Message Queuing Telemetry Transport）是一种**轻量级、低带宽、低功耗**的发布/订阅（Publish/Subscribe）模式消息传输协议，专为物联网（IoT）、资源受限设备（如传感器、嵌入式设备）设计。核心特点是通过中间件（MQTT Broker）实现客户端间的间接通信，客户端无需直接建立连接，仅需与 Broker 交互即可完成消息收发。

常用实现库为 **Eclipse Paho MQTT C Client**（开源、跨平台、轻量、文档完善）
## 一、核心头文件与依赖
- 核心头文件：`#include <MQTTClient.h>`
- 编译依赖：安装 Paho 库后，编译时需链接库文件（Linux 下为 `-lpaho-mqtt3c`，`3c` 表示 C 语言版本，`3cpp` 为 C++ 版本）
- 安装方式（Linux）：`sudo apt-get install libpaho-mqtt-dev`

## 二、核心函数详细解析
### 1. 客户端创建：`MQTTClient_create`
#### 函数原型
```c
int MQTTClient_create(MQTTClient* client, const char* broker, const char* clientId,
                      MQTTCLIENT_PERSISTENCE persistence, void* context);
```
#### 作用
创建一个 MQTT 客户端实例，为后续连接 Broker、收发消息做准备（客户端实例是所有操作的基础）。

#### 参数解析
| 参数名        | 类型                  | 说明                                                                 |
|---------------|-----------------------|----------------------------------------------------------------------|
| `client`      | `MQTTClient*`         | 输出参数：指向 MQTT 客户端实例的指针，函数执行后会初始化该实例       |
| `broker`      | `const char*`         | Broker 连接地址，格式为 `tcp://[IP]:[端口]`（默认1883）或 `ssl://[IP]:[端口]`（TLS加密，默认8883），例：`tcp://test.mosquitto.org:1883` |
| `clientId`    | `const char*`         | 客户端唯一标识（Broker 通过该 ID 区分不同客户端），需保证全局唯一，重复 ID 会导致旧连接被强制断开 |
| `persistence` | `MQTTCLIENT_PERSISTENCE` | 消息持久化模式：<br>- `MQTTCLIENT_PERSISTENCE_NONE`：不持久化（默认，断开后丢失未发送/接收的消息）<br>- `MQTTCLIENT_PERSISTENCE_FILE`：文件持久化（需指定存储路径，断开后消息可恢复） |
| `context`     | `void*`               | 上下文指针，可传递自定义数据（如全局配置、用户信息），后续回调函数中可通过 `context` 参数获取，无需传递时设为 `NULL` |

#### 返回值
- 成功：`MQTTCLIENT_SUCCESS`（宏定义，值为 0）
- 失败：非零错误码（如 `MQTTCLIENT_FAILURE`：创建失败，`MQTTCLIENT_INVALID_PARAMETER`：参数无效）

### 2. 连接 Broker：`MQTTClient_connect`
#### 函数原型
```c
int MQTTClient_connect(MQTTClient client, MQTTClient_connectOptions* conn_opts);
```
#### 作用
建立客户端与 MQTT Broker 之间的网络连接，需在创建客户端后、发布/订阅消息前调用。

#### 参数解析
| 参数名        | 类型                          | 说明                                                                 |
|---------------|-------------------------------|----------------------------------------------------------------------|
| `client`      | `MQTTClient`                  | 已创建的 MQTT 客户端实例（`MQTTClient_create` 初始化后的值）          |
| `conn_opts`   | `MQTTClient_connectOptions*`  | 连接配置选项指针，需先通过 `MQTTClient_connectOptions_initializer` 初始化，核心配置项：<br>- `keepAliveInterval`：保持连接时间（秒，默认60），客户端需在该时间内发送心跳包，否则 Broker 认为连接断开<br>- `cleansession`：会话清除标志（0=保留会话，1=清除会话），`cleansession=1` 时断开连接后 Broker 清除订阅信息和未接收消息<br>- `username`/`password`：Broker 认证用户名/密码（无认证时设为 `NULL`）<br>- `automaticReconnect`：自动重连开关（0=关闭，1=开启，Paho 3.0+ 支持）<br>- `connectTimeout`：连接超时时间（毫秒） |

#### 返回值
- 成功：`MQTTCLIENT_SUCCESS`
- 失败：非零错误码（如 `MQTTCLIENT_CONNECT_FAILED`：连接失败，`MQTTCLIENT_BROKER_UNAVAILABLE`：Broker 不可达，`MQTTCLIENT_AUTHENTICATION_FAILURE`：用户名/密码错误）


### `MQTTClient_connectOptions` 结构体
`MQTTClient_connectOptions` 是 Paho 库中用于配置客户端与 Broker 连接参数的核心结构体，必须通过 `MQTTClient_connectOptions_initializer` 初始化（避免未初始化的野指针问题），核心字段如下：

| 字段名                | 类型                  | 取值范围/宏定义                | 说明                                                                 |
|-----------------------|-----------------------|--------------------------------|----------------------------------------------------------------------|
| `keepAliveInterval`   | `int`                 | 0~3600（秒，默认60）           | 心跳包间隔时间：客户端需在该时间内发送消息（或心跳包），Broker 若超过 1.5 倍该时间未收到数据则主动断开连接；设为0表示禁用心跳 |
| `cleansession`        | `int`                 | 0（保留会话）/ 1（清除会话）   | 会话清除标志：<br>- 1：断开连接后，Broker 清除该客户端的订阅信息、未接收的 QoS1/2 消息<br>- 0：Broker 保留上述信息，客户端重连后可恢复 |
| `automaticReconnect`  | `int`                 | 0（关闭）/ 1（开启）           | 自动重连开关（Paho 3.0+ 支持）：连接丢失后，客户端自动按“1→2→4→...→30秒”递增间隔重试重连 |
| `connectTimeout`      | `long`                | ≥1000（毫秒，默认10000）       | 连接超时时间：客户端发起连接后，超过该时间未建立连接则返回失败       |
| `username`            | `const char*`         | 字符串（无默认，默认NULL）     | Broker 认证用户名：若 Broker 启用用户名密码认证，需填写正确用户名     |
| `password`            | `const char*`         | 字符串（无默认，默认NULL）     | Broker 认证密码：与 `username` 配套使用，需与 Broker 配置的密码一致   |
| `ssl`                 | `MQTTClient_SSLOptions*` | 结构体指针（默认NULL）         | TLS/SSL 加密配置：启用加密通信时需初始化该结构体，配置证书、验证规则等 |
| `will`                | `MQTTClient_willOptions*` | 结构体指针（默认NULL）         | 遗嘱消息配置：客户端异常断开时，Broker 自动向指定主题发布该消息       |
| `retryInterval`       | `int`                 | ≥1（秒，默认1）                | 手动重连间隔（`automaticReconnect=1` 时无效）：连接失败后下次重试的间隔时间 |
| `maxInflight`         | `int`                 | 1~65535（默认10）              | 最大未确认消息数：QoS1/2 消息中，客户端已发送但未收到 Broker 确认的消息上限 |

### 3. 完整配置示例（生产环境常用）
```c
// 初始化连接配置
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

// 1. 基础连接配置
conn_opts.keepAliveInterval = 20;          // 20秒心跳
conn_opts.cleansession = 0;                // 保留会话（重连恢复订阅）
conn_opts.automaticReconnect = 1;          // 开启自动重连
conn_opts.connectTimeout = 15000L;         // 15秒连接超时
conn_opts.username = "sensor_001";         // 认证用户名
conn_opts.password = "sensor_001_123456";  // 认证密码
```







### 3. 设置回调函数：`MQTTClient_setCallbacks`
#### 函数原型
```c
int MQTTClient_setCallbacks(MQTTClient client, void* context,
                            MQTTClient_connectionLost* cl,
                            MQTTClient_messageArrived* ma,
                            MQTTClient_deliveryComplete* dc);
```
#### 作用
注册客户端的回调函数，用于处理异步事件（连接丢失、消息接收、消息发布完成确认），需在连接 Broker 前设置。

#### 参数解析
| 参数名        | 类型                          | 说明                                                                 |
|---------------|-------------------------------|----------------------------------------------------------------------|
| `client`      | `MQTTClient`                  | 已创建的 MQTT 客户端实例                                             |
| `context`     | `void*`                       | 上下文指针，与 `MQTTClient_create` 的 `context` 一致，会传递到所有回调函数中 |
| `cl`          | `MQTTClient_connectionLost*`  | 连接丢失回调函数指针，原型：`void (*)(void* context, char* cause)`，连接断开时触发，`cause` 为断开原因 |
| `ma`          | `MQTTClient_messageArrived*`  | 消息接收回调函数指针，原型：`int (*)(void* context, char* topicName, int topicLen, MQTTClient_message* message)`，Broker 推送消息时触发，需返回 `MQTTCLIENT_SUCCESS` 确认接收 |
| `dc`          | `MQTTClient_deliveryComplete*`| 消息发布完成回调函数指针，原型：`void (*)(void* context, MQTTClient_deliveryToken dt)`，QoS 1/2 消息发布成功并得到 Broker 确认后触发，`dt` 为消息发布令牌 |

#### 返回值
- 成功：`MQTTCLIENT_SUCCESS`
- 失败：非零错误码（如 `MQTTCLIENT_INVALID_PARAMETER`：回调函数指针无效）

### 4. 订阅主题：`MQTTClient_subscribe`
#### 函数原型
```c
int MQTTClient_subscribe(MQTTClient client, const char* topic, int qos);
```
#### 作用
客户端订阅指定主题，订阅后 Broker 会将该主题的所有发布消息推送给客户端（通过 `messageArrived` 回调函数）。

#### 参数解析
| 参数名        | 类型                  | 说明                                                                 |
|---------------|-----------------------|----------------------------------------------------------------------|
| `client`      | `MQTTClient`          | 已连接到 Broker 的客户端实例                                         |
| `topic`       | `const char*`         | 订阅的主题名称，支持通配符：<br>- `+`：匹配单个层级，例：`sensor/+` 匹配 `sensor/temp`、`sensor/hum`，不匹配 `sensor/room1/temp`<br>- `#`：匹配多个层级（仅能放在主题末尾），例：`sensor/#` 匹配 `sensor/temp`、`sensor/room1/hum` |
| `qos`         | `int`                 | 服务质量等级（0/1/2）：<br>- QoS 0：最多一次（消息可能丢失，不确认）<br>- QoS 1：至少一次（消息必达，可能重复，需 Broker 确认）<br>- QoS 2：恰好一次（消息必达且唯一，最可靠但开销最大） |

#### 返回值
- 成功：`MQTTCLIENT_SUCCESS`
- 失败：非零错误码（如 `MQTTCLIENT_NOT_CONNECTED`：客户端未连接，`MQTTCLIENT_INVALID_TOPIC`：主题格式无效）

### 5. 发布消息：`MQTTClient_publishMessage`
#### 函数原型
```c
int MQTTClient_publishMessage(MQTTClient client, const char* topic,
                              MQTTClient_message* msg,
                              MQTTClient_deliveryToken* dt);
```
#### 作用
客户端向指定主题发布消息，Broker 会将消息转发给所有订阅该主题的客户端。

#### 参数解析
| 参数名        | 类型                          | 说明                                                                 |
|---------------|-------------------------------|----------------------------------------------------------------------|
| `client`      | `MQTTClient`                  | 已连接到 Broker 的客户端实例                                         |
| `topic`       | `const char*`                 | 发布消息的主题名称（需与订阅主题一致或匹配通配符）                   |
| `msg`         | `MQTTClient_message*`         | 消息结构体指针，需通过 `MQTTClient_message_initializer` 初始化，核心字段：<br>- `payload`：消息内容缓冲区（字符串或二进制数据）<br>- `payloadlen`：消息内容长度（字节，不含字符串结束符 `\0`）<br>- `qos`：服务质量等级（与订阅端 QoS 一致，取两者较低值）<br>- `retained`：消息保留标志（0=不保留，1=保留），保留消息会被 Broker 缓存，新订阅者连接后会立即收到该消息 |
| `dt`          | `MQTTClient_deliveryToken*`   | 输出参数：消息发布令牌（唯一标识本次发布），QoS 1/2 消息可通过该令牌关联 `deliveryComplete` 回调 |

#### 返回值
- 成功：`MQTTCLIENT_SUCCESS`（消息已发送到 Broker，QoS 0 表示发送完成，QoS 1/2 需等待 `deliveryComplete` 回调确认）
- 失败：非零错误码（如 `MQTTCLIENT_NOT_CONNECTED`：客户端未连接，`MQTTCLIENT_PAYLOAD_TOO_LARGE`：消息体过大）

### 6. 断开连接：`MQTTClient_disconnect`
#### 函数原型
```c
int MQTTClient_disconnect(MQTTClient client, int timeout);
```
#### 作用
主动断开客户端与 Broker 的连接，释放网络资源，需在客户端退出前调用。

#### 参数解析
| 参数名        | 类型          | 说明                                                                 |
|---------------|---------------|----------------------------------------------------------------------|
| `client`      | `MQTTClient`  | 已连接的客户端实例                                                   |
| `timeout`     | `int`         | 断开超时时间（毫秒），等待未完成的 QoS 1/2 消息确认，超时后强制断开   |

#### 返回值
- 成功：`MQTTCLIENT_SUCCESS`
- 失败：非零错误码（如 `MQTTCLIENT_NOT_CONNECTED`：客户端未连接）

### 7. 销毁客户端：`MQTTClient_destroy`
#### 函数原型
```c
void MQTTClient_destroy(MQTTClient* client);
```
#### 作用
销毁 MQTT 客户端实例，释放客户端占用的内存资源（如配置信息、缓冲区），需在断开连接后调用。

#### 参数解析
| 参数名        | 类型          | 说明                                                                 |
|---------------|---------------|----------------------------------------------------------------------|
| `client`      | `MQTTClient*` | 指向已创建客户端实例的指针，销毁后该指针会被置为 `NULL`               |


### 8.回调消息释放函数：1. `MQTTClient_freeMessage`
#### 函数原型
```c
void MQTTClient_freeMessage(MQTTClient_message** msg);
```
#### 作用
释放 `messageArrived` 回调函数中接收的 `MQTTClient_message` 结构体内存（包括结构体本身及内部关联资源），**必须调用**，否则会导致内存泄漏。

#### 参数解析
| 参数名        | 类型                  | 说明                                                                 |
|---------------|-----------------------|----------------------------------------------------------------------|
| `msg`         | `MQTTClient_message**` | 指向接收消息结构体指针的指针（即回调函数中 `MQTTClient_message* message` 的地址），函数执行后会将该指针置为 `NULL` |



#### 调用场景
仅在 `messageArrived` 回调函数中调用，用于释放 Broker 推送的消息内存：
```c
// 回调函数中示例
int messageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* message) {
    // 处理消息...
    MQTTClient_freeMessage(&message); // 释放消息结构体
    return MQTTCLIENT_SUCCESS;
}
```

### 9.内存释放函数：2. `MQTTClient_free`
#### 函数原型
```c
void MQTTClient_free(void* ptr);
```
#### 作用
释放 Paho 库动态分配的内存（如 `messageArrived` 回调中的主题名称字符串），配套 `MQTTClient_freeMessage` 使用，避免内存泄漏。

#### 参数解析
| 参数名        | 类型                  | 说明                                                                 |
|---------------|-----------------------|----------------------------------------------------------------------|
| `ptr`         | `void*`               | 需释放的内存指针（主要用于释放回调函数中的 `topicName` 参数）         |



#### 调用场景
仅在 `messageArrived` 回调函数中调用，用于释放 Broker 推送的主题名称内存：
```c
// 回调函数中示例
int messageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* message) {
    // 处理消息...
    MQTTClient_free(topicName); // 释放主题名称字符串
    MQTTClient_freeMessage(&message); // 释放消息结构体
    return MQTTCLIENT_SUCCESS;
}
```

### 补充说明
这两个释放函数是 MQTT 客户端内存管理的核心，**必须成对调用**（在 `messageArrived` 回调中，先释放 `topicName` 还是 `message` 无顺序要求，但两者都不能遗漏），否则会随着消息接收次数增加导致持续内存泄漏，最终引发程序崩溃。










## 三、MQTT 客户端整体核心流程
MQTT 客户端的完整生命周期可分为 4 个阶段，每个阶段的核心操作如下：

### 阶段 1：初始化与连接（准备阶段）
1. 包含核心头文件 `#include <MQTTClient.h>`
2. 调用 `MQTTClient_create` 创建客户端实例，指定 Broker 地址、Client ID
3. 初始化 `MQTTClient_connectOptions` 结构体，配置 keepalive、认证信息、自动重连等
4. 调用 `MQTTClient_setCallbacks` 注册回调函数（连接丢失、消息接收、发布确认）
5. 调用 `MQTTClient_connect` 连接 Broker，检查返回值确认连接成功

### 阶段 2：消息订阅与接收（订阅端流程）
1. 调用 `MQTTClient_subscribe` 订阅目标主题（指定 QoS 等级）
2. 客户端阻塞等待或进入业务循环，Broker 推送消息时触发 `messageArrived` 回调
3. 在回调函数中解析消息主题和内容，处理业务逻辑
4. 处理完成后调用 `MQTTClient_freeMessage` 和 `MQTTClient_free(topicName)` 释放内存

### 阶段 3：消息发布（发布端流程）
1. 初始化 `MQTTClient_message` 结构体，设置消息内容（`payload`）、长度（`payloadlen`）、QoS、保留标志（`retained`）
2. 调用 `MQTTClient_publishMessage` 向指定主题发布消息，获取发布令牌（`dt`）
3. QoS 1/2 消息需等待 `deliveryComplete` 回调，确认消息已被 Broker 接收
4. 若消息内容为动态分配（如 `malloc`），发布后需手动释放 `payload` 内存

### 阶段 4：资源释放（退出阶段）
1. 调用 `MQTTClient_unsubscribe`（可选）取消订阅不需要的主题
2. 调用 `MQTTClient_disconnect` 断开与 Broker 的连接，指定超时时间
3. 调用 `MQTTClient_destroy` 销毁客户端实例，释放内存
4. 程序退出

## 四、完整示例代码（发布+订阅一体化）
```c
#include <MQTTClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 配置参数
#define BROKER_URL "tcp://test.mosquitto.org:1883"  // 公共测试Broker
#define CLIENT_ID "mqtt_c_client_" __FILE__ "_" __LINE__  // 唯一Client ID（结合文件名行号）
#define SUB_TOPIC "iot/sensor/temp"                 // 订阅主题
#define PUB_TOPIC "iot/sensor/temp"                 // 发布主题
#define QOS 1                                       // QoS等级（1级：至少一次）
#define CONN_TIMEOUT 10000L                         // 连接超时（10秒）
#define DISCONN_TIMEOUT 5000L                       // 断开超时（5秒）

// 消息接收回调函数：Broker推送消息时触发
int messageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* message) {
    // 解析消息内容（忽略topicLen，topicName已以\0结尾）
    printf("\n【消息接收】\n");
    printf("主题：%s\n", topicName);
    printf("长度：%d 字节\n", message->payloadlen);
    printf("内容：%s\n", (char*)message->payload);

    // 必须释放消息内存，避免内存泄漏
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);

    return MQTTCLIENT_SUCCESS;  // 告知Broker已成功接收
}

// 连接丢失回调函数：与Broker断开时触发
void connectionLost(void* context, char* cause) {
    printf("\n【连接丢失】原因：%s\n", cause);
    // 实际开发可添加自动重连逻辑：循环调用MQTTClient_connect
}

// 消息发布完成回调函数：QoS1/2消息确认时触发
void deliveryComplete(void* context, MQTTClient_deliveryToken dt) {
    printf("\n【发布确认】消息令牌：%d 发布成功\n", dt);
}

int main() {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc = MQTTCLIENT_FAILURE;

    // 阶段1：初始化与连接
    printf("=== 初始化MQTT客户端 ===\n");
    // 1. 创建客户端实例
    rc = MQTTClient_create(&client, BROKER_URL, CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "创建客户端失败！错误码：%d\n", rc);
        return rc;
    }
    printf("客户端创建成功（Client ID：%s）\n", CLIENT_ID);

    // 2. 设置回调函数
    rc = MQTTClient_setCallbacks(client, NULL, connectionLost, messageArrived, deliveryComplete);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "设置回调函数失败！错误码：%d\n", rc);
        MQTTClient_destroy(&client);
        return rc;
    }
    printf("回调函数注册成功\n");

    // 3. 配置连接参数
    conn_opts.keepAliveInterval = 20;          // 20秒发送一次心跳包
    conn_opts.cleansession = 1;                // 断开后清除会话
    conn_opts.automaticReconnect = 1;          // 开启自动重连
    conn_opts.connectTimeout = CONN_TIMEOUT;   // 连接超时10秒

    // 4. 连接Broker
    printf("连接到 Broker：%s...\n", BROKER_URL);
    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "连接Broker失败！错误码：%d\n", rc);
        MQTTClient_destroy(&client);
        return rc;
    }
    printf("Broker连接成功\n");

    // 阶段2：订阅主题
    printf("\n=== 订阅主题 ===\n");
    rc = MQTTClient_subscribe(client, SUB_TOPIC, QOS);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "订阅主题 %s 失败！错误码：%d\n", SUB_TOPIC, rc);
        MQTTClient_disconnect(client, DISCONN_TIMEOUT);
        MQTTClient_destroy(&client);
        return rc;
    }
    printf("成功订阅主题：%s（QoS=%d）\n", SUB_TOPIC, QOS);

    // 阶段3：发布消息（循环发布3条测试消息）
    printf("\n=== 发布消息 ===\n");
    MQTTClient_message pub_msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    char payload[100];

    for (int i = 1; i <= 3; i++) {
        // 构造消息内容
        snprintf(payload, sizeof(payload), "传感器温度数据 - 第%d条：25.%d℃", i, i*2);
        pub_msg.payload = payload;
        pub_msg.payloadlen = strlen(payload);
        pub_msg.qos = QOS;
        pub_msg.retained = 0;  // 不保留消息

        // 发布消息
        rc = MQTTClient_publishMessage(client, PUB_TOPIC, &pub_msg, &token);
        if (rc != MQTTCLIENT_SUCCESS) {
            fprintf(stderr, "发布第%d条消息失败！错误码：%d\n", i, rc);
        } else {
            printf("发布第%d条消息（令牌：%d）：%s\n", i, token, payload);
        }
        sleep(2);  // 每隔2秒发布一条
    }

    // 保持连接，等待接收消息（阻塞10秒）
    printf("\n=== 等待接收消息（10秒）===\n");
    sleep(10);

    // 阶段4：资源释放
    printf("\n=== 释放资源 ===\n");
    // 1. 取消订阅（可选）
    rc = MQTTClient_unsubscribe(client, SUB_TOPIC);
    if (rc == MQTTCLIENT_SUCCESS) {
        printf("成功取消订阅主题：%s\n", SUB_TOPIC);
    }

    // 2. 断开连接
    rc = MQTTClient_disconnect(client, DISCONN_TIMEOUT);
    if (rc == MQTTCLIENT_SUCCESS) {
        printf("已断开与Broker的连接\n");
    }

    // 3. 销毁客户端
    MQTTClient_destroy(&client);
    printf("客户端实例已销毁\n");

    return 0;
}
```

### 编译与运行（Linux）
1. 编译命令：`gcc mqtt_complete_demo.c -o mqtt_demo -lpaho-mqtt3c`
2. 运行命令：`./mqtt_demo`
3. 预期输出：
```
=== 初始化MQTT客户端 ===
客户端创建成功（Client ID：mqtt_c_client_mqtt_complete_demo.c_58）
回调函数注册成功
连接到 Broker：tcp://test.mosquitto.org:1883...
Broker连接成功

=== 订阅主题 ===
成功订阅主题：iot/sensor/temp（QoS=1）

=== 发布消息 ===
发布第1条消息（令牌：1）：传感器温度数据 - 第1条：25.2℃

【发布确认】消息令牌：1 发布成功

【消息接收】
主题：iot/sensor/temp
长度：33 字节
内容：传感器温度数据 - 第1条：25.2℃
发布第2条消息（令牌：2）：传感器温度数据 - 第2条：25.4℃

【发布确认】消息令牌：2 发布成功

【消息接收】
主题：iot/sensor/temp
长度：33 字节
内容：传感器温度数据 - 第2条：25.4℃
发布第3条消息（令牌：3）：传感器温度数据 - 第3条：25.6℃

【发布确认】消息令牌：3 发布成功

【消息接收】
主题：iot/sensor/temp
长度：33 字节
内容：传感器温度数据 - 第3条：25.6℃

=== 等待接收消息（10秒）===

=== 释放资源 ===
成功取消订阅主题：iot/sensor/temp
已断开与Broker的连接
客户端实例已销毁
```

### 1. 核心参数补充说明
- **Client ID**：必须全局唯一，建议结合设备ID、进程PID（`getpid()`）或随机数生成，例：`snprintf(clientId, sizeof(clientId), "sensor_001_pid_%d", getpid())`

### 2. 关键注意事项
#### （1）内存管理
- 接收消息时：`messageArrived` 回调中必须调用 `MQTTClient_freeMessage(&message)` 和 `MQTTClient_free(topicName)`，否则会导致内存泄漏
- 发布消息时：若 `payload` 是动态分配（如 `char* payload = malloc(100)`），发布后需手动 `free(payload)`（Broker 会复制消息内容，不依赖客户端的缓冲区）

#### （2）线程安全
- 回调函数（`messageArrived`、`connectionLost`、`deliveryComplete`）运行在 Paho 库的内部线程中，避免在回调中执行长时间阻塞操作（如 `sleep`、文件读写）
- 回调中访问全局变量或共享资源时，需加互斥锁（如 `pthread_mutex_t`），防止线程安全问题

#### （3）重连机制
- 开启 `automaticReconnect = 1` 后，客户端会在连接丢失后自动尝试重连（默认重试间隔递增：1秒、2秒、4秒...最大30秒）
- 手动重连：在 `connectionLost` 回调中循环调用 `MQTTClient_connect`，需添加重试间隔（如 `sleep(1)`），避免频繁重试占用CPU

