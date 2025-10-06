#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/tcp.h>

// 操作失败时打印错误并退出
#define handle_error(cmd, result) \
    if (result < 0)               \
    {                             \
        perror(cmd);              \
        return -1;                \
    }

// 线程参数结构体
typedef struct {
    int client_fd;                // 客户端套接字描述符
    struct sockaddr_in client_addr;// 客户端地址信息
} ThreadArgs;

/**
 * 处理客户端连接的线程函数
 * 负责接收客户端数据并回复确认信息
 */
void *handle_client(void *args)
{
    ThreadArgs *thread_args = (ThreadArgs *)args;
    int client_fd = thread_args->client_fd;
    struct sockaddr_in client_addr = thread_args->client_addr;
    free(args);  // 释放动态分配的参数内存

    char *read_buf = malloc(1024);   // 接收缓冲区
    char *write_buf = malloc(1024);  // 发送缓冲区
    
    // 检查内存分配
    if (!read_buf || !write_buf) {
        perror("malloc failed");
        close(client_fd);
        return NULL;
    }

    // 循环接收客户端数据
    ssize_t count;
    while ((count = recv(client_fd, read_buf, 1023, 0)) > 0) {
        read_buf[count] = '\0';  // 确保字符串终止
        printf("Received from %s:%d (fd=%d): %s",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               client_fd,
               read_buf);

        // 发送确认信息
        snprintf(write_buf, 1024, "Received: %ld bytes\n", count);
        ssize_t send_count = send(client_fd, write_buf, strlen(write_buf), 0);
        if (send_count < 0) {
            perror("send failed");
            break;
        }
        
        memset(read_buf, 0, 1024);  // 清空缓冲区
    }

    // 处理连接关闭
    if (count == 0) {
        printf("Client %s:%d (fd=%d) closed connection\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               client_fd);
    } else if (count < 0) {
        perror("recv error");
    }

    // 清理资源
    snprintf(write_buf, 1024, "Connection closed\n");
    send(client_fd, write_buf, strlen(write_buf), 0);
    
    close(client_fd);
    free(read_buf);
    free(write_buf);
    return NULL;
}

int main(int argc, char const *argv[])
{
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 初始化地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 监听所有网络接口
    server_addr.sin_port = htons(6666);               // 绑定到6666端口

    // 创建TCP套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    handle_error("socket creation failed", server_fd);

    // 设置端口复用：解决服务器重启时的"地址已在使用"问题
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }

    // 绑定套接字到指定地址和端口
    handle_error("bind failed", bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)));

    // 进入监听状态，最大等待队列长度128
    handle_error("listen failed", listen(server_fd, 128));

    printf("Server listening on port 6666...\n");

    // 循环接受客户端连接
    while (1) {
        // 接受新连接
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        handle_error("accept failed", client_fd);

        printf("New connection from %s:%d (fd=%d)\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               client_fd);

        // 为线程分配参数（避免局部变量地址传递问题）
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (!args) {
            perror("malloc args failed");
            close(client_fd);
            continue;
        }
        args->client_fd = client_fd;
        args->client_addr = client_addr;

        // 创建线程处理客户端
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, args) != 0) {
            perror("pthread_create failed");
            free(args);
            close(client_fd);
            continue;
        }

        // 分离线程：自动回收资源，无需pthread_join
        pthread_detach(tid);
    }

    // 正常情况下不会到达这里
    close(server_fd);
    return 0;
}
    