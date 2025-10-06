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




## 启动监听
`listen` 是服务器端套接字进入监听状态的函数，用于将流式套接字（TCP）从关闭状态转换为被动监听状态，准备接受客户端的连接请求。
```c
#include <sys/socket.h>
int listen(int sockfd, int backlog);
```

### 功能与行为
- 仅用于 `SOCK_STREAM` 类型的套接字（TCP），UDP 无需监听
- 将套接字标记为被动套接字，使其能够接受客户端的 `connect` 请求
- 维护一个连接请求队列，暂时存放已完成三次握手但未被 `accept` 处理的连接

### 参数说明
- `sockfd`：已绑定（`bind`）的套接字描述符
- `backlog`：指定未完成连接队列的最大长度（等待 `accept` 处理的连接数上限）：
  - 实际队列长度可能受系统限制（如 Linux 中默认取 `backlog` 与 `/proc/sys/net/core/somaxconn` 中的较小值）
  - 超过该值的新连接请求会被拒绝（客户端可能收到 `ECONNREFUSED` 错误）
### 返回值
- 成功：返回 `0`
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EINVAL` 表示套接字未绑定，`ENOTSOCK` 表示 `sockfd` 不是套接字）
### 关键特性
1. **状态转换**：使套接字从 `CLOSED` 状态进入 `LISTEN` 状态，是 TCP 服务器的必经步骤
2. **队列管理**：内核会维护两个队列：
   - 未完成连接队列：正在进行三次握手的连接
   - 已完成连接队列：已建立连接等待 `accept` 提取的连接
   - `backlog` 通常指这两个队列的总长度上限（具体实现因系统而异）
### 注意事项
1. **调用时机**：必须在 `bind` 之后、`accept` 之前调用
2. **UDP 不适用**：`SOCK_DGRAM` 类型套接字调用 `listen` 会返回错误（`EOPNOTSUPP`）



## 发起请求
`connect` 是客户端端主动发起连接请求的函数
```c
#include <sys/socket.h>
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
### 功能
- 仅对 `SOCK_STREAM` 类型套接字（TCP）有效，会触发 TCP 三次握手过程
- 对于 `SOCK_DGRAM` 类型（UDP），调用后仅记录默认目标地址，不建立实际连接
- 成功后，套接字与目标服务器的地址/端口绑定，后续可直接通过 `send()`/`recv()` 收发数据
### 参数说明
- `sockfd`：`socket()` 函数创建的套接字描述符（客户端套接字）
- `addr`：指向 `struct sockaddr` 类型的指针，存储目标服务器的地址信息：
  - 格式与 `bind()` 的 `addr` 参数一致（IPv4 用 `sockaddr_in`，IPv6 用 `sockaddr_in6`）
  - 需包含服务器的 IP 地址（`sin_addr`）和端口号（`sin_port`），均为网络字节序
- `addrlen`：`addr` 指向的结构体大小（字节数）

### 返回值
- 成功：返回 `0`（TCP 已完成三次握手，UDP 已设置默认目标）
- 失败：返回 `-1`，并设置 `errno` 表示错误原因
### 关键特性
1. **TCP 连接建立**：会阻塞等待三次握手完成（默认行为），可通过 `fcntl()` 设置非阻塞模式改变行为
2. **自动绑定**：若客户端套接字未调用 `bind()`，`connect()` 会自动分配本地临时端口和地址
3. **UDP 伪连接**：UDP 调用后仅设置默认目标，后续 `send()` 可省略地址参数，但本质仍是无连接通信
4. **重连处理**：连接失败后需重新创建套接字才能再次调用 `connect()`（同一套接字不可重复连接）






## 接受请求
`accept` 是服务器端接受客户端连接请求的函数，用于从已完成三次握手的连接队列中提取一个连接，创建新的套接字用于与客户端通信。
```c
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

### 功能与行为
- 仅用于 `SOCK_STREAM` 类型的套接字（TCP），在服务器端调用
- 从 `listen` 维护的已完成连接队列中取出一个连接，创建新的套接字描述符
- 新套接字专门用于与对应的客户端通信，原监听套接字（`sockfd`）继续接受其他连接

### 参数说明
- `sockfd`：处于监听状态的套接字描述符（通过 `listen` 初始化）
- `addr`：输出参数，用于存储客户端的地址信息（如 IP 和端口），需传入对应协议族的结构体指针（如 `struct sockaddr_in*`），可设为 `NULL` 表示不获取客户端地址
- `addrlen`：输入输出参数：- 必须是定义了的指针，socklen_t client_addr_len = sizeof(struct sockaddr_in);
  - 输入时：指定 `addr` 缓冲区的大小（字节数）
  - 输出时：返回实际存储的客户端地址长度

### 返回值
- 成功：返回新的套接字描述符（用于与该客户端通信）
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EAGAIN` 表示非阻塞模式下无可用连接，`EBADF` 表示 `sockfd` 无效）

