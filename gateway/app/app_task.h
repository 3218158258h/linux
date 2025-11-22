#if !defined(__APP_TASK_H__)
#define __APP_TASK_H__

typedef void (*Task)(void *);

int app_task_init(int executors);

int app_task_register(Task task, void *args);

void app_task_wait();

void app_task_close();

#endif // __APP_TASK_H__
