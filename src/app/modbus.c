/**
 * @file modbus.c
 * @brief Modbus RTU协议栈实现 - 憨云DTU专用
 * @version 1.0.0
 * @date 2025-12-06
 */

#include "system.h"
#include "modbus.h"
#include "uart.h"
#include <string.h>

// ============================================================================
// 内部数据结构和变量
// ============================================================================

/**
 * @brief Modbus控制块
 */
typedef struct
{
    modbus_config_t config;                   // 配置信息
    modbus_slave_callbacks_t slave_callbacks; // 从站回调函数
    uint8_t tx_buffer[MODBUS_MAX_FRAME_SIZE]; // 发送缓冲区
    uint8_t rx_buffer[MODBUS_MAX_FRAME_SIZE]; // 接收缓冲区
    uint16_t tx_length;                       // 发送数据长度
    uint16_t rx_length;                       // 接收数据长度
    uint32_t last_activity_time;              // 最后活动时间
    uint32_t tx_count;                        // 发送计数
    uint32_t rx_count;                        // 接收计数
    uint32_t error_count;                     // 错误计数
    bool initialized;                         // 初始化标志
    bool busy;                                // 忙碌标志
} modbus_control_t;

// Modbus控制块
static modbus_control_t g_modbus = {0};

// CRC16查表法预计算表
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
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};

// ============================================================================
// 寄存器映射定义
// ============================================================================

// 保持寄存器映射表 (64个寄存器)
static uint16_t g_holding_registers[64] = {
    // 0x0000-0x000F: 系统状态寄存器 (16个)
    [0x00] = 0x0100, // 设备型号 (高字节)
    [0x01] = 0x0001, // 设备型号 (低字节) - 憨云DTU v1.0
    [0x02] = 0x0001, // 软件版本 (v1.0)
    [0x03] = 0x0000, // 硬件版本 (v1.0)
    [0x04] = 0x0000, // 系统状态 (0=正常)
    [0x05] = 0x0000, // 错误代码
    [0x06] = 0x0000, // 运行时间 (高字节，小时)
    [0x07] = 0x0000, // 运行时间 (低字节，分钟)
    [0x08] = 0x0000, // 通信计数 (发送)
    [0x09] = 0x0000, // 通信计数 (接收)
    [0x0A] = 0x0000, // 通信计数 (错误)
    [0x0B] = 0x0000, // 保留
    [0x0C] = 0x0000, // 保留
    [0x0D] = 0x0000, // 保留
    [0x0E] = 0x0000, // 保留
    [0x0F] = 0x0000, // 保留

    // 0x0010-0x001F: 传感器数据寄存器 (16个)
    [0x10] = 0x0000, // 温度1 (0.1°C精度)
    [0x11] = 0x0000, // 湿度1 (0.1%RH精度)
    [0x12] = 0x0000, // 温度2 (0.1°C精度)
    [0x13] = 0x0000, // 湿度2 (0.1%RH精度)
    [0x14] = 0x0000, // 电压1 (0.01V精度)
    [0x15] = 0x0000, // 电流1 (0.001A精度)
    [0x16] = 0x0000, // 电压2 (0.01V精度)
    [0x17] = 0x0000, // 电流2 (0.001A精度)
    [0x18] = 0x0000, // 传感器状态1
    [0x19] = 0x0000, // 传感器状态2
    [0x1A] = 0x0000, // 保留
    [0x1B] = 0x0000, // 保留
    [0x1C] = 0x0000, // 保留
    [0x1D] = 0x0000, // 保留
    [0x1E] = 0x0000, // 保留
    [0x1F] = 0x0000, // 保留

    // 0x0020-0x002F: 配置参数寄存器 (16个)
    [0x20] = 0x0001, // Modbus地址 (1-247)
    [0x21] = 0x2580, // 波特率 (9600)
    [0x22] = 0x0001, // 数据位 (8)
    [0x23] = 0x0000, // 停止位 (1)
    [0x24] = 0x0000, // 校验位 (无)
    [0x25] = 0x03E8, // 采样周期 (1000ms)
    [0x26] = 0x0000, // 报警使能
    [0x27] = 0x0000, // 温度上限 (50.0°C)
    [0x28] = 0x0000, // 温度下限 (-10.0°C)
    [0x29] = 0x0000, // 湿度上限 (95.0%RH)
    [0x2A] = 0x0000, // 湿度下限 (5.0%RH)
    [0x2B] = 0x0000, // 保留
    [0x2C] = 0x0000, // 保留
    [0x2D] = 0x0000, // 保留
    [0x2E] = 0x0000, // 保留
    [0x2F] = 0x0000, // 保留

    // 0x0030-0x003F: 统计和诊断寄存器 (16个)
    [0x30] = 0x0000, // 总运行时间 (高字节)
    [0x31] = 0x0000, // 总运行时间 (低字节)
    [0x32] = 0x0000, // 总采样次数 (高字节)
    [0x33] = 0x0000, // 总采样次数 (低字节)
    [0x34] = 0x0000, // 报警次数
    [0x35] = 0x0000, // 复位次数
    [0x36] = 0x0000, // 最大温度 (0.1°C)
    [0x37] = 0x0000, // 最小温度 (0.1°C)
    [0x38] = 0x0000, // 最大湿度 (0.1%RH)
    [0x39] = 0x0000, // 最小湿度 (0.1%RH)
    [0x3A] = 0x0000, // 保留
    [0x3B] = 0x0000, // 保留
    [0x3C] = 0x0000, // 保留
    [0x3D] = 0x0000, // 保留
    [0x3E] = 0x0000, // 保留
    [0x3F] = 0x0000, // 保留
};

