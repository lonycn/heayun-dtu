/**
 * @file uart.c
 * @brief 憨云DTU UART驱动实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * NANO100B UART驱动实现，专为Modbus通信优化
 */

#include "system.h"
#include "uart.h"
#include "gpio.h"
#include <string.h> // for memset

// ============================================================================
// NANO100B UART寄存器定义
// ============================================================================

#define UART_BASE_ADDR 0x40070000
#define UART_PORT_OFFSET 0x10000

// UART寄存器偏移
#define UART_THR_OFFSET 0x00  // 发送保持寄存器
#define UART_RBR_OFFSET 0x00  // 接收缓冲寄存器
#define UART_IER_OFFSET 0x04  // 中断使能寄存器
#define UART_FCR_OFFSET 0x08  // FIFO控制寄存器
#define UART_LCR_OFFSET 0x0C  // 线路控制寄存器
#define UART_MCR_OFFSET 0x10  // Modem控制寄存器
#define UART_LSR_OFFSET 0x14  // 线路状态寄存器
#define UART_MSR_OFFSET 0x18  // Modem状态寄存器
#define UART_TOR_OFFSET 0x20  // 超时寄存器
#define UART_BAUD_OFFSET 0x24 // 波特率分频寄存器

// 寄存器访问宏
#define UART_PORT_BASE(port) (UART_BASE_ADDR + (port) * UART_PORT_OFFSET)
#define UART_THR(port) (*(volatile uint32_t *)(UART_PORT_BASE(port) + UART_THR_OFFSET))
#define UART_RBR(port) (*(volatile uint32_t *)(UART_PORT_BASE(port) + UART_RBR_OFFSET))
#define UART_IER(port) (*(volatile uint32_t *)(UART_PORT_BASE(port) + UART_IER_OFFSET))
#define UART_FCR(port) (*(volatile uint32_t *)(UART_PORT_BASE(port) + UART_FCR_OFFSET))
#define UART_LCR(port) (*(volatile uint32_t *)(UART_PORT_BASE(port) + UART_LCR_OFFSET))
#define UART_LSR(port) (*(volatile uint32_t *)(UART_PORT_BASE(port) + UART_LSR_OFFSET))
#define UART_BAUD(port) (*(volatile uint32_t *)(UART_PORT_BASE(port) + UART_BAUD_OFFSET))

// LSR寄存器位定义
#define UART_LSR_RX_READY (1 << 0)   // 接收数据就绪
#define UART_LSR_OVERRUN (1 << 1)    // 溢出错误
#define UART_LSR_PARITY_ERR (1 << 2) // 奇偶校验错误
#define UART_LSR_FRAME_ERR (1 << 3)  // 帧错误
#define UART_LSR_BREAK (1 << 4)      // 中断信号
#define UART_LSR_TX_EMPTY (1 << 5)   // 发送FIFO空
#define UART_LSR_TX_IDLE (1 << 6)    // 发送器空闲

// UART引脚定义
#define UART0_TX_PORT GPIO_PORT_A // UART0 TX端口
#define UART0_TX_PIN 1            // UART0 TX引脚
#define UART0_RX_PORT GPIO_PORT_A // UART0 RX端口
#define UART0_RX_PIN 2            // UART0 RX引脚

// ============================================================================
// 全局变量和缓冲区
// ============================================================================

