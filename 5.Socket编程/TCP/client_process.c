#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

// 错误处理宏：操作失败时打印错误并返回-1
#define handle_error(cmd, result) \
    if (result < 0)               \
    {                             \
        perror(cmd);              \
        return -1;                \
    }

/**
 * 从服务器读取数据的线程函数
 * 功能：持续接收服务器消息并显示
 */
void *read_from_server(void *argv)
{
    int sockfd = *(int *)argv;
    char *read_buf = malloc(1024);
    ssize_t count = 0;

    if (!read_buf) {
        perror("malloc read_buf failed");
        return NULL;
    }

    // 循环接收服务器数据
    while ((count = recv(sockfd, read_buf, 1024, 0)) > 0) {
        fputs(read_buf, stdout);
        memset(read_buf, 0, 1024);
    }

    printf("收到服务端关闭信号\n");
    free(read_buf);
    return NULL;
}

/**
 * 向服务器发送数据的线程函数
 * 功能：从标准输入读取数据并发送到服务器
 */
void *write_to_server(void *argv)
{
    int sockfd = *(int *)argv;
    char *write_buf = malloc(1024);
    ssize_t send_count;

    if (!write_buf) {
        printf("写缓存分配失败\n");
        shutdown(sockfd, SHUT_WR);
        perror("malloc write_buf failed");
        return NULL;
    }

    // 循环读取输入并发送
    while (fgets(write_buf, 1024, stdin) != NULL) {
        send_count = send(sockfd, write_buf, strlen(write_buf), 0);
        if (send_count < 0) {
            perror("send failed");
            break;
        }
        memset(write_buf, 0, 1024);
    }

    printf("停止发送数据，关闭连接\n");
    shutdown(sockfd, SHUT_WR);
    free(write_buf);
    return NULL;
}

int main(int argc, char const *argv[])
{
    int sockfd, temp_result;
    pthread_t pid_read, pid_write;
    struct sockaddr_in server_addr;

    // 初始化服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 连接本地(127.0.0.1)
    server_addr.sin_port = htons(6666);                    // 目标端口

    // 创建TCP套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    handle_error("socket creation failed", sockfd);

    // 连接服务器
    temp_result = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    handle_error("connect failed", temp_result);

    // 创建读写线程
    pthread_create(&pid_read, NULL, read_from_server, (void *)&sockfd);
    pthread_create(&pid_write, NULL, write_to_server, (void *)&sockfd);

    // 等待线程结束
    pthread_join(pid_read, NULL);
    pthread_join(pid_write, NULL);

    // 清理资源
    close(sockfd);
    return 0;
}
    