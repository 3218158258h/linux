#include "../include/app_device.h"
#include <stdlib.h>
#include "../thirdparty/log.c/log.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

#define BUFFER_LEN 16384

static void *app_device_backgroundTask(void *argv)
{
    unsigned char buf[1024];
    Device *device = argv;
    while (device->is_running)
    {
        int buf_len = read(device->fd, buf, 1024);
        if (buf_len < 0)
        {
            log_warn("read device data error");
            usleep(100000);
            continue;
        }
        
        if (buf_len == 0) {
            usleep(10000);
            continue;
        }

        if (device->vptr->post_read)
        {
            device->vptr->post_read(device, buf, &buf_len);
        }

        if (buf_len > 0)
        {
            app_buffer_write(device->recv_buffer, buf, buf_len);
            app_task_register(device->vptr->recv_task, device);
        }
    }

    return NULL;
}

static void app_device_defaultRecvTask(void *argv)
{
    unsigned char buf[1024];
    Device *device = argv;
    
    if (device->recv_buffer->len < 3) {
        return;
    }
    
    // 先 peek 头部，不移除
    if (app_buffer_peek(device->recv_buffer, buf, 3) < 3) {
        return;
    }
    
    int id_len = buf[1];
    int data_len = buf[2];
    int total_len = id_len + data_len;
    
    // 检查完整消息是否到达
    if (device->recv_buffer->len < 3 + total_len) {
        return;  // 数据不完整，等待更多数据
    }
    
    // 读取头部（从缓冲区移除）
    app_buffer_read(device->recv_buffer, buf, 3);
    
    // 读取剩余数据
    app_buffer_read(device->recv_buffer, buf + 3, total_len);
    
    int buf_len = 3 + total_len;
    
    if (device->vptr && device->vptr->recv_callback) {
        int retry = 0;
        while (device->vptr->recv_callback(buf, buf_len) < 0 && retry < 3)
        {
            usleep(100000);
            retry++;
        }
        if (retry >= 3) {
            log_error("Callback failed after 3 retries");
        }
    }
}

static void app_device_defaultSendTask(void *argv)
{
    unsigned char buf[1024];
    int buf_len = 0;
    Device *device = argv;
    
    if (device->send_buffer->len < 3) {
        return;
    }
    
    app_buffer_read(device->send_buffer, buf, 3);
    
    int id_len = buf[1];
    int data_len = buf[2];
    
    if (device->send_buffer->len < id_len + data_len) {
        log_warn("Incomplete send data");
        return;
    }
    
    app_buffer_read(device->send_buffer, buf + 3, id_len + data_len);
    buf_len = 3 + id_len + data_len;

    if (device->vptr->pre_write)
    {
        device->vptr->pre_write(device, buf, &buf_len);
    }
    if (buf_len > 0)
    {
        write(device->fd, buf, buf_len);
    }
}

int app_device_init(Device *device, char *filename)
{
    if (!device || !filename) {
        log_warn("Invalid device or filename");
        return -1;
    }
    
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

    strcpy(device->filename, filename);
    device->fd = open(device->filename, O_RDWR | O_NOCTTY);
    if (device->fd < 0)
    {
        log_warn("Device open failed: %s", filename);
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
    if (!device) return -1;
    
    if (device->is_running)
    {
        return 0;
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
    if (!device || !ptr || len <= 0) return -1;
    
    if (app_buffer_write(device->send_buffer, ptr, len) < 0)
    {
        return -1;
    }
    if (app_task_register(device->vptr->send_task, device) < 0)
    {
        return -1;
    }
    return 0;
}

void app_device_registerRecvCallback(Device *device, int (*recv_callback)(void *, int))
{
    if (device && device->vptr) {
        device->vptr->recv_callback = recv_callback;
    }
}

void app_device_stop(Device *device)
{
    if (!device) return;
    
    if (device->is_running)
    {
        device->is_running = 0;
        
        if (device->fd >= 0) {
            //shutdown(device->fd, SHUT_RDWR);
            close(device->fd);
            device->fd = -1;
        }
        
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 2;
        
        if (pthread_timedjoin_np(device->background_thread, NULL, &ts) != 0) {
            pthread_cancel(device->background_thread);
            pthread_join(device->background_thread, NULL);
        }
    }
}

void app_device_close(Device *device)
{
    if (!device) return;
    
    app_device_stop(device);
    
    if (device->send_buffer) {
        app_buffer_close(device->send_buffer);
        free(device->send_buffer);
        device->send_buffer = NULL;
    }
    if (device->recv_buffer) {
        app_buffer_close(device->recv_buffer);
        free(device->recv_buffer);
        device->recv_buffer = NULL;
    }
    if (device->fd >= 0) {
        close(device->fd);
        device->fd = -1;
    }
    if (device->vptr) {
        free(device->vptr);
        device->vptr = NULL;
    }
    if (device->filename) {
        free(device->filename);
        device->filename = NULL;
    }
}
