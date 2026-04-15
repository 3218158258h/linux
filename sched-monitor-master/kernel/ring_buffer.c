/*
 * ring_buffer.c - Per-CPU环形缓冲区实现
 *
 * 用于高性能事件存储的无锁环形缓冲区。
 * 每个CPU拥有独立缓冲区，避免跨CPU竞争。
 */

#include "kernel_internal.h"

static inline int atomic_fetch_add_compat(int i, atomic_t *v)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    return atomic_fetch_add(i, v);
#else
    return atomic_add_return(i, v) - i;
#endif
}

/* ========== 缓冲区分配 ========== */

static int alloc_per_cpu_buffer(struct per_cpu_buffer *buf, u32 size)
{
    struct page *page;
    void *data;
    int order;

    if (atomic_cmpxchg(&g_state.fi_alloc_fail_once, 1, 0) == 1)
        return -ENOMEM;

    /* 计算分配所需的页阶数 */
    order = get_order(size * sizeof(struct sched_event));

    /* 分配连续物理页 */
    page = alloc_pages(GFP_KERNEL | __GFP_ZERO, order);
    if (!page) {
        mon_err("分配缓冲区页面失败 (order=%d)", order);
        return -ENOMEM;
    }

    data = page_address(page);

    buf->data = data;
    buf->size = (PAGE_SIZE << order) / sizeof(struct sched_event);
    atomic_set(&buf->head, 0);
    atomic_set(&buf->tail, 0);
    atomic_set(&buf->count, 0);
    atomic64_set(&buf->overrun, 0);
    spin_lock_init(&buf->lock);

    mon_debug("已分配缓冲区：大小=%u 个事件，order=%d", buf->size, order);

    return 0;
}

static void free_per_cpu_buffer(struct per_cpu_buffer *buf)
{
    if (buf->data) {
        int order = get_order(buf->size * sizeof(struct sched_event));
        free_pages((unsigned long)buf->data, order);
        buf->data = NULL;
        buf->size = 0;
    }
}

/* ========== 环形缓冲区操作 ========== */

/*
 * ring_buffer_write_event - 向Per-CPU缓冲区写入事件
 *
 * 基于原子操作的无锁实现。
 * 写入位置（head）原子自增，事件写入计算出的位置。
 *
 * 返回：成功0，缓冲区满返回-ENOSPC
 */
int ring_buffer_write_event(struct sched_event *event, int cpu)
{
    struct per_cpu_buffer *buf;
    u32 head, tail, pos;

    buf = per_cpu_ptr(g_state.buffers, cpu);

    /* 原子获取并递增头指针 */
    head = atomic_fetch_add_compat(1, &buf->head);

    /* 计算环形缓冲区实际位置 */
    pos = head % buf->size;

    /* 检查缓冲区溢出 */
    tail = atomic_read(&buf->tail);
    if (head - tail >= buf->size) {
        /* 缓冲区已满 - 增加溢出计数 */
        atomic64_inc(&buf->overrun);
        return -ENOSPC;
    }

    /* 向缓冲区写入事件 */
    memcpy(&buf->data[pos], event, sizeof(struct sched_event));

    /* 事件计数自增 */
    atomic_inc(&buf->count);

    return 0;
}

/*
 * ring_buffer_read_events - 从所有CPU缓冲区读取事件
 *
 * 从Per-CPU缓冲区读取事件并复制到提供的数组中。
 * 返回实际读取的事件总数。
 *
 * 注意：只能在进程上下文（工作队列）中调用。
 */
int ring_buffer_read_events(struct sched_event *events, int max_count,
                            int *cpu_map, int *count_map)
{
    int cpu;
    int total_read = 0;
    int events_per_cpu;
    struct per_cpu_buffer *buf;
    u32 head, tail, count, i, pos;
    unsigned long flags;

    for_each_online_cpu(cpu) {
        if (total_read >= max_count)
            break;

        buf = per_cpu_ptr(g_state.buffers, cpu);

        spin_lock_irqsave(&buf->lock, flags);

        head = atomic_read(&buf->head);
        tail = atomic_read(&buf->tail);
        count = head - tail;

        if (count == 0) {
            spin_unlock_irqrestore(&buf->lock, flags);
            continue;
        }

        /* 限制每个CPU读取的事件数 */
        events_per_cpu = min_t(int, count, max_count - total_read);
        events_per_cpu = min_t(int, events_per_cpu, MAX_EVENTS_PER_MSG);

        /* 复制事件 */
        for (i = 0; i < events_per_cpu; i++) {
            pos = (tail + i) % buf->size;
            memcpy(&events[total_read + i], &buf->data[pos],
                   sizeof(struct sched_event));
        }

        /* 更新尾指针 */
        atomic_add(events_per_cpu, &buf->tail);
        atomic_sub(events_per_cpu, &buf->count);

        spin_unlock_irqrestore(&buf->lock, flags);

        if (cpu_map)
            cpu_map[total_read / MAX_EVENTS_PER_MSG] = cpu;
        if (count_map)
            count_map[total_read / MAX_EVENTS_PER_MSG] = events_per_cpu;

        total_read += events_per_cpu;
    }

    return total_read;
}

/*
 * ring_buffer_reset - 重置所有Per-CPU缓冲区
 */
void ring_buffer_reset(void)
{
    int cpu;
    struct per_cpu_buffer *buf;

    for_each_online_cpu(cpu) {
        buf = per_cpu_ptr(g_state.buffers, cpu);

        spin_lock(&buf->lock);
        atomic_set(&buf->head, 0);
        atomic_set(&buf->tail, 0);
        atomic_set(&buf->count, 0);
        atomic64_set(&buf->overrun, 0);
        spin_unlock(&buf->lock);
    }

    mon_info("环形缓冲区已重置");
}

/* ========== 初始化与清理 ========== */

int ring_buffer_init(void)
{
    int cpu;
    int ret;

    mon_info("正在初始化Per-CPU环形缓冲区");

    /* 分配Per-CPU缓冲区结构体 */
    g_state.buffers = alloc_percpu(struct per_cpu_buffer);
    if (!g_state.buffers) {
        mon_err("分配Per-CPU缓冲区结构体失败");
        return -ENOMEM;
    }

    /* 初始化每个CPU的缓冲区 */
    for_each_online_cpu(cpu) {
        struct per_cpu_buffer *buf;

        buf = per_cpu_ptr(g_state.buffers, cpu);
        ret = alloc_per_cpu_buffer(buf, g_state.config.buffer_size);
        if (ret) {
            mon_err("为CPU %d 分配缓冲区失败", cpu);
            goto err_free;
        }
    }

    mon_info("环形缓冲区初始化成功");
    return 0;

err_free:
    for_each_online_cpu(cpu) {
        struct per_cpu_buffer *buf;
        buf = per_cpu_ptr(g_state.buffers, cpu);
        free_per_cpu_buffer(buf);
    }
    free_percpu(g_state.buffers);
    g_state.buffers = NULL;

    return ret;
}

void ring_buffer_exit(void)
{
    int cpu;

    if (!g_state.buffers)
        return;

    mon_info("正在清理环形缓冲区");

    for_each_online_cpu(cpu) {
        struct per_cpu_buffer *buf;
        buf = per_cpu_ptr(g_state.buffers, cpu);
        free_per_cpu_buffer(buf);
    }

    free_percpu(g_state.buffers);
    g_state.buffers = NULL;

    mon_info("环形缓冲区已清理完成");
}