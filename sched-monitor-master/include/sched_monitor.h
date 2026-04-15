/*
 * sched_monitor.h - Linux Kernel Performance Monitoring Platform
 * 
 * Common header file for kernel module and user-space tools
 * 
 * Copyright (C) 2024
 */

#ifndef _SCHED_MONITOR_H_
#define _SCHED_MONITOR_H_

#include <linux/types.h>

/* ========== Netlink Configuration ========== */
#define SCHED_MONITOR_NETLINK      31      /* Custom Netlink protocol number */
#define SCHED_MONITOR_GROUP        1       /* Multicast group ID */

/* Netlink message types */
enum sched_msg_type {
    SCHED_MSG_EVENT       = 1,    /* Schedule event data */
    SCHED_MSG_CONFIG      = 2,    /* Configuration message */
    SCHED_MSG_STATS       = 3,    /* Statistics query */
    SCHED_MSG_ACK         = 4,    /* Acknowledgment */
    SCHED_MSG_CTRL        = 5,    /* Control command */
};

/* Configuration commands */
enum sched_config_cmd {
    SCHED_CFG_SET_FILTER_PID   = 1,
    SCHED_CFG_SET_THRESHOLD    = 2,
    SCHED_CFG_SET_INTERVAL     = 3,
    SCHED_CFG_CLEAR_STATS      = 4,
    SCHED_CFG_ENABLE           = 5,
    SCHED_CFG_DISABLE          = 6,
};

/* Control commands */
enum sched_ctrl_cmd {
    SCHED_CTRL_START      = 1,
    SCHED_CTRL_STOP       = 2,
    SCHED_CTRL_RESET      = 3,
};

/* ========== Schedule Event Structure ========== */
#define TASK_COMM_LEN    16

struct sched_event {
    __u64 timestamp;            /* Event timestamp (nanoseconds) */
    __u32 prev_pid;             /* Previous task PID */
    __u32 prev_tgid;            /* Previous task TGID */
    __u32 next_pid;             /* Next task PID */
    __u32 next_tgid;            /* Next task TGID */
    char  prev_comm[TASK_COMM_LEN];  /* Previous task name */
    char  next_comm[TASK_COMM_LEN];  /* Next task name */
    __s32 prev_prio;            /* Previous task priority */
    __s32 next_prio;            /* Next task priority */
    __s32 prev_state;           /* Previous task state */
    __u32 cpu;                  /* CPU core number */
    __u64 delay_ns;             /* Schedule delay (nanoseconds) */
    __u64 runtime_ns;           /* Previous task runtime */
} __attribute__((packed, aligned(64)));

/* ========== Statistics Structure ========== */
#define HISTOGRAM_BINS    16

struct delay_histogram {
    __u64 bins[HISTOGRAM_BINS]; /* Delay distribution bins */
    __u64 bin_bounds[HISTOGRAM_BINS + 1]; /* Bin boundaries (ns) */
};

struct sched_stats {
    __u64 total_events;         /* Total events captured */
    __u64 dropped_events;       /* Dropped events count */
    __u64 total_delay_ns;       /* Total delay accumulated */
    __u64 max_delay_ns;         /* Maximum delay observed */
    __u64 min_delay_ns;         /* Minimum delay observed */
    __u64 anomaly_count;        /* Anomaly events count */
    struct delay_histogram hist; /* Delay distribution */
    __u64 start_time;           /* Monitoring start time */
    __u64 last_update;          /* Last update timestamp */
};

/* ========== Configuration Structure ========== */
struct monitor_config {
    __u32 enabled;              /* Monitoring enabled flag */
    __u32 filter_pid;           /* PID filter (0 = no filter) */
    __u64 threshold_ns;         /* Anomaly threshold (nanoseconds) */
    __u32 interval_ms;          /* Workqueue interval (milliseconds) */
    __u32 buffer_size;          /* Per-CPU buffer size (events) */
    __u32 flags;                /* Additional flags */
};

/* Configuration flags */
#define SCHED_FLAG_FILTER_PID     (1 << 0)
#define SCHED_FLAG_ANOMALY_DETECT (1 << 1)
#define SCHED_FLAG_VERBOSE        (1 << 2)

/* ========== Netlink Message Structures ========== */
struct nl_msg_header {
    __u32 type;                 /* Message type */
    __u32 seq;                  /* Sequence number */
    __u32 len;                  /* Payload length */
};

struct nl_event_payload {
    __u32 count;                /* Number of events */
    struct sched_event events[]; /* Event array */
};

struct nl_config_payload {
    __u32 cmd;                  /* Configuration command */
    __u32 value;                /* Configuration value */
    __u64 value64;              /* 64-bit value */
};

struct nl_stats_payload {
    struct sched_stats stats;
};

struct nl_ack_payload {
    __s32 status;               /* Status code */
    __u32 reserved;
    char message[128];          /* Status message */
};

/* ========== Delay Histogram Bin Boundaries ========== */
/* Exponential scale: 1us, 10us, 100us, 1ms, 10ms, 100ms, 1s, 10s */
#define DELAY_BIN_BOUNDARIES { \
    0,              /* [0, 1us) */ \
    1000,           /* [1us, 10us) */ \
    10000,          /* [10us, 100us) */ \
    100000,         /* [100us, 1ms) */ \
    1000000,        /* [1ms, 10ms) */ \
    10000000,       /* [10ms, 100ms) */ \
    100000000,      /* [100ms, 1s) */ \
    1000000000,     /* [1s, 10s) */ \
    10000000000ULL, /* [10s, +inf) */ \
    0, 0, 0, 0, 0, 0, 0, 0 /* Reserved for finer granularity */ \
}

/* ========== Utility Macros ========== */
#define NS_TO_US(ns)    ((ns) / 1000)
#define NS_TO_MS(ns)    ((ns) / 1000000)
#define US_TO_NS(us)    ((us) * 1000)
#define MS_TO_NS(ms)    ((ms) * 1000000)

/* Maximum events per Netlink message */
#define MAX_EVENTS_PER_MSG    64

/* Default configuration values */
#define DEFAULT_BUFFER_SIZE    4096
#define DEFAULT_INTERVAL_MS    10
#define DEFAULT_THRESHOLD_NS   100000000ULL  /* 100ms */

#endif /* _SCHED_MONITOR_H_ */
