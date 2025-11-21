#### `epoll_create` 基本介绍
`epoll_create()` 是 **用于创建 epoll 实例的系统调用**，是 epoll 机制的初始化函数。
epoll 是 Linux 特有的 I/O 多路复用技术，用于高效监控多个文件描述符（如网络套接字、管道等）的 I/O 事件（可读、可写、异常等）

```c
#include <sys/epoll.h>
int epoll_create(int size);
```
- **参数**：`size`，在 Linux 2.6.8 及以后版本中已被忽略（仅需传入一个大于 0 的整数），早期版本用于指定 epoll 实例可监控的最大文件描述符数量。
- **返回值**：
  - 成功：返回一个非负整数（epoll 实例的文件描述符）
  - 失败：返回 `-1`，并设置 `errno` 表示错误原因（如系统资源不足时为 `ENFILE`）。

#### 核心功能：创建 epoll 实例
`epoll_create()` 的核心作用是创建一个 epoll 实例（本质是内核中的一个数据结构），该实例用于管理需要监控的文件描述符集合及其事件。后续通过 `epoll_ctl` 向实例中添加、修改或删除监控对象，通过 `epoll_wait` 等待事件触发。

#############################################################################################################
#### `epoll_create1` 基本介绍
`epoll_create1()` 是 **Linux 系统中创建 epoll 实例的扩展系统调用**，在 `epoll_create` 的基础上增加了标志参数，用于更灵活地控制 epoll 实例的创建行为，是现代 epoll 机制的推荐初始化函数。

```c
#include <sys/epoll.h>
int epoll_create1(int flags);
```
- **参数**：`flags` 用于指定创建 epoll 实例的额外选项，可取以下值：
  - `0`：与 `epoll_create(1)` 行为一致，创建普通的 epoll 实例。
  - `EPOLL_CLOEXEC`：设置 epoll 实例文件描述符的 `FD_CLOEXEC` 标志，即进程执行 `execve` 时自动关闭该文件描述符，避免子进程继承无用的 epoll 实例。
- **返回值**：
  - 成功：返回 epoll 实例的文件描述符（非负整数），用于后续 `epoll_ctl` 和 `epoll_wait` 操作。
  - 失败：返回 `-1`，并设置 `errno`（如 `flags` 无效时为 `EINVAL`，系统资源不足时为 `ENFILE`）。

#################################################################################################################
#### `epoll_ctl` 基本介绍
`epoll_ctl()` 是 **Linux 系统中用于管理 epoll 实例监控列表的系统调用**，是 epoll 机制的“控制接口”。它负责向已创建的 epoll 实例**添加、修改或删除**需要监控的文件描述符（FD）及其关联的 I/O 事件（如可读、可写、异常）
```c
#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```
- **参数**：
  1. `epfd`：`epoll_create()` 返回的 epoll 实例文件描述符，指定要操作的目标 epoll 实例。
  2. `op`：操作类型，指定对 `fd` 执行的动作，取值为以下宏之一：
     - `EPOLL_CTL_ADD`：将 `fd` 及对应的 `event` 加入 epoll 监控列表。
     - `EPOLL_CTL_MOD`：修改已在监控列表中的 `fd` 对应的 `event`（如变更监控的事件类型）。
     - `EPOLL_CTL_DEL`：将 `fd` 从 epoll 监控列表中移除（此时 `event` 参数可设为 `NULL`）。
  3. `fd`：需要添加/修改/删除的目标文件描述符（如网络套接字 `socketfd`、管道读端 `pipefd[0]`）。
  4. `event`：指向 `struct epoll_event` 结构体的指针，定义 `fd` 对应的监控事件类型及附加数据，结构体定义如下：
     ```c
     struct epoll_event {
         uint32_t events;  // 监控的事件类型，通过位或组合
         epoll_data_t data;// 附加数据，用于标识 fd 或传递自定义信息
     };
     // 附加数据的联合体，可存储 fd 或自定义指针
     typedef union epoll_data {
         void    *ptr;
         int      fd;
         uint32_t u32;
         uint64_t u64;
     } epoll_data_t;
     ```
     其中 `events` 常见取值：
     - `EPOLLIN`：`fd` 可读（如 socket 有数据到达、管道有数据可读）。
     - `EPOLLOUT`：`fd` 可写（如 socket 发送缓冲区空闲、管道可写入数据）。
     - `EPOLLERR`：`fd` 发生错误（无需显式设置，内核会自动通知）。
     - `EPOLLET`：设置为**边缘触发（ET）模式**（默认是水平触发 LT 模式）。
     - `EPOLLONESHOT`：仅监控一次事件，事件触发后需重新添加 `fd` 才会再次监控。

