
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
int main()
{
    char *path = "/home/test/linux1";
    if(access(path,F_OK) == -1){
    if(mkfifo(path,0666) == -1)
    {
        perror("mkfifo");
        exit(1);
    }}
    int fd ;
    if((fd = open(path,O_WRONLY))==-1)
    {
        perror("open");
        exit(1);
    }
    char buf[1024];
    ssize_t count;
    while((count = read(0,buf,50))>0)
    {
        write(fd,buf,count);
    }
    printf("输入结束");
    close(fd);
    return 0;
}