// UART环形缓冲区结构
typedef struct
{
    uint8_t buffer[UART_RX_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
    bool overflow;
} uart_ring_buffer_t;

// UART端口控制块
typedef struct
{
    uart_config_t config;           // 配置信息
    uart_status_t status;           // 当前状态
    uart_ring_buffer_t rx_buffer;   // 接收缓冲区
    uart_ring_buffer_t tx_buffer;   // 发送缓冲区
    uart_rx_callback_t rx_callback; // 接收回调
    uint32_t tx_count;              // 发送计数
    uint32_t rx_count;              // 接收计数
    uint32_t error_count;           // 错误计数
    bool initialized;               // 初始化标志
} uart_control_block_t;

// UART控制块数组
static uart_control_block_t uart_cb[UART_PORT_COUNT] = {0};

// UART初始化状态
static bool uart_module_initialized = false;

// ============================================================================
// 内部函数
// ============================================================================

/**
 * @brief 检查UART端口是否有效
 * @param port UART端口
 * @return true: 有效, false: 无效
 */
static bool uart_is_valid_port(uart_port_t port)
{
    return (port < UART_PORT_COUNT);
}

/**
 * @brief 计算波特率分频值
 * @param baudrate 波特率
 * @return 分频值
 */
static uint32_t uart_calc_baudrate_div(uart_baudrate_t baudrate)
{
    // NANO100B UART波特率计算: UART_CLK / (16 * BAUD_DIV) = baudrate
    // 假设UART_CLK = 32MHz (系统时钟)
    uint32_t uart_clk = 32000000;
    uint32_t div = uart_clk / (16 * (uint32_t)baudrate);
    return div;
}

/**
 * @brief 环形缓冲区初始化
 * @param buffer 缓冲区指针
 */
static void ring_buffer_init(uart_ring_buffer_t *buffer)
{
    if (!buffer)
        return;

    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->overflow = false;
}

/**
 * @brief 向环形缓冲区写入一个字节
 * @param buffer 缓冲区指针
 * @param data 数据
 * @return true: 成功, false: 缓冲区满
 */
static bool ring_buffer_put(uart_ring_buffer_t *buffer, uint8_t data)
{
    if (!buffer)
        return false;

    if (buffer->count >= UART_RX_BUFFER_SIZE)
    {
        buffer->overflow = true;
        return false; // 缓冲区满
    }

    buffer->buffer[buffer->head] = data;
    buffer->head = (buffer->head + 1) % UART_RX_BUFFER_SIZE;
    buffer->count++;

    return true;
}

/**
 * @brief 从环形缓冲区读取一个字节
 * @param buffer 缓冲区指针
 * @param data 数据指针
 * @return true: 成功, false: 缓冲区空
 */
static bool ring_buffer_get(uart_ring_buffer_t *buffer, uint8_t *data)
{
    if (!buffer || !data)
        return false;

    if (buffer->count == 0)
    {
        return false; // 缓冲区空
    }

    *data = buffer->buffer[buffer->tail];
    buffer->tail = (buffer->tail + 1) % UART_RX_BUFFER_SIZE;
    buffer->count--;

    return true;
}

/**
 * @brief 获取环形缓冲区可用数据数量
 * @param buffer 缓冲区指针
 * @return 可用数据字节数
 */
static uint16_t ring_buffer_available(uart_ring_buffer_t *buffer)
{
    if (!buffer)
        return 0;
    return buffer->count;
}

/**
 * @brief 清空环形缓冲区
 * @param buffer 缓冲区指针
 */
static void ring_buffer_flush(uart_ring_buffer_t *buffer)
{
    if (!buffer)
        return;

    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->overflow = false;
}

// ============================================================================
// UART驱动接口实现
// ============================================================================

/**
 * @brief UART模块初始化
 * @return true: 成功, false: 失败
 */
bool uart_init(void)
{
    if (uart_module_initialized)
    {
        return true; // 已经初始化
    }

    // 初始化所有UART控制块
    for (int i = 0; i < UART_PORT_COUNT; i++)
    {
        memset(&uart_cb[i], 0, sizeof(uart_control_block_t));
        ring_buffer_init(&uart_cb[i].rx_buffer);
        ring_buffer_init(&uart_cb[i].tx_buffer);
        uart_cb[i].status = UART_STATUS_OK;
    }

    uart_module_initialized = true;

    debug_printf("[UART] UART module initialized\n");
    return true;
}

/**
 * @brief 配置UART端口
 * @param config UART配置结构体
 * @return true: 成功, false: 失败
 */
bool uart_config(const uart_config_t *config)
{
    if (!config || !uart_is_valid_port(config->port))
    {
        return false;
    }

    uart_port_t port = config->port;
    uart_control_block_t *cb = &uart_cb[port];

    // 保存配置
    cb->config = *config;

    // 配置GPIO引脚
    // 这里需要根据NANO100B的引脚映射进行配置
    // 简化实现，只配置UART0的GPIO
    if (port == UART_PORT_0)
    {
        gpio_config_t tx_config = {
            .port = UART0_TX_PORT,
            .pin = UART0_TX_PIN,
            .mode = GPIO_MODE_OUTPUT,
            .initial_state = true,
            .int_type = GPIO_INT_RISING,
            .callback = NULL};
        gpio_config_pin(&tx_config);

        gpio_config_t rx_config = {
            .port = UART0_RX_PORT,
            .pin = UART0_RX_PIN,
            .mode = GPIO_MODE_INPUT_PULLUP,
            .initial_state = false,
            .int_type = GPIO_INT_RISING,
            .callback = NULL};
        gpio_config_pin(&rx_config);
    }

    // 计算并设置波特率
    uint32_t baud_div = uart_calc_baudrate_div(config->baudrate);
    UART_BAUD(port) = baud_div;

    // 配置线路控制寄存器
    uint32_t lcr_value = 0;

    // 数据位设置
    switch (config->databits)
    {
    case UART_DATABITS_5:
        lcr_value |= 0x00;
        break;
    case UART_DATABITS_6:
        lcr_value |= 0x01;
        break;
    case UART_DATABITS_7:
        lcr_value |= 0x02;
        break;
    case UART_DATABITS_8:
        lcr_value |= 0x03;
        break;
    }

    // 停止位设置
    if (config->stopbits == UART_STOPBITS_2)
    {
        lcr_value |= (1 << 2);
    }

    // 奇偶校验设置
    if (config->parity != UART_PARITY_NONE)
    {
        lcr_value |= (1 << 3); // 使能奇偶校验
        if (config->parity == UART_PARITY_EVEN)
        {
            lcr_value |= (1 << 4); // 偶校验
        }
    }

    UART_LCR(port) = lcr_value;

    // 配置FIFO
    UART_FCR(port) = 0x07; // 使能FIFO，清空收发FIFO

    // 配置中断使能
    uint32_t ier_value = 0;
    if (config->enable_rx_int)
    {
        ier_value |= 0x01; // 接收数据中断
    }
    if (config->enable_tx_int)
    {
        ier_value |= 0x02; // 发送缓冲区空中断
    }
    UART_IER(port) = ier_value;

    cb->initialized = true;
    cb->status = UART_STATUS_OK;

    debug_printf("[UART] UART%d configured: %d bps, %dd%ds%s\n",
                 port, config->baudrate, config->databits, config->stopbits,
                 config->parity == UART_PARITY_NONE ? "N" : (config->parity == UART_PARITY_EVEN ? "E" : "O"));

    return true;
}

/**
 * @brief 启用UART端口
 * @param port UART端口
 * @param enable true: 启用, false: 禁用
 * @return true: 成功, false: 失败
 */
bool uart_enable(uart_port_t port, bool enable)
{
    if (!uart_is_valid_port(port))
    {
        return false;
    }

    // TODO: 在NANO100B中使能/禁用UART
    // 这里需要参考具体的寄存器配置

    uart_cb[port].status = enable ? UART_STATUS_OK : UART_STATUS_ERROR;

    return true;
}

/**
 * @brief UART发送数据 (阻塞方式)
 * @param port UART端口
 * @param data 发送数据指针
 * @param length 数据长度
 * @param timeout_ms 超时时间(毫秒)
 * @return 实际发送的字节数
 */
uint16_t uart_send_blocking(uart_port_t port, const uint8_t *data, uint16_t length, uint32_t timeout_ms)
{
    if (!uart_is_valid_port(port) || !data || length == 0)
    {
        return 0;
    }

    uart_control_block_t *cb = &uart_cb[port];
    uint16_t sent = 0;
    uint32_t start_time = system_get_tick();

    cb->status = UART_STATUS_BUSY;

    for (uint16_t i = 0; i < length; i++)
    {
        // 等待发送缓冲区准备就绪
        while (!(UART_LSR(port) & UART_LSR_TX_EMPTY))
        {
            if ((system_get_tick() - start_time) >= timeout_ms)
            {
                cb->status = UART_STATUS_TIMEOUT;
                return sent;
            }
            system_delay_ms(1);
        }

        // 发送一个字节
        UART_THR(port) = data[i];
        sent++;
        cb->tx_count++;
    }

    cb->status = UART_STATUS_OK;
    return sent;
}

/**
 * @brief UART发送数据 (非阻塞方式)
 * @param port UART端口
 * @param data 发送数据指针
 * @param length 数据长度
 * @return true: 启动成功, false: 启动失败
 */
bool uart_send_async(uart_port_t port, const uint8_t *data, uint16_t length)
{
    if (!uart_is_valid_port(port) || !data || length == 0)
    {
        return false;
    }

    uart_control_block_t *cb = &uart_cb[port];

    // 将数据放入发送缓冲区
    for (uint16_t i = 0; i < length; i++)
    {
        if (!ring_buffer_put(&cb->tx_buffer, data[i]))
        {
            return false; // 缓冲区满
        }
    }

    // 启动发送中断 (如果发送器空闲)
    if (UART_LSR(port) & UART_LSR_TX_EMPTY)
    {
        uint8_t tx_data;
        if (ring_buffer_get(&cb->tx_buffer, &tx_data))
        {
            UART_THR(port) = tx_data;
            cb->tx_count++;
        }
    }

    return true;
}

/**
 * @brief UART接收数据 (阻塞方式)
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param max_length 最大接收长度
 * @param timeout_ms 超时时间(毫秒)
 * @return 实际接收的字节数
 */
uint16_t uart_receive_blocking(uart_port_t port, uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms)
{
    if (!uart_is_valid_port(port) || !buffer || max_length == 0)
    {
        return 0;
    }

    uart_control_block_t *cb = &uart_cb[port];
    uint16_t received = 0;
    uint32_t start_time = system_get_tick();

    while (received < max_length)
    {
        // 首先检查缓冲区中是否有数据
        if (ring_buffer_get(&cb->rx_buffer, &buffer[received]))
        {
            received++;
            cb->rx_count++;
            continue;
        }

        // 检查硬件FIFO中是否有数据
        if (UART_LSR(port) & UART_LSR_RX_READY)
        {
            buffer[received] = (uint8_t)UART_RBR(port);
            received++;
            cb->rx_count++;
            continue;
        }

        // 检查超时
        if ((system_get_tick() - start_time) >= timeout_ms)
        {
            cb->status = UART_STATUS_TIMEOUT;
            break;
        }

        system_delay_ms(1);
    }

    return received;
}

/**
 * @brief UART接收数据 (非阻塞方式)
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param max_length 最大接收长度
 * @return 实际接收的字节数
 */
uint16_t uart_receive_available(uart_port_t port, uint8_t *buffer, uint16_t max_length)
{
    if (!uart_is_valid_port(port) || !buffer || max_length == 0)
    {
        return 0;
    }

    uart_control_block_t *cb = &uart_cb[port];
    uint16_t received = 0;

    // 从缓冲区中读取数据
    while (received < max_length && ring_buffer_get(&cb->rx_buffer, &buffer[received]))
    {
        received++;
        cb->rx_count++;
    }

    return received;
}

/**
 * @brief 检查接收缓冲区是否有数据
 * @param port UART端口
 * @return 可用数据字节数
 */
uint16_t uart_get_rx_count(uart_port_t port)
{
    if (!uart_is_valid_port(port))
    {
        return 0;
    }

    return ring_buffer_available(&uart_cb[port].rx_buffer);
}

/**
 * @brief 检查发送缓冲区是否为空
 * @param port UART端口
 * @return true: 空, false: 非空
 */
bool uart_is_tx_empty(uart_port_t port)
{
    if (!uart_is_valid_port(port))
    {
        return true;
    }

    uart_control_block_t *cb = &uart_cb[port];
    return (ring_buffer_available(&cb->tx_buffer) == 0) &&
           (UART_LSR(port) & UART_LSR_TX_IDLE);
}

/**
 * @brief 清空接收缓冲区
 * @param port UART端口
 * @return true: 成功, false: 失败
 */
bool uart_flush_rx(uart_port_t port)
{
    if (!uart_is_valid_port(port))
    {
        return false;
    }

    ring_buffer_flush(&uart_cb[port].rx_buffer);

    // 清空硬件FIFO
    while (UART_LSR(port) & UART_LSR_RX_READY)
    {
        (void)UART_RBR(port); // 读取并丢弃数据
    }

    return true;
}

/**
 * @brief 等待发送完成
 * @param port UART端口
 * @param timeout_ms 超时时间(毫秒)
 * @return true: 完成, false: 超时
 */
bool uart_wait_tx_complete(uart_port_t port, uint32_t timeout_ms)
{
    if (!uart_is_valid_port(port))
    {
        return false;
    }

    uint32_t start_time = system_get_tick();

    while (!uart_is_tx_empty(port))
    {
        if ((system_get_tick() - start_time) >= timeout_ms)
        {
            return false; // 超时
        }
        system_delay_ms(1);
    }

    return true;
}

// ============================================================================
// Modbus专用接口实现
// ============================================================================

/**
 * @brief 配置Modbus UART (典型配置: 9600, 8N1)
 * @param port UART端口
 * @param baudrate 波特率
 * @return true: 成功, false: 失败
 */
bool uart_config_modbus(uart_port_t port, uart_baudrate_t baudrate)
{
    uart_config_t config = {
        .port = port,
        .baudrate = baudrate,
        .databits = UART_DATABITS_8,
        .stopbits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE,
        .enable_rx_int = true,
        .enable_tx_int = false};

    return uart_config(&config);
}

/**
 * @brief Modbus帧发送 (自动计算帧间隔)
 * @param port UART端口
 * @param frame Modbus帧数据
 * @param length 帧长度
 * @return true: 成功, false: 失败
 */
bool uart_send_modbus_frame(uart_port_t port, const uint8_t *frame, uint16_t length)
{
    if (!uart_is_valid_port(port) || !frame || length == 0)
    {
        return false;
    }

    // Modbus帧间间隔计算 (至少3.5个字符时间)
    uart_control_block_t *cb = &uart_cb[port];
    uint32_t char_time_us = (11 * 1000000) / cb->config.baudrate; // 11位/字符 (8N1 + 起始停止位)
    uint32_t frame_gap_us = char_time_us * 4;                     // 4个字符时间

    // 确保前一帧发送完成
    uart_wait_tx_complete(port, 100);

    // 等待帧间间隔
    if (frame_gap_us >= 1000)
    {
        system_delay_ms(frame_gap_us / 1000);
    }

    // 发送帧数据
    uint16_t sent = uart_send_blocking(port, frame, length, 1000);

    return (sent == length);
}

/**
 * @brief Modbus帧接收 (检测帧结束)
 * @param port UART端口
 * @param buffer 接收缓冲区
 * @param max_length 最大长度
 * @param timeout_ms 超时时间(毫秒)
 * @return 接收到的帧长度，0表示超时
 */
uint16_t uart_receive_modbus_frame(uart_port_t port, uint8_t *buffer, uint16_t max_length, uint32_t timeout_ms)
{
    if (!uart_is_valid_port(port) || !buffer || max_length == 0)
    {
        return 0;
    }

    uint16_t received = 0;
    uint32_t last_rx_time = system_get_tick();
    uint32_t start_time = last_rx_time;

    while ((system_get_tick() - start_time) < timeout_ms)
    {
        uint8_t data;

        // 尝试接收一个字节
        if (uart_receive_available(port, &data, 1) == 1)
        {
            if (received < max_length)
            {
                buffer[received++] = data;
                last_rx_time = system_get_tick();
            }
        }

        // 检查帧结束条件 (3.5个字符时间无数据)
        if (received > 0)
        {
            uint32_t silence_time = system_get_tick() - last_rx_time;
            if (silence_time >= UART_FRAME_TIMEOUT_MS)
            {
                break; // 帧结束
            }
        }

        system_delay_ms(1);
    }

    return received;
}

// ============================================================================
// 调试输出接口实现
// ============================================================================

/**
 * @brief 配置调试UART
 * @param port UART端口 (通常是UART0)
 * @return true: 成功, false: 失败
 */
bool uart_config_debug(uart_port_t port)
{
    uart_config_t config = {
        .port = port,
        .baudrate = UART_BAUDRATE_115200,
        .databits = UART_DATABITS_8,
        .stopbits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE,
        .enable_rx_int = false,
        .enable_tx_int = false};

    return uart_config(&config);
}

/**
 * @brief 调试字符输出
 * @param port UART端口
 * @param ch 字符
 */
void uart_putchar(uart_port_t port, char ch)
{
    if (!uart_is_valid_port(port))
    {
        return;
    }

    // 等待发送缓冲区准备就绪
    while (!(UART_LSR(port) & UART_LSR_TX_EMPTY))
    {
        // 忙等待
    }

    UART_THR(port) = (uint8_t)ch;
}

/**
 * @brief 调试字符串输出
 * @param port UART端口
 * @param str 字符串
 */
void uart_puts(uart_port_t port, const char *str)
{
    if (!str)
        return;

    while (*str)
    {
        uart_putchar(port, *str++);
    }
}

/**
 * @brief 调试hex数据输出
 * @param port UART端口
 * @param data 数据指针
 * @param length 数据长度
 */
void uart_print_hex(uart_port_t port, const uint8_t *data, uint16_t length)
{
    if (!data)
        return;

    for (uint16_t i = 0; i < length; i++)
    {
        char hex_str[4];
        // 简化的hex转换
        uint8_t high = (data[i] >> 4) & 0x0F;
        uint8_t low = data[i] & 0x0F;

        hex_str[0] = (high < 10) ? ('0' + high) : ('A' + high - 10);
        hex_str[1] = (low < 10) ? ('0' + low) : ('A' + low - 10);
        hex_str[2] = ' ';
        hex_str[3] = '\0';

        uart_puts(port, hex_str);
    }
    uart_puts(port, "\n");
}

// ============================================================================
// 中断和回调接口实现
// ============================================================================

/**
 * @brief 设置接收回调函数
 * @param port UART端口
 * @param callback 回调函数
 * @return true: 成功, false: 失败
 */
bool uart_set_rx_callback(uart_port_t port, uart_rx_callback_t callback)
{
    if (!uart_is_valid_port(port))
    {
        return false;
    }

    uart_cb[port].rx_callback = callback;
    return true;
}

/**
 * @brief UART中断处理函数 (在中断服务程序中调用)
 * @param port UART端口
 */
void uart_interrupt_handler(uart_port_t port)
{
    if (!uart_is_valid_port(port))
    {
        return;
    }

    uart_control_block_t *cb = &uart_cb[port];

    // 处理接收中断
    if (UART_LSR(port) & UART_LSR_RX_READY)
    {
        uint8_t data = (uint8_t)UART_RBR(port);

        if (!ring_buffer_put(&cb->rx_buffer, data))
        {
            cb->error_count++; // 缓冲区溢出
        }

        // 调用回调函数 (简化实现，实际应该在帧完成时调用)
        if (cb->rx_callback)
        {
            uint8_t temp_buffer[1] = {data};
            cb->rx_callback(port, temp_buffer, 1);
        }
    }

    // 处理发送中断
    if (UART_LSR(port) & UART_LSR_TX_EMPTY)
    {
        uint8_t tx_data;
        if (ring_buffer_get(&cb->tx_buffer, &tx_data))
        {
            UART_THR(port) = tx_data;
            cb->tx_count++;
        }
    }

    // 处理错误
    uint32_t lsr = UART_LSR(port);
    if (lsr & (UART_LSR_OVERRUN | UART_LSR_PARITY_ERR | UART_LSR_FRAME_ERR))
    {
        cb->error_count++;
        if (lsr & UART_LSR_OVERRUN)
            cb->status = UART_STATUS_OVERRUN;
        if (lsr & UART_LSR_PARITY_ERR)
            cb->status = UART_STATUS_PARITY_ERROR;
        if (lsr & UART_LSR_FRAME_ERR)
            cb->status = UART_STATUS_FRAME_ERROR;
    }
}

// ============================================================================
// 状态和调试接口实现
// ============================================================================

/**
 * @brief 获取UART状态
 * @param port UART端口
 * @return UART状态
 */
uart_status_t uart_get_status(uart_port_t port)
{
    if (!uart_is_valid_port(port))
    {
        return UART_STATUS_ERROR;
    }

    return uart_cb[port].status;
}

/**
 * @brief 获取UART统计信息
 * @param port UART端口
 * @param tx_count 发送字节计数
 * @param rx_count 接收字节计数
 * @param error_count 错误计数
 */
void uart_get_stats(uart_port_t port, uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count)
{
    if (!uart_is_valid_port(port))
    {
        return;
    }

    uart_control_block_t *cb = &uart_cb[port];

    if (tx_count)
        *tx_count = cb->tx_count;
    if (rx_count)
        *rx_count = cb->rx_count;
    if (error_count)
        *error_count = cb->error_count;
}

/**
 * @brief 打印UART状态信息 (调试用)
 * @param port UART端口
 */
void uart_print_status(uart_port_t port)
{
    if (!uart_is_valid_port(port))
    {
        return;
    }

    uart_control_block_t *cb = &uart_cb[port];

    debug_printf("\n[UART] UART%d Status:\n", port);
    debug_printf("Initialized: %s\n", cb->initialized ? "Yes" : "No");
    debug_printf("Status: %d\n", cb->status);
    debug_printf("Baudrate: %d\n", cb->config.baudrate);
    debug_printf("TX Count: %lu\n", cb->tx_count);
    debug_printf("RX Count: %lu\n", cb->rx_count);
    debug_printf("Error Count: %lu\n", cb->error_count);
    debug_printf("RX Buffer: %d/%d\n",
                 ring_buffer_available(&cb->rx_buffer), UART_RX_BUFFER_SIZE);
}

/**
 * @brief 打印所有UART状态 (调试用)
 */
void uart_print_all_status(void)
{
    debug_printf("\n[UART] All UART Status:\n");
    debug_printf("Module initialized: %s\n", uart_module_initialized ? "Yes" : "No");

    for (int i = 0; i < UART_PORT_COUNT; i++)
    {
        if (uart_cb[i].initialized)
        {
            uart_print_status((uart_port_t)i);
        }
    }
    debug_printf("\n");
}