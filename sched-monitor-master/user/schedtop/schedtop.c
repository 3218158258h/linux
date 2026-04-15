/*
 * schedtop.c - 用于sched_monitor的交互式监控工具
 * 
 * Linux内核性能监控平台
 * 
 * 类top工具，用于查看调度延迟统计信息
 * 
 * Copyright (C) 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>

#include "user_common.h"

/* ========== 全局变量 ========== */
int g_log_level = LOG_WARN;
FILE *g_log_file = NULL;

static volatile int g_running = 1;
static struct stats_cache g_stats;
static int g_refresh_interval = 1;  /* 秒 */
static int g_show_histogram = 0;
static int g_top_n = 10;
static uint32_t g_filter_pid = 0;

/* 终端处理相关 */
static struct termios g_orig_termios;
static int g_term_modified = 0;

/* ========== 信号处理 ========== */

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* ========== 终端操作函数 ========== */

static void terminal_setup(void)
{
    struct termios new_termios;
    
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    
    new_termios = g_orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    g_term_modified = 1;
}

static void terminal_restore(void)
{
    if (g_term_modified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
        g_term_modified = 0;
    }
}

static void clear_screen(void)
{
    printf("\033[2J\033[H");
}

static void set_color(int color)
{
    printf("\033[%dm", color);
}

/* 颜色定义 */
#define COLOR_RESET     0
#define COLOR_RED       31
#define COLOR_GREEN     32
#define COLOR_YELLOW    33
#define COLOR_BLUE      34
#define COLOR_MAGENTA   35
#define COLOR_CYAN      36
#define COLOR_WHITE     37

/* ========== 显示函数 ========== */
// 打印标题栏
static void print_header(void)
{
    time_t now;
    struct tm *tm_info;
    char time_str[32];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    set_color(COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                     Linux Kernel Schedule Monitor v1.0                       ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║  Time: %-20s  Refresh: %ds  PID Filter: %-8u              ║\n",
           time_str, g_refresh_interval, g_filter_pid);
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    set_color(COLOR_RESET);
}

// 打印内核统计信息
static void print_kernel_stats(struct sched_stats *stats)
{
    char max_delay[32], avg_delay[32];
    uint64_t avg_ns = 0;
    
    if (stats->total_events > 0) {
        avg_ns = stats->total_delay_ns / stats->total_events;
    }
    
    ns_to_string(stats->max_delay_ns, max_delay, sizeof(max_delay));
    ns_to_string(avg_ns, avg_delay, sizeof(avg_delay));
    
    set_color(COLOR_GREEN);
    printf("\n┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│                           内核统计信息                                       │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│  总事件数: %-12llu  丢失事件: %-10llu  异常事件: %-10llu    │\n",
           (unsigned long long)stats->total_events,
           (unsigned long long)stats->dropped_events,
           (unsigned long long)stats->anomaly_count);
    printf("│  最大延迟: %-16s  平均延迟: %-14s                        │\n",
           max_delay, avg_delay);
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    set_color(COLOR_RESET);
}

