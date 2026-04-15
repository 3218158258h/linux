/*
 * netlink_client.c - 用户态Netlink客户端实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "user_common.h"

/* ========== 全局变量 ========== */
static int g_nl_sock = -1;
static uint32_t g_nl_seq = 0;
static uint32_t g_local_port = 0;

/* ========== 内部函数 ========== */

// 创建Netlink套接字
static int create_nl_socket(void)
{
    int sock;
    struct sockaddr_nl addr;
    
    sock = socket(AF_NETLINK, SOCK_RAW, SCHED_MONITOR_NETLINK);
    if (sock < 0) {
        log_error("创建Netlink套接字失败: %s", strerror(errno));
        return -1;
    }
    
    /* 绑定本地地址 */
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = SCHED_MONITOR_GROUP;  /* 加入多播组 */
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_error("绑定Netlink套接字失败: %s", strerror(errno));
        close(sock);
        return -1;
    }
    
    g_local_port = addr.nl_pid;
    
    return sock;
}

// 发送Netlink消息
static ssize_t send_nl_msg(int sock, void *buf, size_t len)
{
    struct sockaddr_nl dest;
    struct msghdr msg;
    struct iovec iov;
    
    memset(&dest, 0, sizeof(dest));
    dest.nl_family = AF_NETLINK;
    dest.nl_pid = 0;      /* 内核 */
    dest.nl_groups = 0;   /* 单播 */
    
    memset(&msg, 0, sizeof(msg));
    iov.iov_base = buf;
    iov.iov_len = len;
    msg.msg_name = &dest;
    msg.msg_namelen = sizeof(dest);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    return sendmsg(sock, &msg, 0);
}

// 接收Netlink消息（带超时）
static ssize_t recv_nl_msg(int sock, void *buf, size_t len, int timeout_ms)
{
    struct sockaddr_nl src;
    struct msghdr msg;
    struct iovec iov;
    fd_set read_fds;
    struct timeval tv;
    int ret;
    
    /* 设置超时 */
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    ret = select(sock + 1, &read_fds, NULL, NULL, &tv);
    if (ret < 0) {
        log_error("select() 执行失败: %s", strerror(errno));
        return -1;
    } else if (ret == 0) {
        return -2;  /* 超时 */
    }
    
    memset(&msg, 0, sizeof(msg));
    iov.iov_base = buf;
    iov.iov_len = len;
    msg.msg_name = &src;
    msg.msg_namelen = sizeof(src);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    
    return recvmsg(sock, &msg, 0);
}

/* ========== 对外接口 ========== */

// 初始化Netlink客户端
int netlink_client_init(void)
{
    if (g_nl_sock >= 0) {
        log_warn("Netlink客户端已初始化");
        return 0;
    }
    
    g_nl_sock = create_nl_socket();
    if (g_nl_sock < 0) {
        return ERR_SOCKET;
    }
    
    g_nl_seq = 0;
    
    log_info("Netlink客户端初始化完成 (套接字=%d, 端口=%u)", 
             g_nl_sock, g_local_port);
    
    return ERR_SUCCESS;
}

// 清理Netlink客户端资源
void netlink_client_cleanup(void)
{
    if (g_nl_sock >= 0) {
        close(g_nl_sock);
        g_nl_sock = -1;
        log_info("Netlink客户端资源清理完成");
    }
}

