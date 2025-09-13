#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid == 0)//子进程写入
    {
        close(pipefd[0]);
        write(pipefd[1],argv[1],strlen(argv[1]));
        close(pipefd[1]);
    }
    else
    {
        waitpid(pid,NULL,0);
        char buf[1024];
        ssize_t count;
        close(pipefd[1]);//父进程读出
        while((count=(read(pipefd[0],buf,1))) > 0)
        {
            write(1,buf,count);
        }
        close(pipefd[0]);

    }
    return 0;

}
