#include "app_bluetooth.h"
#include <unistd.h>
#include <string.h>
#include "thirdparty/log.c/log.h"



static unsigned char read_buffer[128];
static int read_buffer_len = 0;

static const unsigned char fix_header[] = {0xF1, 0xDD};

// 从buffer头部删除n字节
static void app_bluetooth_ignoreBuffer(int n)
{
    read_buffer_len -= n;
    memmove(read_buffer, read_buffer + n, read_buffer_len);
}

static int app_bluetooth_waitACK(SerialDevice *serial_device)
{
    // 等待200ms
    usleep(200000);
    unsigned char buf[4];
    read(serial_device->super.fd, buf, 4);
    if (memcmp(buf, "OK\r\n", 4) == 0)
    {
        return 0;
    }
    return -1;
}

int app_bluetooth_setConnectionType(SerialDevice *serial_device)
{
    // 设置类型
    serial_device->super.connection_type = CONNECTION_TYPE_BLE_MESH;
    serial_device->super.vptr->post_read = app_bluetooth_postRead;
    serial_device->super.vptr->pre_write = app_bluetooth_preWrite;
    // 初始化蓝牙连接
    app_serial_setBaudRate(serial_device, SERIAL_BAUD_RATE_9600);
    app_serial_setBlockMode(serial_device, 0);
    app_serial_flush(serial_device);
    if (app_bluetooth_status(serial_device) == 0)
    {
        if (app_bluetooth_setMAddr(serial_device, "0001") < 0)
        {
            log_error("Bluetooth: set maddr failed");
            return -1;
        }
        if (app_bluetooth_setNetID(serial_device, "1111") < 0)
        {
            log_error("Bluetooth: set netid failed");
            return -1;
        }
        if (app_bluetooth_setBaudRate(serial_device, SERIAL_BAUD_RATE_9600) < 0)
        {
            log_error("Bluetooth: set baudrate failed");
            return -1;
        }
        if (app_bluetooth_reset(serial_device) < 0)
        {
            log_error("Bluetooth: reset failed");
            return -1;
        }
    }
    app_serial_setBaudRate(serial_device, SERIAL_BAUD_RATE_115200);
    app_serial_setBlockMode(serial_device, 1);
    // 将蓝牙重启之后的数据抛弃
    sleep(1);
    app_serial_flush(serial_device);
    return 0;
}

int app_bluetooth_status(SerialDevice *serial_device)
{
    write(serial_device->super.fd, "AT\r\n", 4);
    return app_bluetooth_waitACK(serial_device);
}

int app_bluetooth_setBaudRate(SerialDevice *serial_device, SerialBaudRate baud_rate)
{
    char buf[] = "AT+BAUD8\r\n";
    buf[7] = baud_rate;
    write(serial_device->super.fd, buf, 10);
    return app_bluetooth_waitACK(serial_device);
}

int app_bluetooth_reset(SerialDevice *serial_device)
{
    write(serial_device->super.fd, "AT+RESET\r\n", 10);
    return app_bluetooth_waitACK(serial_device);
}

int app_bluetooth_setNetID(SerialDevice *serial_device, char *net_id)
{
    char buf[] = "AT+NETID1111\r\n";
    memcpy(buf + 8, net_id, 4);
    write(serial_device->super.fd, buf, 14);
    return app_bluetooth_waitACK(serial_device);
}

int app_bluetooth_setMAddr(SerialDevice *serial_device, char *m_addr)
{
    char buf[] = "AT+MADDR0001\r\n";
    memcpy(buf + 8, m_addr, 4);
    write(serial_device->super.fd, buf, 14);
    return app_bluetooth_waitACK(serial_device);
}

int app_bluetooth_postRead(Device *device, void *ptr, int *len)
{
    // 由于收到的数据可能不是是完整的，所以需要拼包
    memcpy(read_buffer + read_buffer_len, ptr, *len);
    read_buffer_len += *len;

    // 检查数据帧
    if (read_buffer_len < 4)
    {
        // 返回空数据
        *len = 0;
        return 0;
    }

    for (int i = 0; i < read_buffer_len - 3; i++)
    {
        if (memcmp(read_buffer + i, "OK\r\n", 4) == 0)
        {
            // 找到ACK数据帧
            // 处理ACK
            app_bluetooth_ignoreBuffer(i + 4);
            // 不需要返回数据
            *len = 0;
            return 0;
        }
        else if (memcmp(read_buffer + i, fix_header, 2) == 0)
        {
            // 找到接收数据帧
            // 将buffer头部不需要的数据抛弃
            app_bluetooth_ignoreBuffer(i);
            if (read_buffer_len < read_buffer[2] + 3)
            {
                // 目前数据帧还不完整
                *len = 0;
                return 0;
            }
            // 准备解析数据

            // 首先写入连接类型
            memcpy(ptr, &device->connection_type, 1);
            // 写入id长度
            int temp = 2;
            memcpy(ptr + 1, &temp, 1);
            // 写入数据长度
            temp = read_buffer[2] - 4;
            memcpy(ptr + 2, &temp, 1);
            // 写入peer地址
            memcpy(ptr + 3, read_buffer + 3, 2);
            // 写入数据
            memcpy(ptr + 5, read_buffer + 7, read_buffer[2] - 4);
            *len = read_buffer[2] + 1;
            return 0;
        }
    }

    // 没找到任何想要的数据帧
    *len = 0;
    return 0;
}

int app_bluetooth_preWrite(Device *device, void *ptr, int *len)
{
    int temp = 0;
    unsigned char buf[30];
    // 读取连接类型
    memcpy(&temp, ptr, 1);
    if (temp != CONNECTION_TYPE_BLE_MESH)
    {
        *len = 0;
        return 0;
    }

    // 读取ID长度
    memcpy(&temp, ptr + 1, 1);
    if (temp != 2)
    {
        *len = 0;
        return 0;
    }
    // 首先拼头部
    memcpy(buf, "AT+MESH", 8);
    memcpy(buf + 8, ptr + 3, 2);

    // 读取数据长度
    memcpy(&temp, ptr + 2, 1);
    memcpy(buf + 10, ptr + 5, temp);
    memcpy(buf + 10 + temp, "\r\n", 2);
    *len = temp + 12;
    memcpy(ptr, buf, *len);
    return 0;
}
