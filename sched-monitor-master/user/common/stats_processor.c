/*
 * stats_processor.c - 用户态统计信息处理模块
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "user_common.h"

/* ========== 初始化与清理 ========== */

// 初始化统计缓存
void stats_processor_init(struct stats_cache *cache)
{
    if (!cache)
        return;
    
    memset(cache, 0, sizeof(struct stats_cache));
    pthread_mutex_init(&cache->lock, NULL);
    
    log_debug("统计处理器初始化完成");
}

// 清理统计缓存资源
void stats_processor_cleanup(struct stats_cache *cache)
{
    struct proc_stats *proc, *next;
    
    if (!cache)
        return;
    
    pthread_mutex_lock(&cache->lock);
    
    /* 释放进程统计信息链表 */
    proc = cache->proc_list;
    while (proc) {
        next = proc->next;
        free(proc);
        proc = next;
    }
    
    pthread_mutex_unlock(&cache->lock);
    pthread_mutex_destroy(&cache->lock);
    
    log_debug("统计处理器资源清理完成");
}

/* ========== 进程统计信息管理 ========== */

// 查找或创建进程统计信息
static struct proc_stats *find_or_create_proc(struct stats_cache *cache,
                                              uint32_t pid, uint32_t tgid,
                                              const char *comm)
{
    struct proc_stats *proc;
    
    /* 查找已存在的记录 */
    for (proc = cache->proc_list; proc; proc = proc->next) {
        if (proc->pid == pid) {
            return proc;
        }
    }
    
    /* 创建新记录 */
    proc = calloc(1, sizeof(struct proc_stats));
    if (!proc) {
        log_error("分配进程统计信息内存失败");
        return NULL;
    }
    
    proc->pid = pid;
    proc->tgid = tgid;
    strncpy(proc->comm, comm, TASK_COMM_LEN - 1);
    proc->next = cache->proc_list;
    cache->proc_list = proc;
    
    return proc;
}

// 更新统计信息（根据调度事件）
void stats_processor_update(struct stats_cache *cache,
                           struct sched_event *events, int count)
{
    int i;
    struct proc_stats *proc;
    
    if (!cache || !events || count <= 0)
        return;
    
    pthread_mutex_lock(&cache->lock);
    
    for (i = 0; i < count; i++) {
        struct sched_event *ev = &events[i];
        
        /* 更新前一个进程的统计信息 */
        proc = find_or_create_proc(cache, ev->prev_pid, ev->prev_tgid,
                                   ev->prev_comm);
        if (proc) {
            proc->total_delay += ev->delay_ns;
            if (ev->delay_ns > proc->max_delay)
                proc->max_delay = ev->delay_ns;
            proc->switch_count++;
            
            /* 检查是否为异常延迟 */
            if (ev->delay_ns >= 100000000ULL) {  /* 100毫秒阈值 */
                proc->anomaly_count++;
            }
        }
        
        /* 更新下一个进程的统计信息（仅切换次数） */
        proc = find_or_create_proc(cache, ev->next_pid, ev->next_tgid,
                                   ev->next_comm);
        if (proc) {
            proc->switch_count++;
        }
    }
    
    cache->last_update = get_time_ns();
    
    pthread_mutex_unlock(&cache->lock);
}

// 更新内核全局统计信息
void stats_processor_update_kernel_stats(struct stats_cache *cache,
                                         struct sched_stats *stats)
{
    if (!cache || !stats)
        return;
    
    pthread_mutex_lock(&cache->lock);
    memcpy(&cache->kernel_stats, stats, sizeof(struct sched_stats));
    cache->last_update = get_time_ns();
    pthread_mutex_unlock(&cache->lock);
}

// 根据PID查找进程统计信息
struct proc_stats *stats_processor_find_proc(struct stats_cache *cache,
                                             uint32_t pid)
{
    struct proc_stats *proc;
    
    if (!cache)
        return NULL;
    
    pthread_mutex_lock(&cache->lock);
    
    for (proc = cache->proc_list; proc; proc = proc->next) {
        if (proc->pid == pid) {
            pthread_mutex_unlock(&cache->lock);
            return proc;
        }
    }
    
    pthread_mutex_unlock(&cache->lock);
    return NULL;
}

/* ========== Top N 进程处理 ========== */

// 进程统计信息比较函数（按总延迟降序）
static int compare_proc_stats(const void *a, const void *b)
{
    const struct proc_stats *pa = *(const struct proc_stats **)a;
    const struct proc_stats *pb = *(const struct proc_stats **)b;
    
    /* 按总延迟排序（降序） */
    if (pa->total_delay > pb->total_delay)
        return -1;
    if (pa->total_delay < pb->total_delay)
        return 1;
    return 0;
}

// 获取延迟最高的N个进程
void stats_processor_get_top_n(struct stats_cache *cache,
                               struct proc_stats *result, int n)
{
    struct proc_stats **array;
    struct proc_stats *proc;
    int count = 0;
    int i;
    
    if (!cache || !result || n <= 0)
        return;
    
    pthread_mutex_lock(&cache->lock);
    
    /* 统计进程数量 */
    for (proc = cache->proc_list; proc; proc = proc->next)
        count++;
    
    if (count == 0) {
        pthread_mutex_unlock(&cache->lock);
        return;
    }
    
    /* 分配排序数组 */
    array = malloc(count * sizeof(struct proc_stats *));
    if (!array) {
        pthread_mutex_unlock(&cache->lock);
        return;
    }
    
    /* 填充数组 */
    i = 0;
    for (proc = cache->proc_list; proc; proc = proc->next)
        array[i++] = proc;
    
    /* 按总延迟排序 */
    qsort(array, count, sizeof(struct proc_stats *), compare_proc_stats);
    
    /* 复制前N个结果 */
    memset(result, 0, n * sizeof(struct proc_stats));
    for (i = 0; i < n && i < count; i++) {
        memcpy(&result[i], array[i], sizeof(struct proc_stats));
    }
    
    free(array);
    pthread_mutex_unlock(&cache->lock);
}

/* ========== 直方图打印 ========== */

// 打印延迟分布直方图
void print_histogram(struct delay_histogram *hist)
{
    int i;
    char lower[32], upper[32];
    
    printf("\n延迟分布直方图:\n");
    printf("================================\n");
    
    for (i = 0; i < HISTOGRAM_BINS; i++) {
        if (hist->bins[i] == 0)
            continue;
            
        ns_to_string(hist->bin_bounds[i], lower, sizeof(lower));
        if (i < HISTOGRAM_BINS - 1) {
            ns_to_string(hist->bin_bounds[i + 1], upper, sizeof(upper));
            printf("  [%10s - %10s]: %10llu\n", lower, upper,
                   (unsigned long long)hist->bins[i]);
        } else {
            printf("  [%10s -       inf]: %10llu\n", lower,
                   (unsigned long long)hist->bins[i]);
        }
    }
}