// 发送配置消息到内核
int netlink_client_send_config(uint32_t cmd, uint32_t value, uint64_t value64)
{
    struct {
        struct nlmsghdr nlh;
        struct nl_msg_header msg;
        struct nl_config_payload cfg;
    } packet;
    ssize_t ret;
    
    if (g_nl_sock < 0) {
        log_error("Netlink客户端未初始化");
        return ERR_SOCKET;
    }
    
    /* 构造消息 */
    memset(&packet, 0, sizeof(packet));
    
    packet.nlh.nlmsg_len = sizeof(packet);
    packet.nlh.nlmsg_type = NLMSG_DONE;
    packet.nlh.nlmsg_flags = NLM_F_REQUEST;
    packet.nlh.nlmsg_seq = g_nl_seq++;
    packet.nlh.nlmsg_pid = g_local_port;
    
    packet.msg.type = SCHED_MSG_CONFIG;
    packet.msg.seq = g_nl_seq;
    packet.msg.len = sizeof(struct nl_config_payload);
    
    packet.cfg.cmd = cmd;
    packet.cfg.value = value;
    packet.cfg.value64 = value64;
    
    ret = send_nl_msg(g_nl_sock, &packet, sizeof(packet));
    if (ret < 0) {
        log_error("发送配置消息失败: %s", strerror(errno));
        return ERR_SEND;
    }
    
    log_debug("已发送配置消息: 命令=%u, 参数=%u, 64位参数=%lu",
              cmd, value, value64);
    
    return ERR_SUCCESS;
}

// 接收内核发送的调度事件
int netlink_client_recv_events(struct sched_event *events, int max_count,
                               int timeout_ms)
{
    char buf[65536];
    struct nlmsghdr *nlh;
    struct nl_msg_header *msg;
    struct nl_event_payload *payload;
    ssize_t ret;
    int count;
    
    if (g_nl_sock < 0) {
        log_error("Netlink客户端未初始化");
        return ERR_SOCKET;
    }
    
    if (!events || max_count <= 0) {
        return ERR_INVALID_MSG;
    }
    
    ret = recv_nl_msg(g_nl_sock, buf, sizeof(buf), timeout_ms);
    if (ret < 0) {
        if (ret == -2) {
            return ERR_TIMEOUT;
        }
        return ERR_RECV;
    }
    
    nlh = (struct nlmsghdr *)buf;
    if (nlh->nlmsg_len < NLMSG_HDRLEN + sizeof(struct nl_msg_header)) {
        log_warn("接收的消息长度过短");
        return ERR_INVALID_MSG;
    }
    
    msg = NLMSG_DATA(nlh);
    
    if (msg->type != SCHED_MSG_EVENT) {
        log_debug("接收到非事件消息: 类型=%u", msg->type);
        return 0;
    }
    
    payload = (struct nl_event_payload *)(msg + 1);
    count = payload->count;
    
    if (count > max_count) {
        count = max_count;
    }
    
    memcpy(events, payload->events, count * sizeof(struct sched_event));
    
    return count;
}

// 接收内核统计信息
int netlink_client_recv_stats(struct sched_stats *stats, int timeout_ms)
{
    char buf[4096];
    struct nlmsghdr *nlh;
    struct nl_msg_header *msg;
    struct nl_stats_payload *payload;
    ssize_t ret;
    
    if (g_nl_sock < 0) {
        log_error("Netlink客户端未初始化");
        return ERR_SOCKET;
    }
    
    if (!stats) {
        return ERR_INVALID_MSG;
    }
    
    ret = recv_nl_msg(g_nl_sock, buf, sizeof(buf), timeout_ms);
    if (ret < 0) {
        if (ret == -2) {
            return ERR_TIMEOUT;
        }
        return ERR_RECV;
    }
    
    nlh = (struct nlmsghdr *)buf;
    if (nlh->nlmsg_len < NLMSG_HDRLEN + sizeof(struct nl_msg_header)) {
        log_warn("接收的消息长度过短");
        return ERR_INVALID_MSG;
    }
    
    msg = NLMSG_DATA(nlh);
    
    if (msg->type != SCHED_MSG_STATS) {
        log_debug("接收到非统计消息: 类型=%u", msg->type);
        return ERR_INVALID_MSG;
    }
    
    payload = (struct nl_stats_payload *)(msg + 1);
    memcpy(stats, &payload->stats, sizeof(struct sched_stats));
    
    return ERR_SUCCESS;
}