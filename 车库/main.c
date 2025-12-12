#include "include/head.h"
#include "thirdparty/log.c/log.h"



pid_t p1 = -1, p2 = -1, p3 = -1, p4 = -1;

//子模块崩溃次数
static int count_RFID = 0,count_SQLite = 0,count_Audio = 0,count_Video = 0;
static int exit_flag = 0;

//通知子进程创建完成的信号量
sem_t *s;

/*************************************************
功能:创建子进程
pid_t *p:创建的子进程ID指针
char *argv[0]:子进程名称 如"RFID"
char *argv[1]:子进程运行的程序"./RFID"
char *argv[2]:设备参数，为空则是NULL
*************************************************/
int create_process(pid_t *p, char *argv[],sem_t *s)
{
	
	*p=fork();
	if(*p == -1)
	{
		log_error("create %s failed\n",argv[0]);
		return -1;
	}
	if(*p == 0)    //返回0时是子进程分支
	{
		if(argv[2] != NULL)
		{
			execl(argv[1], argv[0], argv[2], NULL);//传入串口设备路径argv[2]
			log_error("%s start failed\n",argv[0]);
			_exit(EXIT_FAILURE);
		
		}
		else{
			execl(argv[1], argv[0], NULL);
			log_error("%s start failed\n",argv[0]);
			_exit(EXIT_FAILURE);
			
		}
	}
	log_log("%s starting ..\n",argv[0]);
	sem_wait(s); // 等待子模块启动成功      
	log_log("%s started\n",argv[0]); 
	return 0;
}

/*************************************************
功能:将所有打开的子进程都杀死并退出程序
*************************************************/
void kill_all_children() {
    if (p1 > 0) { // 先判断pid是否有效（>0表示子进程已创建）
        kill(p1, SIGKILL);
        waitpid(p1, NULL, WNOHANG); // 非阻塞回收僵尸进程
    }
    if (p2 > 0) {
        kill(p2, SIGKILL);
        waitpid(p2, NULL, WNOHANG);
    }
    if (p3 > 0) {
        kill(p3, SIGKILL);
        waitpid(p3, NULL, WNOHANG);
    }
    if (p4 > 0) {
        kill(p4, SIGKILL);
        waitpid(p4, NULL, WNOHANG);
    }
}

/*************************************************
功能:清理资源
*************************************************/
void clean_up()
{
	if (s != NULL && s != SEM_FAILED) {
        sem_close(s); 
    }
	sem_unlink(SEM_OK);
    //sem_unlink(SEM_TAKEPHOTO);
    unlink(RFID2SQLiteIN);
    unlink(RFID2SQLiteOUT);
    unlink(SQLite2Audio);
    unlink(Video2SQLite);
    
}

/*************************************************
功能:如果刷卡、数据库、语音、视频这4个子进程关闭尝试重启
多次关闭则将所有打开的子进程都杀死并退出程序
*************************************************/


void is_restart(pid_t *p,char *argv[],sem_t *s,int *count)
{ 
	
	if(*count >= 3)
	{
		log_error("子进程持续崩溃，正在退出...\n");
		kill_all_children();
		clean_up();
		exit(EXIT_FAILURE);
	}
	else {
		log_error("子进程崩溃%d次，正在重启...\n",*count);
		create_process(p, argv,s);
		exit_flag = 0;
		sleep(1);

	}
}

//信号处理函数
void quit(int sig) {
    (void)sig; // 消除未使用参数警告
    pid_t t;
    int status;
    // 循环回收所有退出的子进程
    while ((t = waitpid(-1, &status, WNOHANG)) > 0) {
        switch(t) {
            case p1: exit_flag = 1; count_RFID++; break;
            case p2: exit_flag = 2; count_SQLite++; break;
            case p3: exit_flag = 3; count_Audio++; break;
            case p4: exit_flag = 4; count_Video++; break;
            default: break; // 处理非目标子进程
        }
    }
}

