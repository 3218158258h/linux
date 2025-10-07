// 引入网络编程、进程管理、信号处理核心头文件
#include <sys/socket.h>  // 套接字操作（socket/bind/listen/accept/recv/send）
#include <sys/types.h>   // 系统数据类型（pid_t等）
#include <sys/wait.h>    // 子进程回收（waitpid）
#include <netinet/in.h>  // 网络地址结构（sockaddr_in/htons等）
#include <stdio.h>       // 标准输入输出（虽用syslog，但预留基础接口）
#include <stdlib.h>      // 内存分配、进程退出（malloc/exit）
#include <string.h>      // 字符串处理（memset/sprintf）
#include <arpa/inet.h>   // IP地址转换（inet_ntoa：网络字节序IP转字符串）
#include <pthread.h>     // 线程头文件（代码未实际使用，预留扩展）
#include <unistd.h>      // 系统调用（close/fork/sleep）
#include <signal.h>      // 信号处理（SIGCHLD/SIGTERM）
#include <syslog.h>      // 系统日志（服务端无终端，用syslog输出日志）

int sockfd;  // 全局变量：监听套接字（接收客户端连接，信号处理函数需访问）

// 信号处理函数：处理SIGCHLD（子进程退出信号），回收僵尸进程
void zombie_dealer(int sig)
{
    pid_t pid;          // 存储回收的子进程PID
    int status;         // 存储子进程退出状态（正常/信号终止）
    char buf[1024];
    memset(buf, 0, 1024);  // 清空日志缓冲区，避免垃圾数据干扰

    // 关键逻辑：while循环+WNOHANG（非阻塞）回收所有退出子进程
    // 原因：一个SIGCHLD可能对应多个子进程同时退出，if仅能回收一个，会漏收
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status))  // 判断子进程是否"正常退出"（如exit(0)）
        {
            sprintf(buf, "子进程: %d 以状态 %d 正常退出，已回收", pid, WEXITSTATUS(status));
            syslog(LOG_INFO, "%s", buf);  // 日志级别：信息（LOG_INFO）
        }
        else if (WIFSIGNALED(status))  // 判断子进程是否"被信号杀死"（如kill -9）
        {
            sprintf(buf, "子进程: %d 被信号 %d 杀死，已回收", pid, WTERMSIG(status));
            syslog(LOG_INFO, "%s", buf);
        }
        else  // 其他退出原因（极少发生，如异常崩溃）
        {
            sprintf(buf, "子进程: %d 因未知原因退出，已回收", pid);
            syslog(LOG_WARNING, "%s", buf);  // 日志级别：警告（LOG_WARNING）
        }
    }
}

// 信号处理函数：处理SIGTERM（守护进程发送的终止信号），实现服务端优雅退出
void sigterm_handler(int sig) {
    syslog(LOG_NOTICE, "服务端收到守护进程的SIGTERM信号，准备退出...");
    syslog(LOG_NOTICE, "释放监听套接字（sockfd）");
    close(sockfd);  // 关闭监听套接字，避免端口占用（下次启动报错）
    syslog(LOG_NOTICE, "关闭syslog连接，服务端进程终止");
    closelog();     // 释放系统日志资源
    exit(EXIT_SUCCESS);  // 正常退出服务端
}

// 客户端通信逻辑：读取客户端数据并回复"已收到"
// 参数argv：客户端套接字地址（需强转为int*，因pthread参数要求void*）
void read_from_client_then_write(void *argv)
{
    int client_fd = *(int *)argv;  // 获取客户端套接字（与该客户端的通信句柄）
    ssize_t count = 0;             // recv返回值：读取到的字节数
    ssize_t send_count = 0;        // send返回值：发送成功的字节数
    char *read_buf = NULL;         // 读缓冲区：存储客户端发送的数据
    char *write_buf = NULL;        // 写缓冲区：存储服务端回复的数据
    char log_buf[1024];
    memset(log_buf, 0, 1024);

    // 1. 分配读缓冲区（1024字节，满足常规文本传输）
    read_buf = malloc(sizeof(char) * 1024);
    if (!read_buf)  // 内存分配失败（系统资源不足）
    {
        sprintf(log_buf, "服务端pid: %d 读缓存创建失败，断开client_fd: %d", getpid(), client_fd);
        syslog(LOG_ERR, "%s", log_buf);  // 日志级别：错误（LOG_ERR）
        shutdown(client_fd, SHUT_WR);    // 关闭写方向，告知客户端不再发数据
        close(client_fd);                // 关闭客户端套接字，释放资源
        return;
    }

    // 2. 分配写缓冲区（1024字节）
    write_buf = malloc(sizeof(char) * 1024);
    if (!write_buf)
    {
        sprintf(log_buf, "服务端pid: %d 写缓存创建失败，断开client_fd: %d", getpid(), client_fd);
        syslog(LOG_ERR, "%s", log_buf);
        free(read_buf);                  // 释放已分配的读缓存，避免内存泄漏
        shutdown(client_fd, SHUT_WR);
        close(client_fd);
        return;
    }

    // 3. 循环读取客户端数据（核心通信逻辑）
    // recv返回0：客户端主动关闭连接；返回-1：读取错误；返回正数：实际读取字节数
    while ((count = recv(client_fd, read_buf, 1024, 0)))
    {
        if (count < 0)  // 读取错误（如网络中断、套接字异常）
        {
            syslog(LOG_ERR, "服务端pid: %d recv错误（client_fd: %d）", getpid(), client_fd);
            break;  // 跳出循环，清理资源
        }

        // 记录客户端数据到日志
        sprintf(log_buf, "服务端pid: %d 从client_fd: %d 收到数据：%s", getpid(), client_fd, read_buf);
        syslog(LOG_INFO, "%s", log_buf);
        memset(log_buf, 0, 1024);  // 清空日志缓冲区

        // 构造回复数据，告知客户端"已收到"
        sprintf(write_buf, "服务端pid: %d：已收到你的消息！\n", getpid());
        send_count = send(client_fd, write_buf, 1024, 0);  // 发送回复

        // 优化点：此处未判断send_count是否成功（若网络异常，发送可能失败，需补充错误处理）
        memset(read_buf, 0, 1024);  // 清空读缓存，避免下次读取旧数据
    }

    // 4. 客户端关闭连接，清理资源（退出循环后的收尾）
    sprintf(log_buf, "服务端pid: %d 客户端client_fd: %d 请求关闭连接", getpid(), client_fd);
    syslog(LOG_NOTICE, "%s", log_buf);  // 日志级别：通知（LOG_NOTICE）

    // 发送最后一条关闭通知
    sprintf(write_buf, "服务端pid: %d：已收到关闭请求，即将断开连接\n", getpid());
    send_count = send(client_fd, write_buf, 1024, 0);

    // 释放所有资源
    sprintf(log_buf, "服务端pid: %d 释放client_fd: %d 资源", getpid(), client_fd);
    syslog(LOG_NOTICE, "%s", log_buf);
    shutdown(client_fd, SHUT_WR);  // 关闭写方向，确保数据发送完成
    close(client_fd);              // 关闭客户端套接字
    free(read_buf);                // 释放读缓存（避免内存泄漏）
    free(write_buf);               // 释放写缓存

    return;
}

