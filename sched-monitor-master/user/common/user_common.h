/*
 * user_common.h - 用户态工具通用定义
 */

#ifndef _USER_COMMON_H_
#define _USER_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/* 包含共享定义 */
#include "sched_monitor.h"

/* ========== 配置 ========== */

#define DEFAULT_SOCKET_PATH    "/var/run/sched_monitor.sock"
#define DEFAULT_PID_FILE       "/var/run/monitord.pid"
#define DEFAULT_LOG_FILE       "/var/log/sched_monitor.log"

/* ========== 错误码 ========== */
#define ERR_SUCCESS           0
#define ERR_SOCKET           -1
#define ERR_BIND             -2
#define ERR_CONNECT          -3
#define ERR_SEND             -4
#define ERR_RECV             -5
#define ERR_TIMEOUT          -6
#define ERR_INVALID_MSG      -7
#define ERR_NO_MEMORY        -8
#define ERR_THREAD           -9

/* ========== 工具函数 ========== */

/* 将纳秒转换为人类可读的字符串 */
static inline void ns_to_string(uint64_t ns, char *buf, size_t size)
{
    if (ns < 1000) {
        snprintf(buf, size, "%lu ns", ns);
    } else if (ns < 1000000) {
        snprintf(buf, size, "%.2f us", ns / 1000.0);
    } else if (ns < 1000000000ULL) {
        snprintf(buf, size, "%.2f ms", ns / 1000000.0);
    } else {
        snprintf(buf, size, "%.2f s", ns / 1000000000.0);
    }
}

/* 获取当前时间戳（纳秒） */
static inline uint64_t get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* 获取当前时间戳字符串 */
static inline void get_timestamp_string(char *buf, size_t size)
{
    time_t now;
    struct tm *tm_info;
    
    time(&now);
    tm_info = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* ========== 日志宏 ========== */

#define LOG_DEBUG    0
#define LOG_INFO     1
#define LOG_WARN     2
#define LOG_ERROR    3

extern int g_log_level;
extern FILE *g_log_file;

#define log_debug(fmt, ...) \
    do { \
        if (g_log_level <= LOG_DEBUG) { \
            char _ts[32]; \
            get_timestamp_string(_ts, sizeof(_ts)); \
            fprintf(g_log_file ? g_log_file : stderr, \
                    "[%s] [DEBUG] " fmt "\n", _ts, ##__VA_ARGS__); \
        } \
    } while(0)

#define log_info(fmt, ...) \
    do { \
        if (g_log_level <= LOG_INFO) { \
            char _ts[32]; \
            get_timestamp_string(_ts, sizeof(_ts)); \
            fprintf(g_log_file ? g_log_file : stdout, \
                    "[%s] [INFO] " fmt "\n", _ts, ##__VA_ARGS__); \
        } \
    } while(0)

#define log_warn(fmt, ...) \
    do { \
        if (g_log_level <= LOG_WARN) { \
            char _ts[32]; \
            get_timestamp_string(_ts, sizeof(_ts)); \
            fprintf(g_log_file ? g_log_file : stderr, \
                    "[%s] [WARN] " fmt "\n", _ts, ##__VA_ARGS__); \
        } \
    } while(0)

#define log_error(fmt, ...) \
    do { \
        if (g_log_level <= LOG_ERROR) { \
            char _ts[32]; \
            get_timestamp_string(_ts, sizeof(_ts)); \
            fprintf(g_log_file ? g_log_file : stderr, \
                    "[%s] [ERROR] " fmt "\n", _ts, ##__VA_ARGS__); \
        } \
    } while(0)

/* ========== 数据结构 ========== */

/* 进程统计信息 */
struct proc_stats {
    uint32_t pid;//进程PID
    uint32_t tgid;//线程组ID
    char comm[TASK_COMM_LEN];//进程名称
    uint64_t total_delay;//总延迟时间（纳秒）
    uint64_t max_delay;//最大延迟时间（纳秒）
    uint64_t switch_count;//切换次数
    uint64_t anomaly_count;//异常次数
    struct proc_stats *next;//下一个进程节点
};

/* 全局统计信息缓存 */
struct stats_cache {
    struct sched_stats kernel_stats;//内核统计信息
    struct proc_stats *proc_list;//进程统计信息链表头
    pthread_mutex_t lock;//互斥锁
    uint64_t last_update;//上次更新时间（纳秒）
};

/* ========== 函数声明 ========== */

/* netlink_client.c */
int netlink_client_init(void);//初始化Netlink客户端
void netlink_client_cleanup(void);//清理Netlink客户端
int netlink_client_send_config(uint32_t cmd, uint32_t value, uint64_t value64);//发送配置消息到内核
int netlink_client_recv_events(struct sched_event *events, int max_count, 
                               int timeout_ms);//接收事件消息
int netlink_client_recv_stats(struct sched_stats *stats, int timeout_ms);//接收统计信息

/* stats_processor.c */
void stats_processor_init(struct stats_cache *cache);//初始化统计处理器
void stats_processor_cleanup(struct stats_cache *cache);//清理统计处理器
void stats_processor_update(struct stats_cache *cache, 
                           struct sched_event *events, int count);//更新统计信息
void stats_processor_update_kernel_stats(struct stats_cache *cache,
                                         struct sched_stats *stats);//更新内核统计信息
struct proc_stats *stats_processor_find_proc(struct stats_cache *cache, 
                                             uint32_t pid);//查找进程统计信息
void stats_processor_get_top_n(struct stats_cache *cache, 
                               struct proc_stats *result, int n);//获取Top N进程

#endif /* _USER_COMMON_H_ */