// ============================================================================
// 内部函数声明
// ============================================================================

static uint16_t modbus_build_request(uint8_t slave_id, uint8_t function_code,
                                     uint16_t start_addr, uint16_t quantity,
                                     const uint8_t *data, uint16_t data_length);

static modbus_status_t modbus_send_frame(const uint8_t *frame, uint16_t length);
static modbus_status_t modbus_receive_frame(uint8_t *frame, uint16_t *length, uint32_t timeout_ms);
static modbus_status_t modbus_parse_response(const uint8_t *frame, uint16_t length, modbus_request_t *request);
static modbus_status_t modbus_process_slave_request(const uint8_t *frame, uint16_t length);
static uint16_t modbus_build_exception_response(uint8_t slave_id, uint8_t function_code, uint8_t exception_code);

// ============================================================================
// 公共接口实现
// ============================================================================

/**
 * @brief Modbus协议栈初始化
 */
modbus_status_t modbus_init(const modbus_config_t *config)
{
    if (!config)
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    if (g_modbus.initialized)
    {
        modbus_deinit();
    }

    // 复制配置
    memcpy(&g_modbus.config, config, sizeof(modbus_config_t));

    // 配置UART
    uart_config_t uart_cfg = {
        .port = config->uart_port,
        .baudrate = config->baudrate,
        .databits = UART_DATABITS_8,
        .stopbits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE,
        .enable_rx_int = false,
        .enable_tx_int = false};

    if (!uart_config(&uart_cfg))
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    if (!uart_enable(config->uart_port, true))
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    // 初始化控制块
    g_modbus.tx_length = 0;
    g_modbus.rx_length = 0;
    g_modbus.last_activity_time = system_get_tick();
    g_modbus.tx_count = 0;
    g_modbus.rx_count = 0;
    g_modbus.error_count = 0;
    g_modbus.busy = false;
    g_modbus.initialized = true;

    // 清空回调函数
    memset(&g_modbus.slave_callbacks, 0, sizeof(modbus_slave_callbacks_t));

    if (config->enable_debug)
    {
        debug_printf("[MODBUS] Initialized - Role: %s, Port: %d, Baud: %d\n",
                     (config->role == MODBUS_ROLE_MASTER) ? "Master" : "Slave",
                     config->uart_port, config->baudrate);
    }

    return MODBUS_STATUS_OK;
}

/**
 * @brief Modbus协议栈去初始化
 */
