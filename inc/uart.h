/**
 * @file uart.h
 * @brief 憨云DTU UART驱动接口
 * @version 1.0.0
 * @date 2025-03-28
 *
 * NANO100B UART驱动，专为Modbus通信和调试输出优化
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// UART配置定义
// ============================================================================

// UART端口定义
typedef enum
{
    UART_PORT_0 = 0, // UART0 - 调试输出
    UART_PORT_1 = 1, // UART1 - Modbus通信
    UART_PORT_2 = 2, // UART2 - 扩展通信
    UART_PORT_3 = 3, // UART3 - 扩展通信
    UART_PORT_4 = 4, // UART4 - 扩展通信
    UART_PORT_COUNT = 5
} uart_port_t;

// UART波特率定义
typedef enum
{
    UART_BAUDRATE_9600 = 9600,
    UART_BAUDRATE_19200 = 19200,
    UART_BAUDRATE_38400 = 38400,
    UART_BAUDRATE_57600 = 57600,
    UART_BAUDRATE_115200 = 115200,
    UART_BAUDRATE_230400 = 230400,
    UART_BAUDRATE_460800 = 460800
} uart_baudrate_t;

// UART数据位
typedef enum
{
    UART_DATABITS_5 = 5,
    UART_DATABITS_6 = 6,
    UART_DATABITS_7 = 7,
    UART_DATABITS_8 = 8
} uart_databits_t;

// UART停止位
typedef enum
{
    UART_STOPBITS_1 = 1,
    UART_STOPBITS_2 = 2
} uart_stopbits_t;

// UART奇偶校验
typedef enum
{
    UART_PARITY_NONE = 0, // 无校验
    UART_PARITY_ODD = 1,  // 奇校验
    UART_PARITY_EVEN = 2  // 偶校验
} uart_parity_t;

// UART配置结构体
typedef struct
{
    uart_port_t port;         // UART端口
    uart_baudrate_t baudrate; // 波特率
    uart_databits_t databits; // 数据位
    uart_stopbits_t stopbits; // 停止位
    uart_parity_t parity;     // 奇偶校验
    bool enable_rx_int;       // 接收中断使能
    bool enable_tx_int;       // 发送中断使能
} uart_config_t;

// UART状态
typedef enum
{
    UART_STATUS_OK = 0,          // 正常
    UART_STATUS_BUSY = 1,        // 忙碌
    UART_STATUS_ERROR = 2,       // 错误
    UART_STATUS_TIMEOUT = 3,     // 超时
    UART_STATUS_OVERRUN = 4,     // 数据溢出
    UART_STATUS_FRAME_ERROR = 5, // 帧错误
    UART_STATUS_PARITY_ERROR = 6 // 校验错误
} uart_status_t;

// 接收回调函数类型
typedef void (*uart_rx_callback_t)(uart_port_t port, uint8_t *data, uint16_t length);

// ============================================================================
// UART缓冲区配置 (为8KB RAM优化)
// ============================================================================

#define UART_RX_BUFFER_SIZE 256  // 接收缓冲区大小
#define UART_TX_BUFFER_SIZE 256  // 发送缓冲区大小
#define UART_FRAME_TIMEOUT_MS 10 // 帧超时时间(毫秒)

// ============================================================================
// UART驱动接口
// ============================================================================

/**
 * @brief UART模块初始化
 * @return true: 成功, false: 失败
 */
bool uart_init(void);

/**
 * @brief 配置UART端口
 * @param config UART配置结构体
 * @return true: 成功, false: 失败
 */
bool uart_config(const uart_config_t *config);

/**
 * @brief 启用UART端口
 * @param port UART端口
 * @param enable true: 启用, false: 禁用
 * @return true: 成功, false: 失败
 */
bool uart_enable(uart_port_t port, bool enable);

/**
 * @brief UART发送数据 (阻塞方式)
 * @param port UART端口
 * @param data 发送数据指针
 * @param length 数据长度
 * @param timeout_ms 超时时间(毫秒)
 * @return 实际发送的字节数
 */
uint16_t uart_send_blocking(uart_port_t port, const uint8_t *data, uint16_t length, uint32_t timeout_ms);

/**
 * @brief UART发送数据 (非阻塞方式)
 * @param port UART端口
 * @param data 发送数据指针
 * @param length 数据长度
 * @return true: 启动成功, false: 启动失败
 */
