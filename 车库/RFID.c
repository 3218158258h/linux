//  描述: 使用RFID读卡器读取RFID卡片信息

#include <stdio.h>
#include <assert.h>
#include <fcntl.h> 
#include <unistd.h>
#include <termios.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#include "thirdparty/log.c/log.h"
#include "head.h"

// 定义读卡器两种状态
enum state{IN, OUT};
enum state RFID_state;

bool cardOn = false;
void *waitting(void *arg)
{
    char r[] = {'-', '\\', '|', '/'};
    for(int i=0;;i++)
    {
        fprintf(stderr, "\r%c", r[i%4]);
		if(cardOn)
            usleep(100*1000);
		else
            usleep(400*1000);
    }
}

//初始化串口
int init_tty(int fd)
{
	struct termios old_flags,new_flags;   //定义两个结构体
	bzero(&new_flags,sizeof(new_flags));  //// 将new_flags结构体的内存清零 
	
	//1. 获取旧的属性
	tcgetattr(fd,&old_flags);       //终端恢复到原来的状态
	
	//2. 设置原始模式
	cfmakeraw(&new_flags);
	
	//3. 激活本地连接CLOCAL与接收使能CREAD的选项  忽略调制解调器控制线
	new_flags.c_cflag |= CLOCAL | CREAD; 
	
	//4. 设置波特率
	cfsetispeed(&new_flags, B9600); 
	cfsetospeed(&new_flags, B9600);
	
	//5. 设置数据位为8位
	new_flags.c_cflag &= ~CSIZE; //清空原有的数据位
	new_flags.c_cflag |= CS8;
	
	//6. 设置奇偶检验位
	new_flags.c_cflag &= ~PARENB;
	
	//7. 设置一位停止位
	new_flags.c_cflag &= ~CSTOPB;
	
	//8. 设置等待时间，最少接收字符个数     在这种情况下，read()函数会被阻塞，直到有 MIN 个字符可以读取时才返 回，返回值是读取的字符数量。到达文件尾时返回 0。
	new_flags.c_cc[VTIME] = 0;
	new_flags.c_cc[VMIN] = 1;
	
	//9. 清空串口缓冲区    tcflush()清空缓冲区对接收到而未被读取的数据进行清空处理；
	tcflush(fd, TCIFLUSH);
	
	//10. 设置串口的属性到文件中    配置立即生效
	if(tcsetattr(fd, TCSANOW, &new_flags) != 0)
	{
		perror("设置串口失败");
		exit(0);
	}
	
	return 0;
}

//校正
char get_bcc(char *buf,int n)
{
	char bcc;
	int i;
	for(i=0;i<n;i++)
	{
		bcc ^= buf[i];
	}
	
	return (~bcc);
}

void usage(int argc, char **argv)
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s <tty>\n", argv[0]);
		exit(0);
	}
}

/*************************************************
功能: 初始化卡片放置标志位，当检查到卡片后，flag置为false，当卡片移除后1秒，
执行refresh标志，将flag标志从false重新置为true，flag为真意味着：卡片刚放上去。
*************************************************/
bool flag = true; 
void refresh(int sig)
{
	// 卡片离开1秒后
	flag = true;
}

void *in_out(void *arg)
{
	// 按回车切换
	while(1)
	{
		RFID_state = IN;
		fprintf(stderr, "\r当前状态：【入库】\n");
		getchar();

		RFID_state = OUT;
		fprintf(stderr, "\r当前状态：【出库】\n");
		getchar();
	}
}