##### 使用前一般只需初始化两个
```c
struct epoll_event ev;
ev.data.fd = sockfd;
ev.events = EPOLLIN;

```
- **返回值**：
  - 成功：返回 `0`。
  - 失败：返回 `-1`，并设置 `errno`（如 `epfd` 无效为 `EBADF`、`op` 非法为 `EINVAL`、`fd` 已在列表中为 `EEXIST`）。

#### 核心功能：动态管理监控列表
`epoll_ctl()` 的核心作用是实现 epoll 监控列表的“动态维护”——无需像 `select/poll` 那样每次等待事件都重新传入完整的 FD 集合，而是通过 `EPOLL_CTL_ADD/MOD/DEL` 三个操作，在程序运行中灵活调整需要监控的 FD 及其事件，大幅提升高并发场景下的效率。

#### 关键概念与注意事项
1. **事件触发模式（LT vs ET）**：
   - **水平触发（LT，默认）**：只要 `fd` 满足事件条件（如可读），每次 `epoll_wait` 都会返回该事件，直到 `fd` 中的数据被读完。
   - **边缘触发（ET，需设置 `EPOLLET`）**：仅在 `fd` 事件状态从“未就绪”变为“就绪”时触发一次，后续即使数据未读完，也不会再触发（需一次性读完数据，避免遗漏）。
   - 注意：ET 模式下 `fd` 需设为**非阻塞**，否则可能因读取/写入阻塞导致程序卡住。

2. **`fd` 的状态限制**：
   - 被监控的 `fd` 必须是**非阻塞的吗？**：LT 模式下可阻塞，但 ET 模式下必须非阻塞（核心原则）。

#################################################################################################################
#### `epoll_wait` 基本介绍
`epoll_wait()` 是 **Linux 系统中用于等待 epoll 实例监控的文件描述符（FD）触发 I/O 事件的系统调用**，是 epoll 机制的“事件等待接口”。它会`阻塞`等待，直到监控列表中至少有一个 FD 触发了注册的事件（如可读、可写），或超时时间到达，然后返回就绪的事件列表，供程序处理。

```c
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```
- **参数**：
  1. `epfd`：`epoll_create()` 或 `epoll_create1()` 返回的 epoll 实例文件描述符，指定要等待的目标实例。
  2. `events`：指向 `struct epoll_event` `数组`的指针，用于`存储内核返回的就绪事件（输出参数）`。
  3. `maxevents`：`events` 数组的最大长度（即最多可接收的就绪事件数），必须大于 0。
  4. `timeout`：超时时间（毫秒），控制阻塞行为：
     - `>0`：最多阻塞 `timeout` 毫秒，若超时前无事件就绪，则返回 0。
     - `0`：不阻塞，立即返回当前就绪的事件（若无可就绪事件，返回 0）。
     - `-1`：无限期阻塞，直到至少有一个事件就绪。
- **返回值**：
  - 成功：返回就绪事件的数量（`>0`），或超时返回 0（`timeout>0` 时）。
  - 失败：返回 `-1`，并设置 `errno`（如 `epfd` 无效为 `EBADF`、`maxevents<=0` 为 `EINVAL`）。