### 关键特性
1. **连接分离**：监听套接字与新创建的通信套接字分离，前者继续接受新连接，后者专注于单客户端通信
2. **阻塞特性**：默认会阻塞等待，直到有新连接到来（可通过 `fcntl` 设置非阻塞模式）
3. **队列消费**：每次调用会从已完成连接队列中移除一个连接，若队列为空则阻塞

### 注意事项
1. **资源管理**：每个 `accept` 返回的新套接字需单独调用 `close` 关闭，否则会导致资源泄漏
2. **并发处理**：单一进程/线程调用 `accept` 会串行处理连接，需结合多进程、多线程或 I/O 复用（如 `select`/`epoll`）实现并发
3. **地址长度处理**：`addrlen` 必须初始化为 `addr` 缓冲区的大小，否则可能导致缓冲区溢出或错误
4. **错误区分**：非阻塞模式下返回 `-1` 时，需检查 `errno` 是否为 `EAGAIN`/`EWOULDBLOCK`（表示暂时无连接），避免误判为真正错误





## 发送数据
`send` 是用于通过套接字发送数据的函数，主要用于已建立连接的流式套接字（TCP），也可用于设置了默认目标的数据包套接字（UDP）。
```c
#include <sys/socket.h>
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```
### 功能
- 将 `buf` 指向的缓冲区中长度为 `len` 的数据通过 `sockfd` 标识的套接字发送
- 对于 TCP（`SOCK_STREAM`）：数据会按字节流顺序发送，成功返回不代表对方已接收（仅表示数据进入内核发送缓冲区）
- 对于 UDP（`SOCK_DGRAM`）：需先通过 `connect` 设置默认目标，否则需用 `sendto` 指定地址
### 参数说明
- `sockfd`：已连接的套接字描述符（TCP 为 `accept` 返回的通信套接字，UDP 为 `connect` 后的套接字）
- `buf`：指向待发送数据的缓冲区指针
- `len`：要发送的数据长度（字节数），建议不超过套接字发送缓冲区大小（可通过 `SO_SNDBUF` 选项查询）
  - 一般用`strlen`而不是`sizeof`，前者是有效字符的大小即要发送的数据长度，后者是整个缓存区的大小
- `flags`：发送标志，通常设为 `0`，常用选项：
  - `MSG_OOB`：发送带外数据（TCP 紧急数据，用于优先级传输）
  - `MSG_DONTWAIT`：非阻塞发送（覆盖套接字本身的阻塞模式）
  - `MSG_NOSIGNAL`：发送失败时不产生 `SIGPIPE` 信号

### 返回值
- 成功：返回实际发送的字节数（可能小于 `len`，尤其是非阻塞模式或缓冲区满时）
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EPIPE` 表示连接已关闭，`EAGAIN` 表示暂时无法发送）

### 注意事项
1. **返回值处理**：需检查实际发送字节数，若小于 `len`，需考虑循环发送剩余数据
2. **阻塞与非阻塞**：阻塞模式下可能因缓冲区满而阻塞，非阻塞模式需处理 `EAGAIN` 错误并重试
3. **TCP 连接状态**：若对方已关闭连接，继续 `send` 会触发 `SIGPIPE` 信号（默认终止进程），建议使用 `MSG_NOSIGNAL` 避免



## 接收数据
`recv` 是用于通过套接字接收数据的函数，可从已建立连接的流式套接字（TCP）或设置了默认目标的数据包套接字（UDP）中读取数据。
```c
#include <sys/socket.h>
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