modbus_status_t modbus_deinit(void)
{
    if (!g_modbus.initialized)
    {
        return MODBUS_STATUS_OK;
    }

    // 禁用UART
    uart_enable(g_modbus.config.uart_port, false);

    // 清空控制块
    memset(&g_modbus, 0, sizeof(modbus_control_t));

    debug_printf("[MODBUS] Deinitialized\n");
    return MODBUS_STATUS_OK;
}

/**
 * @brief 设置从站回调函数
 */
modbus_status_t modbus_set_slave_callbacks(const modbus_slave_callbacks_t *callbacks)
{
    if (!g_modbus.initialized || !callbacks)
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    memcpy(&g_modbus.slave_callbacks, callbacks, sizeof(modbus_slave_callbacks_t));
    return MODBUS_STATUS_OK;
}

/**
 * @brief Modbus任务处理函数
 */
void modbus_task(void)
{
    if (!g_modbus.initialized)
    {
        return;
    }

    // 检查UART接收数据
    uint16_t rx_available = uart_get_rx_count(g_modbus.config.uart_port);
    if (rx_available > 0)
    {
        // 限制接收长度
        if (rx_available > MODBUS_MAX_FRAME_SIZE)
        {
            rx_available = MODBUS_MAX_FRAME_SIZE;
        }

        // 读取数据到缓冲区
        uint16_t actual_read = uart_receive_available(g_modbus.config.uart_port,
                                                      g_modbus.rx_buffer,
                                                      rx_available);

        if (actual_read > 0)
        {
            g_modbus.rx_length = actual_read;
            g_modbus.last_activity_time = system_get_tick();
            g_modbus.rx_count++;

            if (g_modbus.config.enable_debug)
            {
                debug_printf("[MODBUS] Received %d bytes\n", actual_read);
            }

            // 处理接收到的帧
            if (g_modbus.config.role == MODBUS_ROLE_SLAVE)
            {
                modbus_process_slave_request(g_modbus.rx_buffer, g_modbus.rx_length);
            }
            else
            {
                // 主站模式处理响应
                modbus_request_t dummy_request = {0};
                modbus_parse_response(g_modbus.rx_buffer, g_modbus.rx_length, &dummy_request);
            }
        }
    }

    // 主站模式：检查响应超时
    if (g_modbus.config.role == MODBUS_ROLE_MASTER && g_modbus.busy)
    {
        uint32_t elapsed = system_get_tick() - g_modbus.last_activity_time;
        if (elapsed > g_modbus.config.timeout_ms)
        {
            g_modbus.busy = false;
            g_modbus.error_count++;
            if (g_modbus.config.enable_debug)
            {
                debug_printf("[MODBUS] Master timeout\n");
            }
        }
    }
}

// ============================================================================
// Modbus主站接口实现
// ============================================================================

/**
 * @brief 读取保持寄存器
 */
modbus_status_t modbus_read_holding_registers(uint8_t slave_id, uint16_t start_addr,
                                              uint16_t quantity, uint16_t *values)
{
    if (!g_modbus.initialized || g_modbus.config.role != MODBUS_ROLE_MASTER)
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    if (g_modbus.busy)
    {
        return MODBUS_STATUS_BUSY;
    }

    if (!values || quantity == 0 || quantity > 125)
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    g_modbus.busy = true;
    g_modbus.last_activity_time = system_get_tick();

    // 构建请求帧
    uint16_t frame_length = modbus_build_request(slave_id, MODBUS_FC_READ_HOLDING_REGISTERS,
                                                 start_addr, quantity, NULL, 0);

    // 发送请求
    modbus_status_t status = modbus_send_frame(g_modbus.tx_buffer, frame_length);
    if (status != MODBUS_STATUS_OK)
    {
        g_modbus.busy = false;
        return status;
    }

    // 等待响应
    uint16_t rx_length;
    status = modbus_receive_frame(g_modbus.rx_buffer, &rx_length, g_modbus.config.timeout_ms);
    if (status != MODBUS_STATUS_OK)
    {
        g_modbus.busy = false;
        g_modbus.error_count++;
        return status;
    }

    // 解析响应
    modbus_request_t response = {0};
    status = modbus_parse_response(g_modbus.rx_buffer, rx_length, &response);
    if (status != MODBUS_STATUS_OK)
    {
        g_modbus.busy = false;
        g_modbus.error_count++;
        return status;
    }

    // 检查响应有效性
    if (response.slave_id != slave_id || response.function_code != MODBUS_FC_READ_HOLDING_REGISTERS)
    {
        g_modbus.busy = false;
        g_modbus.error_count++;
        return MODBUS_STATUS_FRAME_ERROR;
    }

    // 复制寄存器值
    for (uint16_t i = 0; i < quantity && i < response.data_length / 2; i++)
    {
        values[i] = (response.data[i * 2] << 8) | response.data[i * 2 + 1];
    }

    g_modbus.busy = false;
    g_modbus.rx_count++;
    return MODBUS_STATUS_OK;
}

