#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

//操作失败时打印错误并退出
#define handle_error(cmd, result) \
    if (result < 0)               \
    {                             \
        perror(cmd);              \
        exit(EXIT_FAILURE);       \
    }

// 客户端状态结构体：线程间共享套接字和运行状态
typedef struct {
    int sockfd;       // 连接套接字描述符
    int running;      // 运行状态标记(1-运行,0-退出)
} ClientState;

/**
 * 从服务器读取数据的线程函数
 * @param argv 指向ClientState结构体的指针
 */
void *read_from_server(void *argv)
{
    ClientState *state = (ClientState *)argv;
    int sockfd = state->sockfd;
    char *read_buf = malloc(1024);  // 接收缓冲区
    
    if (!read_buf) {
        perror("malloc read_buf failed");
        state->running = 0;
        return NULL;
    }

    // 循环接收服务器数据直到退出信号
    while (state->running) {
        memset(read_buf, 0, 1024);  // 清空缓冲区
        ssize_t count = recv(sockfd, read_buf, 1023, 0);  // 留1字节给终止符
        
        if (count < 0) {  // 接收错误
            perror("recv failed");
            break;
        } else if (count == 0) {  // 服务器关闭连接
            printf("\nServer disconnected\n");
            break;
        }

        read_buf[count] = '\0';
        printf("\nServer: %s", read_buf);
        printf("Enter message: ");
        //刷新缓冲区
        fflush(stdout);
    }

    state->running = 0;  // 通知写线程退出
    free(read_buf);
    return NULL;
}

/**
 * 向服务器发送数据的线程函数
 * @param argv 指向ClientState结构体的指针
 */
void *write_to_server(void *argv)
{
    ClientState *state = (ClientState *)argv;
    int sockfd = state->sockfd;
    char *write_buf = malloc(1024);  // 发送缓冲区
    
    if (!write_buf) {
        perror("malloc write_buf failed");
        state->running = 0;
        return NULL;
    }

    printf("Enter message (exit to quit): ");
    // 循环读取输入并发送直到退出信号
    while (state->running && fgets(write_buf, 1024, stdin) != NULL) {


        // 发送实际输入的字节数
        ssize_t send_count = send(sockfd, write_buf, strlen(write_buf), 0);
        if (send_count < 0) {
            perror("send failed");
            break;
        }
        
        printf("Enter message: ");
    }

    shutdown(sockfd, SHUT_WR);  // 关闭写方向，告知服务器结束发送
    state->running = 0;         // 通知读线程退出
    free(write_buf);
    return NULL;
}

int main(int argc, char const *argv[])
{
    int sockfd;
    pthread_t pid_read, pid_write;
    ClientState state;
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
    handle_error("connect failed", connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)));

    // 初始化客户端状态
    state.sockfd = sockfd;
    state.running = 1;

    // 创建读写线程
    if (pthread_create(&pid_read, NULL, read_from_server, &state) != 0) {
        perror("read thread creation failed");
        close(sockfd);
        exit(1);
    }

    if (pthread_create(&pid_write, NULL, write_to_server, &state) != 0) {
        perror("write thread creation failed");
        pthread_cancel(pid_read);
        close(sockfd);
        exit(1);
    }


    pthread_join(pid_read, NULL);
    pthread_join(pid_write, NULL);


    printf("Closing connection\n");
    close(sockfd);

    return 0;
}
    