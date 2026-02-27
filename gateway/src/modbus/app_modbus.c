/**
 * @file app_modbus.c
 * @brief Modbus协议栈实现
 */

#include "app_modbus.h"
#include "../thirdparty/log.c/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* CRC16查表法 */
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t modbus_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }
    
    return crc;
}

/* 错误描述 */
static const char *error_strings[] = {
    "Success",
    "Illegal function",
    "Illegal data address",
    "Illegal data value",
    "Slave device failure",
    "Acknowledge",
    "Slave device busy",
    "Unknown error",
    "Memory parity error",
    "Unknown error",
    "Gateway path unavailable",
    "Gateway target device failed"
};

const char *modbus_strerror(uint16_t error_code)
{
    if (error_code <= 11) {
        return error_strings[error_code];
    }
    return "Unknown error";
}

uint16_t modbus_get_last_error(ModbusContext *ctx)
{
    return ctx ? ctx->last_error : 0;
}

/* 配置串口 */
static int configure_serial(int fd, int baud_rate, int data_bits, int stop_bits, char parity)
{
    struct termios tty;
    
    if (tcgetattr(fd, &tty) != 0) {
        log_error("tcgetattr failed: %s", strerror(errno));
        return -1;
    }
    
    /* 波特率 */
    speed_t speed = B9600;
    switch (baud_rate) {
        case 1200:   speed = B1200; break;
        case 2400:   speed = B2400; break;
        case 4800:   speed = B4800; break;
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    /* 数据位 */
    tty.c_cflag &= ~CSIZE;
    switch (data_bits) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        case 8: tty.c_cflag |= CS8; break;
    }
    
    /* 停止位 */
    if (stop_bits == 2) {
        tty.c_cflag |= CSTOPB;
    } else {
        tty.c_cflag &= ~CSTOPB;
    }
    
    /* 校验位 */
    tty.c_cflag &= ~PARENB;
    if (parity == 'E') {
        tty.c_cflag |= PARENB;
        tty.c_cflag &= ~PARODD;
    } else if (parity == 'O') {
        tty.c_cflag |= PARENB;
        tty.c_cflag |= PARODD;
    }
    
    /* 其他设置 */
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    /* 超时设置 */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;  /* 0.5秒 */
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        log_error("tcsetattr failed: %s", strerror(errno));
        return -1;
    }
    
    tcflush(fd, TCIOFLUSH);
    return 0;
}

int modbus_init(ModbusContext *ctx, const ModbusConfig *config)
{
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(ModbusContext));
    
    if (config) {
        memcpy(&ctx->config, config, sizeof(ModbusConfig));
    } else {
        ctx->config.type = MODBUS_TYPE_RTU;
        ctx->config.baud_rate = 9600;
        ctx->config.data_bits = 8;
        ctx->config.stop_bits = 1;
        ctx->config.parity = 'N';
        ctx->config.slave_id = 1;
        ctx->config.timeout_ms = 1000;
        ctx->config.retry_count = 3;
    }
    
    ctx->fd = -1;
    ctx->is_connected = 0;
    ctx->transaction_id = 0;
    
    return 0;
}

int modbus_init_rtu(ModbusContext *ctx, const char *device, int baud_rate)
{
    ModbusConfig config = {0};
    config.type = MODBUS_TYPE_RTU;
    strncpy(config.serial_device, device, sizeof(config.serial_device) - 1);
    config.baud_rate = baud_rate;
    config.data_bits = 8;
    config.stop_bits = 1;
    config.parity = 'N';
    config.slave_id = 1;
    config.timeout_ms = 1000;
    config.retry_count = 3;
    
    return modbus_init(ctx, &config);
}

int modbus_init_tcp(ModbusContext *ctx, const char *host, int port)
{
    ModbusConfig config = {0};
    config.type = MODBUS_TYPE_TCP;
    strncpy(config.host, host, sizeof(config.host) - 1);
    config.port = port;
    config.slave_id = 1;
    config.timeout_ms = 1000;
    config.retry_count = 3;
    
    return modbus_init(ctx, &config);
}

void modbus_close(ModbusContext *ctx)
{
    if (!ctx) return;
    
    modbus_disconnect(ctx);
    memset(ctx, 0, sizeof(ModbusContext));
}