### 功能与行为
- 从 `sockfd` 标识的套接字接收数据，存储到 `buf` 指向的缓冲区中，最多接收 `len` 字节
- 对于 TCP（`SOCK_STREAM`）：按字节流接收数据，不保证与发送端的 `send` 调用一一对应（可能合并或拆分）
- 对于 UDP（`SOCK_DGRAM`）：接收完整的数据报，若数据长度超过 `len`，超出部分会被截断且不通知

### 参数说明
- `sockfd`：已连接的套接字描述符（TCP 为通信套接字，UDP 为 `connect` 后的套接字）
- `buf`：指向接收数据的缓冲区指针，需预先分配足够内存
- `len`：缓冲区的最大容量（字节数），决定了一次最多能接收的数据量
- `flags`：接收标志，通常设为 `0`，常用选项：
  - `MSG_OOB`：接收带外数据（TCP 紧急数据）
  - `MSG_PEEK`：查看数据但不从缓冲区移除（可用于预览数据）
  - `MSG_DONTWAIT`：非阻塞接收（覆盖套接字本身的阻塞模式）
  - `MSG_WAITALL`：阻塞直到接收满 `len` 字节（或发生错误/连接关闭）

### 返回值
- 成功：返回实际接收的字节数（`>0`）；若返回 `0`，表示 TCP 连接已被对方正常关闭（对 UDP 无意义）
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EAGAIN` 表示非阻塞模式下无数据，`ECONNRESET` 表示连接被重置）

### 注意事项
1. **缓冲区大小**：`len` 应根据预期数据大小合理设置，过小可能导致数据截断（尤其 UDP），过大则浪费内存
2. **返回值为 0 的处理**：TCP 中表示对方正常关闭连接，需关闭本地套接字；UDP 中可忽略（空数据报合法）
3. **部分读取**：TCP 中 `recv` 可能返回小于 `len` 的值（即使未设置 `MSG_WAITALL`），需循环读取直到获取完整数据
4. **信号中断**：若接收过程被信号中断，可能返回 `-1` 并置 `errno` 为 `EINTR`，通常需重试






## 选择性关闭
`shutdown` 是用于部分或完全关闭套接字连接的函数，可精细控制套接字的发送或接收功能，区别于 `close` 直接关闭整个套接字。
```c
#include <sys/socket.h>
int shutdown(int sockfd, int how);
```

### 功能与行为
- 关闭套接字的部分或全部通信功能，不直接释放套接字文件描述符
- 主要用于 TCP 连接的优雅关闭，确保数据在关闭前被正确传输
- 对 UDP 套接字效果有限（因 UDP 无连接，主要影响 `connect` 设置的默认目标）

### 参数说明
- `sockfd`：需要操作的套接字描述符
- `how`：关闭方式，决定关闭发送、接收或全部功能：
  - `SHUT_RD`：关闭接收功能，不再接收数据（已在缓冲区的数据仍可读取，后续数据被丢弃），两次挥手
  - `SHUT_WR`：关闭发送功能，即“半关闭”（发送缓冲区数据会继续传输，之后无法再发送数据），两次挥手
  - `SHUT_RDWR`：同时关闭接收和发送功能，等价于先 `SHUT_RD` 再 `SHUT_WR`，四次挥手

### 返回值
- 成功：返回 `0`
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EBADF` 表示 `sockfd` 无效，`ENOTCONN` 表示套接字未连接）

### 关键特性
1. **半关闭机制**：`SHUT_WR` 是 TCP 优雅关闭的核心，允许一方在发送完所有数据后关闭发送功能，同时仍能接收对方数据（直到对方关闭连接）
2. **与 `close` 的区别**：
   - `shutdown` 仅关闭通信功能，不释放文件描述符（需仍调用 `close` 释放）
   - `close` 会减少套接字引用计数，计数为 0 时才彻底关闭（可能导致数据丢失，若其他进程仍引用该套接字）
3. **数据完整性**：`SHUT_WR` 会确保内核发送缓冲区中的数据全部发送，并触发 TCP 四次挥手的第一个 `FIN` 报文

### 注意事项
1. **优雅关闭流程**：TCP 中通常先 `shutdown(SHUT_WR)` 发送剩余数据并通知对方，待接收完对方数据后再 `close`
2. **不可逆转**：关闭后无法重新开启对应功能（如 `SHUT_RD` 后无法再接收数据）
3. **对 UDP 的影响**：`shutdown(SHUT_WR)` 会使后续 `send` 失败，但 `recv` 仍可接收任意来源的数据（除非已 `connect`）
4. **多进程共享**：若套接字被多个进程共享，`shutdown` 会影响所有进程（全局关闭功能），而 `close` 仅影响当前进程的引用






