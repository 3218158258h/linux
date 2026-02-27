#include "../include/app_task.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../thirdparty/log.c/log.h"
#include <errno.h>
#include <signal.h>
static volatile sig_atomic_t task_should_stop = 0;

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
static int is_initialized = 0;

static void cleanup_handler(void *arg) {
    log_info("Executor %d cleaned up", *(int*)arg);

}
static void *app_task_executor(void *argv)
{
    int executor_id = *(int*)argv;
    free(argv);
    log_info("Executor %d start!", executor_id);

    int id = executor_id;
    pthread_cleanup_push(cleanup_handler, &id);

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    struct TaskStruct task_struct;
    ssize_t ret;

    while (1)
    {
        do {
            ret = read(pipe_fd[0], &task_struct, MSG_LEN);
        } while (ret < 0 && errno == EINTR);

        if (ret < 0)
        {
            if (errno == EBADF || errno == EINVAL)
            {
                log_info("Pipe closed, executor %d exit", executor_id);
                break;
            }
            log_warn("Pipe read error (executor %d): %s", executor_id, strerror(errno));
            usleep(10000);
            continue;
        }
        else if (ret == 0)
        {
            log_info("Pipe write end closed, executor %d exit", executor_id);
            break;
        }
        else if (ret != MSG_LEN)
        {
            log_warn("Received incomplete task data");
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

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        task_struct.task(task_struct.argv);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        pthread_testcancel();
    }

    pthread_cleanup_pop(1);
    return NULL;
}

int app_task_init(int executors)
{
    if (pthread_mutex_init(&write_lock, NULL) != 0)
    {
        log_error("pthread_mutex_init failed");
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
        log_error("Invalid executors count");
        pthread_mutex_destroy(&write_lock);
        return -1;
    }

    executors_count = executors;
    executor_ptr = malloc(executors_count * sizeof(pthread_t));
    if (!executor_ptr)
    {
        pthread_mutex_destroy(&write_lock);
        executors_count = 0;
        return -1;
    }
    memset(executor_ptr, 0, executors_count * sizeof(pthread_t));

    if (pipe(pipe_fd) < 0)
    {
        log_error("pipe create failed");
        goto FAIL_EXIT;
    }

    int i;
    for (i = 0; i < executors_count; i++)
    {
        int *t = malloc(sizeof(int));
        if (!t)
        {
            goto THREAD_EXIT;
        }
        *t = i;
        if (pthread_create(executor_ptr + i, NULL, app_task_executor, (void *)t) != 0)
        {
            free(t);
            goto THREAD_EXIT;
        }
    }

    is_initialized = 1;
    log_info("Task manager started with %d executors", executors_count);
    return 0;

THREAD_EXIT:
    if (pipe_fd[1] >= 0) close(pipe_fd[1]);
    if (pipe_fd[0] >= 0) close(pipe_fd[0]);
    pipe_fd[0] = pipe_fd[1] = -1;
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

int app_task_register(Task task, void *args)
{
    if (!is_initialized || pipe_fd[1] < 0)
    {
        return -1;
    }

    struct TaskStruct task_struct = {
        .task = task,
        .argv = args,
    };

    pthread_mutex_lock(&write_lock);
    ssize_t written;
    do {
        written = write(pipe_fd[1], &task_struct, MSG_LEN);
    } while (written < 0 && errno == EINTR);

    pthread_mutex_unlock(&write_lock);

    return (written == MSG_LEN) ? 0 : -1;
}

void app_task_signal_stop(void)
{
    task_should_stop = 1;
}

void app_task_wait(void)
{
    while (!task_should_stop && is_initialized) {
        usleep(100000);
    }
}

void app_task_close(void)
{
    log_info("Closing task manager...");

    if (!is_initialized || executor_ptr == NULL || executors_count == 0)
    {
        return;
    }

    if (pipe_fd[1] >= 0) {
        close(pipe_fd[1]);
        pipe_fd[1] = -1;
    }

    for (int i = 0; i < executors_count; i++)
    {
        if (executor_ptr[i])
        {
            pthread_cancel(executor_ptr[i]);
            pthread_join(executor_ptr[i], NULL);
            executor_ptr[i] = 0;
        }
    }

    free(executor_ptr);
    executor_ptr = NULL;

    if (pipe_fd[0] >= 0) close(pipe_fd[0]);
    pipe_fd[0] = pipe_fd[1] = -1;

    pthread_mutex_destroy(&write_lock);
    executors_count = 0;
    is_initialized = 0;

    log_info("Task manager closed");
}
