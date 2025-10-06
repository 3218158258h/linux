## 创建
`socket` 是创建套接字的核心函数，用于在程序中创建一个网络端点
```c
#include <sys/socket.h>
#include <sys/types.h>
int socket(int domain, int type, int protocol);
```
### 功能
- 创建一个套接字描述符，作为后续网络操作（绑定、连接、收发数据等）的句柄
- 根据参数指定网络协议族、套接字类型和具体协议，确定通信方式
### 参数说明
- `domain`：协议族（地址族），指定网络通信的地址类型，常用值：
  - `AF_INET`：IPv4 协议（最常用）
  - `AF_INET6`：IPv6 协议
  - `AF_UNIX`/`AF_LOCAL`：本地进程间通信（通过文件系统路径）
  - `AF_PACKET`：链路层协议（用于原始套接字，直接操作网络帧）

- `type`：套接字类型，决定数据传输方式，常用值：
  - `SOCK_STREAM`：流式套接字，基于 TCP 协议，提供可靠、面向连接、字节流传输
  - `SOCK_DGRAM`：数据报套接字，基于 UDP 协议，提供不可靠、无连接、数据报传输

- `protocol`：具体协议，通常设为 `0` 表示使用 `domain` 和 `type` 对应的默认协议：
  - 对于 `AF_INET` + `SOCK_STREAM`，默认是 `IPPROTO_TCP`（TCP 协议）
  - 对于 `AF_INET` + `SOCK_DGRAM`，默认是 `IPPROTO_UDP`（UDP 协议）
### 返回值
- 成功：返回非负整数（套接字描述符，类似文件描述符）
- 失败：返回 `-1`，并设置 `errno` 表示错误原因
### 关键特性
1. **抽象端点**：套接字本质是内核中的一个数据结构，代表网络通信的端点，不直接关联物理设备
2. **文件描述符特性**：套接字描述符可使用 `read()`/`write()` 等文件操作函数（但更推荐用 `send()`/`recv()` 等专用函数）
3. **资源释放**：使用完毕后需调用 `close()` 关闭套接字，释放系统资源





## 绑定端口/地址
`bind` 是将套接字与特定地址和端口绑定的函数，用于服务器端指定监听的网络地址和端口，或客户端指定本地使用的地址和端口。
```c
#include <sys/socket.h>
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

### 功能与行为
- 将 `sockfd` 标识的套接字与 `addr` 指向的网络地址绑定，确立套接字对应的网络端点
- 对于服务器：必须调用 `bind` 绑定端口，否则系统会随机分配端口，客户端无法确定连接目标
- 对于客户端：通常无需显式调用，系统会自动分配本地地址和端口

### 参数说明
- `sockfd`：`socket()` 函数返回的套接字描述符
- `addr`：指向 `struct sockaddr` 类型的指针，存储要绑定的地址信息，需根据协议族使用不同结构体：
  - IPv4 用 `struct sockaddr_in`（需强制转换为 `struct sockaddr*`）：
    ```c
    struct sockaddr_in {
        sa_family_t    sin_family; /* AF_INET */
        in_port_t      sin_port;   /* 端口号（网络字节序） */
        struct in_addr sin_addr;   /* IP 地址（网络字节序） */
    };
    struct in_addr {
        uint32_t       s_addr;     /* IPv4 地址 */
    };
    ```
  - IPv6 用 `struct sockaddr_in6`，本地通信用 `struct sockaddr_un`
- `addrlen`：`addr` 指向的结构体大小（字节数）

### 返回值
- 成功：返回 `0`
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EADDRINUSE` 表示端口已被占用，`EADDRNOTAVAIL` 表示地址不可用）

### 关键特性
1. **地址关联**：绑定后，套接字与特定地址/端口形成唯一关联，数据会通过该端点收发
2. **端口分配**：`sin_port` 设为 `0` 时，系统会自动分配一个未使用的临时端口（常用于客户端）
3. **通配地址**：IPv4 中 `sin_addr.s_addr` 设为 `INADDR_ANY`（值为 0），表示绑定到本机所有网络接口的对应端口，方便多网卡机器监听

### 注意事项
1. **字节序转换**：端口号和 IP 地址需用 `htons()`/`htonl()` 转换为网络字节序（大端序），避免因主机字节序差异导致错误
2. **端口范围**：1024 以下的端口是特权端口，普通用户程序无法绑定，需使用 1024~65535 范围内的端口
3. **端口复用**：默认情况下，同一端口不能被多个套接字绑定，可通过 `setsockopt()` 设置 `SO_REUSEADDR` 选项允许端口复用（如服务器重启时快速重新绑定）





## 发送
`sendto` 是用于在无连接套接字（如 UDP）上发送数据的函数，也可用于有连接套接字（如 TCP），能指定数据发送的目标地址信息。
```c
#include <sys/socket.h>
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
```

### 功能与行为
- 适用于 `SOCK_DGRAM` 类型的套接字（UDP），也可用于 `SOCK_STREAM` 类型（TCP）但通常不推荐
- 向 `dest_addr` 指定的目标地址发送 `buf` 中的数据，长度为 `len` 字节
- 对于 UDP 等无连接协议，每次发送都需指定目标地址；对于 TCP 等有连接协议，`dest_addr` 会被忽略（使用已建立的连接地址）

