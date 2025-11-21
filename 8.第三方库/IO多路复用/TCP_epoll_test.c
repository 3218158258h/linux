// 网络编程相关头文件：提供socket、地址结构等定义
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 标准输入输出和工具函数头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// 多线程和I/O复用相关头文件
#include <pthread.h>
#include <sys/epoll.h>
#include <fcntl.h>

// 宏定义：服务器端口、缓冲区大小、最大事件数
#define SEVER_PORT 6666         // 服务器监听端口（注意：原代码有拼写错误，应为SERVER_PORT）
#define BUFFER_SIZE 1024        // 数据缓冲区大小
#define MAX_EVENTS 10           // epoll最大可处理的事件数

// 错误处理宏：简化错误检查和处理流程
#define handle_error(cmd, result) \
    if (result < 0)               \
    {                             \
        perror(cmd);              \
        exit(EXIT_FAILURE);       \
    }

// 全局缓冲区：用于读取和发送数据
char *read_buf = NULL;    // 读取缓冲区
char *write_buf = NULL;   // 发送缓冲区

// 初始化缓冲区函数：分配内存并初始化
void init_buf()
{
    // 为读缓冲区分配内存并检查是否成功
    read_buf = malloc(sizeof(char) * BUFFER_SIZE);
    if (!read_buf)
    {
        printf("服务端读缓存创建异常，断开连接\n");
        perror("malloc sever read_buf");
        exit(EXIT_FAILURE);
    }

    // 为写缓冲区分配内存并检查是否成功
    write_buf = malloc(sizeof(char) * BUFFER_SIZE);
    if (!write_buf)
    {
        printf("服务端写缓存创建异常，断开连接\n");
        free(read_buf);  // 释放已分配的读缓冲区
        perror("malloc server write_buf");
        exit(EXIT_FAILURE);
    }

    // 初始化缓冲区（清零）
    memset(read_buf, 0, BUFFER_SIZE);
    memset(write_buf, 0, BUFFER_SIZE);
}

// 清空缓冲区函数
void clear_buf(char *buf)
{
    memset(buf, 0, BUFFER_SIZE);
}

// 设置文件描述符为非阻塞模式
void set_nonblocking(int sockfd)
{
    int opts = fcntl(sockfd, F_GETFL);  // 获取当前文件状态标志
    if (opts < 0)
    {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }

    opts |= O_NONBLOCK;  // 添加非阻塞标志
    int res = fcntl(sockfd, F_SETFL, opts);  // 设置新的文件状态标志
    if (res < 0)
    {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[])
{
    init_buf();  // 初始化缓冲区

    // 声明服务器和客户端socket描述符，以及临时结果变量
    int sockfd, client_fd, temp_result;

    // 声明服务端和客户端地址结构（IPv4）
    struct sockaddr_in server_addr, client_addr;

    // 初始化地址结构（清零）
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // 配置服务器地址信息
    server_addr.sin_family = AF_INET;                // 使用IPv4协议
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 绑定到所有可用网络接口
    server_addr.sin_port = htons(SEVER_PORT);        // 设置端口（主机字节序转网络字节序）

    // 创建服务器socket（IPv4，流式套接字TCP）
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    handle_error("socket", sockfd);  // 错误检查

    // 绑定socket到指定地址和端口
    temp_result = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    handle_error("bind", temp_result);

    // 进入监听状态，允许最大128个未处理连接
    temp_result = listen(sockfd, 128);
    handle_error("listen", temp_result);

    // 将服务器socket设置为非阻塞模式
    set_nonblocking(sockfd);

    // 初始化epoll相关变量
    int epollfd, nfds;
    struct epoll_event ev, events[MAX_EVENTS];  // 事件结构体和事件数组

    // 创建epoll实例
    epollfd = epoll_create1(0);
    handle_error("epoll_createl", epollfd);

    // 注册服务器socket到epoll，监听读事件（新连接）
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;  // 关注读事件
    temp_result = epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
    handle_error("epoll_ctl", temp_result);

    socklen_t cliaddr_len = sizeof(client_addr);
    // 主循环：处理epoll事件
    while (1)
    {
        // 等待事件发生，-1表示无限阻塞
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        handle_error("epoll_wait", nfds);

        // 处理所有发生的事件
        for (int i = 0; i < nfds; i++)
        {
            // 如果是服务器socket的事件，表示有新连接
            if (events[i].data.fd == sockfd)
            {
                // 接受客户端连接
                client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &cliaddr_len);
                handle_error("accept", client_fd);
                
                // 将客户端socket设置为非阻塞
                set_nonblocking(client_fd);

                // 打印连接信息
                printf("与客户端 from %s at PORT %d 文件描述符 %d 建立连接\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);
                
                // 将新客户端socket注册到epoll，使用边缘触发模式
                ev.data.fd = client_fd;
                ev.events = EPOLLIN | EPOLLET;  // 关注读事件，边缘触发
                epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev);
            }
            // 客户端socket的读事件
            else if (events[i].events & EPOLLIN)
            {
                int count = 0, send_count = 0;
                client_fd = events[i].data.fd;
                
                // 循环读取所有数据（边缘触发模式需要一次性读完）
                while ((count = recv(client_fd, read_buf, BUFFER_SIZE, 0)) > 0)
                {
                    printf("reveive message from client_fd: %d: %s\n", client_fd, read_buf);
                    clear_buf(read_buf);  // 清空读缓冲区

                    // 发送响应给客户端
                    strcpy(write_buf, "reveived~\n");
                    send_count = send(client_fd, write_buf, strlen(write_buf), 0);
                    handle_error("send", send_count);
                    clear_buf(write_buf);  // 清空写缓冲区
                }

                // 处理读取结果
                if (count == -1 && errno == EAGAIN)
                {
                    // 没有更多数据可读（非阻塞模式下的正常情况）
                    printf("来自客户端client_fd: %d当前批次的数据已读取完毕，继续监听文件描述符集\n", client_fd);
                }
                else if (count == 0)
                {
                    // 客户端关闭连接
                    printf("客户端client_fd: %d请求关闭连接......\n", client_fd);
                    
                    // 发送关闭确认
                    strcpy(write_buf, "receive your shutdown signal\n");
                    send_count = send(client_fd, write_buf, strlen(write_buf), 0);
                    handle_error("send", send_count);
                    clear_buf(write_buf);

                    // 从epoll中移除客户端socket
                    printf("从epoll文件描述符集中移除client_fd: %d\n", client_fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, NULL);

                    // 关闭连接并释放资源
                    printf("释放client_fd: %d资源\n", client_fd);
                    shutdown(client_fd, SHUT_WR);
                    close(client_fd);
                }
            }
        }
    }

    // 释放资源（理论上不会执行到这里，因为主循环是无限的）
    printf("释放资源\n");
    close(epollfd);
    close(sockfd);
    free(read_buf);
    free(write_buf);

    return 0;
}
