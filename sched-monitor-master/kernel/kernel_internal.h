/*
 * kernel_internal.h - Internal kernel module definitions
 * 
 * Copyright (C) 2024
 */

#ifndef _KERNEL_INTERNAL_H_
#define _KERNEL_INTERNAL_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <linux/ktime.h>
#include <linux/atomic.h>
#include <linux/tracepoint.h>
#include <linux/hashtable.h>
#include <linux/version.h>
#include <trace/events/sched.h>

#include "sched_monitor.h"

/* ========== Module Information ========== */
#define MODULE_NAME     "sched_monitor"
#define SCHED_MONITOR_VERSION  "1.0.0"
#define SCHED_MONITOR_AUTHOR   "Your Name"
#define MODULE_DESC     "Linux Kernel Performance Monitoring Platform"

/* ========== Ring Buffer Configuration ========== */
#define RING_BUFFER_ORDER    2    /* 4 pages = 16KB per CPU */
#define RING_BUFFER_SIZE     (4096 << RING_BUFFER_ORDER)

/* ========== Per-CPU Ring Buffer ========== */
struct per_cpu_buffer {
    struct sched_event *data;   /* Buffer data area */
    u32 size;                   /* Buffer size (number of events) */
    atomic_t head;              /* Write position */
    atomic_t tail;              /* Read position */
    atomic_t count;             /* Current event count */
    atomic64_t overrun;         /* Overrun counter */
    spinlock_t lock;            /* Protection for read operations */
};

/* ========== Global Module State ========== */
struct monitor_state {
    /* Configuration */
    struct monitor_config config;
    struct mutex config_lock;
    
    /* Per-CPU buffers */
    struct per_cpu_buffer __percpu *buffers;
    
    /* Statistics */
    struct sched_stats __percpu *per_cpu_stats;
    struct sched_stats global_stats;
    spinlock_t stats_lock;
    
    /* Netlink */
    struct sock *nl_sock;
    u32 nl_seq;
    
    /* Workqueue */
    struct workqueue_struct *wq;
    struct delayed_work dwork;
    
    /* Tracepoint registration */
    bool trace_registered;
    
    /* Control flags */
    atomic_t active;
    atomic_t event_seq;
    atomic_t fi_alloc_fail_once;
};

/* ========== Function Declarations ========== */
/* trace_hook.c */
int trace_hook_init(void);
void trace_hook_exit(void);

/* ring_buffer.c */
int ring_buffer_init(void);
void ring_buffer_exit(void);
int ring_buffer_write_event(struct sched_event *event, int cpu);
int ring_buffer_read_events(struct sched_event *events, int max_count, 
                            int *cpu_map, int *count_map);
void ring_buffer_reset(void);

/* netlink_comm.c */
int netlink_init(void);
void netlink_exit(void);
int netlink_send_events(struct sched_event *events, u32 count);
int netlink_send_stats(struct sched_stats *stats);
int netlink_send_ack(s32 status, const char *message);
void netlink_process_config(struct nl_config_payload *cfg);

/* async_worker.c */
int async_worker_init(void);
void async_worker_exit(void);
void async_worker_start(void);
void async_worker_stop(void);

/* config.c */
void config_init(void);
void config_set_filter_pid(u32 pid);
void config_set_threshold(u64 threshold_ns);
void config_set_interval(u32 interval_ms);
bool config_check_filter(struct sched_event *event);
bool config_check_anomaly(struct sched_event *event);

/* stats.c */
void stats_init(void);
void stats_update(struct sched_event *event, int cpu);
void stats_reset(void);
void stats_aggregate(struct sched_stats *result);

/* ========== Global State Access ========== */
extern struct monitor_state g_state;

/* ========== Debug Macros ========== */
#define MONITOR_DEBUG    1

#if MONITOR_DEBUG
#define mon_debug(fmt, ...) \
    pr_debug("[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)
#else
#define mon_debug(fmt, ...) do {} while(0)
#endif

#define mon_info(fmt, ...) \
    pr_info("[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)

#define mon_warn(fmt, ...) \
    pr_warn("[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)

#define mon_err(fmt, ...) \
    pr_err("[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)

#endif /* _KERNEL_INTERNAL_H_ */