#### 核心功能：等待并获取就绪事件
`epoll_wait()` 的核心作用是“阻塞等待事件”，并将所有就绪的 FD 及其事件信息填充到 `events` 数组中。与 `select/poll` 不同，它`仅返回状态变化的 FD`（无需遍历所有监控对象），大幅减少了用户态与内核态的交互开销，这也是 epoll 高效性的关键。

例如：
- 网络服务器中，`epoll_wait()` 会阻塞等待客户端连接请求（`listenfd` 可读）或已连接客户端发送数据（`clientfd` 可读）。
- 事件就绪后，程序通过`遍历 events` 数组，针对性地处理每个就绪 FD（如 `accept` 新连接、`read` 数据）。


#### 关键概念与注意事项
1. **事件处理逻辑**：
   - `epoll_wait()` 返回的 `nfds` 是就绪事件的实际数量，需遍历 `events[0..nfds-1]` 处理所有事件。
   - 每个 `events[i]` 包含触发的事件类型（`events[i].events`）和附加数据（`events[i].data`，通常是 FD）。

2. **水平触发（LT）与边缘触发（ET）的处理差异**：
   - **LT 模式（默认）**：若 FD 未被完全处理（如数据未读完），下次 `epoll_wait()` 会再次返回该事件，适合简单场景，但需避免重复处理。
   - **ET 模式**：仅在 FD 状态从“未就绪”变为“就绪”时触发一次，需一次性处理完所有数据（通常结合非阻塞 FD 和循环读取），否则会遗漏事件。

#################################################################################################################
#### `fcntl` 基本介绍
`fcntl()`（file control）是 **Linux 系统中用于对文件描述符（FD）进行各种控制操作的多功能系统调用**，支持对文件状态标志、文件锁、I/O 操作模式等进行设置或查询
```c
#include <unistd.h>
#include <fcntl.h>
int fcntl(int fd, int cmd, ... /* arg */ );
```
- **参数**：
  1. `fd`：目标文件描述符（如通过 `open()` 打开的文件、socket 等）。
  2. `cmd`：操作命令，指定要执行的控制动作（如查询状态、设置标志等），决定后续参数的类型和含义。
  3. `...`：可选参数，根据 `cmd` 不同而变化（如整数、指针等）。
- **返回值**：
  - 成功：返回值因 `cmd` 而异（如查询操作返回标志值，设置操作返回 0）。
  - 失败：返回 `-1`，并设置 `errno` 表示错误原因（如 `fd` 无效为 `EBADF`、`cmd` 非法为 `EINVAL`）。


#### 核心功能
|----------------|-----------------------------------------------------------------------------------|
| **文件状态标志操作** | `F_GETFL`：获取 `fd` 的状态标志（如 `O_RDONLY`、`O_NONBLOCK`）<br>`F_SETFL`：设置 `fd` 的状态标志（如添加 `O_NONBLOCK` 非阻塞模式） |

##### 设置文件描述符为非阻塞模式
```c
// 将文件描述符设置为非阻塞模式
int set_nonblocking(int fd) {
    // 1. 获取当前状态标志
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL 失败");
        return -1;
    }
    // 2. 添加 O_NONBLOCK 标志（不改变原有其他标志）
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL 失败");
        return -1;
    }
    return 0;
}

```


**非阻塞模式的应用**：
   - 非阻塞模式（`O_NONBLOCK`）是 `fcntl` 最常用的功能之一，配合 `epoll` 等 I/O 多路复用技术使用时，可避免 I/O 操作阻塞进程，是高并发程序的基础。
   - 注意：设置非阻塞后，`read()`/`write()` 可能返回 `-1` 且 `errno=EAGAIN` 或 `EWOULDBLOCK`，表示“当前无数据/缓冲区满，需稍后重试”。