## 全部关闭
`close` 是用于关闭文件描述符（包括套接字）的通用函数，用于释放套接字占用的系统资源，终止网络通信。
```c
#include <unistd.h>
int close(int fd);
```
### 功能与行为
- 关闭 `fd` 标识的文件描述符（对套接字而言，会终止其对应的网络连接）
- 减少套接字的引用计数，当计数降至 0 时，彻底释放内核中的套接字资源
- 对 TCP 套接字，会触发四次挥手过程（若连接未关闭），确保资源正确回收

### 参数说明
- `fd`：需要关闭的文件描述符（此处为 `socket()` 或 `accept()` 返回的套接字描述符）
### 返回值
- 成功：返回 `0`
- 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `EBADF` 表示 `fd` 不是有效的文件描述符）

### 关键特性
1. **引用计数机制**：若套接字被多个进程/线程共享（如通过 `fork` 继承），`close` 仅减少引用计数，不立即释放资源，直到最后一个引用被关闭
2. **TCP 连接终止**：
   - 若连接未关闭，`close` 会触发四次挥手（等价于 `shutdown(SHUT_RDWR)` + 资源释放）
   - 会等待内核发送缓冲区中的数据传输完成（但超时可能导致数据丢失）
3. **与 `shutdown` 的差异**：
   - `close` 释放文件描述符，`shutdown` 仅关闭通信功能（不释放描述符）
   - `close` 受引用计数影响，`shutdown` 对所有引用进程生效

### 注意事项
1. **资源泄漏风险**：忘记调用 `close` 会导致文件描述符耗尽，尤其在循环处理连接的服务器中
2. **数据丢失可能**：若发送缓冲区仍有未传输数据，`close` 可能在数据发送完成前超时（需结合 `shutdown` 确保数据完整）
3. **重复关闭**：对已关闭的描述符调用 `close` 会导致未定义行为（可能影响其他文件描述符）
4. **TCP  TIME_WAIT 状态**：主动关闭的一方会进入 `TIME_WAIT` 状态，短暂保留资源以处理延迟报文，是正常现象


########################################################################################################


## 常用结构体
### struct sockaddr
`struct sockaddr` 是网络编程中用于表示通用套接字地址的结构体，作为各种具体地址类型的统一接口，用于 `bind`、`connect`、`accept` 等函数中传递地址信息。
```c
#include <sys/socket.h>
struct sockaddr {
    sa_family_t sa_family;  /* 地址族（协议族），如 AF_INET、AF_INET6 等 */
    char        sa_data[14];/* 地址数据，长度固定为 14 字节，存储具体地址信息 */
};
```

1. **通用性与兼容性**：
   - 本身不直接使用，而是通过强制转换适配具体地址类型（如 `struct sockaddr_in` 转换为 `struct sockaddr*`）


2. **与具体地址类型的关系**：
   - **IPv4 地址**：使用 `struct sockaddr_in`（需转换为 `struct sockaddr*` 传入函数）
     ```c
     struct sockaddr_in {
         sa_family_t    sin_family; /* 必须为 AF_INET */
         in_port_t      sin_port;   /* 端口号（网络字节序） */
         struct in_addr sin_addr;   /* IPv4 地址（网络字节序） */
         unsigned char  sin_zero[8];/* 填充字段，需设为 0，保证与 struct sockaddr 大小兼容 */
     };
     struct in_addr {
         uint32_t s_addr; /* IPv4 地址的 32 位整数表示 */
     };
     ```
   - **IPv6 地址**：使用 `struct sockaddr_in6`
   - **本地进程通信**：使用 `struct sockaddr_un`

3. **长度匹配**：具体地址结构体的总大小需与 `struct sockaddr` 兼容（通过填充字段保证），确保地址数据完整传递

 **初始化步骤**：
   - 根据协议族定义具体地址结构体（如 `struct sockaddr_in` 用于 IPv4）
   - 设置 `sin_family` 为对应协议族（如 `AF_INET`）
   - 转换端口号和 IP 地址为网络字节序（用 `htons`、`htonl` 等）
   - 将具体结构体强制转换为 `struct sockaddr*` 传入套接字函数



