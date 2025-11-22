#include "app_runner.h"
#include "app_task.h"
#include "app_serial.h"
#include "app_bluetooth.h"
#include "app_router.h"
#include <signal.h>
#include <assert.h>

#define Device_Node "/dev/ttyUSB0"

static SerialDevice device;

static void app_runner_stop(int sig)
{
    assert(sig == SIGINT || sig == SIGTERM);
    app_router_close();
    app_task_close();
}
int app_runner_run()
{
    // 注册信号处理
    signal(SIGINT, app_runner_stop);
    signal(SIGTERM, app_runner_stop);

    // 初始化线程池
    app_task_init(5);

    // 初始化设备
    app_serial_init(&device, Device_Node);
    app_bluetooth_setConnectionType(&device);

    // 初始化消息路由
    app_router_init();

    // 注册设备
    app_router_registerDevice((Device *)&device);

    // 等待线程池关闭
    app_task_wait();

    return 0;
}