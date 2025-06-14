/**
 * @file modbus.h
 * @brief Modbus RTU协议栈实现 - 憨云DTU专用
 * @version 1.0.0
 * @date 2025-12-06
 *
 * 基于UART驱动的完整Modbus RTU实现，支持主站和从站模式
 */

#ifndef MODBUS_H
#define MODBUS_H

#include <stdint.h>
#include <stdbool.h>
#include "uart.h"

// ============================================================================
// Modbus协议常量定义
// ============================================================================

#define MODBUS_MAX_FRAME_SIZE 256      // 最大帧长度
#define MODBUS_MAX_DATA_SIZE 252       // 最大数据长度
#define MODBUS_CRC_SIZE 2              // CRC校验字节数
#define MODBUS_SLAVE_ID_BROADCAST 0    // 广播地址
#define MODBUS_DEFAULT_TIMEOUT_MS 1000 // 默认超时时间
#define MODBUS_INTER_FRAME_DELAY 3     // 帧间延时(字符时间)

// Modbus功能码定义
#define MODBUS_FC_READ_COILS 0x01               // 读线圈
#define MODBUS_FC_READ_DISCRETE_INPUTS 0x02     // 读离散输入
#define MODBUS_FC_READ_HOLDING_REGISTERS 0x03   // 读保持寄存器
#define MODBUS_FC_READ_INPUT_REGISTERS 0x04     // 读输入寄存器
#define MODBUS_FC_WRITE_SINGLE_COIL 0x05        // 写单个线圈
#define MODBUS_FC_WRITE_SINGLE_REGISTER 0x06    // 写单个寄存器
#define MODBUS_FC_WRITE_MULTIPLE_COILS 0x0F     // 写多个线圈
#define MODBUS_FC_WRITE_MULTIPLE_REGISTERS 0x10 // 写多个寄存器

// Modbus异常码定义
#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION 0x01
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS 0x02
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE 0x03
#define MODBUS_EXCEPTION_SLAVE_DEVICE_FAILURE 0x04
#define MODBUS_EXCEPTION_ACKNOWLEDGE 0x05
#define MODBUS_EXCEPTION_SLAVE_DEVICE_BUSY 0x06
#define MODBUS_EXCEPTION_MEMORY_PARITY_ERROR 0x08
#define MODBUS_EXCEPTION_GATEWAY_PATH_UNAVAILABLE 0x0A
#define MODBUS_EXCEPTION_GATEWAY_TARGET_FAILED 0x0B

// ============================================================================
// 数据类型定义
// ============================================================================

/**
 * @brief Modbus角色类型
 */
typedef enum
{
    MODBUS_ROLE_MASTER = 0, // 主站模式
    MODBUS_ROLE_SLAVE = 1   // 从站模式
} modbus_role_t;

/**
 * @brief Modbus传输状态
 */
typedef enum
{
    MODBUS_STATUS_OK = 0,           // 正常
    MODBUS_STATUS_TIMEOUT,          // 超时
    MODBUS_STATUS_CRC_ERROR,        // CRC错误
    MODBUS_STATUS_FRAME_ERROR,      // 帧格式错误
    MODBUS_STATUS_EXCEPTION,        // 异常响应
    MODBUS_STATUS_INVALID_SLAVE,    // 无效从站地址
    MODBUS_STATUS_INVALID_FUNCTION, // 无效功能码
    MODBUS_STATUS_INVALID_ADDRESS,  // 无效地址
    MODBUS_STATUS_INVALID_DATA,     // 无效数据
    MODBUS_STATUS_BUSY              // 忙碌状态
} modbus_status_t;

/**
 * @brief Modbus配置结构体
 */
typedef struct
{
    uart_port_t uart_port;    // UART端口
    uart_baudrate_t baudrate; // 波特率
    uint8_t slave_id;         // 从站地址 (仅从站模式)
    modbus_role_t role;       // 主站/从站模式
    uint32_t timeout_ms;      // 超时时间
    bool enable_debug;        // 调试输出使能
} modbus_config_t;

/**
 * @brief Modbus请求/响应结构体
 */
typedef struct
{
    uint8_t slave_id;       // 从站地址
    uint8_t function_code;  // 功能码
    uint16_t start_address; // 起始地址
    uint16_t quantity;      // 数量
    uint8_t *data;          // 数据指针
    uint16_t data_length;   // 数据长度
    uint8_t exception_code; // 异常码
} modbus_request_t;

/**
 * @brief 寄存器读写回调函数类型
 */
typedef modbus_status_t (*modbus_read_registers_cb_t)(uint16_t addr, uint16_t quantity, uint16_t *values);
typedef modbus_status_t (*modbus_write_registers_cb_t)(uint16_t addr, uint16_t quantity, const uint16_t *values);
typedef modbus_status_t (*modbus_read_coils_cb_t)(uint16_t addr, uint16_t quantity, uint8_t *values);
typedef modbus_status_t (*modbus_write_coils_cb_t)(uint16_t addr, uint16_t quantity, const uint8_t *values);

/**
 * @brief Modbus从站回调函数结构体
 */
