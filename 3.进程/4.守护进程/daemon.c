
#include <stdio.h>    
#include <stdlib.h>   
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/stat.h>   
#include <sys/wait.h>   
#include <syslog.h>    
#include <string.h>     
#include <fcntl.h>     
#include <signal.h>     
#include <errno.h>     



// 1. 核心目标：将进程转为“守护进程”（脱离终端、后台运行）
// 通过 my_daemonize() 函数完成“守护进程化”，关键步骤（Linux守护进程标准流程）：

// 两次fork：第一次fork让父进程退出，子进程成为“孤儿进程”（由系统init进程接管），脱离原终端；
// 第二次fork避免子进程意外获得终端控制权（setsid后的进程可能申请终端）。
// 创建新会话（setsid）：脱离原进程组、会话组，避免被终端信号（如Ctrl+C）影响。
// 环境清理：重置文件权限掩码（umask(0)，确保创建文件权限可控）、切换工作目录到根目录（/，避免原目录删除导致进程异常）、关闭所有文件描述符（无需标准输入/输出/错误）。
// 日志初始化：通过 openlog() 绑定系统日志，替代printf（守护进程无终端，日志需写入系统日志）。



// 2. 核心工作：监控TCP服务器，实现“崩溃自动重启”
// 主循环（main() 中while(1)）是守护进程的核心逻辑：

// 创建子进程运行服务器：通过 fork() 创建子进程，子进程用 execve() 加载并运行TCP服务器程序（tcp_server）。
// 等待子进程退出：父进程（守护进程）用 waitpid() 监听子进程状态，若子进程退出（服务器崩溃）：
// 若未收到退出信号（is_shutdown=0）：3秒后重新fork子进程，重启服务器。
// 若收到退出信号（is_shutdown=1）：不再重启，关闭日志并退出。






pid_t pid;               // 全局变量：存储TCP服务器子进程的PID
int is_shutdown = 0;     // 全局标志：标记是否收到"优雅退出"信号

// 信号处理函数：处理SIGHUP（终端断开）、SIGTERM（终止进程）信号
void signal_handler(int sig)
{
    switch (sig)
    {
    case SIGHUP:  // 终端断开信号（守护进程通常忽略或记录）
        syslog(LOG_WARNING, "收到SIGHUP信号...");  // 写入系统日志（警告级别）
        break;
    case SIGTERM: // 终止信号（如kill命令默认发送），触发优雅退出
        syslog(LOG_NOTICE, "接收到终止信号，准备退出守护进程...");
        syslog(LOG_NOTICE, "向子进程发送SIGTERM信号...");
        is_shutdown = 1;          // 置位退出标志，避免重启子进程
        kill(pid, SIGTERM);       // 向TCP服务器子进程发送终止信号
        break;
    default:      // 未处理的其他信号
        syslog(LOG_INFO, "Received unhandled signal");
    }
}

// 守护进程初始化函数：将进程变为"守护进程"（后台运行、脱离终端）
void my_daemonize()
{
    pid_t pid;

    // 第一步：fork子进程，退出父进程——脱离原终端控制
    pid = fork();
    if (pid < 0)          // fork失败（系统资源不足）
        exit(EXIT_FAILURE);
    if (pid > 0)          // 父进程：直接退出，子进程成为孤儿进程（由init进程接管）
        exit(EXIT_SUCCESS);

    // 第二步：创建新会话（setsid）——脱离原会话组、进程组，避免终端影响
    if (setsid() < 0)     // setsid失败（仅进程组组长可调用，此处子进程不是组长，安全）
        exit(EXIT_FAILURE);

    // 第三步：注册信号处理函数——处理SIGHUP和SIGTERM，实现优雅退出
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);

    // 第四步：再次fork子进程，退出父进程——避免守护进程意外获得终端（setsid后的进程可能申请终端）
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)          // 父进程（原会话组长）退出，子进程成为最终守护进程（无终端控制权）
        exit(EXIT_SUCCESS);

    // 第五步：重置文件权限掩码（umask）——确保守护进程创建的文件权限可控
    umask(0);             // 清除掩码，文件权限由open/mknod等参数直接指定

    // 第六步：切换工作目录到根目录（/）——避免原工作目录被删除导致守护进程异常
    chdir("/");

    // 第七步：关闭所有打开的文件描述符——守护进程无需标准输入/输出/错误（0、1、2）
    for (int x = 0; x <= sysconf(_SC_OPEN_MAX); x++)  // _SC_OPEN_MAX：系统最大打开文件数
    {
        close(x);
    }

    // 第八步：初始化系统日志——后续用syslog输出，替代printf（守护进程无终端）
    openlog("this is our daemonize process: ", LOG_PID, LOG_DAEMON);
    // 参数1：日志前缀；参数2：LOG_PID——日志中包含进程PID；参数3：LOG_DAEMON——日志分类为"守护进程"
}

int main()
{
    my_daemonize();  // 初始化守护进程（执行上述8步）

    // 守护进程核心逻辑：循环监控TCP服务器子进程
    while (1)
    {
        pid = fork();  // 创建子进程，用于启动TCP服务器

        if (pid > 0)   // 父进程（守护进程）：监控子进程状态
        {
            syslog(LOG_INFO, "守护进程正在监听服务端进程...");  // 日志记录
            waitpid(-1, NULL, 0);  // 等待任意子进程退出（此处只有1个服务器子进程）
            // 注意：优化点——应改为waitpid(pid, NULL, 0)，避免误等其他无关子进程

            if (is_shutdown) {  // 若收到退出信号，不再重启，直接退出
                syslog(LOG_NOTICE, "子进程已被回收，即将关闭syslog连接，守护进程退出");
                closelog();      // 关闭系统日志连接
                exit(EXIT_SUCCESS);
            }

            // 若未收到退出信号，说明服务器意外崩溃，3秒后重启
            syslog(LOG_ERR, "服务端进程终止，3s后重启...");
            sleep(3);  // 避免频繁重启（防止死循环）
        }
        else if (pid == 0)  // 子进程：启动TCP服务器（替换当前进程镜像）
        {
            syslog(LOG_INFO, "子进程fork成功");
            syslog(LOG_INFO, "启动服务端进程");

            char *path = "/home/atguigu/daemon_and_multiplex/tcp_server";  // 服务器可执行文件路径
            char *argv[] = {"my_tcp_server", NULL};  // 传给服务器的命令行参数（argv[0]为进程名）
            errno = 0;  // 重置错误码，便于判断execve失败原因

            // 执行服务器程序：替换当前子进程的代码段、数据段（若成功，后续代码不执行）
            execve(path, argv, NULL);  // 参数3：环境变量（NULL表示继承父进程环境）

            // 若execve返回，说明执行失败（如路径错误、权限不足）
            char buf[1024];
            sprintf(buf, "errno: %d", errno);  // 记录错误码（如2=文件不存在，13=权限不足）
            syslog(LOG_ERR, "%s", buf);
            syslog(LOG_ERR, "服务端进程启动失败");
            exit(EXIT_FAILURE);  // 退出失败的子进程
        }
        else  // fork失败（系统资源不足）
        {
            syslog(LOG_ERR, "子进程fork失败");
        }
    }
    
    return EXIT_SUCCESS;
}