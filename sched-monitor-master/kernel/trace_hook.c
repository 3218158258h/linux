/*
 * trace_hook.c - Tracepoint hook implementation
 * 
 * This module registers probe functions for scheduler tracepoints
 * to capture scheduling events.
 * 
 * Copyright (C) 2024
 */

#include "kernel_internal.h"

/* ========== Helper Functions ========== */
struct wakeup_entry {
    u32 pid;
    u64 ts_ns;
    struct hlist_node node;
};

#define WAKEUP_HASH_BITS 10
static DEFINE_HASHTABLE(wakeup_ts_table, WAKEUP_HASH_BITS);
static DEFINE_SPINLOCK(wakeup_ts_lock);
static struct tracepoint *tp_sched_switch;
static struct tracepoint *tp_sched_wakeup;

static inline u64 get_timestamp_ns(void)
{
    return ktime_get_ns();
}

static void find_tracepoint_cb(struct tracepoint *tp, void *priv)
{
    if (!tp || !tp->name)
        return;

    if (!strcmp(tp->name, "sched_switch"))
        tp_sched_switch = tp;
    else if (!strcmp(tp->name, "sched_wakeup"))
        tp_sched_wakeup = tp;
}

static void record_wakeup_ts(u32 pid, u64 ts_ns)
{
    struct wakeup_entry *entry;
    struct wakeup_entry *new_entry = NULL;
    unsigned long flags;

    spin_lock_irqsave(&wakeup_ts_lock, flags);
    hash_for_each_possible(wakeup_ts_table, entry, node, pid) {
        if (entry->pid == pid) {
            entry->ts_ns = ts_ns;
            spin_unlock_irqrestore(&wakeup_ts_lock, flags);
            return;
        }
    }
    spin_unlock_irqrestore(&wakeup_ts_lock, flags);

    new_entry = kmalloc(sizeof(*new_entry), GFP_ATOMIC);
    if (!new_entry) {
        mon_warn("failed to allocate wakeup entry for pid=%u", pid);
        return;
    }

    spin_lock_irqsave(&wakeup_ts_lock, flags);
    hash_for_each_possible(wakeup_ts_table, entry, node, pid) {
        if (entry->pid == pid) {
            entry->ts_ns = ts_ns;
            spin_unlock_irqrestore(&wakeup_ts_lock, flags);
            kfree(new_entry);
            return;
        }
    }

    new_entry->pid = pid;
    new_entry->ts_ns = ts_ns;
    hash_add(wakeup_ts_table, &new_entry->node, pid);
    spin_unlock_irqrestore(&wakeup_ts_lock, flags);
}

static u64 consume_wakeup_ts(u32 pid, u64 now_ns)
{
    struct wakeup_entry *entry;
    struct wakeup_entry *to_free = NULL;
    u64 delay_ns = 0;
    unsigned long flags;

    spin_lock_irqsave(&wakeup_ts_lock, flags);
    hash_for_each_possible(wakeup_ts_table, entry, node, pid) {
        if (entry->pid == pid) {
            if (now_ns >= entry->ts_ns) {
                delay_ns = now_ns - entry->ts_ns;
            } else {
                mon_debug("wakeup timestamp anomaly pid=%u now=%llu wake=%llu",
                          pid, now_ns, entry->ts_ns);
            }
            hash_del(&entry->node);
            to_free = entry;
            break;
        }
    }
    spin_unlock_irqrestore(&wakeup_ts_lock, flags);

    kfree(to_free);
    return delay_ns;
}

static void clear_wakeup_table(void)
{
    int bkt;
    struct wakeup_entry *entry;
    struct hlist_node *tmp;
    unsigned long flags;

    spin_lock_irqsave(&wakeup_ts_lock, flags);
    hash_for_each_safe(wakeup_ts_table, bkt, tmp, entry, node) {
        hash_del(&entry->node);
        kfree(entry);
    }
    spin_unlock_irqrestore(&wakeup_ts_lock, flags);
}