/*************************************************
功能:如果主控程序退出就调用该函数，将所有打开的子进程都杀死并退出程序
*************************************************/
void stop(int sig)
{
	exit_flag = -1;
}

	int main(int argc, char **argv)
{
	

	if(argc != 3)
	{
		log_error("参数错误，请指定串口和摄像头\n");
		log_log("例如: ./main /dev/ttySAC2 /dev/video7\n");
		exit(EXIT_FAILURE);
	}

	// 初始化子进程参数数组
    // RFID子进程参数
	//传入串口设备路径
    char *str_RFID[3] = {"RFID", "./RFID", argv[1]}; 

    // SQLite子进程参数
    char *str_SQLite[3] = {"SQLite", "./SQLite", NULL};

    // Audio子进程参数
    char *str_Audio[3] = {"Audio", "./Audio", NULL};

    // Video子进程参数
	//传入摄像头设备路径
    char *str_Video[3] = {"Video", "./Video", argv[2]};


	// 创建各种管道，用于各模块之间的数据交互
	// 若已存在则忽略错误（EEXIST）
    if (mkfifo(RFID2SQLiteIN, 0666) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", RFID2SQLiteIN, strerror(errno));
        goto exit_all;
    }
    if (mkfifo(RFID2SQLiteOUT, 0666) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", RFID2SQLiteOUT, strerror(errno));
        goto unlink_RFID2SQLiteIN;
    }
    if (mkfifo(SQLite2Audio, 0666) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", SQLite2Audio, strerror(errno));
        goto unlink_RFID2SQLiteOUT;
    }
    if (mkfifo(Video2SQLite, 0666) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", Video2SQLite, strerror(errno));
        goto unlink_SQLite2Audio;
    }

	// 创建信号量，用于各模块之间的互相通知
	sem_unlink(SEM_OK);
	//sem_unlink(SEM_TAKEPHOTO);
	s= sem_open(SEM_OK, O_CREAT, 0666, 0);
	if(s==SEM_FAILED){
		log_error("创建信号量 %s 失败: %s\n", SEM_OK, strerror(errno));
		goto kill;
	}
	//sem_t *s_takePhoto = sem_open(SEM_TAKEPHOTO, O_CREAT, 0777, 0);

	// 如果有子模块启动失败，则主控程序也相应地退出   
	//子进程结束的时候，会产生一个SIGCHLD信号
	// 替换错误的 sigaction(SIGCHLD, quit);
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa)); // 初始化结构体
	sa.sa_handler = quit;       // 绑定处理函数
	sa.sa_flags = SA_NOCLDSTOP; // 可选：子进程暂停时不发送SIGCHLD
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    	log_error("注册SIGCHLD信号失败: %s\n", strerror(errno));
    	goto kill; // 注册失败时退出
	}                   

	// 如果主控程序退出，那么也依次退出清除所有的子模块
	signal(SIGINT/*ctrl+c*/, stop);


	// 创建子进程: 执行刷卡
	if((create_process(&p1, str_RFID,s)) != 0)
	{
		goto kill;
	}
	// 创建子进程: 执行数据库
	if((create_process(&p2, str_SQLite,s)) != 0)
	{
		goto kill;
	}
	
	// 创建子进程: 执行语音
	if((create_process(&p3, str_Audio,s)) != 0)
	{
		goto kill;
	}
	
	// 创建子进程: 执行视频
	if((create_process(&p4, str_Video,s)) != 0)
	{
		goto kill;
	}
	

    // 主程序不能退出
	while(1)
	{
    	pause(); // 阻塞，直到收到任意信号
    	if (exit_flag >= 1 && exit_flag <= 4) 
		{
        	switch(exit_flag) {
            	case 1: is_restart(&p1, str_RFID, s, &count_RFID); break;
            	case 2: is_restart(&p2, str_SQLite, s, &count_SQLite); break;
            	case 3: is_restart(&p3, str_Audio, s, &count_Audio); break;
            	case 4: is_restart(&p4, str_Video, s, &count_Video); break;
        		}
    	}
    	if (exit_flag == -1) {
        	kill_all_children();
        	clean_up();
        	exit(EXIT_SUCCESS);
    	}
	}
	kill:
	kill_all_children();

	unlink_Video2SQLite:
	unlink(Video2SQLite);

	unlink_SQLite2Audio:
	unlink(SQLite2Audio);

	unlink_RFID2SQLiteOUT:
	unlink(RFID2SQLiteOUT);

	unlink_RFID2SQLiteIN:
	unlink(RFID2SQLiteIN);

	exit_all:
	exit(EXIT_FAILURE);

}