// 打印进程统计信息
static void print_top_processes(struct stats_cache *cache)
{
    struct proc_stats *top;
    char delay_str[32];
    int i;
    
    top = calloc(g_top_n, sizeof(struct proc_stats));
    if (!top) {
        return;
    }
    
    stats_processor_get_top_n(cache, top, g_top_n);
    
    set_color(COLOR_YELLOW);
    printf("\n┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│   按总延迟排序的前%d个进程                                                  │\n", g_top_n);
    printf("├───────┬──────────────────┬─────────────┬─────────────┬──────────┬─────────┤\n");
    printf("│  PID  │       进程名     │  总延迟     │  最大延迟   │ 切换次数 │ 异常数  │\n");
    printf("├───────┼──────────────────┼─────────────┼─────────────┼──────────┼─────────┤\n");
    set_color(COLOR_RESET);
    
    for (i = 0; i < g_top_n; i++) {
        if (top[i].pid == 0)
            break;
            
        ns_to_string(top[i].total_delay, delay_str, sizeof(delay_str));
        
        /* 根据异常数量设置显示颜色 */
        if (top[i].anomaly_count > 100) {
            set_color(COLOR_RED);
        } else if (top[i].anomaly_count > 10) {
            set_color(COLOR_YELLOW);
        } else {
            set_color(COLOR_GREEN);
        }
        
        printf("│ %5u │ %-16s │ %11s │ %11lu │ %8lu │ %7lu │\n",
               top[i].pid, top[i].comm, delay_str, top[i].max_delay,
               top[i].switch_count, top[i].anomaly_count);
    }
    
    set_color(COLOR_YELLOW);
    printf("└───────┴──────────────────┴─────────────┴─────────────┴──────────┴─────────┘\n");
    set_color(COLOR_RESET);
    
    free(top);
}

// 打印延迟分布直方图
static void print_histogram_display(struct delay_histogram *hist)
{
    int i;
    uint64_t max_count = 0;
    int bar_width = 50;
    int bar_len;
    
    /* 查找最大计数值用于缩放 */
    for (i = 0; i < HISTOGRAM_BINS; i++) {
        if (hist->bins[i] > max_count)
            max_count = hist->bins[i];
    }
    
    if (max_count == 0) {
        return;
    }
    
    set_color(COLOR_MAGENTA);
    printf("\n┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│                           延迟分布统计                                       │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    set_color(COLOR_RESET);
    
    for (i = 0; i < HISTOGRAM_BINS; i++) {
        char lower[16], upper[16];
        
        if (hist->bins[i] == 0)
            continue;
            
        ns_to_string(hist->bin_bounds[i], lower, sizeof(lower));
        
        bar_len = (int)((double)hist->bins[i] / max_count * bar_width);
        
        printf("  %8s - ", lower);
        if (i < HISTOGRAM_BINS - 1 && hist->bin_bounds[i + 1] > 0) {
            ns_to_string(hist->bin_bounds[i + 1], upper, sizeof(upper));
            printf("%8s: ", upper);
        } else {
            printf("      inf: ");
        }
        
        set_color(COLOR_CYAN);
        for (int j = 0; j < bar_len; j++) {
            printf("█");
        }
        set_color(COLOR_RESET);
        
        printf(" %llu\n", (unsigned long long)hist->bins[i]);
    }
    
    set_color(COLOR_MAGENTA);
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    set_color(COLOR_RESET);
}

// 打印帮助信息
static void print_help(void)
{
    set_color(COLOR_WHITE);
    printf("\n┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│                              操作命令                                       │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│  q - 退出程序        h - 切换直方图显示                                     │\n");
    printf("│  p - 设置PID过滤     r - 设置刷新间隔                                       │\n");
    printf("│  c - 清空统计信息     s - 发送配置到内核                                     │\n");
    printf("│  ? - 显示此帮助信息                                                       │\n");
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    set_color(COLOR_RESET);
}

/* ========== 事件处理 ========== */
// 处理内核事件
static void process_events(void)
{
    struct sched_event events[MAX_EVENTS_PER_MSG];
    int ret;
    
    ret = netlink_client_recv_events(events, MAX_EVENTS_PER_MSG, 100);
    if (ret > 0) {
        stats_processor_update(&g_stats, events, ret);
    }
}

/* ========== 主循环 ========== */
// 程序主循环
static void main_loop(void)
{
    char input;
    fd_set fds;
    struct timeval tv;
    int ret;
    
    while (g_running) {
        clear_screen();
        print_header();
        print_kernel_stats(&g_stats.kernel_stats);
        print_top_processes(&g_stats);
        
        if (g_show_histogram) {
            print_histogram_display(&g_stats.kernel_stats.hist);
        }
        
        print_help();
        
        /* 检测用户输入 */
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        tv.tv_sec = g_refresh_interval;
        tv.tv_usec = 0;
        
        ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
            if (read(STDIN_FILENO, &input, 1) > 0) {
                switch (input) {
                case 'q':
                case 'Q':
                    g_running = 0;
                    break;
                case 'h':
                case 'H':
                    g_show_histogram = !g_show_histogram;
                    break;
                case 'p':
                case 'P':
                    printf("输入PID过滤值(0表示不过滤): ");
                    fflush(stdout);
                    terminal_restore();
                    scanf("%u", &g_filter_pid);
                    terminal_setup();
                    netlink_client_send_config(SCHED_CFG_SET_FILTER_PID,
                                               g_filter_pid, 0);
                    break;
                case 'r':
                case 'R':
                    printf("输入刷新间隔(秒): ");
                    fflush(stdout);
                    terminal_restore();
                    scanf("%d", &g_refresh_interval);
                    terminal_setup();
                    break;
                case 'c':
                case 'C':
                    netlink_client_send_config(SCHED_CFG_CLEAR_STATS, 0, 0);
                    stats_processor_cleanup(&g_stats);
                    stats_processor_init(&g_stats);
                    break;
                case 's':
                case 'S':
                    /* 发送当前配置 */
                    netlink_client_send_config(SCHED_CFG_ENABLE, 1, 0);
                    break;
                case '?':
                    /* 帮助信息已显示 */
                    sleep(2);
                    break;
                }
            }
        }
        
        /* 处理所有待处理的事件 */
        process_events();
    }
}

