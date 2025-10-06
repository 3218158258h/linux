

## 一、核心结构体：`struct sockaddr_un`
本地套接字通过该结构体定义“通信地址”，核心是**文件系统路径**（替代 TCP 的“IP+端口”）

```c
#include <sys/un.h>

struct sockaddr_un {
    sa_family_t sun_family;  // 地址族，固定为 AF_UNIX 或 AF_LOCAL（本地通信专用）
    char        sun_path[108];// 套接字对应的文件路径（如 "/tmp/my_uds.socket",也可以直接在当前地址创建文件如"socket_IPC_file"，无需/）
};
```
### 关键说明
- `sun_family`：必须设为 `AF_UNIX`（或 `AF_LOCAL`，两者等价），告诉内核这是本地套接字；
- `sun_path`：最大长度 107 字节（含终止符 `\0`），超过会导致 `bind` 失败；路径需是合法的文件路径，程序运行时会创建该“套接字文件”（类型为 `s`，可通过 `ls -l` 查看）；
- 与 TCP 对比：TCP 用 `struct sockaddr_in` 存“IP+端口”，UDS 用 `struct sockaddr_un` 存“文件路径”。


## 二、核心函数与实现流程
本地套接字支持 **流式（SOCK_STREAM，类似 TCP）** 和 **数据报（SOCK_DGRAM，类似 UDP）**，此处以常用的**流式套接字**为例（流程与 TCP 几乎一致，仅地址结构体不同）。


### 1. 创建套接字：`socket()`
与 TCP 共用 `socket` 函数，仅需将 `domain` 设为 `AF_UNIX`，`type` 设为 `SOCK_STREAM`。

```c
#include <sys/socket.h>
#include <sys/types.h>

int socket(int domain, int type, int protocol);
```
#### 功能
创建本地套接字描述符，作为后续通信的句柄。
#### 参数说明
- `domain`：`AF_UNIX`（本地通信协议族）；
- `type`：`SOCK_STREAM`（流式，可靠、面向连接，类似 TCP）；
- `protocol`：设为 `0`，使用 `AF_UNIX + SOCK_STREAM` 对应的默认协议（本地流式通信协议）。
#### 返回值
- 成功：返回非负整数（套接字描述符）；
- 失败：返回 `-1`，设置 `errno`（如 `EMFILE` 表示文件描述符耗尽）。


### 2. 绑定地址：`bind()`
将创建的套接字与 `sun_path` 中的文件路径绑定，类似 TCP 绑定“IP+端口”。

```c
#include <sys/socket.h>

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
#### 功能
将套接字 `sockfd` 与本地路径 `sun_path` 关联，内核会根据该路径识别套接字。
#### 参数说明
- `sockfd`：`socket()` 创建的本地套接字描述符；
- `addr`：强制转换为 `struct sockaddr*` 类型的 `struct sockaddr_un` 指针（存储本地路径）；
- `addrlen`：`struct sockaddr_un` 的实际大小（用 `sizeof(struct sockaddr_un)` 获取）。

#### 示例代码
```c
struct sockaddr_un uds_addr;
// 1. 初始化地址结构体（清空垃圾值）
memset(&uds_addr, 0, sizeof(struct sockaddr_un));
// 2. 设置地址族和路径
uds_addr.sun_family = AF_UNIX;
strncpy(uds_addr.sun_path, "/tmp/my_uds.socket", sizeof(uds_addr.sun_path) - 1); // 留1字节存\0

// 4. 绑定套接字与路径
if (bind(sock_fd, (struct sockaddr*)&uds_addr, sizeof(uds_addr)) < 0) {
    perror("bind failed");
    close(sock_fd); // 失败时关闭套接字
    exit(EXIT_FAILURE);
}
```

### 3. 监听连接：`listen()`

```c
#include <sys/socket.h>

