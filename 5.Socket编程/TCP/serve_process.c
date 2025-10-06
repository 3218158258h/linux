#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

// 操作失败时打印错误并返回-1
#define handle_error(cmd, result) \
    if (result < 0)               \
    {                             \
        perror(cmd);              \
        return -1;                \
    }

/**
 * 僵尸进程处理函数：响应SIGCHLD信号
 * 作用：回收所有已退出的子进程，避免产生僵尸进程
 */
void zombie_dealer(int sig) {
    pid_t pid;
    int status;
    // 使用WNOHANG非阻塞模式，循环回收所有已退出子进程
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            printf("子进程: %d 以状态 %d 正常退出\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("子进程: %d 被信号 %d 终止\n", pid, WTERMSIG(status));
        } else {
            printf("子进程: %d 异常退出\n", pid);
        }
    }
}

/**
 * 客户端数据处理函数
 * 功能：接收客户端数据并返回确认信息
 * 参数：客户端套接字描述符
 */
void *read_from_client_then_write(void *argv)
{
    int client_fd = *(int *)argv;
    ssize_t count = 0, send_count = 0;
    char *read_buf = malloc(1024);  // 接收缓冲区
    char *write_buf = malloc(1024); // 发送缓冲区

    // 内存分配检查
    if (!read_buf) {
        printf("服务端pid: %d: 读缓存分配失败\n", getpid());
        close(client_fd);
        perror("malloc read_buf");
        return NULL;
    }

    if (!write_buf) {
        printf("服务端pid: %d: 写缓存分配失败\n", getpid());
        free(read_buf);
        close(client_fd);
        perror("malloc write_buf");
        return NULL;
    }

    // 循环接收客户端数据
    while ((count = recv(client_fd, read_buf, 1024, 0)) > 0) {
        printf("服务端pid: %d: 收到客户端(fd=%d)数据: %s\n", getpid(), client_fd, read_buf);
        
        // 构建响应信息，包含当前进程ID
        sprintf(write_buf, "服务端pid: %d: 已收到数据\n", getpid());
        send_count = send(client_fd, write_buf, strlen(write_buf), 0);
        if (send_count < 0) {
            perror("send failed");
            break;
        }
        memset(read_buf, 0, 1024);  // 清空缓冲区
    }

    // 处理连接关闭
    printf("服务端pid: %d: 客户端(fd=%d)请求关闭连接\n", getpid(), client_fd);
    sprintf(write_buf, "服务端pid: %d: 已收到关闭信号\n", getpid());
    send(client_fd, write_buf, strlen(write_buf), 0);

    // 资源清理
    close(client_fd);
    free(read_buf);
    free(write_buf);
    return NULL;
}

int main(int argc, char const *argv[])
{
    int sockfd, temp_result;
    struct sockaddr_in server_addr, client_addr;
    socklen_t cliaddr_len = sizeof(client_addr);

    // 初始化服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 监听所有网络接口
    server_addr.sin_port = htons(6666);               // 绑定到6666端口

    // 创建TCP套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    handle_error("socket creation failed", sockfd);

    // 绑定地址与端口
    temp_result = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    handle_error("bind failed", temp_result);

    // 开始监听，最大等待队列128
    temp_result = listen(sockfd, 128);
    handle_error("listen failed", temp_result);

    // 注册SIGCHLD信号处理函数，解决僵尸进程问题
    signal(SIGCHLD, zombie_dealer);

    // 主循环：持续接受客户端连接
    while (1) {
        // 接受新连接
        int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &cliaddr_len);
        handle_error("accept failed", client_fd);

        // 创建子进程处理客户端
        pid_t pid = fork();

        if (pid > 0) {  // 父进程
            // 父进程关闭客户端套接字（引用计数减1）
            close(client_fd);
            printf("父进程(pid=%d): 继续等待新连接\n", getpid());
        }
        else if (pid == 0) {  // 子进程
            // 子进程关闭监听套接字（不再接受新连接）
            close(sockfd);
            printf("子进程(pid=%d): 与客户端 %s:%d(fd=%d)建立连接\n",
                   getpid(), inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port), client_fd);
            
            // 处理客户端通信
            read_from_client_then_write((void *)&client_fd);
            
            // 处理完毕，退出子进程
            exit(EXIT_SUCCESS);
        }
        else {  // fork失败
            perror("fork failed");
            close(client_fd);
        }
    }

    // 正常情况下不会执行到这里
    close(sockfd);
    return 0;
}
