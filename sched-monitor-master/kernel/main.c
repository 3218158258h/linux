/*
 * main.c - Kernel module main entry point
 * 
 * Linux Kernel Performance Monitoring Platform
 * 
 * Copyright (C) 2024
 */

#include "kernel_internal.h"

/* ========== Global Module State ========== */
struct monitor_state g_state;

/* ========== Module Parameters ========== */
static unsigned int buffer_size = DEFAULT_BUFFER_SIZE;
module_param(buffer_size, uint, 0644);
MODULE_PARM_DESC(buffer_size, "Per-CPU buffer size (number of events)");

static unsigned int interval_ms = DEFAULT_INTERVAL_MS;
module_param(interval_ms, uint, 0644);
MODULE_PARM_DESC(interval_ms, "Workqueue execution interval (milliseconds)");

static unsigned long threshold_ns = DEFAULT_THRESHOLD_NS;
module_param(threshold_ns, ulong, 0644);
MODULE_PARM_DESC(threshold_ns, "Anomaly detection threshold (nanoseconds)");

static unsigned int filter_pid = 0;
module_param(filter_pid, uint, 0644);
MODULE_PARM_DESC(filter_pid, "PID filter (0 = no filter)");

static bool enabled = true;
module_param(enabled, bool, 0644);
MODULE_PARM_DESC(enabled, "Enable monitoring on load");

static bool fi_alloc_fail_once = false;
module_param(fi_alloc_fail_once, bool, 0644);
MODULE_PARM_DESC(fi_alloc_fail_once, "Fault injection: fail one buffer allocation");

/* ========== Initialization Functions ========== */
static void state_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    
    /* Initialize configuration */
    g_state.config.enabled = enabled ? 1 : 0;
    g_state.config.filter_pid = filter_pid;
    g_state.config.threshold_ns = threshold_ns;
    g_state.config.interval_ms = interval_ms;
    g_state.config.buffer_size = buffer_size;
    g_state.config.flags = SCHED_FLAG_ANOMALY_DETECT;
    
    mutex_init(&g_state.config_lock);
    spin_lock_init(&g_state.stats_lock);
    atomic_set(&g_state.active, 0);
    atomic_set(&g_state.event_seq, 0);
    atomic_set(&g_state.fi_alloc_fail_once, fi_alloc_fail_once ? 1 : 0);
    g_state.nl_seq = 0;
    g_state.trace_registered = false;
}

static int __init sched_monitor_init(void)
{
    int ret;
    
    mon_info("Loading %s version %s", MODULE_NAME, SCHED_MONITOR_VERSION);
    mon_info("Parameters: buffer_size=%u, interval_ms=%u, threshold_ns=%lu, filter_pid=%u",
             buffer_size, interval_ms, threshold_ns, filter_pid);
    
    /* Initialize global state */
    state_init();
    
    /* Initialize statistics */
    stats_init();
    
    /* Initialize ring buffers */
    ret = ring_buffer_init();
    if (ret) {
        mon_err("Failed to initialize ring buffers: %d", ret);
        goto err_buffer;
    }
    
    /* Initialize Netlink communication */
    ret = netlink_init();
    if (ret) {
        mon_err("Failed to initialize Netlink: %d", ret);
        goto err_netlink;
    }
    
    /* Initialize async worker */
    ret = async_worker_init();
    if (ret) {
        mon_err("Failed to initialize async worker: %d", ret);
        goto err_worker;
    }
    
    /* Initialize tracepoint hooks */
    ret = trace_hook_init();
    if (ret) {
        mon_err("Failed to initialize trace hooks: %d", ret);
        goto err_trace;
    }
    
    /* Start monitoring if enabled */
    if (g_state.config.enabled) {
        atomic_set(&g_state.active, 1);
        async_worker_start();
        mon_info("Monitoring started");
    }
    
    mon_info("Module loaded successfully");
    return 0;
    
err_trace:
    async_worker_exit();
err_worker:
    netlink_exit();
err_netlink:
    ring_buffer_exit();
err_buffer:
    return ret;
}

static void __exit sched_monitor_exit(void)
{
    mon_info("Unloading %s", MODULE_NAME);
    
    /* Stop monitoring */
    atomic_set(&g_state.active, 0);
    
    /* Stop async worker first */
    async_worker_stop();
    async_worker_exit();
    
    /* Unregister trace hooks */
    trace_hook_exit();
    
    /* Cleanup Netlink */
    netlink_exit();
    
    /* Cleanup ring buffers */
    ring_buffer_exit();
    
    /* Print final statistics */
    mon_info("Final statistics:");
    mon_info("  Total events: %llu", g_state.global_stats.total_events);
    mon_info("  Dropped events: %llu", g_state.global_stats.dropped_events);
    mon_info("  Anomaly count: %llu", g_state.global_stats.anomaly_count);
    mon_info("  Max delay: %llu ns", g_state.global_stats.max_delay_ns);
    
    mon_info("Module unloaded successfully");
}

module_init(sched_monitor_init);
module_exit(sched_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(SCHED_MONITOR_AUTHOR);
MODULE_DESCRIPTION(MODULE_DESC);
MODULE_VERSION(SCHED_MONITOR_VERSION);