/**
 * @brief 写入单个寄存器
 */
modbus_status_t modbus_write_single_register(uint8_t slave_id, uint16_t register_addr, uint16_t value)
{
    if (!g_modbus.initialized || g_modbus.config.role != MODBUS_ROLE_MASTER)
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    if (g_modbus.busy)
    {
        return MODBUS_STATUS_BUSY;
    }

    g_modbus.busy = true;
    g_modbus.last_activity_time = system_get_tick();

    uint8_t data[2] = {(uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};

    // 构建请求帧
    uint16_t frame_length = modbus_build_request(slave_id, MODBUS_FC_WRITE_SINGLE_REGISTER,
                                                 register_addr, 1, data, 2);

    // 发送请求
    modbus_status_t status = modbus_send_frame(g_modbus.tx_buffer, frame_length);
    if (status != MODBUS_STATUS_OK)
    {
        g_modbus.busy = false;
        return status;
    }

    // 等待响应
    uint16_t rx_length;
    status = modbus_receive_frame(g_modbus.rx_buffer, &rx_length, g_modbus.config.timeout_ms);
    if (status != MODBUS_STATUS_OK)
    {
        g_modbus.busy = false;
        g_modbus.error_count++;
        return status;
    }

    // 解析响应
    modbus_request_t response = {0};
    status = modbus_parse_response(g_modbus.rx_buffer, rx_length, &response);
    if (status != MODBUS_STATUS_OK)
    {
        g_modbus.busy = false;
        g_modbus.error_count++;
        return status;
    }

    // 检查响应有效性
    if (response.slave_id != slave_id || response.function_code != MODBUS_FC_WRITE_SINGLE_REGISTER)
    {
        g_modbus.busy = false;
        g_modbus.error_count++;
        return MODBUS_STATUS_FRAME_ERROR;
    }

    g_modbus.busy = false;
    g_modbus.rx_count++;
    return MODBUS_STATUS_OK;
}

// ============================================================================
// 工具函数实现
// ============================================================================

/**
 * @brief 计算Modbus CRC16校验
 */
uint16_t modbus_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ crc16_table[index];
    }

    return crc;
}

/**
 * @brief 获取Modbus状态描述字符串
 */
const char *modbus_status_to_string(modbus_status_t status)
{
    switch (status)
    {
    case MODBUS_STATUS_OK:
        return "OK";
    case MODBUS_STATUS_TIMEOUT:
        return "Timeout";
    case MODBUS_STATUS_CRC_ERROR:
        return "CRC Error";
    case MODBUS_STATUS_FRAME_ERROR:
        return "Frame Error";
    case MODBUS_STATUS_EXCEPTION:
        return "Exception";
    case MODBUS_STATUS_INVALID_SLAVE:
        return "Invalid Slave";
    case MODBUS_STATUS_INVALID_FUNCTION:
        return "Invalid Function";
    case MODBUS_STATUS_INVALID_ADDRESS:
        return "Invalid Address";
    case MODBUS_STATUS_INVALID_DATA:
        return "Invalid Data";
    case MODBUS_STATUS_BUSY:
        return "Busy";
    default:
        return "Unknown";
    }
}

/**
 * @brief 获取Modbus统计信息
 */