int modbus_connect(ModbusContext *ctx)
{
    if (!ctx) return -1;
    
    if (ctx->is_connected) {
        return 0;
    }
    
    if (ctx->config.type == MODBUS_TYPE_RTU) {
        /* 打开串口 */
        ctx->fd = open(ctx->config.serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
        if (ctx->fd < 0) {
            log_error("Failed to open serial port: %s", ctx->config.serial_device);
            return -1;
        }
        
        /* 配置串口 */
        if (configure_serial(ctx->fd, ctx->config.baud_rate,
                            ctx->config.data_bits, ctx->config.stop_bits,
                            ctx->config.parity) != 0) {
            close(ctx->fd);
            ctx->fd = -1;
            return -1;
        }
        
        log_info("Modbus RTU connected: %s @ %d", 
                 ctx->config.serial_device, ctx->config.baud_rate);
    } else {
        /* TCP连接 */
        ctx->fd = socket(AF_INET, SOCK_STREAM, 0);
        if (ctx->fd < 0) {
            log_error("Failed to create socket: %s", strerror(errno));
            return -1;
        }
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ctx->config.port);
        inet_pton(AF_INET, ctx->config.host, &addr.sin_addr);
        
        if (connect(ctx->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            log_error("Failed to connect to %s:%d", ctx->config.host, ctx->config.port);
            close(ctx->fd);
            ctx->fd = -1;
            return -1;
        }
        
        log_info("Modbus TCP connected: %s:%d", ctx->config.host, ctx->config.port);
    }
    
    ctx->is_connected = 1;
    return 0;
}

void modbus_disconnect(ModbusContext *ctx)
{
    if (!ctx) return;
    
    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }
    
    ctx->is_connected = 0;
}

void modbus_set_slave(ModbusContext *ctx, int slave_id)
{
    if (ctx) {
        ctx->config.slave_id = slave_id;
    }
}

void modbus_set_timeout(ModbusContext *ctx, int timeout_ms)
{
    if (ctx) {
        ctx->config.timeout_ms = timeout_ms;
    }
}

/* 发送RTU请求 */
static int send_rtu_request(ModbusContext *ctx, uint8_t *req, size_t req_len)
{
    /* 添加CRC */
    uint16_t crc = modbus_crc16(req, req_len);
    req[req_len] = crc & 0xFF;
    req[req_len + 1] = (crc >> 8) & 0xFF;
    req_len += 2;
    
    /* 发送 */
    ssize_t ret = write(ctx->fd, req, req_len);
    if (ret != (ssize_t)req_len) {
        log_error("RTU write failed: %zd != %zu", ret, req_len);
        return -1;
    }
    
    tcdrain(ctx->fd);
    return 0;
}

/* 发送TCP请求 */
static int send_tcp_request(ModbusContext *ctx, uint8_t *req, size_t req_len)
{
    /* MBAP头 */
    uint8_t mbap[7];
    ctx->transaction_id++;
    mbap[0] = (ctx->transaction_id >> 8) & 0xFF;
    mbap[1] = ctx->transaction_id & 0xFF;
    mbap[2] = 0;  /* 协议ID */
    mbap[3] = 0;
    mbap[4] = (req_len >> 8) & 0xFF;
    mbap[5] = req_len & 0xFF;
    mbap[6] = ctx->config.slave_id;
    
    /* 发送MBAP头 */
    if (write(ctx->fd, mbap, 7) != 7) {
        log_error("TCP write MBAP failed");
        return -1;
    }
    
    /* 发送PDU */
    if (write(ctx->fd, req, req_len) != (ssize_t)req_len) {
        log_error("TCP write PDU failed");
        return -1;
    }
    
    return 0;
}