### sockaddr_in
`struct sockaddr_in` 是专门用于表示 IPv4 协议地址的结构体，用于存储和传递 IPv4 地址及端口信息，是网络编程中处理 IPv4 通信的核心数据结构。
```c
#include <netinet/in.h>
struct sockaddr_in {
    sa_family_t    sin_family;   /* 地址族，必须设为 AF_INET（标识 IPv4 协议） */
    in_port_t      sin_port;     /* 16 位端口号（需转换为网络字节序） */
    struct in_addr sin_addr;     /* 32 位 IPv4 地址（需转换为网络字节序） */
    unsigned char  sin_zero[8];  /* 填充字段，需设为 0，用于与 struct sockaddr 大小对齐 */
};

/* 嵌套的 IPv4 地址结构体 */
struct in_addr {
    uint32_t s_addr;  /* IPv4 地址的 32 位无符号整数表示（网络字节序） */
};
```

### 功能与行为
- 专门用于 IPv4 协议的地址描述，包含完整的“IP 地址 + 端口号”信息
- 通过强制转换为 `struct sockaddr*` 类型，适配 `bind`、`connect`、`accept` 等通用套接字函数
- 解决了 `struct sockaddr` 通用性带来的字段模糊问题，提供明确的 IPv4 地址操作接口

### 参数详解
1. **sin_family**：
   - 固定为 `AF_INET`（定义在 `<sys/socket.h>`），标识当前地址为 IPv4 类型
   - 必须与 `socket()` 函数创建套接字时指定的 `domain` 参数一致，否则会导致绑定或连接失败

2. **sin_port**：
   - 存储 16 位端口号（范围 0~65535），必须通过 `htons()` 转换为网络字节序
   - 特殊值：设为 `0` 时，系统会自动分配一个未使用的临时端口（常用于客户端）

3. **sin_addr**：
   - 嵌套的 `struct in_addr` 结构体，其 `s_addr` 字段存储 32 位 IPv4 地址
   - 地址需通过 `htonl()` 或 `inet_addr()` 转换为网络字节序
   - 特殊值：`INADDR_ANY`（值为 0）表示绑定到本机所有网络接口的对应端口（服务器常用）

4. **sin_zero**：
   - 8 字节的填充字段，无实际意义，必须初始化为 `0`（通常用 `memset` 整体初始化结构体）
   - 作用是使 `struct sockaddr_in` 的总大小与 `struct sockaddr` 一致（均为 16 字节），确保类型转换安全

### 使用流程示例
```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

// 创建 IPv4 TCP 套接字
int sockfd = socket(AF_INET, SOCK_STREAM, 0);

// 初始化 IPv4 地址结构体
struct sockaddr_in addr;
memset(&addr, 0, sizeof(addr));  // 整体清零，包括 sin_zero 字段
addr.sin_family = AF_INET;       // 指定 IPv4 协议
addr.sin_port = htons(8080);     // 端口号转换为网络字节序
addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 绑定到所有本地接口

// 绑定地址（强制转换为通用地址类型）
bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
```

### 关键特性
1. **类型专属**：仅用于 IPv4 协议，IPv6 需使用 `struct sockaddr_in6`，本地通信需使用 `struct sockaddr_un`
2. **字节序要求**：端口和 IP 地址必须使用网络字节序（大端序），这是跨主机通信的基础
3. **兼容性设计**：通过 `sin_family` 和 `sin_zero` 字段与通用 `struct sockaddr` 兼容，实现函数接口统一

### 注意事项
1. **初始化**：必须用 `memset` 将结构体清零，尤其是 `sin_zero` 字段，否则可能导致地址解析异常
2. **转换函数**：端口必须用 `htons()`，IP 地址用 `htonl()` 或 `inet_addr()`（`inet_addr` 直接返回网络字节序）
3. **INADDR_ANY 的使用**：服务器通常用 `INADDR_ANY` 绑定所有网络接口，避免硬编码 IP 地址导致的兼容性问题
4. **类型转换安全**：向套接字函数传递时必须强制转换为 `struct sockaddr*`，但内部仍保持 IPv4 地址的完整信息