void modbus_get_statistics(uint32_t *tx_count, uint32_t *rx_count, uint32_t *error_count)
{
    if (tx_count)
        *tx_count = g_modbus.tx_count;
    if (rx_count)
        *rx_count = g_modbus.rx_count;
    if (error_count)
        *error_count = g_modbus.error_count;
}

/**
 * @brief 打印Modbus状态信息
 */
void modbus_print_status(void)
{
    if (!g_modbus.initialized)
    {
        debug_printf("[MODBUS] Not initialized\n");
        return;
    }

    debug_printf("[MODBUS] Status:\n");
    debug_printf("  Role: %s\n", (g_modbus.config.role == MODBUS_ROLE_MASTER) ? "Master" : "Slave");
    debug_printf("  Port: %d, Baud: %d\n", g_modbus.config.uart_port, g_modbus.config.baudrate);
    debug_printf("  TX: %lu, RX: %lu, Errors: %lu\n",
                 g_modbus.tx_count, g_modbus.rx_count, g_modbus.error_count);
    debug_printf("  Busy: %s\n", g_modbus.busy ? "Yes" : "No");
}

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 构建Modbus请求帧
 */
static uint16_t modbus_build_request(uint8_t slave_id, uint8_t function_code,
                                     uint16_t start_addr, uint16_t quantity,
                                     const uint8_t *data, uint16_t data_length)
{
    uint16_t index = 0;

    // 从站地址
    g_modbus.tx_buffer[index++] = slave_id;

    // 功能码
    g_modbus.tx_buffer[index++] = function_code;

    // 起始地址 (大端序)
    g_modbus.tx_buffer[index++] = (uint8_t)(start_addr >> 8);
    g_modbus.tx_buffer[index++] = (uint8_t)(start_addr & 0xFF);

    // 数量或值 (大端序)
    g_modbus.tx_buffer[index++] = (uint8_t)(quantity >> 8);
    g_modbus.tx_buffer[index++] = (uint8_t)(quantity & 0xFF);

    // 数据（如果有）
    if (data && data_length > 0)
    {
        memcpy(&g_modbus.tx_buffer[index], data, data_length);
        index += data_length;
    }

    // 计算并添加CRC
    uint16_t crc = modbus_crc16(g_modbus.tx_buffer, index);
    g_modbus.tx_buffer[index++] = (uint8_t)(crc & 0xFF); // CRC低字节
    g_modbus.tx_buffer[index++] = (uint8_t)(crc >> 8);   // CRC高字节

    return index;
}

/**
 * @brief 发送Modbus帧
 */
static modbus_status_t modbus_send_frame(const uint8_t *frame, uint16_t length)
{
    uint16_t sent = uart_send_blocking(g_modbus.config.uart_port, frame, length,
                                       g_modbus.config.timeout_ms);

    if (sent != length)
    {
        return MODBUS_STATUS_TIMEOUT;
    }

    g_modbus.tx_count++;
    return MODBUS_STATUS_OK;
}

/**
 * @brief 接收Modbus帧
 */
static modbus_status_t modbus_receive_frame(uint8_t *frame, uint16_t *length, uint32_t timeout_ms)
{
    *length = 0;

    // 非阻塞接收，用于从站模式
    if (timeout_ms == 0)
    {
        *length = uart_receive_available(g_modbus.config.uart_port, frame, MODBUS_MAX_FRAME_SIZE);
        return (*length > 0) ? MODBUS_STATUS_OK : MODBUS_STATUS_TIMEOUT;
    }

    // 阻塞接收，用于主站模式
    *length = uart_receive_blocking(g_modbus.config.uart_port, frame,
                                    MODBUS_MAX_FRAME_SIZE, timeout_ms);

    if (*length == 0)
    {
        return MODBUS_STATUS_TIMEOUT;
    }

    // 验证最小帧长度
    if (*length < 4)
    { // 至少包含地址+功能码+CRC
        return MODBUS_STATUS_FRAME_ERROR;
    }

    // 验证CRC
    uint16_t received_crc = frame[*length - 2] | (frame[*length - 1] << 8);
    uint16_t calculated_crc = modbus_crc16(frame, *length - 2);

    if (received_crc != calculated_crc)
    {
        return MODBUS_STATUS_CRC_ERROR;
    }

    return MODBUS_STATUS_OK;
}