/* 接收RTU响应 */
static int recv_rtu_response(ModbusContext *ctx, uint8_t *rsp, size_t max_len, int timeout_ms)
{
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(ctx->fd, &readfds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = select(ctx->fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0) {
        log_error("RTU receive timeout");
        return -1;
    }
    
    ssize_t len = read(ctx->fd, rsp, max_len);
    if (len <= 0) {
        log_error("RTU read failed");
        return -1;
    }
    
    /* 验证CRC */
    if (len < 4) {
        log_error("RTU response too short");
        return -1;
    }
    
    uint16_t crc = modbus_crc16(rsp, len - 2);
    uint16_t recv_crc = rsp[len - 2] | (rsp[len - 1] << 8);
    
    if (crc != recv_crc) {
        log_error("RTU CRC error: 0x%04X != 0x%04X", crc, recv_crc);
        return -1;
    }
    
    return len - 2;  /* 不包含CRC */
}

/* 接收TCP响应 */
static int recv_tcp_response(ModbusContext *ctx, uint8_t *rsp, size_t max_len, int timeout_ms)
{
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(ctx->fd, &readfds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = select(ctx->fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0) {
        log_error("TCP receive timeout");
        return -1;
    }
    
    /* 接收MBAP头 */
    uint8_t mbap[7];
    if (read(ctx->fd, mbap, 7) != 7) {
        log_error("TCP read MBAP failed");
        return -1;
    }
    
    uint16_t len = (mbap[4] << 8) | mbap[5];
    if (len > max_len) {
        log_error("TCP response too large");
        return -1;
    }
    
    /* 接收PDU */
    if (read(ctx->fd, rsp, len) != len) {
        log_error("TCP read PDU failed");
        return -1;
    }
    
    return len;
}

/* 发送请求并接收响应 */
static int modbus_transaction(ModbusContext *ctx, uint8_t *req, size_t req_len,
                              uint8_t *rsp, size_t rsp_max_len)
{
    if (!ctx->is_connected) {
        log_error("Modbus not connected");
        return -1;
    }
    
    int ret;
    
    for (int retry = 0; retry < ctx->config.retry_count; retry++) {
        if (ctx->config.type == MODBUS_TYPE_RTU) {
            ret = send_rtu_request(ctx, req, req_len);
            if (ret != 0) continue;
            
            ret = recv_rtu_response(ctx, rsp, rsp_max_len, ctx->config.timeout_ms);
        } else {
            ret = send_tcp_request(ctx, req, req_len);
            if (ret != 0) continue;
            
            ret = recv_tcp_response(ctx, rsp, rsp_max_len, ctx->config.timeout_ms);
        }
        
        if (ret > 0) {
            /* 检查异常响应 */
            if (rsp[0] & 0x80) {
                ctx->last_error = rsp[1];
                log_warn("Modbus exception: %s", modbus_strerror(rsp[1]));
                return -1;
            }
            return ret;
        }
    }
    
    return -1;
}

int modbus_read_bits(ModbusContext *ctx, int addr, int nb, uint8_t *dest)
{
    if (!ctx || !dest || nb < 1 || nb > 2000) return -1;
    
    // 数组大小应该是6字节
    uint8_t req[6] = {
        ctx->config.slave_id,
        0x01,  /* 读线圈 */
        (addr >> 8) & 0xFF, addr & 0xFF,
        (nb >> 8) & 0xFF, nb & 0xFF
    };
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 6, rsp, sizeof(rsp));
    
    if (ret < 2) return -1;
    
    int byte_count = rsp[1];
    for (int i = 0; i < nb && i < byte_count * 8; i++) {
        dest[i] = (rsp[2 + i / 8] >> (i % 8)) & 0x01;
    }
    
    return nb;
}

int modbus_read_input_bits(ModbusContext *ctx, int addr, int nb, uint8_t *dest)
{
    if (!ctx || !dest || nb < 1 || nb > 2000) return -1;
    
    uint8_t req[6] = {
        ctx->config.slave_id,
        0x02,  /* 读离散输入 */
        (addr >> 8) & 0xFF, addr & 0xFF,
        (nb >> 8) & 0xFF, nb & 0xFF
    };
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 6, rsp, sizeof(rsp));
    
    if (ret < 2) return -1;
    
    int byte_count = rsp[1];
    for (int i = 0; i < nb && i < byte_count * 8; i++) {
        dest[i] = (rsp[2 + i / 8] >> (i % 8)) & 0x01;
    }
    
    return nb;
}

int modbus_read_registers(ModbusContext *ctx, int addr, int nb, uint16_t *dest)
{
    if (!ctx || !dest || nb < 1 || nb > 125) return -1;
    
    uint8_t req[6] = {
        ctx->config.slave_id,
        0x03,  /* 读保持寄存器 */
        (addr >> 8) & 0xFF, addr & 0xFF,
        (nb >> 8) & 0xFF, nb & 0xFF
    };
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 6, rsp, sizeof(rsp));
    
    if (ret < 2) return -1;
    
    int byte_count = rsp[1];
    for (int i = 0; i < nb && i * 2 + 3 < byte_count + 2; i++) {
        dest[i] = (rsp[2 + i * 2] << 8) | rsp[3 + i * 2];
    }
    
    return nb;
}

