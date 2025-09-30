#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

//从客户端接收消息
void * rec_from_client(void* arg)
{
    //获取客户端连接套接字
    int client_fd = *(int*)arg;
    int count = 0;
    char buf[1024] ={0};
    //循环读取并打印
    while((count = recv(client_fd,buf,1023,0))>0)
    {
        buf[count] ='\0';
        printf("收到消息："); 
        printf("%s\n",buf);
        memset(buf,0,sizeof(buf));
    }

    printf("客户端已断开连接\n");
    return NULL;

}

//向客户端发送消息
void  *send_to_client(void* arg)
{
    //获取客户端连接套接字
    int client_fd = *(int*)arg;
    char buf[1024] ={0};
    //循环读取并打印
    while(fgets(buf,1023,stdin)!= NULL)
    {
        if(send(client_fd,buf,strlen(buf),0) == -1)
        {
            perror("send");
        }
        memset(buf,0,sizeof(buf));
    }

    printf("客户端已断开连接\n");
    return NULL;
}

int main() {
    //定义读写线程
    pthread_t read_thread,write_thread;

    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    //定义服务端监听套接字、客户端连接套接字
    int socketfd,clientfd;
    //定义地址端口结构体
    struct sockaddr_in serve_addr,client_addr;
    memset(&serve_addr,0,sizeof(serve_addr));
    memset(&client_addr,0,sizeof(client_addr));

    serve_addr.sin_family = AF_INET;
    serve_addr.sin_port = htons(1265);

    if(!inet_aton("192.168.31.74",&serve_addr.sin_addr))
    {
        perror("inet_aton");
        
        return 0;
    }
    //创建服务端套接字
    if((socketfd=socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        perror("socket");
        return 0;
    }

    //绑定地址与端口号
    if(bind(socketfd,(struct sockaddr*)&serve_addr,sizeof(serve_addr)) == -1)
    {
        perror("bind");
        close(socketfd);
        return 0;
    }

    //启动监听并阻塞直到有客户端发起连接
    if(listen(socketfd,3) == -1)
    {
        perror("listen");
        close(socketfd);
        return 0;
    }
    //同意客户端的连接请求并获取客户端连接套接字
    if((clientfd = accept(socketfd,(struct sockaddr *)&client_addr, &client_addr_len)) == -1)
    {
        perror("accept");
        close(socketfd);
        return 0;
    }

    printf("客户端连接成功\n地址:%s   端口号:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));


    //创建读线程
    if(pthread_create(&read_thread,NULL,rec_from_client,(void*)&clientfd) != 0)
    {
        printf("创建读线程错误");
        close(socketfd);
        return 0;
    }

    //创建写线程
    if(pthread_create(&write_thread,NULL,send_to_client,(void*)&clientfd) != 0)
    {
        printf("创建写线程错误");
        close(socketfd);
        return 0;
    }

    pthread_join(read_thread, NULL);
    pthread_join(write_thread, NULL);
    close(socketfd);
    close(clientfd);
    printf("服务端已关闭\n");


    return 0;
}