`struct sockaddr_in` 是 IPv4 网络编程的核心地址结构体，它将抽象的网络地址具体化为“IP + 端口”的明确格式，同时通过兼容性设计适配通用套接字函数。正确初始化和设置该结构体的各个字段，是实现 IPv4 通信的基础步骤。

########################################################################################################
## 字节序
在网络编程中，特别是在跨平台和网络通信时，字节序（Byte Order）是非常重要的概念。字节序指的是多字节数据在内存中的存储顺序。主要有两种字节序：
- 大端字节序：高位字节存储在内存的低地址处，低位字节存储在高地址处。这种字节序遵循自然数字的书写习惯，也被称为网络字节序或网络标准字节序，因为它在网络通信中被广泛采用，如IP协议就要求使用大端字节序。

- 小端字节序：低位字节存储在内存的低地址处，高位字节存储在高地址处。这是Intel x86-64架构以及其他一些现代处理器普遍采用的字节序，称为主机字节序


## 转大端字节序(地址)
`htonl` 【`h`ost `to` `n`et `l`ong,主机字节序转为长 网络字节序】是将主机字节序的 32 位整数转换为网络字节序的函数，用于网络通信中整数类型数据（如 IPv4 地址）的字节序统一。
```c
#include <arpa/inet.h>
uint32_t htonl(uint32_t hostlong);
```
- `hostlong`：主机字节序的 32 位无符号整数（如 IPv4 地址的 32 位表示，或其他需要跨网络传输的 32 位数值）
- 返回值
- 转换后的网络字节序 32 位无符号整数





### 转大端字节序(端口)
`htons` 是将主机字节序的 16 位整数转换为网络字节序的函数，主要用于网络通信中`端口号`等 16 位数据的字节序标准化。
```c
#include <arpa/inet.h>
uint16_t htons(uint16_t hostshort);
```
- `hostshort`：主机字节序的 16 位无符号整数（通常为网络端口号，范围 0~65535）
- 返回值
- 转换后的网络字节序 16 位无符号整数



## 转主机字节序(地址)
`ntohl` 【`n`et `to` `h`ost `l`ong,网络字节序转为长 主机字节序】是将网络字节序的 32 位整数转换为主主机字节序的函数，用于网络通信中接收的整数类型数据（如 IPv4 地址）转换为本地可处理的字节序。
```c
#include <arpa/inet.h>
uint32_t ntohl(uint32_t netlong);
```
- `netlong`：网络字节序的 32 位无符号整数（如从网络接收的 IPv4 地址的 32 位表示，或其他 32 位数值）
- 返回值
- 转换后的主机字节序 32 位无符号整数





### 转主机字节序(端口)
`ntohs` 是将网络字节序的 16 位整数转换为主机字节序的函数，主要用于网络通信中接收的`端口号`等 16 位数据转换为本地可处理的字节序。
```c
#include <arpa/inet.h>
uint16_t ntohs(uint16_t netshort);
```
- `netshort`：网络字节序的 16 位无符号整数（通常为从网络接收的端口号）
- 返回值
- 转换后的主机字节序 16 位无符号整数




## IP字符串转网络字节序（IPv4/IPv6通用）
`inet_pton`【`in`ternet `p`resentation `to` `n`etwork，网络表示转网络字节序】是将点分十进制（IPv4）或冒分十六进制（IPv6）的IP字符串转换为网络字节序二进制数据的函数，支持IPv4和IPv6双栈，是现代网络编程的首选。
```c
#include <arpa/inet.h>
int inet_pton(int af, const char *src, void *dst);
```
- `af`：地址族，`AF_INET`表示IPv4，`AF_INET6`表示IPv6
- `src`：待转换的IP字符串（如`"192.168.1.100"`或`"2001:db8::1"`）
- `dst`：存放转换结果的缓冲区（IPv4时指向`struct in_addr`，IPv6时指向`struct in6_addr`）
- 返回值：成功返回`1`；无效地址返回`0`；错误（如协议族不支持）返回`-1`（并设置`errno`）


