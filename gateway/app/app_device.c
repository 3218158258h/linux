#include "app_device.h"
#include <stdlib.h>
#include "thirdparty/log.c/log.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#define BUFFER_LEN 16384

/**
 * @brief 默认设备同步线程
 *
 * @param argv 设备指针
 * @return void*
 */
static void *app_device_backgroundTask(void *argv)
{
    unsigned char buf[1024];
    Device *device = argv;
    while (1)
    {
        // 读取设备数据
        int buf_len = read(device->fd, buf, 1024);
        if (buf_len < 0)
        {
            log_warn("read device data error");
            continue;
        }

        // 写入缓存
        if (device->vptr->post_read)
        {
            device->vptr->post_read(device, buf, &buf_len);
        }

        if (buf_len > 0)
        {
            app_buffer_write(device->recv_buffer, buf, buf_len);
        }
        // 注册一个发送任务，让线程池处理
        app_task_register(device->vptr->recv_task, device);
    }

    return NULL;
}

static void app_device_defaultRecvTask(void *argv)
{
    unsigned char buf[1024];
    Device *device = argv;
    // 读取缓冲区的数据
    app_buffer_read(device->recv_buffer, buf, 3);
    app_buffer_read(device->recv_buffer, buf + 3, buf[1] + buf[2]);
    int buf_len = buf[1] + buf[2] + 3;
    // 调用发送回调函数
    while (device->vptr->recv_callback(buf, buf_len) < 0)
    {
        usleep(100000);
    }
}

static void app_device_defaultSendTask(void *argv)
{
    unsigned char buf[1024];
    int buf_len = 0;
    Device *device = argv;
    // 从缓冲区中读取二进制消息
    app_buffer_read(device->send_buffer, buf, 3);
    app_buffer_read(device->send_buffer, buf + 3, buf[1] + buf[2]);
    buf_len = 3 + buf[1] + buf[2];

    // 将消息写入到设备文件描述符
    if (device->vptr->pre_write)
    {
        device->vptr->pre_write(device, buf, &buf_len);
    }
    // 约定，如果写前处理函数处理后的数据长度为0, 表示不需要写入。
    if (buf_len > 0)
    {
        write(device->fd, buf, buf_len);
    }
}

int app_device_init(Device *device, char *filename)
{
    device->filename = malloc(strlen(filename) + 1);
    if (!device->filename)
    {
        log_warn("Not enough memory for device %s", filename);
        goto DEVICE_EXIT;
    }
    device->vptr = malloc(sizeof(struct VTable));
    if (!device->vptr)
    {
        log_warn("Not enough memory for device %s", filename);
        goto DEVICE_FILENAME_EXIT;
    }
    device->recv_buffer = malloc(sizeof(Buffer));
    if (!device->recv_buffer)
    {
        log_warn("Not enough memory for device %s", filename);
        goto DEVICE_VTABLE_EXIT;
    }
    device->send_buffer = malloc(sizeof(Buffer));
    if (!device->send_buffer)
    {
        log_warn("Not enough memory for device %s", filename);
        goto DEVICE_RECV_BUFFER_EXIT;
    }

    // 对于device的属性进行初始化
    strcpy(device->filename, filename);
    device->fd = open(device->filename, O_RDWR | O_NOCTTY);
    if (device->fd < 0)
    {
        log_warn("Device open failed");
        goto DEVICE_SEND_BUFFER_EXIT;
    }
    device->connection_type = CONNECTION_TYPE_NONE;
    if (app_buffer_init(device->recv_buffer, BUFFER_LEN) < 0)
    {
        goto DEVICE_OPEN_FAIL;
    }
    if (app_buffer_init(device->send_buffer, BUFFER_LEN) < 0)
    {
        goto DEVICE_RECV_INIT_FAIL;
    }
    device->is_running = 0;

    // TODO 虚表也需要初始化
    device->vptr->background_task = app_device_backgroundTask;
    device->vptr->recv_task = app_device_defaultRecvTask;
    device->vptr->send_task = app_device_defaultSendTask;
    device->vptr->pre_write = NULL;
    device->vptr->post_read = NULL;
    device->vptr->recv_callback = NULL;

    log_info("Device %s initialized", device->filename);

    return 0;
DEVICE_RECV_INIT_FAIL:
    app_buffer_close(device->recv_buffer);
DEVICE_OPEN_FAIL:
    close(device->fd);
DEVICE_SEND_BUFFER_EXIT:
    free(device->send_buffer);
DEVICE_RECV_BUFFER_EXIT:
    free(device->recv_buffer);
DEVICE_VTABLE_EXIT:
    free(device->vptr);
DEVICE_FILENAME_EXIT:
    free(device->filename);
DEVICE_EXIT:
    return -1;
}

int app_device_start(Device *device)
{
    if (device->is_running)
    {
        return -1;
    }

    if (pthread_create(&device->background_thread, NULL, device->vptr->background_task, device) < 0)
    {
        log_error("Failed to create background thread");
        return -1;
    }
    device->is_running = 1;
    log_info("Device %s started", device->filename);
    return 0;
}

int app_device_write(Device *device, void *ptr, int len)
{
    // 写入缓存
    if (app_buffer_write(device->send_buffer, ptr, len) < 0)
    {
        return -1;
    }
    // 注册一个Task，作用是将缓存区的数据写到设备文件描述符
    if (app_task_register(device->vptr->send_task, device) < 0)
    {
        return -1;
    }
    return 0;
}

void app_device_registerRecvCallback(Device *device, int (*recv_callback)(void *, int))
{
    device->vptr->recv_callback = recv_callback;
}

void app_device_stop(Device *device)
{
    if (device->is_running)
    {
        pthread_cancel(device->background_thread);
        pthread_join(device->background_thread, NULL);
    }
    device->is_running = 0;
}

void app_device_close(Device *device)
{
    app_buffer_close(device->send_buffer);
    app_buffer_close(device->recv_buffer);
    close(device->fd);
    free(device->send_buffer);
    free(device->recv_buffer);
    free(device->vptr);
    free(device->filename);
}
