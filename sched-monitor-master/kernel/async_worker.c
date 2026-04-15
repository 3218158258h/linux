/*
 * async_worker.c - 异步工作队列实现
 * 使用工作队列在调度路径之外执行事件处理和Netlink发送
 */

#include "kernel_internal.h"

/* ========== 工作队列处理函数 ========== */

static void async_work_handler(struct work_struct *work)
{
    struct sched_event *events;
    int event_count;
    int ret;

    /* 检查监控是否处于激活状态 */
    if (!atomic_read(&g_state.active))
        return;

    /* 分配临时事件缓冲区 */
    events = kmalloc_array(MAX_EVENTS_PER_MSG, sizeof(struct sched_event),
                           GFP_KERNEL);
    if (!events) {
        mon_warn("分配事件缓冲区失败");
        return;
    }

    /* 从环形缓冲区读取事件 */
    event_count = ring_buffer_read_events(events, MAX_EVENTS_PER_MSG,
                                          NULL, NULL);

    if (event_count > 0) {
        /* 通过Netlink发送事件 */
        ret = netlink_send_events(events, event_count);
        if (ret) {
            mon_debug("发送事件失败: %d", ret);
        }
    }

    kfree(events);

    /* 定期发送统计信息更新 */
    if (atomic_read(&g_state.event_seq) % 100 == 0) {
        struct sched_stats stats;
        stats_aggregate(&stats);
        netlink_send_stats(&stats);
    }

    if (atomic_read(&g_state.active))
        queue_delayed_work(g_state.wq, &g_state.dwork,
                           msecs_to_jiffies(g_state.config.interval_ms));
}

/* ========== 初始化与清理 ========== */

int async_worker_init(void)
{
    mon_info("正在初始化异步工作队列");

    /* 创建专用工作队列 */
    g_state.wq = alloc_workqueue("sched_monitor_wq",
                                  WQ_UNBOUND | WQ_HIGHPRI,
                                  1);
    if (!g_state.wq) {
        mon_err("创建工作队列失败");
        return -ENOMEM;
    }

    /* 初始化延迟工作项 */
    INIT_DELAYED_WORK(&g_state.dwork, async_work_handler);

    mon_info("异步工作队列初始化完成");

    return 0;
}

void async_worker_exit(void)
{
    if (g_state.wq) {
        /* 取消所有待处理的工作 */
        cancel_delayed_work_sync(&g_state.dwork);

        /* 刷新并销毁工作队列 */
        flush_workqueue(g_state.wq);
        destroy_workqueue(g_state.wq);
        g_state.wq = NULL;

        mon_info("异步工作队列已销毁");
    }
}

void async_worker_start(void)
{
    if (!g_state.wq)
        return;

    mod_delayed_work(g_state.wq, &g_state.dwork,
                     msecs_to_jiffies(g_state.config.interval_ms));

    mon_debug("异步工作队列已启动 (间隔=%u 毫秒)",
              g_state.config.interval_ms);
}

void async_worker_stop(void)
{
    if (!g_state.wq)
        return;

    /*
     * 先设置active=0，再取消工作，避免并发处理函数在停止后重新入队
     */
    smp_mb__before_atomic();
    atomic_set(&g_state.active, 0);
    smp_mb__after_atomic();
    cancel_delayed_work_sync(&g_state.dwork);

    mon_debug("异步工作队列已停止");
}