#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc,char *argv[])
{
int fd;
char buf[64] = {0};//定义 buf 缓存
//打开设备节点
fd = open("/dev/hello_misc",O_RDWR);
if(fd < 0)
{
//打开设备节点失败
perror("open error \n");
return fd;
}
// atoi()将字符串转为整型，这里将第一个参数转化为整型后，存放在 buf[0]中
buf[0] = atoi(argv[1]);
//把缓冲区数据写入文件中
write(fd,buf,sizeof(buf));
printf("buf is %d\n",buf[0]);
close(fd);
return 0;
}