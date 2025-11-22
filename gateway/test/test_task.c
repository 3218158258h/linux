#include "app/app_task.h"
#include <pthread.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void task_impl(void *argv)
{
    int *p = argv;
    pthread_mutex_lock(&mutex);
    *p += 1;
    pthread_mutex_unlock(&mutex);
}

int main(void)
{
    int a = 5;
    app_task_init(5);
    for (size_t i = 0; i < 100; i++)
    {
        app_task_register(task_impl, &a);
    }

    sleep(5);

    assert(a == 105);

    app_task_close();
    app_task_wait();

    return 0;
}