int main(int argc, char **argv)
{
	usage(argc, argv);  //验证是否传入两位
	
	// 设置定时器到达指定时间执行的函数
	signal(SIGALRM, refresh);       //绑定超时函数 

	// 准备好通信管道
	int fifoIN  = open(RFID2SQLiteIN,  O_RDWR);       //可读可写 成功返回fd ,失败返回-1
	int fifoOUT = open(RFID2SQLiteOUT, O_RDWR);
	if(fifoIN==-1 || fifoOUT==-1)
	{
		perror("RFID模块打开管道失败");
		exit(0);
	}

	// 初始化串口
	int fd = open(argv[1]/*/dev/ttySACx*/, O_RDWR | O_NOCTTY);  //该标志用于告知系统/dev/ttymxc2 它不会成为进程的控 制终端
	if(fd == -1)
	{
		printf("open %s failed: %s\n", argv[1], strerror(errno));
		exit(0);
	}
	init_tty(fd);


	// 将串口设置为非阻塞状态，避免第一次运行卡住的情况
	long state = fcntl(fd, F_GETFL); //获取文件标志状态
	state |= O_NONBLOCK;
	fcntl(fd, F_SETFL, state);//添加非阻塞

	// 向主控程序回到本模块启动成功
	sem_t *s = sem_open(SEM_OK, 0666);
	sem_post(s); // 将信号量SEM_OK的值+1   用于通知其他进程（如主控程序）本模块（RFID 模块）已启动成功  信号量由操作系统内核管理，进程通过名称 (SEM_OK) 引用同一内核对象。即使 s 是 RFID 模块的局部变量，其指向的信号量是全局的

	// 延迟一会儿启动
	sleep(1);

	// 创建一条专门用来切换读卡器状态的线程
	pthread_t tid;
	pthread_create(&tid, NULL, in_out,   NULL);   //   阻塞等待用户输入  每次调用 getchar() 时，程序会暂停执行，直到用户按下回车键。这用于实现手动切换状态（入库/出库）的触发条件
	pthread_create(&tid, NULL, waitting, NULL);

	int id;
	while(1)
	{
		// 检测附近是否有卡片，准备请求命令字
		char wbuf[7];
		bzero(wbuf,7);
		
		char rbuf[8]; 
		
		wbuf[0] = 0x07;//帧长
		wbuf[1] = 0x02;//命令类型
		wbuf[2] = 0x41;//命令字
		wbuf[3] = 0x01;//数据长度
		wbuf[4] = 0x52;//请求模式  ALL
		wbuf[5] = get_bcc(wbuf,wbuf[0]-2); //检验和
		wbuf[6] = 0x03;//结束标志
	
		// 通过串口往RFID模块中写入请求命令，一旦探测到卡片就退出
		while(1)
		{
			tcflush(fd, TCIFLUSH);  //清空缓存
			write(fd,wbuf,7);
			
			usleep(10000); //确保串口把全部的请求命令数据发送过去
			
			bzero(rbuf,8);//读取响应
			read(fd,rbuf,8);
			
			//代表RFID附近有卡
			if(rbuf[2] == 0x00)
			{
				cardOn = true;
				printf("get rfid ok!\n");
				usleep(300000);
				break;
			}
			cardOn = false;
		}
		// =======================================

		// 获取卡号
		char kbuf[8];
		bzero(kbuf,8);
		
		char Rbuf[10];
		bzero(Rbuf,10);
		
		kbuf[0] = 0x08;//帧长
		kbuf[1] = 0x02;//命令类型  0x02  ISO14443A命令字
		kbuf[2] = 0x42;//命令
		kbuf[3] = 0x02;//数据长度
		kbuf[4] = 0x93;//一级防碰撞
		kbuf[5] = 0x00;
		kbuf[6] = get_bcc(kbuf,kbuf[0]-2);
		kbuf[7] = 0x03;//结束标志
		
		tcflush(fd, TCIFLUSH);
		write(fd,kbuf,8);
			
		usleep(10000); //确保串口把全部的请求命令数据发送过去
		
		read(fd,Rbuf,10);
		
		int cardid;
		if(Rbuf[2] == 0x00) //读取卡号成功
		{
			int i,j;
			for(i=3,j=0;i>=0;i--,j++)
			{
				memcpy((char *)&cardid+j,&Rbuf[4+i],1);	
			}	
		}
		else
		{
			printf("get cardid error!\n");
			return -1;
		}
		
		printf("cardid:%d\n",cardid);   // # 代表带格式输出 
		if(cardid == 0 || cardid == 0xFFFFFFFF)
		{
			continue;
		}


		// flag为真意味着：卡片刚放上去
		if(flag)
		{
			if(RFID_state == IN)
			{
				// 此时RFID_state是IN，代表是新车入库，通过fifoIN管道将卡号发给SQLite，
				// 告知数据库有新车进来，数据库中需要添加对应车辆信息
				int m = write(fifoIN, &cardid, sizeof(cardid));
				if(m <= 0)
				{
					perror("发送卡号失败");
				}
			}

			if(RFID_state == OUT)
			{
				// 此时RFID_state是OUT，代表是有车出库，通过fifoOUT管道将卡号发给SQLite，
				// 告知数据库有车要出去了，要将数据库中对应车辆信息删除。
				int m = write(fifoOUT, &cardid, sizeof(cardid));
				if(m <= 0)
				{
					perror("发送卡号失败");
				}
			}

			flag = false;
		}

		// 设置定时器，每一秒执行一次refresh()函数，
		alarm(1);
	}

	close(fd);
	exit(0);
}
