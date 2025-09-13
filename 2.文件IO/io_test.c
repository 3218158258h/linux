#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc,char * argv[])
{
	int fd = open("io.txt",O_RDONLY);
	if(fd == -1)
	{
		perror("打开文件失败");
		exit(1);
	}
	char buf[500];
	int count;
	while((count = read(fd,buf,sizeof(buf))) > 0)
	{
		write(1,buf,count);
	}
	if(count == -1)
	{
		perror("输出失败");
	}
	close(fd);
	return 0;
}