bool uart_send_async(uart_port_t port, const uint8_t *data, uint16_t length);

/**
 * @brief UART接收数据 (阻塞方式)
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param max_length 最大接收长度
 * @param timeout_ms 超时时间(毫秒)
 * @return 实际接收的字节数
 */
uint16_t uart_receive_blocking(uart_port_t port, uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms);

/**
 * @brief UART接收数据 (非阻塞方式)
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param max_length 最大接收长度
 * @return 实际接收的字节数
 */
uint16_t uart_receive_available(uart_port_t port, uint8_t *buffer, uint16_t max_length);

/**
 * @brief 检查接收缓冲区是否有数据
 * @param port UART端口
 * @return 可用数据字节数
 */
uint16_t uart_get_rx_count(uart_port_t port);

/**
 * @brief 检查发送缓冲区是否为空
 * @param port UART端口
 * @return true: 空, false: 非空
 */
bool uart_is_tx_empty(uart_port_t port);

/**
 * @brief 清空接收缓冲区
 * @param port UART端口
 * @return true: 成功, false: 失败
 */
bool uart_flush_rx(uart_port_t port);

/**
 * @brief 等待发送完成
 * @param port UART端口
 * @param timeout_ms 超时时间(毫秒)
 * @return true: 完成, false: 超时
 */
bool uart_wait_tx_complete(uart_port_t port, uint32_t timeout_ms);

// ============================================================================
// Modbus专用接口
// ============================================================================

/**
 * @brief 配置Modbus UART (典型配置: 9600, 8N1)
 * @param port UART端口
 * @param baudrate 波特率
 * @return true: 成功, false: 失败
 */
bool uart_config_modbus(uart_port_t port, uart_baudrate_t baudrate);

/**
 * @brief Modbus帧发送 (自动计算帧间隔)
 * @param port UART端口
 * @param frame Modbus帧数据
 * @param length 帧长度
 * @return true: 成功, false: 失败
 */
bool uart_send_modbus_frame(uart_port_t port, const uint8_t *frame, uint16_t length);

/**
 * @brief Modbus帧接收 (检测帧结束)
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param max_length 最大长度
 * @param timeout_ms 超时时间(毫秒)
 * @return 接收到的帧长度，0表示超时
 */
uint16_t uart_receive_modbus_frame(uart_port_t port, uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms);

// ============================================================================
// 调试输出接口
// ============================================================================

/**
 * @brief 配置调试UART
 * @param port UART端口 (通常是UART0)
 * @return true: 成功, false: 失败
 */
bool uart_config_debug(uart_port_t port);

/**
 * @brief 调试字符输出
 * @param port UART端口
 * @param ch 字符
 */
void uart_putchar(uart_port_t port, char ch);

/**
 * @brief 调试字符串输出
 * @param port UART端口
 * @param str 字符串
 */
void uart_puts(uart_port_t port, const char *str);

/**
 * @brief 调试hex数据输出
 * @param port UART端口
 * @param data 数据指针
 * @param length 数据长度
 */
void uart_print_hex(uart_port_t port, const uint8_t *data, uint16_t length);

// ============================================================================
// 中断和回调接口
// ============================================================================

/**
 * @brief 设置接收回调函数
 * @param port UART端口
 * @param callback 回调函数
 * @return true: 成功, false: 失败
 */
bool uart_set_rx_callback(uart_port_t port, uart_rx_callback_t callback);

/**
 * @brief UART中断处理函数 (在中断服务程序中调用)
 * @param port UART端口
 */
void uart_interrupt_handler(uart_port_t port);

// ============================================================================
// 状态和调试接口
// ============================================================================

/**
 * @brief 获取UART状态
 * @param port UART端口
 * @return UART状态
 */
uart_status_t uart_get_status(uart_port_t port);

/**
 * @brief 获取UART统计信息
 * @param port UART端口
 * @param tx_count 发送字节计数
 * @param rx_count 接收字节计数
 * @param error_count 错误计数
 */
void uart_get_stats(uart_port_t port, uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count);

/**
 * @brief 打印UART状态信息 (调试用)
 * @param port UART端口
 */
void uart_print_status(uart_port_t port);

/**
 * @brief 打印所有UART状态 (调试用)
 */
void uart_print_all_status(void);

#endif // UART_H