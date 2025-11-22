#include "app_task.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "thirdparty/log.c/log.h"
#include <errno.h>


#define MSG_LEN sizeof(struct TaskStruct)

struct TaskStruct
{
    Task task;
    void *argv;
};

static pthread_t *executor_ptr = NULL;
static int executors_count = 0;
static int pipe_fd[2] = {-1, -1};
static pthread_mutex_t write_lock;
static int is_initialized = 0;  // 2. 新增：跟踪初始化状态

// 工作线程核心逻辑（修复取消点和清理）
static void *app_task_executor(void *argv)
{
    int executor_id = *(int*)argv;
    free(argv);
    log_info("Executor %d start!", executor_id);

    // 清理处理程序
    void cleanup_handler(void *arg) {
        log_info("Executor %d cleaned up", *(int*)arg);
    }
    int id = executor_id;
    pthread_cleanup_push(cleanup_handler, &id);

    // 设置延迟取消
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    struct TaskStruct task_struct;
    ssize_t ret;

    while (1)
    {
        // 处理 EINTR 重试
        do {
            ret = read(pipe_fd[0], &task_struct, MSG_LEN);
        } while (ret < 0 && errno == EINTR);

        if (ret < 0)
        {
            if (errno == ECANCELED)
            {
                log_info("Executor %d exit due to cancel", executor_id);
                break;
            }
            log_warn("Pipe read error (executor %d): %s", executor_id, strerror(errno));
            continue;
        }
        else if (ret == 0)
        {
            log_info("Pipe closed, executor %d exit", executor_id);
            break;
        }
        else if (ret != MSG_LEN)
        {
            log_warn("Received incomplete task data (executor %d): expected %zu, got %zd",
                     executor_id, MSG_LEN, ret);
            // 丢弃剩余数据
            char buf[MSG_LEN];
            ssize_t remain = MSG_LEN - ret;
            while (remain > 0)
            {
                ssize_t r = read(pipe_fd[0], buf, remain);
                if (r <= 0) break;
                remain -= r;
            }
            continue;
        }

        // 执行任务期间禁止取消
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        task_struct.task(task_struct.argv);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        // 取消点：任务完成后允许取消
        pthread_testcancel();
    }

    pthread_cleanup_pop(1);
    return NULL;
}

// 初始化（替换命名管道为匿名管道）
int app_task_init(int executors)
{
    if (pthread_mutex_init(&write_lock, NULL) != 0)
    {
        log_error("pthread_mutex_init failed: %s", strerror(errno));
        return -1;
    }

    if (is_initialized || executor_ptr != NULL || executors_count > 0)
    {
        log_error("Task manager already initialized");
        pthread_mutex_destroy(&write_lock);
        return -1;
    }
    if (executors <= 0)
    {
        log_error("Invalid executors count: %d", executors);
        pthread_mutex_destroy(&write_lock);
        return -1;
    }

    executors_count = executors;
    executor_ptr = malloc(executors_count * sizeof(pthread_t));
    if (!executor_ptr)
    {
        log_error("Not enough memory for task manager");
        pthread_mutex_destroy(&write_lock);
        executors_count = 0;
        return -1;
    }
    memset(executor_ptr, 0, executors_count * sizeof(pthread_t));

    // 3. 用匿名管道替代命名管道
    if (pipe(pipe_fd) < 0)
    {
        log_error("pipe create failed: %s", strerror(errno));
        goto FAIL_EXIT;
    }

    // 4. 创建工作线程（添加 malloc 检查）
    int i;
    for (i = 0; i < executors_count; i++)
    {
        int *t = malloc(sizeof(int));
        if (!t)
        {
            log_error("malloc failed for executor %d", i);
            goto THREAD_EXIT;
        }
        *t = i;
        if (pthread_create(executor_ptr + i, NULL, app_task_executor, (void *)t) != 0)
        {
            free(t);
            log_error("pthread_create failed for executor %d: %s", i, strerror(errno));
            goto THREAD_EXIT;
        }
    }

    is_initialized = 1;
    log_info("Task manager started with %d executors", executors_count);
    return 0;

THREAD_EXIT:
    // 5. 线程创建失败时关闭管道
    if (pipe_fd[1] >= 0) close(pipe_fd[1]);
    if (pipe_fd[0] >= 0) close(pipe_fd[0]);
    // 取消已创建的线程
    for (int j = 0; j < i; j++)
    {
        if (executor_ptr[j])
        {
            pthread_cancel(executor_ptr[j]);
            pthread_join(executor_ptr[j], NULL);
        }
    }

FAIL_EXIT:
    free(executor_ptr);
    executor_ptr = NULL;
    pthread_mutex_destroy(&write_lock);
    executors_count = 0;
    return -1;
}

// 任务注册（修复 EINTR 处理）
int app_task_register(Task task, void *args)
{
    if (!is_initialized || pipe_fd[1] < 0)
    {
        log_warn("Task manager not initialized or pipe closed");
        return -1;
    }

    struct TaskStruct task_struct = {
        .task = task,
        .argv = args,
    };
    int result = -1;

    pthread_mutex_lock(&write_lock);
    do
    {
        ssize_t written;
        // 处理 EINTR 重试
        do {
            written = write(pipe_fd[1], &task_struct, MSG_LEN);
        } while (written < 0 && errno == EINTR);

        if (written < 0)
        {
            log_warn("Pipe write error: %s", strerror(errno));
            break;
        }
        if (written != MSG_LEN)
        {
            log_warn("Partial write (expected %zu, got %zd)", MSG_LEN, written);
            break;
        }
        result = 0;
    } while (0);
    pthread_mutex_unlock(&write_lock);

    log_debug("Task %p registered", task);
    return result;
}


// 等待所有任务执行完毕（不取消线程，需确保队列无新任务加入）
void app_task_wait(void)
{
    // 检查是否已初始化
    if (executor_ptr == NULL || executors_count == 0)
    {
        log_warn("Task manager not initialized");
        return;
    }

    // 核心修正：仅等待线程，不关闭写端、不销毁资源（与第二段完全一致）
    // 线程会持续阻塞在read(pipe_fd[0])，等待新任务
    for (int i = 0; i < executors_count; i++)
    {
        if (executor_ptr[i])
        {
            pthread_join(executor_ptr[i], NULL); // 无任务时，线程阻塞，此处也会阻塞
        }
    }

    log_info("All tasks completed.");
}
// 强制关闭（保持逻辑，补充初始化检查）
void app_task_close(void)
{
    log_info("Closing task manager...");

    if (!is_initialized || executor_ptr == NULL || executors_count == 0)
    {
        log_warn("Task manager not initialized");
        return;
    }

    // 取消所有线程
    for (int i = 0; i < executors_count; i++)
    {
        if (executor_ptr[i])
        {
            pthread_cancel(executor_ptr[i]);
            pthread_join(executor_ptr[i], NULL);
            executor_ptr[i] = 0;
        }
    }

    // 清理资源
    free(executor_ptr);
    executor_ptr = NULL;

    if (pipe_fd[1] >= 0) close(pipe_fd[1]);
    if (pipe_fd[0] >= 0) close(pipe_fd[0]);

    pthread_mutex_destroy(&write_lock);
    executors_count = 0;
    is_initialized = 0;

    log_info("Task manager closed");
}
