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
    struct sockaddr_in server_addr;  // 服务器地址结构体
    char *buf = malloc(1024); // 分配1024字节缓冲区

    // 初始化服务器地址结构体
    memset(&server_addr, 0, sizeof(server_addr));

    // 设置服务器地址信息
    server_addr.sin_family = AF_INET;                  // 使用IPv4协议
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 连接到本地回环地址（127.0.0.1）
    server_addr.sin_port = htons(6666);                // 连接到服务器的6666端口

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    handle_error("socket", sockfd);  

    // 循环与服务器通信
    do
    {
        // 提示用户输入要发送的内容（输出到标准输出）
        write(STDOUT_FILENO, "请输入: ", 34);
        
        // 从标准输入（键盘）读取用户输入，最多1023字节（留1字节给终止符）
        int buf_len = read(STDIN_FILENO, buf, 1023);

        // 发送数据到服务器
        temp_result = sendto(sockfd, buf, buf_len, 0,(struct sockaddr *)&server_addr,sizeof(server_addr));
        handle_error("sendto", temp_result);  

        // 清空缓冲区，准备接收服务器回复
        memset(buf, 0, 1024);

        // 接收服务器的回复
        // NULL, NULL：不关心服务器地址（已知是连接的服务器）
        temp_result = recvfrom(sockfd, buf, 1024, 0, NULL, NULL);
        handle_error("recvfrom", temp_result);  

        // 判断服务器回复是否为"EOF"
        if (strncmp(buf, "EOF", 3) != 0)
        {
            // 打印服务器回复的消息
            printf("received msg from %s at port %d: %s",inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port),buf);                             // 回复的消息
        }
    } while (strncmp(buf, "EOF", 3) != 0);  // 直到收到服务器的EOF回复才退出

    free(buf);

    return 0;
}