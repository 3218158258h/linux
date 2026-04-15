/*
 * stats.c - Statistics management implementation
 * 
 * Maintains per-CPU and global statistics for scheduling events.
 * 
 * Copyright (C) 2024
 */

#include "kernel_internal.h"

/* ========== Histogram Bin Boundaries ========== */
static const u64 delay_bin_bounds[HISTOGRAM_BINS + 1] = DELAY_BIN_BOUNDARIES;

/* ========== Helper Functions ========== */

static inline int find_histogram_bin(u64 delay_ns)
{
    int i;
    
    for (i = 0; i < HISTOGRAM_BINS; i++) {
        if (delay_ns < delay_bin_bounds[i + 1])
            return i;
    }
    
    return HISTOGRAM_BINS - 1;  /* Last bin for very large delays */
}

/* ========== Statistics Operations ========== */

void stats_init(void)
{
    int cpu;
    
    mon_info("Initializing statistics");
    
    /* Allocate per-CPU statistics */
    g_state.per_cpu_stats = alloc_percpu(struct sched_stats);
    if (!g_state.per_cpu_stats) {
        mon_err("Failed to allocate per-CPU statistics");
        return;
    }
    
    /* Initialize per-CPU statistics */
    for_each_online_cpu(cpu) {
        struct sched_stats *stats;
        
        stats = per_cpu_ptr(g_state.per_cpu_stats, cpu);
        memset(stats, 0, sizeof(struct sched_stats));
        memcpy(stats->hist.bin_bounds, delay_bin_bounds, 
               sizeof(delay_bin_bounds));
    }
    
    /* Initialize global statistics */
    memset(&g_state.global_stats, 0, sizeof(struct sched_stats));
    memcpy(g_state.global_stats.hist.bin_bounds, delay_bin_bounds,
           sizeof(delay_bin_bounds));
    g_state.global_stats.start_time = ktime_get_ns();
    
    spin_lock_init(&g_state.stats_lock);
    
    mon_info("Statistics initialized");
}

void stats_update(struct sched_event *event, int cpu)
{
    struct sched_stats *stats;
    int bin;
    
    stats = this_cpu_ptr(g_state.per_cpu_stats);
    
    /* Update counters */
    stats->total_events++;
    
    /* Update delay statistics */
    if (event->delay_ns > 0) {
        stats->total_delay_ns += event->delay_ns;
        
        if (event->delay_ns > stats->max_delay_ns)
            stats->max_delay_ns = event->delay_ns;
        
        if (stats->min_delay_ns == 0 || 
            event->delay_ns < stats->min_delay_ns)
            stats->min_delay_ns = event->delay_ns;
        
        /* Update histogram */
        bin = find_histogram_bin(event->delay_ns);
        stats->hist.bins[bin]++;
    }
    
    /* Check for anomaly */
    if (config_check_anomaly(event))
        stats->anomaly_count++;
    
    stats->last_update = ktime_get_ns();
}

void stats_reset(void)
{
    int cpu;
    
    mon_info("Resetting statistics");
    
    /* Reset per-CPU statistics */
    for_each_online_cpu(cpu) {
        struct sched_stats *stats;
        
        stats = per_cpu_ptr(g_state.per_cpu_stats, cpu);
        memset(stats, 0, sizeof(struct sched_stats));
        memcpy(stats->hist.bin_bounds, delay_bin_bounds,
               sizeof(delay_bin_bounds));
    }
    
    /* Reset global statistics */
    spin_lock(&g_state.stats_lock);
    memset(&g_state.global_stats, 0, sizeof(struct sched_stats));
    memcpy(g_state.global_stats.hist.bin_bounds, delay_bin_bounds,
           sizeof(delay_bin_bounds));
    g_state.global_stats.start_time = ktime_get_ns();
    spin_unlock(&g_state.stats_lock);
}

void stats_aggregate(struct sched_stats *result)
{
    int cpu, i;
    
    if (!result)
        return;
    
    spin_lock(&g_state.stats_lock);
    
    /* Start with global stats */
    memcpy(result, &g_state.global_stats, sizeof(struct sched_stats));
    
    /* Aggregate per-CPU statistics */
    for_each_online_cpu(cpu) {
        struct sched_stats *stats;
        
        stats = per_cpu_ptr(g_state.per_cpu_stats, cpu);
        
        result->total_events += stats->total_events;
        result->dropped_events += stats->dropped_events;
        result->total_delay_ns += stats->total_delay_ns;
        result->anomaly_count += stats->anomaly_count;
        
        if (stats->max_delay_ns > result->max_delay_ns)
            result->max_delay_ns = stats->max_delay_ns;
        
        if (result->min_delay_ns == 0 || 
            stats->min_delay_ns < result->min_delay_ns)
            result->min_delay_ns = stats->min_delay_ns;
        
        /* Aggregate histogram */
        for (i = 0; i < HISTOGRAM_BINS; i++) {
            result->hist.bins[i] += stats->hist.bins[i];
        }
        
        if (stats->last_update > result->last_update)
            result->last_update = stats->last_update;
    }
    
    spin_unlock(&g_state.stats_lock);
}