int main(int argc, char const *argv[])
{
    int temp_result;  // 临时变量：存储系统调用返回值（判断是否成功）
    struct sockaddr_in server_addr, client_addr;  // 服务器/客户端地址结构
    socklen_t cliaddr_len = sizeof(client_addr);  // 客户端地址长度（accept输出参数）
    char log_buf[1024];
    memset(log_buf, 0, 1024);

    // 1. 初始化服务器地址结构（清垃圾数据，避免随机值干扰）
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // 2. 配置服务器地址信息
    server_addr.sin_family = AF_INET;         // 地址族：IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 绑定所有网卡（0.0.0.0），支持外部访问
    server_addr.sin_port = htons(6666);       // 绑定端口6666（需与客户端一致，避免特权端口<1024）

    // 3. 创建监听套接字：IPv4、流式TCP（SOCK_STREAM）、默认协议（0）
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // 优化点：此处未判断socket是否成功（若失败，后续调用会崩溃，需补充if (sockfd < 0) { perror("socket"); exit(1); }）

    // 4. 绑定套接字到指定地址（IP+端口）
    temp_result = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // 优化点：未判断bind是否成功（如端口被占用会失败，需补充错误处理）

    // 5. 将套接字设为监听模式，等待客户端连接
    // 第二个参数128：监听队列最大长度（超过则新连接被拒绝）
    temp_result = listen(sockfd, 128);
    // 优化点：未判断listen是否成功（需补充错误处理）

    // 6. 注册信号处理：解决僵尸进程+实现优雅退出
    signal(SIGCHLD, zombie_dealer);  // 子进程退出时自动回收
    signal(SIGTERM, sigterm_handler);  // 收到终止信号时优雅退出

    // 7. 初始化系统日志（服务端无终端，用syslog输出）
    openlog("tcp_server_process: ", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "服务端启动成功，监听端口 6666...");

    // 8. 循环接受客户端连接（服务端核心逻辑，永不退出除非收到SIGTERM）
    while (1)
    {
        // 接受客户端连接：创建新的"客户端套接字"（与该客户端通信，区别于监听套接字）
        int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &cliaddr_len);
        // 优化点：未判断accept是否成功（如被信号中断会返回-1，需补充处理EINTR错误）

        // 9. 创建子进程：父进程继续接受新连接，子进程处理当前客户端通信
        pid_t pid = fork();
        if (pid > 0)  // 父进程：无需处理客户端，关闭client_fd（引用计数-1）
        {
            sprintf(log_buf, "父进程pid: %d 继续接受新连接，子进程pid: %d 处理client_fd: %d", getpid(), pid, client_fd);
            syslog(LOG_INFO, "%s", log_buf);
            memset(log_buf, 0, 1024);
            close(client_fd);  // 父进程释放client_fd，避免文件描述符泄漏
        }
        else if (pid == 0)  // 子进程：处理当前客户端通信（关闭监听套接字，避免占用）
        {
            close(sockfd);  // 子进程无需监听，关闭sockfd（引用计数-1，父进程仍可用）

            // 记录客户端连接信息（IP+端口）
            sprintf(log_buf, "与客户端 [IP: %s, 端口: %d] 建立连接，client_fd: %d，子进程pid: %d", 
                    inet_ntoa(client_addr.sin_addr),  // 网络字节序IP转字符串
                    ntohs(client_addr.sin_port),      // 网络字节序端口转主机序
                    client_fd, getpid());
            syslog(LOG_INFO, "%s", log_buf);

            // 调用通信逻辑，处理客户端数据
            read_from_client_then_write((void *)&client_fd);

            // 通信结束，子进程退出（避免成为僵尸进程，虽有SIGCHLD处理，但主动退出更规范）
            exit(EXIT_SUCCESS);
        }
        else  // fork失败（系统资源不足）
        {
            syslog(LOG_ERR, "fork子进程失败，无法处理client_fd: %d", client_fd);
            close(client_fd);  // 关闭未处理的client_fd，避免泄漏
        }
    }

    // 理论上不会执行到此处（while(1)除非exit退出）
    return 0;
}