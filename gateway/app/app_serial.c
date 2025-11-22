#include "app_serial.h"
#include <unistd.h>

static int app_serial_setCS8(SerialDevice *serial_device)
{
    // 设置串口的数据为为8bit
    struct termios options;
    // 获取串口的属性结构体
    if (tcgetattr(serial_device->super.fd, &options) != 0)
    {
        return -1;
    }

    // 设置串口数据位
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // 将属性结构体写回串口
    return tcsetattr(serial_device->super.fd, TCSAFLUSH, &options);
}

static int app_serial_setRaw(SerialDevice *serial_device)
{

    struct termios options;
    // 获取串口的属性结构体
    if (tcgetattr(serial_device->super.fd, &options) != 0)
    {
        return -1;
    }

    // 设置串口为原始模式
    cfmakeraw(&options);

    // 将属性结构体写回串口
    return tcsetattr(serial_device->super.fd, TCSAFLUSH, &options);
}

int app_serial_init(SerialDevice *serial_device, char *filename)
{
    if (app_device_init(&serial_device->super, filename) < 0)
    {
        return -1;
    }

    app_serial_setCS8(serial_device);//数据位为8
    app_serial_setBaudRate(serial_device, SERIAL_BAUD_RATE_9600);//波特率9600
    app_serial_setParity(serial_device, PARITY_NONE);//校验位为无校验
    app_serial_setStopBits(serial_device, STOP_BITS_ONE);//停止位为1
    app_serial_setRaw(serial_device);// 设置为原始模式
    app_serial_setBlockMode(serial_device, 0);//非阻塞

    // 清空缓冲区，并刷新串口，使新设置生效
    return tcflush(serial_device->super.fd, TCIOFLUSH);
}

int app_serial_setBaudRate(SerialDevice *serial_device, SerialBaudRate baud_rate)
{
    struct termios options;
    // 获取串口的属性结构体
    if (tcgetattr(serial_device->super.fd, &options) != 0)
    {
        return -1;
    }

    serial_device->baud_rate = baud_rate;

    // 设置串口的波特率
    switch (baud_rate)
    {
    case SERIAL_BAUD_RATE_9600:
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        break;
    case SERIAL_BAUD_RATE_115200:
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        break;
    default:
        break;
    }

    // 将属性结构体写回串口
    return tcsetattr(serial_device->super.fd, TCSAFLUSH, &options);
}

int app_serial_setStopBits(SerialDevice *serial_device, StopBits stop_bits)
{
    struct termios options;
    // 获取串口的属性结构体
    if (tcgetattr(serial_device->super.fd, &options) != 0)
    {
        return -1;
    }

    serial_device->stop_bits = stop_bits;

    // 设置串口的停止位
    options.c_cflag &= ~CSTOPB;
    options.c_cflag |= stop_bits;

    // 将属性结构体写回串口
    return tcsetattr(serial_device->super.fd, TCSAFLUSH, &options);
}

int app_serial_setParity(SerialDevice *serial_device, Parity parity)
{
    struct termios options;
    // 获取串口的属性结构体
    if (tcgetattr(serial_device->super.fd, &options) != 0)
    {
        return -1;
    }

    serial_device->parity = parity;

    // 设置串口的校验位
    options.c_cflag &= ~(PARENB | PARODD);
    options.c_cflag |= parity;

    // 将属性结构体写回串口
    return tcsetattr(serial_device->super.fd, TCSAFLUSH, &options);
}

int app_serial_flush(SerialDevice *serial_device)
{
    return tcflush(serial_device->super.fd, TCIOFLUSH);
}

int app_serial_setBlockMode(SerialDevice *serial_device, int block_mode)
{
    struct termios options;
    // 获取串口的属性结构体
    if (tcgetattr(serial_device->super.fd, &options) != 0)
    {
        return -1;
    }

    // 设置串口的阻塞/非阻塞
    if (block_mode)
    {
        // 阻塞模式
        options.c_cc[VTIME] = 0;//无限期等待
        options.c_cc[VMIN] = 1; ///读取一个字符
    }
    else
    {
        // 非阻塞模式
        options.c_cc[VTIME] = 5;//5秒内无数据则返回，但是c_cc[VMIN] = 0，所以实际上不等待
        options.c_cc[VMIN] = 0; ///不等待
    }

    // 将属性结构体写回串口
    return tcsetattr(serial_device->super.fd, TCSAFLUSH, &options);
}