int modbus_read_input_registers(ModbusContext *ctx, int addr, int nb, uint16_t *dest)
{
    if (!ctx || !dest || nb < 1 || nb > 125) return -1;
    
    uint8_t req[6] = {
        ctx->config.slave_id,
        0x04,  /* 读输入寄存器 */
        (addr >> 8) & 0xFF, addr & 0xFF,
        (nb >> 8) & 0xFF, nb & 0xFF
    };
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 6, rsp, sizeof(rsp));
    
    if (ret < 2) return -1;
    
    int byte_count = rsp[1];
    for (int i = 0; i < nb && i * 2 + 3 < byte_count + 2; i++) {
        dest[i] = (rsp[2 + i * 2] << 8) | rsp[3 + i * 2];
    }
    
    return nb;
}

int modbus_write_bit(ModbusContext *ctx, int addr, int status)
{
    if (!ctx) return -1;
    
    uint8_t req[6] = {
        ctx->config.slave_id,
        0x05,  /* 写单个线圈 */
        (addr >> 8) & 0xFF, addr & 0xFF,
        status ? 0xFF : 0x00, 0x00
    };
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 6, rsp, sizeof(rsp));
    
    return (ret > 0) ? 0 : -1;
}

int modbus_write_register(ModbusContext *ctx, int addr, uint16_t value)
{
    if (!ctx) return -1;
    
    uint8_t req[6] = {
        ctx->config.slave_id,
        0x06,  /* 写单个寄存器 */
        (addr >> 8) & 0xFF, addr & 0xFF,
        (value >> 8) & 0xFF, value & 0xFF
    };
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 6, rsp, sizeof(rsp));
    
    return (ret > 0) ? 0 : -1;
}

int modbus_write_bits(ModbusContext *ctx, int addr, int nb, const uint8_t *src)
{
    if (!ctx || !src || nb < 1 || nb > 1968) return -1;
    
    int byte_count = (nb + 7) / 8;
    uint8_t req[256];
    req[0] = ctx->config.slave_id;
    req[1] = 0x0F;  /* 写多个线圈 */
    req[2] = (addr >> 8) & 0xFF;
    req[3] = addr & 0xFF;
    req[4] = (nb >> 8) & 0xFF;
    req[5] = nb & 0xFF;
    req[6] = byte_count;
    
    for (int i = 0; i < byte_count; i++) {
        req[7 + i] = src[i];
    }
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 7 + byte_count, rsp, sizeof(rsp));
    
    return (ret > 0) ? 0 : -1;
}

int modbus_write_registers(ModbusContext *ctx, int addr, int nb, const uint16_t *src)
{
    if (!ctx || !src || nb < 1 || nb > 123) return -1;
    
    int byte_count = nb * 2;
    uint8_t req[256];
    req[0] = ctx->config.slave_id;
    req[1] = 0x10;  /* 写多个寄存器 */
    req[2] = (addr >> 8) & 0xFF;
    req[3] = addr & 0xFF;
    req[4] = (nb >> 8) & 0xFF;
    req[5] = nb & 0xFF;
    req[6] = byte_count;
    
    for (int i = 0; i < nb; i++) {
        req[7 + i * 2] = (src[i] >> 8) & 0xFF;
        req[8 + i * 2] = src[i] & 0xFF;
    }
    
    uint8_t rsp[256];
    int ret = modbus_transaction(ctx, req, 7 + byte_count, rsp, sizeof(rsp));
    
    return (ret > 0) ? 0 : -1;
}

void modbus_debug_print(ModbusContext *ctx)
{
    if (!ctx) return;
    
    printf("=== Modbus Context ===\n");
    printf("Type: %s\n", ctx->config.type == MODBUS_TYPE_RTU ? "RTU" : "TCP");
    
    if (ctx->config.type == MODBUS_TYPE_RTU) {
        printf("Device: %s\n", ctx->config.serial_device);
        printf("Baud: %d\n", ctx->config.baud_rate);
    } else {
        printf("Host: %s:%d\n", ctx->config.host, ctx->config.port);
    }
    
    printf("Slave ID: %d\n", ctx->config.slave_id);
    printf("Timeout: %d ms\n", ctx->config.timeout_ms);
    printf("Connected: %s\n", ctx->is_connected ? "yes" : "no");
    printf("======================\n");
}
