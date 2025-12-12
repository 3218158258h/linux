#include "include/head.h"
#include "thirdparty/log.c/log.h"

pid_t p1 = -1, p2 = -1, p3 = -1, p4 = -1;
int count = 0;


/*************************************************
功能:创建子进程
pid_t s:创建的子进程ID 
char *argv1:子进程名称 如"RFID"
char *argv2:子进程运行的程序"./RFID"
*************************************************/
void create_pthreat(pid_t s, char *argv1,char *argv2)
{
	s=fork();
	if(s == -1)
	{
		log_error("create %s failed\n",&argv1);
		goto kill;
	}
	if(s == 0)    //返回0时是子进程分支
	{
		if(execl(&argv2, &argv1, &argv2, NULL))//传入串口设备路径argv[1]
		{
			log_error("RFID start failed\n");
			exit(EXIT_FAILURE);
		}
	}
	log_log("RFID starting ..\n");
	sem_wait(s); // 等待子模块启动成功      
	log_log("RFID started\n"); 
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
	sem_unlink(SEM_OK);
    sem_unlink(SEM_TAKEPHOTO);
    unlink(RFID2SQLiteIN);
    unlink(RFID2SQLiteOUT);
    unlink(SQLite2Audio);
    unlink(Video2SQLite);
    
}

/*************************************************
功能:如果刷卡、数据库、语音、视频这4个子进程关闭就调用该函数
将所有打开的子进程都杀死并退出程序
*************************************************/

void quit(int sig)
{
	kill_all_children();
	clean_up();
	exit(EXIT_FAILURE);
}

/*************************************************
功能:如果主控程序退出就调用该函数，将所有打开的子进程都杀死并退出程序
*************************************************/
void stop(int sig)
{
	kill_all_children
	clean_up();
    exit(EXIT_SUCCESS);
}


int main(int argc, char **argv)
{
	if(argc != 3)
	{
		log_error("参数错误，请指定串口和摄像头\n");
		log_log("例如: ./main /dev/ttySAC2 /dev/video7\n");
		exit(EXIT_FAILURE);
	}

	// 创建各种管道，用于各模块之间的数据交互
	// 若已存在则忽略错误（EEXIST）
    if (mkfifo(RFID2SQLiteIN, 0777) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", RFID2SQLiteIN, strerror(errno));
        goto exit;
    }
    if (mkfifo(RFID2SQLiteOUT, 0777) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", RFID2SQLiteOUT, strerror(errno));
        goto unlink_RFID2SQLiteIN;
    }
    if (mkfifo(SQLite2Audio, 0777) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", SQLite2Audio, strerror(errno));
        goto unlink_RFID2SQLiteOUT;
    }
    if (mkfifo(Video2SQLite, 0777) == -1 && errno != EEXIST)
    {
        log_error("创建管道 %s 失败: %s\n", Video2SQLite, strerror(errno));
        goto unlink_SQLite2Audio;
    }

	// 创建信号量，用于各模块之间的互相通知
	sem_unlink(SEM_OK);
	//sem_unlink(SEM_TAKEPHOTO);
	sem_t *s = sem_open(SEM_OK, O_CREAT, 0777, 0);
	//sem_t *s_takePhoto = sem_open(SEM_TAKEPHOTO, O_CREAT, 0777, 0);

	// 如果有子模块启动失败，则主控程序也相应地退出   
	//子进程结束的时候，会产生一个SIGCHLD信号
	signal(SIGCHLD, quit);                     

	// 如果主控程序退出，那么也依次退出清除所有的子模块
	signal(SIGINT/*ctrl+c*/, stop);


	// 创建子进程: 执行刷卡
	p1=fork();
	if(p1 == -1)
	{
		log_error("create RFID failed\n");
		goto kill;
	}
	if(p1 == 0)    //返回0时是子进程分支
	{
		if(execl("./RFID", "RFID", argv[1], NULL))//传入串口设备路径argv[1]
		{
			log_error("RFID start failed\n");
			exit(EXIT_FAILURE);
		}
	}
	log_log("RFID starting ..\n");
	sem_wait(s); // 等待子模块启动成功      
	log_log("RFID started\n");


	// 创建子进程: 执行数据库
	p2=fork();
	if(p2 == -1)
	{
		log_error("create SQLite failed\n");
		goto kill;
	}
	if(p2 == 0)
	{
		if(execl("./SQLite", "SQLite", NULL))
		{
			log_error("SQLite start failed\n");
			exit(EXIT_FAILURE);
		}
	}
	log_log("SQLite starting ..\n");
	sem_wait(s); // 等待子模块启动成功   p操作减一
	log_log("SQLite started\n");

	

	// 创建子进程: 执行语音
	p3=fork();
	if(p3 == -1)
	{
		log_error("create Audio failed\n");
		goto kill;
	}
	if(p3 == 0)
	{
		if(execl("./Audio", "Audio", NULL))
		{
			log_error("Audio start failed\n");
			exit(EXIT_FAILURE);
		}
	}
	log_log("Audio starting ..\n");
	sem_wait(s); // 等待子模块启动成功
	log_log("Audio started\n");


	// 创建子进程: 执行视频
	p4=fork();
	if(p4 == -1)
	{
		log_error("create Video failed\n");
		goto kill;
	}
	if(p4 == 0)
	{
		if(execl("./Video", "Video", argv[2], NULL))//传入摄像头设备路径argv[2]
		{
			log_error("Video start failed\n");
			exit(EXIT_FAILURE);
		}
	}
	log_log("Video starting ..\n");
	sem_wait(s); // 等待子模块启动成功
	log_log("Video started\n");

    // 主程序不能退出
    pause();
	

	return 0;

	


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

	exit:
	exit(EXIT_FAILURE);

}