/**
 * @brief 解析Modbus响应
 */
static modbus_status_t modbus_parse_response(const uint8_t *frame, uint16_t length, modbus_request_t *request)
{
    if (length < 4 || !request)
    {
        return MODBUS_STATUS_FRAME_ERROR;
    }

    request->slave_id = frame[0];
    request->function_code = frame[1];

    // 检查是否为异常响应
    if (request->function_code & 0x80)
    {
        request->exception_code = frame[2];
        return MODBUS_STATUS_EXCEPTION;
    }

    // 根据功能码解析数据
    switch (request->function_code)
    {
    case MODBUS_FC_READ_HOLDING_REGISTERS:
    case MODBUS_FC_READ_INPUT_REGISTERS:
        request->data_length = frame[2];
        request->data = (uint8_t *)&frame[3];
        break;

    case MODBUS_FC_WRITE_SINGLE_REGISTER:
        request->start_address = (frame[2] << 8) | frame[3];
        request->quantity = (frame[4] << 8) | frame[5];
        break;

    default:
        return MODBUS_STATUS_INVALID_FUNCTION;
    }

    return MODBUS_STATUS_OK;
}

/**
 * @brief 处理从站请求
 */
static modbus_status_t modbus_process_slave_request(const uint8_t *frame, uint16_t length)
{
    if (!frame || length < 4)
    {
        return MODBUS_STATUS_INVALID_DATA;
    }

    // 检查CRC
    uint16_t received_crc = (frame[length - 1] << 8) | frame[length - 2];
    uint16_t calculated_crc = modbus_crc16(frame, length - 2);

    if (received_crc != calculated_crc)
    {
        g_modbus.error_count++;
        if (g_modbus.config.enable_debug)
        {
            debug_printf("[MODBUS] CRC error: received=0x%04X, calculated=0x%04X\n",
                         received_crc, calculated_crc);
        }
        return MODBUS_STATUS_CRC_ERROR;
    }

    uint8_t slave_id = frame[0];
    uint8_t function_code = frame[1];

    // 检查从站地址
    if (slave_id != g_modbus.config.slave_id && slave_id != 0)
    {
        // 不是给我们的请求，忽略
        return MODBUS_STATUS_OK;
    }

    if (g_modbus.config.enable_debug)
    {
        debug_printf("[MODBUS] Processing request: Slave=%d, FC=0x%02X\n", slave_id, function_code);
    }

    uint16_t response_length = 0;
    modbus_status_t status = MODBUS_STATUS_OK;

    // 根据功能码处理请求
    switch (function_code)
    {
    case 0x03: // 读取保持寄存器
    {
        uint16_t start_addr = (frame[2] << 8) | frame[3];
        uint16_t quantity = (frame[4] << 8) | frame[5];

        if (g_modbus.config.enable_debug)
        {
            debug_printf("[MODBUS] Read holding registers: addr=0x%04X, qty=%d\n", start_addr, quantity);
        }

        // 检查参数有效性
        if (quantity == 0 || quantity > 125 ||
            start_addr >= 64 || (start_addr + quantity) > 64)
        {
            response_length = modbus_build_exception_response(slave_id, function_code, 0x02); // 非法数据地址
            break;
        }

        // 构建响应
        g_modbus.tx_buffer[0] = slave_id;
        g_modbus.tx_buffer[1] = function_code;
        g_modbus.tx_buffer[2] = quantity * 2; // 字节数

        for (uint16_t i = 0; i < quantity; i++)
        {
            uint16_t reg_value = g_holding_registers[start_addr + i];
            g_modbus.tx_buffer[3 + i * 2] = (reg_value >> 8) & 0xFF; // 高字节
            g_modbus.tx_buffer[4 + i * 2] = reg_value & 0xFF;        // 低字节
        }

        response_length = 3 + quantity * 2;
        break;
    }

    case 0x06: // 写单个寄存器
    {
        uint16_t reg_addr = (frame[2] << 8) | frame[3];
        uint16_t reg_value = (frame[4] << 8) | frame[5];

        if (g_modbus.config.enable_debug)
        {
            debug_printf("[MODBUS] Write single register: addr=0x%04X, value=0x%04X\n", reg_addr, reg_value);
        }

        // 检查地址有效性
        if (reg_addr >= 64)
        {
            response_length = modbus_build_exception_response(slave_id, function_code, 0x02); // 非法数据地址
            break;
        }

        // 检查寄存器是否可写 (配置参数区域 0x20-0x2F)
        if (reg_addr < 0x20 || reg_addr > 0x2F)
        {
            response_length = modbus_build_exception_response(slave_id, function_code, 0x03); // 非法数据值
            break;
        }

        // 写入寄存器
        g_holding_registers[reg_addr] = reg_value;

        // 构建响应 (回显请求)
        memcpy(g_modbus.tx_buffer, frame, 6);
        response_length = 6;

        if (g_modbus.config.enable_debug)
        {
            debug_printf("[MODBUS] Register 0x%04X written with value 0x%04X\n", reg_addr, reg_value);
        }
        break;
    }

    case 0x10: // 写多个寄存器
    {
        uint16_t start_addr = (frame[2] << 8) | frame[3];
        uint16_t quantity = (frame[4] << 8) | frame[5];
        uint8_t byte_count = frame[6];

        if (g_modbus.config.enable_debug)
        {
            debug_printf("[MODBUS] Write multiple registers: addr=0x%04X, qty=%d\n", start_addr, quantity);
        }

        // 检查参数有效性
        if (quantity == 0 || quantity > 123 ||
            byte_count != quantity * 2 ||
            start_addr >= 64 || (start_addr + quantity) > 64)
        {
            response_length = modbus_build_exception_response(slave_id, function_code, 0x02); // 非法数据地址
            break;
        }

        // 检查寄存器是否可写 (配置参数区域 0x20-0x2F)
        if (start_addr < 0x20 || (start_addr + quantity - 1) > 0x2F)
        {
            response_length = modbus_build_exception_response(slave_id, function_code, 0x03); // 非法数据值
            break;
        }

        // 写入寄存器
        for (uint16_t i = 0; i < quantity; i++)
        {
            uint16_t reg_value = (frame[7 + i * 2] << 8) | frame[8 + i * 2];
            g_holding_registers[start_addr + i] = reg_value;
        }

        // 构建响应
        g_modbus.tx_buffer[0] = slave_id;
        g_modbus.tx_buffer[1] = function_code;
        g_modbus.tx_buffer[2] = (start_addr >> 8) & 0xFF;
        g_modbus.tx_buffer[3] = start_addr & 0xFF;
        g_modbus.tx_buffer[4] = (quantity >> 8) & 0xFF;
        g_modbus.tx_buffer[5] = quantity & 0xFF;

        response_length = 6;
        break;
    }

    default:
        // 不支持的功能码
        response_length = modbus_build_exception_response(slave_id, function_code, 0x01); // 非法功能
        status = MODBUS_STATUS_INVALID_FUNCTION;
        break;
    }

    // 发送响应
    if (response_length > 0)
    {
        // 添加CRC
        uint16_t crc = modbus_crc16(g_modbus.tx_buffer, response_length);
        g_modbus.tx_buffer[response_length] = crc & 0xFF;            // CRC低字节
        g_modbus.tx_buffer[response_length + 1] = (crc >> 8) & 0xFF; // CRC高字节
        response_length += 2;

        // 发送响应
        modbus_status_t send_status = modbus_send_frame(g_modbus.tx_buffer, response_length);
        if (send_status == MODBUS_STATUS_OK)
        {
            g_modbus.tx_count++;
            if (g_modbus.config.enable_debug)
            {
                debug_printf("[MODBUS] Response sent, length=%d\n", response_length);
            }
        }
        else
        {
            g_modbus.error_count++;
            if (g_modbus.config.enable_debug)
            {
                debug_printf("[MODBUS] Failed to send response\n");
            }
        }
    }

    return status;
}