int listen(int sockfd, int backlog);
```
#### 功能
将套接字设为“监听状态”，准备接受客户端连接，仅流式套接字需要（数据报套接字无需监听）。
#### 参数说明
- `sockfd`：已绑定的本地套接字描述符；
- `backlog`：等待连接队列的最大长度（如 `5` 或 `128`，超过的连接会被拒绝）。


### 4. 接受连接：`accept()`

```c
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```
#### 功能
从监听队列中取出一个客户端连接，创建新的套接字用于与该客户端通信（原监听套接字继续监听其他连接）。
#### 参数说明
- `sockfd`：监听状态的本地套接字描述符；
- `addr`：可选，存储客户端的 `struct sockaddr_un` 地址（本地套接字场景下，客户端地址也是一个文件路径，通常无需关注，可设为 `NULL`）；
- `addrlen`：可选，输入输出参数，存储 `addr` 的大小（若 `addr` 为 `NULL`，则设为 `NULL`）。
#### 示例代码
```c
int client_fd;
struct sockaddr_un client_addr;
socklen_t client_addr_len = sizeof(client_addr);

// 阻塞接受客户端连接，获取与客户端通信的新套接字
if ((client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
    perror("accept failed");
    close(sock_fd);
    unlink(uds_addr.sun_path);
    exit(EXIT_FAILURE);
}
printf("Client connected (client path: %s)\n", client_addr.sun_path); // 打印客户端路径（可选）
```


### 5. 收发数据：`read()`/`write()` 或 `recv()`/`send()`
与 TCP 完全兼容，可使用文件操作函数（`read`/`write`）或套接字专用函数（`recv`/`send`），数据直接在本地内核传递，无网络开销。

#### 示例：接收客户端数据并回复
```c
char buf[1024];
ssize_t n;

// 接收客户端数据（阻塞）
n = read(client_fd, buf, sizeof(buf) - 1); // 留1字节存\0
if (n < 0) {
    perror("read failed");
    close(client_fd);
    close(sock_fd);
    unlink(uds_addr.sun_path);
    exit(EXIT_FAILURE);
} else if (n == 0) {
    printf("Client closed connection\n");
    close(client_fd);
    return;
}
buf[n] = '\0'; // 手动添加字符串终止符
printf("Received from client: %s\n", buf);

// 回复客户端
const char *reply = "Hello from UDS server!";
if (write(client_fd, reply, strlen(reply)) < 0) {
    perror("write failed");
    close(client_fd);
    close(sock_fd);
    unlink(uds_addr.sun_path);
    exit(EXIT_FAILURE);
}
```


### 6. 关闭套接字与清理：`close()` + `unlink()`
- `close()`：关闭套接字描述符，释放文件资源，与 TCP 一致；
- `unlink()`：删除 `sun_path` 对应的套接字文件（核心！若不删除，下次程序运行时 `bind` 会失败）。

#### 示例代码
```c
// 关闭与客户端通信的套接字
close(client_fd);
// 关闭监听套接字
close(sock_fd);
// 清理套接字文件（必须执行）
unlink(uds_addr.sun_path);
```


## 三、客户端实现（流式本地套接字）
客户端流程比服务端简单：`创建套接字 → 连接服务端 → 收发数据 → 关闭套接字`，无需 `bind`（客户端会自动分配临时路径）。

### 核心差异：`connect()`
与 TCP 的 `connect` 函数一致，仅需传入服务端的 `struct sockaddr_un` 地址（服务端的 `sun_path`）。

```c
#include <sys/socket.h>

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```
#### 客户端完整示例
```c
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_PATH "/tmp/my_uds.socket" // 服务端的套接字路径
#define BUF_LEN 1024

int main() {
    int sock_fd;
    struct sockaddr_un server_addr;
    char buf[BUF_LEN];

    // 1. 创建本地流式套接字
    if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. 初始化服务端地址（仅需设置地址族和服务端路径）
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SERVER_PATH, sizeof(server_addr.sun_path) - 1);

    // 3. 连接服务端
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to UDS server\n");

    // 4. 发送数据给服务端
    const char *msg = "Hello from UDS client!";
    if (write(sock_fd, msg, strlen(msg)) < 0) {
        perror("write failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // 5. 接收服务端回复
    ssize_t n = read(sock_fd, buf, BUF_LEN - 1);
    if (n < 0) {
        perror("read failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    buf[n] = '\0';
    printf("Received from server: %s\n", buf);

    // 6. 关闭套接字（客户端无需unlink，因未绑定路径）
    close(sock_fd);
    return 0;
}
```


