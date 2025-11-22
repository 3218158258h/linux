#include "app_router.h"
#include "app_mqtt.h"
#include "app_message.h"
#include "thirdparty/log.c/log.h"
#include <string.h>

#define MAX_DEVICE_NUM 10

static Device *devices[MAX_DEVICE_NUM];
static int device_num = 0;

static int app_router_mqttCallback(char *json_str, int len)
{
    unsigned char buf[1024];
    // 将数据转化为二进制
    Message message;
    if (app_message_initByJson(&message, json_str, len) < 0)
    {
        return -1;
    }

    int buf_len = app_message_saveBinary(&message, buf, 1024);

    app_message_free(&message);
    if (buf_len < 0)
    {
        log_warn("app_message_saveBinary: buf not enough");
        return -1;
    }

    // 写入对应设备
    for (int i = 0; i < device_num; i++)
    {
        if ((int)buf[0] == devices[i]->connection_type)
        {
            return app_device_write(devices[i], buf, buf_len);
        }
    }

    log_warn("app_router_mqttCallback: no device found");
    return -1;
}

static int app_router_deviceCallback(void *ptr, int len)
{
    // 将数据转化为Json字符串
    char buf[1024];
    Message message;
    if (app_message_initByBinary(&message, ptr, len) < 0)
    {
        return -1;
    }

    int result = app_message_saveJson(&message, buf, sizeof(buf));

    app_message_free(&message);
    if (result < 0)
    {
        return -1;
    }

    // 通过mqtt发送
    return app_mqtt_send(buf, strlen(buf));
}

int app_router_init()
{
    // 初始化mqtt
    if (app_mqtt_init() < 0)
    {
        return -1;
    }
    // mqtt收到数据后，需要一个回调函数处理数据
    app_mqtt_registerRecvCallback(app_router_mqttCallback);
    return 0;
}

int app_router_registerDevice(Device *device)
{
    devices[device_num++] = device;
    // 设备注册的时候，需要一个回调函数处理设备读取的数据
    app_device_registerRecvCallback(device, app_router_deviceCallback);
    app_device_start(device);
    return 0;
}

void app_router_close()
{
    // 停止并关闭所有设备
    for (int i = 0; i < device_num; i++)
    {
        app_device_stop(devices[i]);
        app_device_close(devices[i]);
    }
    // 关闭mqtt
    app_mqtt_close();
}