typedef struct
{
    modbus_read_registers_cb_t read_holding_registers;
    modbus_write_registers_cb_t write_holding_registers;
    modbus_read_registers_cb_t read_input_registers;
    modbus_read_coils_cb_t read_coils;
    modbus_write_coils_cb_t write_coils;
    modbus_read_coils_cb_t read_discrete_inputs;
} modbus_slave_callbacks_t;

// ============================================================================
// Modbus协议栈接口
// ============================================================================

/**
 * @brief Modbus协议栈初始化
 * @param config 配置结构体指针
 * @return 初始化状态
 */
modbus_status_t modbus_init(const modbus_config_t *config);

/**
 * @brief Modbus协议栈去初始化
 * @return 操作状态
 */
modbus_status_t modbus_deinit(void);

/**
 * @brief 设置从站回调函数
 * @param callbacks 回调函数结构体指针
 * @return 操作状态
 */
modbus_status_t modbus_set_slave_callbacks(const modbus_slave_callbacks_t *callbacks);

/**
 * @brief Modbus任务处理函数 (需要在主循环中调用)
 */
void modbus_task(void);

// ============================================================================
// Modbus主站接口
// ============================================================================

/**
 * @brief 读取保持寄存器
 * @param slave_id 从站地址
 * @param start_addr 起始地址
 * @param quantity 读取数量
 * @param values 读取结果缓冲区
 * @return 操作状态
 */
modbus_status_t modbus_read_holding_registers(uint8_t slave_id, uint16_t start_addr,
                                              uint16_t quantity, uint16_t *values);

/**
 * @brief 读取输入寄存器
 * @param slave_id 从站地址
 * @param start_addr 起始地址
 * @param quantity 读取数量
 * @param values 读取结果缓冲区
 * @return 操作状态
 */
modbus_status_t modbus_read_input_registers(uint8_t slave_id, uint16_t start_addr,
                                            uint16_t quantity, uint16_t *values);

/**
 * @brief 写入单个寄存器
 * @param slave_id 从站地址
 * @param register_addr 寄存器地址
 * @param value 写入值
 * @return 操作状态
 */
modbus_status_t modbus_write_single_register(uint8_t slave_id, uint16_t register_addr, uint16_t value);

/**
 * @brief 写入多个寄存器
 * @param slave_id 从站地址
 * @param start_addr 起始地址
 * @param quantity 写入数量
 * @param values 写入值数组
 * @return 操作状态
 */
modbus_status_t modbus_write_multiple_registers(uint8_t slave_id, uint16_t start_addr,
                                                uint16_t quantity, const uint16_t *values);

/**
 * @brief 读取线圈
 * @param slave_id 从站地址
 * @param start_addr 起始地址
 * @param quantity 读取数量
 * @param values 读取结果缓冲区
 * @return 操作状态
 */
modbus_status_t modbus_read_coils(uint8_t slave_id, uint16_t start_addr,
                                  uint16_t quantity, uint8_t *values);

/**
 * @brief 写入单个线圈
 * @param slave_id 从站地址
 * @param coil_addr 线圈地址
 * @param value 写入值
 * @return 操作状态
 */
modbus_status_t modbus_write_single_coil(uint8_t slave_id, uint16_t coil_addr, bool value);

// ============================================================================
// Modbus工具函数
// ============================================================================

/**
 * @brief 计算Modbus CRC16校验
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC16校验值
 */
uint16_t modbus_crc16(const uint8_t *data, uint16_t length);

/**
 * @brief 获取Modbus状态描述字符串
 * @param status 状态码
 * @return 状态描述字符串
 */
const char *modbus_status_to_string(modbus_status_t status);

/**
 * @brief 获取Modbus统计信息
 * @param tx_count 发送计数
 * @param rx_count 接收计数
 * @param error_count 错误计数
 */
void modbus_get_statistics(uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count);

/**
 * @brief 重置Modbus统计信息
 */
void modbus_reset_statistics(void);

/**
 * @brief 打印Modbus状态信息 (调试用)
 */
void modbus_print_status(void);

/**
 * @brief Modbus处理函数 (供main.c调用，避免函数名冲突)
 */
void modbus_process(void);

/**
 * @brief 更新系统寄存器数据
 * 将系统状态、传感器数据等更新到Modbus寄存器映射中
 */
void modbus_update_system_registers(void);

// ============================================================================
// 憨云DTU专用接口
// ============================================================================

/**
 * @brief 读取传感器数据 (自定义功能码)
 * @param slave_id 从站地址
 * @param sensor_type 传感器类型
 * @param value 读取结果
 * @return 操作状态
 */
modbus_status_t modbus_read_sensor_data(uint8_t slave_id, uint8_t sensor_type, uint16_t *value);

/**
 * @brief 设置报警阈值 (自定义功能码)
 * @param slave_id 从站地址
 * @param alarm_type 报警类型
 * @param threshold 阈值
 * @return 操作状态
 */
modbus_status_t modbus_set_alarm_threshold(uint8_t slave_id, uint8_t alarm_type, uint16_t threshold);

/**
 * @brief 获取设备信息 (自定义功能码)
 * @param slave_id 从站地址
 * @param device_info 设备信息缓冲区
 * @param buffer_size 缓冲区大小
 * @return 操作状态
 */
modbus_status_t modbus_get_device_info(uint8_t slave_id, char *device_info, uint16_t buffer_size);

#endif // MODBUS_H