#include <sys/socket.h>   
#include <sys/types.h>   
#include <netinet/in.h>  
#include <stdio.h>       
#include <stdlib.h>     
#include <string.h>      
#include <arpa/inet.h>    

// 错误处理宏
#define handle_error(cmd, result) \
    if (result < 0)               \
    {                             \
        perror(cmd);              \
        return -1;                \
    }

int main(int argc, char const *argv[])
{
    int sockfd;               
    int temp_result;         
    struct sockaddr_in server_addr ,client_addr;  // 服务器地址结构体 客户端地址结构体（用于存储发送方信息）
    char *buf = malloc(1024); // 分配1024字节缓冲区，用于接收和发送数据

    // 初始化地址结构体
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // 设置服务器地址结构体
    server_addr.sin_family = AF_INET;                // 使用IPv4协议
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 绑定到所有可用网络接口（0.0.0.0）
    server_addr.sin_port = htons(6666);              // 绑定到6666端口（非特权端口，>1024）

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    handle_error("socket", sockfd);  // 检查socket创建是否成功

    // 将套接字绑定到指定地址和端口
    temp_result = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    handle_error("bind", temp_result);  // 检查绑定是否成功

    // 循环处理客户端数据
    do
    {
        // 清空接收缓冲区
        memset(buf, 0, 1024);

        // 接收客户端发送的数据
        socklen_t client_addr_len = sizeof(client_addr);  // 初始化客户端地址长度
        temp_result = recvfrom(sockfd, buf, 1024, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        handle_error("recvfrom", temp_result);  // 检查接收是否成功

        // 判断客户端发送的是否为"EOF"（结束标志）
        if (strncmp(buf, "EOF", 3) != 0)
        {
            // 打印接收到的消息及客户端信息
            printf("received msg: %s",buf);                            
            // 准备回复"OK\n"
            strcpy(buf, "OK\n");
        } 
        else 
        {
            // 收到结束标志，打印退出信息
            printf("received EOF from client, existing\n");
        }

        // 向客户端发送回复（OK或EOF）
        temp_result = sendto(sockfd, buf, 4, 0,(struct sockaddr *)&client_addr,client_addr_len);
        handle_error("sendto", temp_result);  // 检查发送是否成功

    } while (strncmp(buf, "EOF", 3) != 0);  // 直到收到EOF才退出循环

    // 释放缓冲区内存
    free(buf);

    return 0;
}