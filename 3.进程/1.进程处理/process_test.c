#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
int main()
{
	int fd = open("process.txt",O_CREAT | O_APPEND | O_RDWR,0664);
	if(fd == -1)
	{
		perror("open");
		exit(1);
	}
	char buf[1024];
	pid_t pid = fork();
	if(pid == -1)
	{
		perror("fork");
		exit(1);
	}
	else if(pid == 0)
	{
		printf("这是子进程\n");
		char *msg1 = "天生我材必有用\n";
		write(fd,msg1,strlen(msg1));
	}
	else
	{
		int a;
		waitpid(pid,&a,0);
		printf("这是父进程\n");
		char *msg2 = "千金散尽还复来\n";
		write(fd,msg2,strlen(msg2));
		lseek(fd, 0, SEEK_SET);
		int count_read;
		while((count_read = read(fd,buf,sizeof(buf))) > 0 )
		{
			write(1,buf,count_read);
		}
		if(count_read == -1)
        	{
            	perror("read");
		}
	}
	close(fd);
	return 0;
}