/* ========== 命令行参数解析 ========== */
// 打印使用说明
static void print_usage(const char *prog)
{
    printf("用法: %s [选项]\n\n", prog);
    printf("选项:\n");
    printf("  -i, --interval N    刷新间隔(秒) (默认: 1)\n");
    printf("  -n, --top N         显示前N个进程 (默认: 10)\n");
    printf("  -p, --pid PID       按PID过滤\n");
    printf("  -H, --histogram     启动时显示直方图\n");
    printf("  -h, --help          显示帮助信息\n");
}
// 主函数
int main(int argc, char *argv[])
{
    int opt;
    int ret;
    
    static struct option long_options[] = {
        {"interval", required_argument, 0, 'i'},
        {"top",      required_argument, 0, 'n'},
        {"pid",      required_argument, 0, 'p'},
        {"histogram",no_argument,       0, 'H'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    /* 解析命令行参数 */
    while ((opt = getopt_long(argc, argv, "i:n:p:Hh", long_options, NULL)) != -1) {
        switch (opt) {
        case 'i':
            g_refresh_interval = atoi(optarg);
            break;
        case 'n':
            g_top_n = atoi(optarg);
            break;
        case 'p':
            g_filter_pid = atoi(optarg);
            break;
        case 'H':
            g_show_histogram = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }
    
    /* 设置信号处理函数 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 初始化统计处理器 */
    stats_processor_init(&g_stats);
    
    /* 初始化Netlink客户端 */
    ret = netlink_client_init();
    if (ret != ERR_SUCCESS) {
        fprintf(stderr, "Netlink客户端初始化失败\n");
        fprintf(stderr, "请确保sched_monitor内核模块已加载\n");
        return 1;
    }
    
    /* 如果指定了PID，设置初始PID过滤 */
    if (g_filter_pid > 0) {
        netlink_client_send_config(SCHED_CFG_SET_FILTER_PID, g_filter_pid, 0);
    }
    
    /* 配置终端 */
    terminal_setup();
    atexit(terminal_restore);
    
    /* 运行主循环 */
    main_loop();
    
    /* 清理资源 */
    terminal_restore();
    netlink_client_cleanup();
    stats_processor_cleanup(&g_stats);
    
    printf("\n再见！\n");
    
    return 0;
}