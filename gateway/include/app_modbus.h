/**
 * @file app_modbus.h
 * @brief Modbus协议栈 - 支持RTU和TCP
 * 
 * 功能：
 * - Modbus RTU (串行通信)
 * - Modbus TCP (以太网通信)
 * - 完整功能码支持
 * - CRC16校验
 * - 异常处理
 * 
 * @author Gateway Team
 * @version 2.0
 */

#ifndef __APP_MODBUS_H__
#define __APP_MODBUS_H__

#include <stddef.h>
#include <stdint.h>

/* Modbus协议类型 */
typedef enum {
    MODBUS_TYPE_RTU = 0,
    MODBUS_TYPE_TCP = 1
} ModbusType;

/* Modbus异常码 */
typedef enum {
    MODBUS_EX_NONE = 0,
    MODBUS_EX_ILLEGAL_FUNCTION = 1,
    MODBUS_EX_ILLEGAL_DATA_ADDRESS = 2,
    MODBUS_EX_ILLEGAL_DATA_VALUE = 3,
    MODBUS_EX_SLAVE_DEVICE_FAILURE = 4,
    MODBUS_EX_ACKNOWLEDGE = 5,
    MODBUS_EX_SLAVE_DEVICE_BUSY = 6,
    MODBUS_EX_MEMORY_PARITY_ERROR = 8,
    MODBUS_EX_GATEWAY_PATH_UNAVAILABLE = 10,
    MODBUS_EX_GATEWAY_TARGET_DEVICE_FAILED = 11
} ModbusException;

/* Modbus配置 */
typedef struct ModbusConfig {
    ModbusType type;
    
    /* RTU配置 */
    char serial_device[256];
    int baud_rate;
    int data_bits;
    int stop_bits;
    char parity;            /* 'N', 'E', 'O' */
    
    /* TCP配置 */
    char host[256];
    int port;
    
    /* 通用配置 */
    int slave_id;
    int timeout_ms;
    int retry_count;
} ModbusConfig;

/* Modbus上下文 */
typedef struct ModbusContext {
    ModbusConfig config;
    int fd;                 /* 文件描述符 */
    int is_connected;
    int transaction_id;     /* TCP事务ID */
    uint16_t last_error;
} ModbusContext;

/* Modbus响应 */
typedef struct ModbusResponse {
    int slave_id;
    int function_code;
    int exception_code;
    uint8_t *data;
    size_t data_len;
} ModbusResponse;

/* ========== 初始化与销毁 ========== */

/**
 * @brief 初始化Modbus上下文
 * @param ctx 上下文指针
 * @param config 配置
 * @return 0成功, -1失败
 */
int modbus_init(ModbusContext *ctx, const ModbusConfig *config);

/**
 * @brief 初始化RTU
 * @param ctx 上下文指针
 * @param device 串口设备
 * @param baud_rate 波特率
 * @return 0成功, -1失败
 */
int modbus_init_rtu(ModbusContext *ctx, const char *device, int baud_rate);

/**
 * @brief 初始化TCP
 * @param ctx 上下文指针
 * @param host 主机地址
 * @param port 端口
 * @return 0成功, -1失败
 */
int modbus_init_tcp(ModbusContext *ctx, const char *host, int port);

/**
 * @brief 关闭Modbus上下文
 * @param ctx 上下文指针
 */
void modbus_close(ModbusContext *ctx);

/* ========== 连接管理 ========== */

/**
 * @brief 连接
 * @param ctx 上下文指针
 * @return 0成功, -1失败
 */
int modbus_connect(ModbusContext *ctx);

/**
 * @brief 断开连接
 * @param ctx 上下文指针
 */
void modbus_disconnect(ModbusContext *ctx);

/**
 * @brief 设置从站地址
 * @param ctx 上下文指针
 * @param slave_id 从站地址
 */
void modbus_set_slave(ModbusContext *ctx, int slave_id);

/**
 * @brief 设置超时
 * @param ctx 上下文指针
 * @param timeout_ms 超时时间(毫秒)
 */
void modbus_set_timeout(ModbusContext *ctx, int timeout_ms);

/* ========== 读取功能码 ========== */

/**
 * @brief 读线圈 (0x01)
 * @param ctx 上下文指针
 * @param addr 起始地址
 * @param nb 数量
 * @param dest 输出缓冲区
 * @return 成功读取数量, -1失败
 */
int modbus_read_bits(ModbusContext *ctx, int addr, int nb, uint8_t *dest);

/**
 * @brief 读离散输入 (0x02)
 * @param ctx 上下文指针
 * @param addr 起始地址
 * @param nb 数量
 * @param dest 输出缓冲区
 * @return 成功读取数量, -1失败
 */
int modbus_read_input_bits(ModbusContext *ctx, int addr, int nb, uint8_t *dest);

/**
 * @brief 读保持寄存器 (0x03)
 * @param ctx 上下文指针
 * @param addr 起始地址
 * @param nb 数量
 * @param dest 输出缓冲区
 * @return 成功读取数量, -1失败
 */
int modbus_read_registers(ModbusContext *ctx, int addr, int nb, uint16_t *dest);

/**
 * @brief 读输入寄存器 (0x04)
 * @param ctx 上下文指针
 * @param addr 起始地址
 * @param nb 数量
 * @param dest 输出缓冲区
 * @return 成功读取数量, -1失败
 */
int modbus_read_input_registers(ModbusContext *ctx, int addr, int nb, uint16_t *dest);

/* ========== 写入功能码 ========== */

/**
 * @brief 写单个线圈 (0x05)
 * @param ctx 上下文指针
 * @param addr 地址
 * @param status 状态 (0或1)
 * @return 0成功, -1失败
 */
int modbus_write_bit(ModbusContext *ctx, int addr, int status);

/**
 * @brief 写单个寄存器 (0x06)
 * @param ctx 上下文指针
 * @param addr 地址
 * @param value 值
 * @return 0成功, -1失败
 */
int modbus_write_register(ModbusContext *ctx, int addr, uint16_t value);

/**
 * @brief 写多个线圈 (0x0F)
 * @param ctx 上下文指针
 * @param addr 起始地址
 * @param nb 数量
 * @param src 数据源
 * @return 0成功, -1失败
 */
int modbus_write_bits(ModbusContext *ctx, int addr, int nb, const uint8_t *src);

/**
 * @brief 写多个寄存器 (0x10)
 * @param ctx 上下文指针
 * @param addr 起始地址
 * @param nb 数量
 * @param src 数据源
 * @return 0成功, -1失败
 */
int modbus_write_registers(ModbusContext *ctx, int addr, int nb, const uint16_t *src);

/* ========== 错误处理 ========== */

/**
 * @brief 获取最后错误码
 * @param ctx 上下文指针
 * @return 错误码
 */
uint16_t modbus_get_last_error(ModbusContext *ctx);

/**
 * @brief 获取错误描述
 * @param error_code 错误码
 * @return 错误描述字符串
 */
const char *modbus_strerror(uint16_t error_code);

/* ========== 工具函数 ========== */

/**
 * @brief 计算CRC16
 * @param data 数据
 * @param len 长度
 * @return CRC16值
 */
uint16_t modbus_crc16(const uint8_t *data, size_t len);

/**
 * @brief 打印调试信息
 * @param ctx 上下文指针
 */
void modbus_debug_print(ModbusContext *ctx);

#endif /* __APP_MODBUS_H__ */