## IP字符串转网络字节序（仅IPv4）
`inet_aton`【`in`ternet `a`ddress `to` `n`etwork，网络地址转网络字节序】是将点分十进制的IPv4字符串转换为网络字节序二进制数据的函数，专为IPv4设计，错误处理简单清晰。
```c
#include <arpa/inet.h>
int inet_aton(const char *cp, struct in_addr *inp);
```
- `cp`：待转换的IPv4字符串（如`"192.168.1.100"`）
- `inp`：指向`struct in_addr`结构体的指针，用于存储转换后的网络字节序IP
- 返回值：成功返回`1`；失败（无效IPv4地址）返回`0`



### `inet_pton`/`inet_aton` 类与 `htonl` 类函数的核心区别


### 一、核心功能与处理对象
| 类别                | 核心功能                                  | 处理对象                          | 典型用途                          |
|---------------------|-------------------------------------------|-----------------------------------|-----------------------------------|
| `inet_pton`/`inet_aton` 类 | 将「人类可读的IP字符串」转换为「网络字节序的二进制IP」 | 点分十进制（IPv4）/冒分十六进制（IPv6）字符串（如`"192.168.1.1"`） | 初始化`sockaddr_in`结构体中的IP地址字段 |
| `htonl` 类（含`htons`/`ntohl`/`ntohs`） | 实现「主机字节序」与「网络字节序」的整数转换 | 32位整数（如手动构造的IP二进制值）、16位整数（如端口号） | 统一跨设备通信的字节序（IP二进制值、端口号等） |


### 二、输出结果与使用场景
| 关键区别点          | `inet_pton`/`inet_aton` 类                | `htonl` 类                          |
|---------------------|-------------------------------------------|-------------------------------------|
| 输入格式            | 必须是字符串（如`"192.168.1.1"`）         | 必须是整数（如`0xC0A80101`、`8080`） |
| 输出特点            | 结果直接是「网络字节序的二进制IP」，可直接用于网络通信 | 仅完成字节序转换，不处理字符串到二进制的格式转换 |
| 协议支持            | `inet_pton`支持IPv4/IPv6，`inet_aton`仅支持IPv4 | 与协议无关，仅处理整数字节序         |
| 依赖关系            | 转换结果已是网络字节序，**无需再调用`htonl`** | 需配合手动构造的二进制IP或端口号使用 |


### 三、代码示例对比
#### 1. 用 `inet_pton` 设置IP（无需 `htonl`）
```c
struct sockaddr_in addr;
// 直接将字符串IP转为网络字节序二进制（一步到位）
inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr);
// 结果可直接用于bind/connect等操作
```

#### 2. 用 `htonl` 处理手动构造的IP（需先转整数）
```c
struct sockaddr_in addr;
// 先将IP转为整数（手动操作，易出错），再转网络字节序
uint32_t ip_int = 0xC0A80101; // 192.168.1.1的整数形式
addr.sin_addr.s_addr = htonl(ip_int); // 仅处理字节序转换
```


### 总结
- **`inet_pton`/`inet_aton`**：是「IP字符串解析器」，负责将人类可读的IP字符串转换为计算机可直接使用的网络字节序二进制IP，是处理字符串IP的首选。
- **`htonl` 类**：是「字节序转换器」，仅负责整数在主机序和网络序之间的转换，不处理字符串，多用于手动构造IP/端口号时的字节序统一。

二者常配合使用（如用`inet_pton`解析IP，用`htons`设置端口）



## 内存初始化函数
`memset` 是用于将一块块内存的每个字节都设置为指定值的函数，常用于初始化内存区域（如结构体初始化结构体、清空缓冲区等）。
```c
#include <string.h>
void *memset(void *s, int c, size_t n);
```
- `s`：指向要初始化的内存区域的指针（如结构体变量、数组等）
- `c`：要设置的值（以`int`形式传入，但实际会转换为`unsigned char`类型，即只使用低8位）
- `n`：要设置的字节数
- 返回值：返回指向内存区域`s`的指针（方便链式操作）


### 典型使用场景
1. **初始化网络地址结构体**
```c
struct sockaddr_in server_addr;
// 将server_addr的所有字节初始化为0
memset(&server_addr, 0, sizeof(server_addr));
```

2. **清空缓冲区**
```c
char buf[1024];
// 将缓冲区所有字节设为0，清空数据
memset(buf, 0, sizeof(buf));
```

3. **填充特定值（如初始化数组）**
```c
int arr[5];
// 将数组所有字节设为0（注意：只能整体设为0或-1等特殊值）
memset(arr, 0, sizeof(arr));
```
