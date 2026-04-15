/*
 * netlink_comm.c - Netlink communication implementation
 * 
 * Handles kernel-user space communication using Netlink sockets.
 * Supports multicast for multiple client subscriptions.
 * 
 * Copyright (C) 2024
 */

#include "kernel_internal.h"
#include <net/netlink.h>
#include <net/genetlink.h>

/* ========== Generic Netlink Family Definition ========== */

/* Attributes */
enum {
    SCHED_ATTR_UNSPEC = 0,
    SCHED_ATTR_EVENT,
    SCHED_ATTR_CONFIG,
    SCHED_ATTR_STATS,
    SCHED_ATTR_ACK,
    __SCHED_ATTR_MAX,
};
#define SCHED_ATTR_MAX (__SCHED_ATTR_MAX - 1)

/* Commands */
enum {
    SCHED_CMD_UNSPEC = 0,
    SCHED_CMD_GET_EVENT,
    SCHED_CMD_SET_CONFIG,
    SCHED_CMD_GET_STATS,
    SCHED_CMD_CTRL,
    __SCHED_CMD_MAX,
};
#define SCHED_CMD_MAX (__SCHED_CMD_MAX - 1)

/* ========== Netlink Message Construction ========== */

static struct sk_buff *nlmsg_new_buffer(u32 size)
{
    return nlmsg_new(size, GFP_ATOMIC);
}

static int nlmsg_send_multicast(struct sk_buff *skb, u32 portid, int group)
{
    int ret;
    
    if (!g_state.nl_sock) {
        nlmsg_free(skb);
        return -ENOTCONN;
    }
    
    ret = nlmsg_multicast(g_state.nl_sock, skb, portid, group, GFP_ATOMIC);
    if (ret == -ESRCH) {
        /* No listeners - this is normal */
        mon_debug("No Netlink listeners");
        ret = 0;
    } else if (ret < 0) {
        mon_warn("Netlink multicast failed: %d", ret);
    }
    
    return ret;
}

/* ========== Event Sending ========== */

/*
 * netlink_send_events - Send events to user space via Netlink
 * 
 * Packages multiple events into a single Netlink message for
 * efficient transmission.
 */
int netlink_send_events(struct sched_event *events, u32 count)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    struct nl_msg_header *msg_hdr;
    struct nl_event_payload *payload;
    size_t payload_size;
    u32 total_size;
    
    if (!events || count == 0)
        return -EINVAL;
    
    /* Calculate message size */
    payload_size = sizeof(struct nl_msg_header) + 
                   sizeof(struct nl_event_payload) +
                   count * sizeof(struct sched_event);
    total_size = NLMSG_SPACE(payload_size);
    
    /* Allocate message buffer */
    skb = nlmsg_new_buffer(total_size);
    if (!skb) {
        mon_warn("Failed to allocate Netlink message buffer");
        return -ENOMEM;
    }
    
    /* Build Netlink message header */
    nlh = nlmsg_put(skb, 0, g_state.nl_seq++, NLMSG_DONE, payload_size, 0);
    if (!nlh) {
        nlmsg_free(skb);
        return -EMSGSIZE;
    }
    
    /* Fill message header */
    msg_hdr = nlmsg_data(nlh);
    msg_hdr->type = SCHED_MSG_EVENT;
    msg_hdr->seq = atomic_inc_return(&g_state.event_seq);
    msg_hdr->len = count * sizeof(struct sched_event);
    
    /* Fill payload */
    payload = (struct nl_event_payload *)(msg_hdr + 1);
    payload->count = count;
    memcpy(payload->events, events, count * sizeof(struct sched_event));
    
    /* Send multicast */
    return nlmsg_send_multicast(skb, 0, SCHED_MONITOR_GROUP);
}

/*
 * netlink_send_stats - Send statistics to user space
 */
int netlink_send_stats(struct sched_stats *stats)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    struct nl_msg_header *msg_hdr;
    struct nl_stats_payload *payload;
    size_t payload_size;
    
    payload_size = sizeof(struct nl_msg_header) + 
                   sizeof(struct nl_stats_payload);
    
    skb = nlmsg_new_buffer(NLMSG_SPACE(payload_size));
    if (!skb)
        return -ENOMEM;
    
    nlh = nlmsg_put(skb, 0, g_state.nl_seq++, NLMSG_DONE, payload_size, 0);
    if (!nlh) {
        nlmsg_free(skb);
        return -EMSGSIZE;
    }
    
    msg_hdr = nlmsg_data(nlh);
    msg_hdr->type = SCHED_MSG_STATS;
    msg_hdr->seq = g_state.nl_seq;
    msg_hdr->len = sizeof(struct sched_stats);
    
    payload = (struct nl_stats_payload *)(msg_hdr + 1);
    memcpy(&payload->stats, stats, sizeof(struct sched_stats));
    
    return nlmsg_send_multicast(skb, 0, SCHED_MONITOR_GROUP);
}

/*
 * netlink_send_ack - Send acknowledgment message
 */
