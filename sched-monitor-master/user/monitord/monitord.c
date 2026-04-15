/*
 * monitord.c - sched_monitor的监控守护进程
 * 
 * Linux内核性能监控平台
 * 
 * 该守护进程从内核模块接收事件，
 * 并为客户端工具维护统计信息。
 * 
 * Copyright (C) 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>

#include "user_common.h"

/* ========== 全局变量 ========== */
int g_log_level = LOG_INFO;
FILE *g_log_file = NULL;

static volatile int g_running = 1;
static struct stats_cache g_stats;
static char *g_log_file_path = NULL;
static char *g_pid_file_path = NULL;
static int g_daemonize = 0;

/* ========== 信号处理 ========== */

static void signal_handler(int sig)
{
    switch (sig) {
    case SIGINT:
    case SIGTERM:
        log_info("收到信号 %d，正在关闭...", sig);
        g_running = 0;
        break;
    case SIGHUP:
        log_info("收到 SIGHUP 信号，重新打开日志文件...");
        if (g_log_file && g_log_file_path) {
            freopen(g_log_file_path, "a", g_log_file);
        }
        break;
    }
}

static void setup_signals(void)
{
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    
    /* 忽略 SIGPIPE 信号 */
    signal(SIGPIPE, SIG_IGN);
}

/* ========== 守护进程函数 ========== */

static int write_pid_file(const char *path)
{
    FILE *fp;
    
    fp = fopen(path, "w");
    if (!fp) {
        log_error("创建PID文件失败: %s", path);
        return -1;
    }
    
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
    
    return 0;
}

static void daemonize(void)
{
    pid_t pid;
    
    /* 第一次 fork */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid > 0) {
        exit(0);  /* 父进程退出 */
    }
    
    /* 创建新会话 */
    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }
    
    /* 第二次 fork */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid > 0) {
        exit(0);
    }
    
    /* 切换工作目录 */
    chdir("/");
    
    /* 重置文件权限掩码 */
    umask(0);
    
    /* 关闭标准文件描述符 */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    /* 重定向到 /dev/null */
    open("/dev/null", O_RDONLY);  /* 标准输入 */
    open("/dev/null", O_WRONLY);  /* 标准输出 */
    open("/dev/null", O_WRONLY);  /* 标准错误 */
}

/* ========== 事件处理线程 ========== */

static void *event_thread(void *arg)
{
    (void)arg;
    struct sched_event events[MAX_EVENTS_PER_MSG];
    int ret;
    
    log_info("事件处理线程已启动");
    
    while (g_running) {
        ret = netlink_client_recv_events(events, MAX_EVENTS_PER_MSG, 1000);
        
        if (ret == ERR_TIMEOUT) {
            continue;  /* 正常超时 */
        } else if (ret < 0) {
            log_warn("接收事件出错: %d", ret);
            usleep(100000);  /* 重试前等待 */
            continue;
        }
        
        if (ret > 0) {
            stats_processor_update(&g_stats, events, ret);
            log_debug("已处理 %d 个事件", ret);
        }
    }
    
    log_info("事件处理线程已停止");
    return NULL;
}

/* ========== 统计信息查询线程 ========== */

static void *stats_thread(void *arg)
{
    (void)arg;
    struct sched_stats stats;
    int ret;
    
    log_info("统计信息线程已启动");
    
    while (g_running) {
        /* 定期向内核请求统计数据 */
        sleep(5);
        
        if (!g_running)
            break;
        
        ret = netlink_client_recv_stats(&stats, 1000);
        if (ret == ERR_SUCCESS) {
            stats_processor_update_kernel_stats(&g_stats, &stats);
            log_debug("已更新内核统计信息");
        }
    }
    
    log_info("统计信息线程已停止");
    return NULL;
}

/* ========== 主函数 ========== */

static void print_usage(const char *prog)
{
    printf("用法: %s [选项]\n\n", prog);
    printf("选项:\n");
    printf("  -d, --daemon       以守护进程方式运行\n");
    printf("  -l, --log FILE     日志文件路径\n");
    printf("  -p, --pid FILE     PID文件路径\n");
    printf("  -v, --verbose      增加日志详细程度\n");
    printf("  -q, --quiet        减少日志详细程度\n");
    printf("  -h, --help         显示此帮助信息\n");
}

int main(int argc, char *argv[])
{
    int opt;
    pthread_t event_tid, stats_tid;
    int ret;
    
    static struct option long_options[] = {
        {"daemon",  no_argument,       0, 'd'},
        {"log",     required_argument, 0, 'l'},
        {"pid",     required_argument, 0, 'p'},
        {"verbose", no_argument,       0, 'v'},
        {"quiet",   no_argument,       0, 'q'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    /* 解析命令行参数 */
    while ((opt = getopt_long(argc, argv, "dl:p:vqh", long_options, NULL)) != -1) {
        switch (opt) {
        case 'd':
            g_daemonize = 1;
            break;
        case 'l':
            g_log_file_path = optarg;
            break;
        case 'p':
            g_pid_file_path = optarg;
            break;
        case 'v':
            g_log_level--;
            break;
        case 'q':
            g_log_level++;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }
    
    /* 打开日志文件 */
    if (g_log_file_path) {
        g_log_file = fopen(g_log_file_path, "a");
        if (!g_log_file) {
            fprintf(stderr, "打开日志文件失败: %s\n", g_log_file_path);
            return 1;
        }
    }
    
    /* 如果需要，启动守护进程 */
    if (g_daemonize) {
        daemonize();
    }
    
    /* 写入PID文件 */
    if (g_pid_file_path) {
        write_pid_file(g_pid_file_path);
    }
    
    /* 设置信号处理函数 */
    setup_signals();
    
    log_info("========================================");
    log_info("sched_monitor 守护进程正在启动...");
    log_info("========================================");
    
    /* 初始化统计处理器 */
    stats_processor_init(&g_stats);
    
    /* 初始化Netlink客户端 */
    ret = netlink_client_init();
    if (ret != ERR_SUCCESS) {
        log_error("Netlink客户端初始化失败");
        return 1;
    }
    
    /* 创建工作线程 */
    ret = pthread_create(&event_tid, NULL, event_thread, NULL);
    if (ret != 0) {
        log_error("创建事件线程失败");
        return 1;
    }
    
    ret = pthread_create(&stats_tid, NULL, stats_thread, NULL);
    if (ret != 0) {
        log_error("创建统计线程失败");
        g_running = 0;
        pthread_join(event_tid, NULL);
        return 1;
    }
    
    /* 主循环 */
    while (g_running) {
        sleep(1);
    }
    
    /* 清理资源 */
    log_info("正在关闭...");
    
    pthread_join(event_tid, NULL);
    pthread_join(stats_tid, NULL);
    
    netlink_client_cleanup();
    stats_processor_cleanup(&g_stats);
    
    if (g_pid_file_path) {
        unlink(g_pid_file_path);
    }
    
    log_info("守护进程已停止");
    
    if (g_log_file) {
        fclose(g_log_file);
    }
    
    return 0;
}