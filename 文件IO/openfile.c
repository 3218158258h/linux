#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char**argv)
{
        if(argc!=2)
        {
                printf("请输入文件名");
        }
        else
        {
                int fd= open(argv[1],O_RDWR|O_CREAT,0766);
                if(fd==-1)
                {
                        printf("error");
                }
                else
                {
                        char *msg="open_test";
                        write(fd,msg,strlen(msg));
                }
        }
return 0;
}