/**
 * @brief 构建异常响应
 */
static uint16_t modbus_build_exception_response(uint8_t slave_id, uint8_t function_code, uint8_t exception_code)
{
    uint16_t index = 0;

    g_modbus.tx_buffer[index++] = slave_id;
    g_modbus.tx_buffer[index++] = function_code | 0x80; // 异常标志
    g_modbus.tx_buffer[index++] = exception_code;

    // 添加CRC
    uint16_t crc = modbus_crc16(g_modbus.tx_buffer, index);
    g_modbus.tx_buffer[index++] = (uint8_t)(crc & 0xFF);
    g_modbus.tx_buffer[index++] = (uint8_t)(crc >> 8);

    return index;
}

/**
 * @brief Modbus处理函数 (供main.c调用，避免函数名冲突)
 */
void modbus_process(void)
{
    // 直接调用内部的modbus_task函数
    modbus_task();
}

/**
 * @brief 更新系统寄存器数据
 */
void modbus_update_system_registers(void)
{
    if (!g_modbus.initialized)
    {
        return;
    }

    system_state_t *sys_state = system_get_state();
    if (!sys_state)
    {
        return;
    }

    // 更新系统状态寄存器 (0x0000-0x000F)
    g_holding_registers[0x04] = (sys_state->error_code > 0) ? 1 : 0;     // 系统状态
    g_holding_registers[0x05] = sys_state->error_code;                   // 错误代码
    g_holding_registers[0x06] = (sys_state->uptime_seconds / 3600);      // 运行时间(小时)
    g_holding_registers[0x07] = (sys_state->uptime_seconds % 3600) / 60; // 运行时间(分钟)
    g_holding_registers[0x08] = g_modbus.tx_count & 0xFFFF;              // 通信计数(发送)
    g_holding_registers[0x09] = g_modbus.rx_count & 0xFFFF;              // 通信计数(接收)
    g_holding_registers[0x0A] = g_modbus.error_count & 0xFFFF;           // 通信计数(错误)

    // 更新传感器数据寄存器 (0x0010-0x001F)
    g_holding_registers[0x10] = (uint16_t)(sys_state->temperature);         // 温度1 (0.1°C)
    g_holding_registers[0x11] = (uint16_t)(sys_state->humidity);            // 湿度1 (0.1%RH)
    g_holding_registers[0x14] = (uint16_t)(sys_state->supply_voltage / 10); // 电压1 (0.01V)

    // 更新传感器状态
    g_holding_registers[0x18] = 0x0001; // 传感器状态1 (在线)

    // 更新统计和诊断寄存器 (0x0030-0x003F)
    uint32_t total_runtime = sys_state->uptime_seconds;
    g_holding_registers[0x30] = (total_runtime >> 16) & 0xFFFF; // 总运行时间(高字节)
    g_holding_registers[0x31] = total_runtime & 0xFFFF;         // 总运行时间(低字节)

    uint32_t total_samples = sys_state->sensor_read_count;
    g_holding_registers[0x32] = (total_samples >> 16) & 0xFFFF; // 总采样次数(高字节)
    g_holding_registers[0x33] = total_samples & 0xFFFF;         // 总采样次数(低字节)

    g_holding_registers[0x34] = sys_state->alarm_count & 0xFFFF; // 报警次数

    // 更新温度湿度最值 (简化实现)
    static int16_t temp_max = -1000, temp_min = 1000;
    static uint16_t humid_max = 0, humid_min = 1000;

    if (sys_state->temperature > temp_max)
        temp_max = sys_state->temperature;
    if (sys_state->temperature < temp_min)
        temp_min = sys_state->temperature;
    if (sys_state->humidity > humid_max)
        humid_max = sys_state->humidity;
    if (sys_state->humidity < humid_min)
        humid_min = sys_state->humidity;

    g_holding_registers[0x36] = (uint16_t)temp_max; // 最大温度
    g_holding_registers[0x37] = (uint16_t)temp_min; // 最小温度
    g_holding_registers[0x38] = humid_max;          // 最大湿度
    g_holding_registers[0x39] = humid_min;          // 最小湿度
}