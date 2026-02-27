// app_runner.c
#include "../include/app_runner.h"
#include "../include/app_task.h"
#include "../include/app_serial.h"
#include "../include/app_bluetooth.h"
#include "../include/app_router.h"
#include <signal.h>

#define Device_Node "/dev/ttyUSB0"

static SerialDevice device;
static RouterManager router;
static volatile sig_atomic_t stop_requested = 0;

static void app_runner_signal_handler(int sig)
{
    (void)sig;
    stop_requested = 1;
    app_task_signal_stop();
}

int app_runner_run()
{
    signal(SIGINT, app_runner_signal_handler);
    signal(SIGTERM, app_runner_signal_handler);

    if (app_task_init(5) != 0) {
        return -1;
    }

    if (app_serial_init(&device, Device_Node) != 0) {
        app_task_close();
        return -1;
    }
    
    if (app_bluetooth_setConnectionType(&device) != 0) {
        app_device_close((Device *)&device);
        app_task_close();
        return -1;
    }

    if (app_router_init(&router, NULL) != 0) {
        app_device_close((Device *)&device);
        app_task_close();
        return -1;
    }

    app_router_register_device(&router, (Device *)&device);

    if (app_router_start(&router) != 0) {
        app_router_close(&router);
        app_task_close();
        return -1;
    }

    app_task_wait();

    app_router_stop(&router);
    app_router_close(&router);
    app_task_close();

    return 0;
}