### 参数说明
- `sockfd`：套接字描述符（已创建并绑定的套接字）
- `buf`：指向待发送数据的缓冲区指针
- `len`：待发送数据的字节数
- `flags`：发送标志，通常为 `0`，特殊需求时可使用（如 `MSG_DONTROUTE` 表示不经过路由表，`MSG_OOB` 表示发送带外数据）
- `dest_addr`：指向目标地址结构体的指针（如 `struct sockaddr_in*`），包含目标 IP 和端口，TCP 连接模式下可设为 `NULL`
- `addrlen`：`dest_addr` 结构体的字节长度（如 `sizeof(struct sockaddr_in)`）

### 返回值
- 成功：返回实际发送的字节数（可能小于 `len`，尤其在非阻塞模式下）
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EINTR` 表示被信号中断，`EAGAIN` 表示非阻塞模式下发送缓冲区满）

### 关键特性
1. **无连接特性**：UDP 中无需提前建立连接，每次发送都需指定目标地址，灵活性高但可靠性依赖应用层
2. **数据报边界**：UDP 中保持数据报边界，一次 `sendto` 发送的数据会被作为一个完整数据报传递
3. **标志控制**：`flags` 参数可控制发送行为，如 `MSG_CONFIRM` 用于某些协议的地址确认
4. **部分发送**：非阻塞模式下可能只发送部分数据，需通过返回值判断并处理剩余数据

### 注意事项
1. **长度限制**：UDP 数据报有最大长度限制（通常为 65507 字节），超过会导致数据被截断或发送失败
2. **连接与无连接区分**：TCP 连接模式下使用 `sendto` 时，`dest_addr` 和 `addrlen` 会被忽略，建议直接使用 `send`
3. **错误处理**：非阻塞模式下返回 `-1` 且 `errno` 为 `EAGAIN`/`EWOULDBLOCK` 时，可稍后重试发送
4. **缓冲区考虑**：发送数据量较大时，需注意系统发送缓冲区大小，避免频繁调用导致性能下降




## 接收
`recvfrom` 是用于从套接字接收数据的函数，特别适用于无连接协议（如UDP），能够同时获取发送方的地址信息，也可用于有连接协议（如TCP）接收数据。
```c
#include <sys/socket.h>
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
```

### 功能与行为
- 可用于 `SOCK_DGRAM`（UDP）和 `SOCK_STREAM`（TCP）等类型的套接字
- 从指定套接字接收数据并存储到 `buf` 缓冲区，最多接收 `len` 字节
- 对于无连接协议（如UDP），能通过 `src_addr` 获取发送方的地址信息；对于有连接协议（如TCP），`src_addr` 可设为 `NULL`（使用已连接的对端地址）

### 参数说明
- `sockfd`：已创建的套接字描述符（需绑定地址，UDP）或已连接的套接字（TCP）
- `buf`：用于存储接收数据的缓冲区指针
- `len`：缓冲区的最大容量（字节数），决定最多能接收的数据量
- `flags`：接收标志，通常为 `0`，特殊场景可使用（如 `MSG_PEEK` 表示查看数据但不从缓冲区移除，`MSG_OOB` 表示接收带外数据）
- `src_addr`：输出参数，用于存储发送方的地址信息（如IP和端口），需传入对应协议族的结构体指针（如 `struct sockaddr_in*`），可设为 `NULL` 表示不获取发送方地址
- `addrlen`：输入输出参数：
  - 输入时：指定 `src_addr` 缓冲区的大小（字节数）
  - 输出时：返回实际存储的发送方地址长度

### 返回值
- 成功：返回实际接收的字节数（`0` 通常表示对端关闭连接，TCP场景）
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EAGAIN` 表示非阻塞模式下无数据可读，`EINTR` 表示被信号中断）

### 关键特性
1. **地址获取**：在UDP等无连接场景中，无需提前知道发送方，通过 `src_addr` 可动态获取来源地址，支持与多个对端通信
2. **数据边界保留**：对于UDP，一次 `recvfrom` 调用接收一个完整的数据报，保持数据边界；TCP则是字节流，无边界概念
3. **标志灵活性**：通过 `flags` 可实现特殊需求，如 `MSG_WAITALL` 尝试接收完整 `len` 字节数据（可能仍返回部分数据）
4. **阻塞行为**：默认阻塞等待数据到来，可通过 `fcntl` 设置为非阻塞模式

### 注意事项
1. **缓冲区大小**：`buf` 容量需足够容纳可能接收的最大数据（如UDP最大数据报约65507字节），避免数据截断
2. **地址长度初始化**：使用 `src_addr` 时，`addrlen` 必须初始化为其缓冲区大小，否则可能导致错误或不完整的地址信息
3. **非阻塞处理**：非阻塞模式下返回 `-1` 且 `errno` 为 `EAGAIN`/`EWOULDBLOCK` 时，说明当前无数据，可稍后重试
4. **TCP与UDP差异**：TCP中 `src_addr` 无效（始终是已连接的对端），且返回 `0` 表示对端正常关闭；UDP中返回 `0` 表示接收到空数据报（合法场景）