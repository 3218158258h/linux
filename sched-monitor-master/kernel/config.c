/*
 * config.c - Configuration management implementation
 * 
 * Handles runtime configuration and filtering logic.
 * 
 * Copyright (C) 2024
 */

#include "kernel_internal.h"

/* ========== Configuration Access ========== */

void config_init(void)
{
    mutex_init(&g_state.config_lock);
    
    /* Default zero-initialized fields while preserving any pre-set values */
    if (!g_state.config.threshold_ns)
        g_state.config.threshold_ns = DEFAULT_THRESHOLD_NS;
    if (!g_state.config.interval_ms)
        g_state.config.interval_ms = DEFAULT_INTERVAL_MS;
    if (!g_state.config.buffer_size)
        g_state.config.buffer_size = DEFAULT_BUFFER_SIZE;
    g_state.config.flags |= SCHED_FLAG_ANOMALY_DETECT;
    if (g_state.config.filter_pid > 0)
        g_state.config.flags |= SCHED_FLAG_FILTER_PID;
    else
        g_state.config.flags &= ~SCHED_FLAG_FILTER_PID;
    
    mon_info("Configuration initialized: enabled=%u, filter_pid=%u, "
             "threshold=%llu ns, interval=%u ms",
             g_state.config.enabled, g_state.config.filter_pid,
             g_state.config.threshold_ns, g_state.config.interval_ms);
}

void config_set_filter_pid(u32 pid)
{
    mutex_lock(&g_state.config_lock);
    g_state.config.filter_pid = pid;
    if (pid > 0)
        g_state.config.flags |= SCHED_FLAG_FILTER_PID;
    else
        g_state.config.flags &= ~SCHED_FLAG_FILTER_PID;
    mutex_unlock(&g_state.config_lock);
    
    mon_info("Filter PID set to: %u", pid);
}

void config_set_threshold(u64 threshold_ns)
{
    mutex_lock(&g_state.config_lock);
    g_state.config.threshold_ns = threshold_ns;
    mutex_unlock(&g_state.config_lock);
    
    mon_info("Anomaly threshold set to: %llu ns", threshold_ns);
}

void config_set_interval(u32 interval_ms)
{
    mutex_lock(&g_state.config_lock);
    g_state.config.interval_ms = interval_ms;
    mutex_unlock(&g_state.config_lock);
    
    mon_info("Workqueue interval set to: %u ms", interval_ms);
}

/* ========== Filter Functions ========== */

/*
 * config_check_filter - Check if event passes the filter
 * 
 * Returns true if the event should be processed,
 * false if it should be discarded.
 */
bool config_check_filter(struct sched_event *event)
{
    /* Check PID filter */
    if (g_state.config.flags & SCHED_FLAG_FILTER_PID) {
        if (event->prev_pid != g_state.config.filter_pid &&
            event->next_pid != g_state.config.filter_pid) {
            return false;
        }
    }
    
    return true;
}

/*
 * config_check_anomaly - Check if event represents an anomaly
 * 
 * Returns true if the event's delay exceeds the threshold.
 */
bool config_check_anomaly(struct sched_event *event)
{
    if (!(g_state.config.flags & SCHED_FLAG_ANOMALY_DETECT))
        return false;
    
    return event->delay_ns >= g_state.config.threshold_ns;
}