int netlink_send_ack(s32 status, const char *message)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    struct nl_msg_header *msg_hdr;
    struct nl_ack_payload *payload;
    size_t payload_size;
    
    payload_size = sizeof(struct nl_msg_header) + 
                   sizeof(struct nl_ack_payload);
    
    skb = nlmsg_new_buffer(NLMSG_SPACE(payload_size));
    if (!skb)
        return -ENOMEM;
    
    nlh = nlmsg_put(skb, 0, g_state.nl_seq++, NLMSG_DONE, payload_size, 0);
    if (!nlh) {
        nlmsg_free(skb);
        return -EMSGSIZE;
    }
    
    msg_hdr = nlmsg_data(nlh);
    msg_hdr->type = SCHED_MSG_ACK;
    msg_hdr->seq = g_state.nl_seq;
    msg_hdr->len = sizeof(struct nl_ack_payload);
    
    payload = (struct nl_ack_payload *)(msg_hdr + 1);
    payload->status = status;
    payload->reserved = 0;
    strncpy(payload->message, message ? message : "", 
            sizeof(payload->message) - 1);
    
    return nlmsg_send_multicast(skb, 0, SCHED_MONITOR_GROUP);
}

/* ========== Configuration Processing ========== */

static void process_config_msg(struct nl_config_payload *cfg)
{
    if (!cfg)
        return;
    
    mutex_lock(&g_state.config_lock);
    
    switch (cfg->cmd) {
    case SCHED_CFG_SET_FILTER_PID:
        g_state.config.filter_pid = cfg->value;
        if (cfg->value > 0)
            g_state.config.flags |= SCHED_FLAG_FILTER_PID;
        else
            g_state.config.flags &= ~SCHED_FLAG_FILTER_PID;
        mon_info("Set filter PID: %u", cfg->value);
        break;
        
    case SCHED_CFG_SET_THRESHOLD:
        g_state.config.threshold_ns = cfg->value64;
        mon_info("Set threshold: %llu ns", cfg->value64);
        break;
        
    case SCHED_CFG_SET_INTERVAL:
        g_state.config.interval_ms = cfg->value;
        if (atomic_read(&g_state.active))
            async_worker_start();
        mon_info("Set interval: %u ms", cfg->value);
        break;
        
    case SCHED_CFG_CLEAR_STATS:
        stats_reset();
        ring_buffer_reset();
        mon_info("Statistics cleared");
        break;
        
    case SCHED_CFG_ENABLE:
        atomic_set(&g_state.active, 1);
        async_worker_start();
        mon_info("Monitoring enabled");
        break;
        
    case SCHED_CFG_DISABLE:
        atomic_set(&g_state.active, 0);
        async_worker_stop();
        mon_info("Monitoring disabled");
        break;
        
    default:
        mon_warn("Unknown config command: %u", cfg->cmd);
        break;
    }
    
    mutex_unlock(&g_state.config_lock);
}

void netlink_process_config(struct nl_config_payload *cfg)
{
    process_config_msg(cfg);
}

/* ========== Netlink Receive Callback ========== */

static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct nl_msg_header *msg_hdr;
    void *payload;
    
    nlh = nlmsg_hdr(skb);
    if (!nlh) {
        mon_warn("Invalid Netlink message header");
        return;
    }
    
    if (nlh->nlmsg_len < NLMSG_HDRLEN + sizeof(struct nl_msg_header)) {
        mon_warn("Netlink message too short");
        return;
    }
    
    msg_hdr = nlmsg_data(nlh);
    payload = msg_hdr + 1;
    
    mon_debug("Received Netlink message: type=%u, seq=%u, len=%u",
              msg_hdr->type, msg_hdr->seq, msg_hdr->len);
    
    switch (msg_hdr->type) {
    case SCHED_MSG_CONFIG:
        if (msg_hdr->len >= sizeof(struct nl_config_payload)) {
            process_config_msg(payload);
            netlink_send_ack(0, "Configuration applied");
        }
        break;
        
    case SCHED_MSG_STATS:
        {
            struct sched_stats stats;
            stats_aggregate(&stats);
            netlink_send_stats(&stats);
        }
        break;
        
    case SCHED_MSG_CTRL:
        {
            u32 cmd = *(u32 *)payload;
            switch (cmd) {
            case SCHED_CTRL_START:
                atomic_set(&g_state.active, 1);
                async_worker_start();
                netlink_send_ack(0, "Monitoring started");
                break;
            case SCHED_CTRL_STOP:
                atomic_set(&g_state.active, 0);
                async_worker_stop();
                netlink_send_ack(0, "Monitoring stopped");
                break;
            case SCHED_CTRL_RESET:
                stats_reset();
                ring_buffer_reset();
                netlink_send_ack(0, "Reset complete");
                break;
            default:
                netlink_send_ack(-EINVAL, "Unknown control command");
                break;
            }
        }
        break;
        
    default:
        mon_warn("Unknown message type: %u", msg_hdr->type);
        netlink_send_ack(-EINVAL, "Unknown message type");
        break;
    }
}

/* ========== Netlink Configuration ========== */

static struct netlink_kernel_cfg nl_cfg = {
    .input = nl_recv_msg,
    .groups = SCHED_MONITOR_GROUP,
};

/* ========== Initialization and Cleanup ========== */

int netlink_init(void)
{
    mon_info("Initializing Netlink communication");
    
    g_state.nl_sock = netlink_kernel_create(&init_net, 
                                            SCHED_MONITOR_NETLINK,
                                            &nl_cfg);
    if (!g_state.nl_sock) {
        mon_err("Failed to create Netlink socket");
        return -ENOMEM;
    }
    
    g_state.nl_seq = 0;
    
    mon_info("Netlink socket created (protocol=%d, group=%d)",
             SCHED_MONITOR_NETLINK, SCHED_MONITOR_GROUP);
    
    return 0;
}

void netlink_exit(void)
{
    if (g_state.nl_sock) {
        netlink_kernel_release(g_state.nl_sock);
        g_state.nl_sock = NULL;
        mon_info("Netlink socket released");
    }
}