static inline void fill_event_basic(struct sched_event *event,
                                    struct task_struct *prev,
                                    struct task_struct *next,
                                    int cpu,
                                    unsigned int prev_state)
{
    event->timestamp = get_timestamp_ns();
    event->cpu = cpu;
    
    /* Previous task info */
    event->prev_pid = prev->pid;
    event->prev_tgid = prev->tgid;
    event->prev_prio = prev->prio;
    event->prev_state = prev_state;
    memcpy(event->prev_comm, prev->comm, TASK_COMM_LEN);
    
    /* Next task info */
    event->next_pid = next->pid;
    event->next_tgid = next->tgid;
    event->next_prio = next->prio;
    memcpy(event->next_comm, next->comm, TASK_COMM_LEN);
}

static inline void fill_delay_from_wakeup(struct sched_event *event,
                                          struct task_struct *prev,
                                          struct task_struct *next)
{
    u64 now_ns = event->timestamp;

    /* Delay = wakeup timestamp -> scheduled-in timestamp */
    event->delay_ns = consume_wakeup_ts(next->pid, now_ns);
    event->runtime_ns = READ_ONCE(prev->se.sum_exec_runtime);
}

/* ========== Tracepoint Probe Functions ========== */

/*
 * sched_switch probe - called when a context switch occurs
 * 
 * This is the main probe function that captures scheduling events.
 * It must be as lightweight as possible to minimize impact on
 * the scheduling path.
 */
static void probe_sched_switch(void *ignore, bool preempt,
                               struct task_struct *prev,
                               struct task_struct *next,
                               unsigned int prev_state)
{
    struct sched_event event;
    int cpu;
    int ret;
    
    /* Check if monitoring is active */
    if (!atomic_read(&g_state.active))
        return;
    
    /* Get current CPU */
    cpu = smp_processor_id();
    
    /* Fill event structure */
    fill_event_basic(&event, prev, next, cpu, prev_state);
    fill_delay_from_wakeup(&event, prev, next);
    
    /* Apply PID filter if configured */
    if (!config_check_filter(&event))
        return;
    
    /* Write event to per-CPU buffer */
    ret = ring_buffer_write_event(&event, cpu);
    if (ret) {
        /* Buffer full or error - increment dropped counter */
        struct sched_stats *stats = this_cpu_ptr(g_state.per_cpu_stats);
        stats->dropped_events++;
    }
    
    /* Update statistics */
    stats_update(&event, cpu);
}

/*
 * sched_wakeup probe - called when a task is woken up
 * 
 * This can be used to track wakeup latency in future extensions.
 */
static void probe_sched_wakeup(void *ignore, struct task_struct *p)
{
    if (!atomic_read(&g_state.active))
        return;

    record_wakeup_ts(p->pid, get_timestamp_ns());
}

/* ========== Tracepoint Registration ========== */

int trace_hook_init(void)
{
    int ret;
    
    mon_info("Registering tracepoint probes");

    hash_init(wakeup_ts_table);
    tp_sched_switch = NULL;
    tp_sched_wakeup = NULL;
    for_each_kernel_tracepoint(find_tracepoint_cb, NULL);

    if (!tp_sched_switch)
        return -ENOENT;

    ret = tracepoint_probe_register(tp_sched_switch, probe_sched_switch, NULL);
    if (ret) {
        mon_err("Failed to register sched_switch: %d", ret);
        return ret;
    }

    if (tp_sched_wakeup) {
        ret = tracepoint_probe_register(tp_sched_wakeup, probe_sched_wakeup,
                                        NULL);
        if (ret)
            mon_warn("Failed to register sched_wakeup: %d", ret);
    } else {
        mon_warn("sched_wakeup tracepoint not found");
    }

    g_state.trace_registered = true;
    mon_info("Tracepoint probes registered successfully");
    
    return 0;
}

void trace_hook_exit(void)
{
    if (!g_state.trace_registered)
        return;
    
    mon_info("Unregistering tracepoint probes");

    if (tp_sched_wakeup)
        tracepoint_probe_unregister(tp_sched_wakeup, probe_sched_wakeup, NULL);
    if (tp_sched_switch)
        tracepoint_probe_unregister(tp_sched_switch, probe_sched_switch, NULL);
    
    /* Wait for RCU grace period to ensure all probes are quiescent */
    tracepoint_synchronize_unregister();

    clear_wakeup_table();
    
    g_state.trace_registered = false;
    mon_info("Tracepoint probes unregistered");
}
