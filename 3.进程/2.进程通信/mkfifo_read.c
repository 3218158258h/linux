#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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
    if((fd = open(path,O_RDONLY))==-1)
    {
        perror("open");
        exit(1);
    }
    char buf[1024];
    ssize_t count;
    while((count = read(fd,buf,1))>0)
    {
        write(1,buf,count);
    }
    printf("接收结束");
    unlink(path);
    close(fd);
    return 0;
}
