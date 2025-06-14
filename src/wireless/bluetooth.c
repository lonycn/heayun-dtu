/**
 * @file bluetooth.c
 * @brief 憨云DTU - 蓝牙BLE通信模块实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 蓝牙BLE通信模块实现，支持BLE 4.0/5.0协议栈
 * 提供设备配对、数据传输、移动APP通信功能
 */

#include "bluetooth.h"
#include "uart.h"
#include "gpio.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//==============================================================================
// 私有宏定义
//==============================================================================

#define BLE_CMD_BUFFER_SIZE 256      // 命令缓冲区大小
#define BLE_RX_BUFFER_SIZE 512       // 接收缓冲区大小
#define BLE_MAX_CONNECTIONS 4        // 最大连接数
#define BLE_SCAN_RESULT_MAX 16       // 最大扫描结果数
#define BLE_ADV_INTERVAL_DEFAULT 100 // 默认广播间隔(ms)
#define BLE_CONN_INTERVAL_DEFAULT 50 // 默认连接间隔(ms)

//==============================================================================
// 私有类型定义
//==============================================================================

/**
 * @brief 连接信息
 */
typedef struct
{
    bool is_used;                        // 是否使用
    uint16_t conn_handle;                // 连接句柄
    ble_device_info_t device_info;       // 设备信息
    ble_security_level_t security_level; // 安全等级
    uint32_t last_activity;              // 最后活动时间
    bool is_paired;                      // 是否已配对
} ble_connection_t;

/**
 * @brief 扫描结果
 */
typedef struct
{
    ble_device_info_t devices[BLE_SCAN_RESULT_MAX]; // 扫描到的设备
    uint8_t device_count;                           // 设备数量
    bool is_scanning;                               // 是否正在扫描
    uint32_t scan_start_time;                       // 扫描开始时间
} ble_scan_result_t;

/**
 * @brief 蓝牙控制结构
 */
typedef struct
{
    bool initialized;                    // 初始化标志
    ble_config_t config;                 // 配置信息
    ble_state_t state;                   // 当前状态
    ble_status_t status;                 // 状态信息
    ble_event_callback_t event_callback; // 事件回调函数

    // 通信缓冲区
    char cmd_buffer[BLE_CMD_BUFFER_SIZE]; // 命令缓冲区
    char rx_buffer[BLE_RX_BUFFER_SIZE];   // 接收缓冲区
    uint16_t rx_index;                    // 接收索引

    // 连接管理
    ble_connection_t connections[BLE_MAX_CONNECTIONS]; // 连接信息

    // 扫描管理
    ble_scan_result_t scan_result; // 扫描结果

    // 广播管理
    bool is_advertising;         // 是否正在广播
    ble_adv_config_t adv_config; // 广播配置

    // 服务和特征
    ble_service_t services[BLE_MAX_SERVICES];                      // 服务列表
    ble_characteristic_t characteristics[BLE_MAX_CHARACTERISTICS]; // 特征列表
    uint8_t service_count;                                         // 服务数量
    uint8_t characteristic_count;                                  // 特征数量

    // 时间管理
    uint32_t init_time;      // 初始化时间
    uint32_t last_heartbeat; // 最后心跳时间

    // 统计信息
    uint32_t commands_sent;     // 发送的命令数
    uint32_t events_received;   // 接收的事件数
    uint32_t connection_errors; // 连接错误数
} ble_control_t;

//==============================================================================
// 私有变量
//==============================================================================

static ble_control_t ble_ctrl = {0};

//==============================================================================
// 私有函数声明
//==============================================================================

static ble_error_t ble_send_command(const char *cmd, char *response, uint16_t response_len, uint32_t timeout_ms);
static void ble_process_received_data(void);
static void ble_process_events(void);
static void ble_update_status(void);
static uint16_t ble_allocate_connection(void);
static void ble_free_connection(uint16_t conn_handle);
static ble_connection_t *ble_find_connection(uint16_t conn_handle);
static void ble_trigger_event(ble_event_type_t event_type, uint16_t conn_handle,
                              const ble_device_info_t *device_info, const uint8_t *data, uint16_t data_len);
static ble_error_t ble_parse_scan_result(const char *data, ble_device_info_t *device_info);
static ble_error_t ble_validate_mac_address(const char *mac_addr);

//==============================================================================
// 状态名称映射
//==============================================================================

static const char *ble_state_names[] = {
    "OFF",
    "INITIALIZING",
    "IDLE",
    "SCANNING",
    "ADVERTISING",
    "CONNECTING",
    "CONNECTED",
    "PAIRING",
    "PAIRED",
    "ERROR"};

static const char *ble_error_strings[] = {
    "Success",
    "Invalid parameter",
    "Not initialized",
    "Timeout",
    "Not connected",
    "Connection failed",
    "Pair failed",
    "Data too long",
    "Memory error",
    "Hardware error",
    "Unknown error"};

//==============================================================================
// 公共函数实现
//==============================================================================

/**
 * @brief 初始化蓝牙模块
 */
ble_error_t ble_init(const ble_config_t *config, ble_event_callback_t callback)
{
    if (!config || !callback)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    // 保存配置和回调
    memcpy(&ble_ctrl.config, config, sizeof(ble_config_t));
    ble_ctrl.event_callback = callback;

    // 初始化状态
    ble_ctrl.state = BLE_STATE_OFF;
    ble_ctrl.rx_index = 0;
    ble_ctrl.is_advertising = false;
    ble_ctrl.init_time = timer_get_tick();
    ble_ctrl.last_heartbeat = 0;

    // 清空连接信息
    memset(ble_ctrl.connections, 0, sizeof(ble_ctrl.connections));

    // 清空扫描结果
    memset(&ble_ctrl.scan_result, 0, sizeof(ble_scan_result_t));

    // 初始化状态信息
    memset(&ble_ctrl.status, 0, sizeof(ble_status_t));
    ble_ctrl.status.state = BLE_STATE_OFF;

    // 初始化UART（假设使用UART1进行蓝牙通信）
    uart_config_t uart_cfg = {
        .baudrate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = UART_PARITY_NONE,
        .flow_control = false};

    if (uart_init(1, &uart_cfg) != UART_SUCCESS)
    {
        return BLE_ERROR_HARDWARE;
    }

    ble_ctrl.initialized = true;
    ble_ctrl.state = BLE_STATE_INITIALIZING;

    return BLE_SUCCESS;
}

/**
 * @brief 反初始化蓝牙模块
 */
ble_error_t ble_deinit(void)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    // 关闭蓝牙
    ble_power_off();

    // 反初始化UART
    uart_deinit(1);

    // 清空控制结构
    memset(&ble_ctrl, 0, sizeof(ble_control_t));

    return BLE_SUCCESS;
}

/**
 * @brief 蓝牙任务处理
 */
void ble_task(void)
{
    if (!ble_ctrl.initialized)
    {
        return;
    }

    // 处理接收数据
    ble_process_received_data();

    // 处理事件
    ble_process_events();

    // 更新状态
    ble_update_status();

    // 心跳处理
    uint32_t current_time = timer_get_tick();
    if (current_time - ble_ctrl.last_heartbeat > 30000)
    { // 30秒心跳
        if (ble_ctrl.state >= BLE_STATE_IDLE)
        {
            // 发送心跳命令
            ble_send_command("AT", NULL, 0, 1000);
        }
        ble_ctrl.last_heartbeat = current_time;
    }
}

/**
 * @brief 开启蓝牙
 */
ble_error_t ble_power_on(void)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    if (ble_ctrl.state != BLE_STATE_OFF)
    {
        return BLE_SUCCESS; // 已经开启
    }

    // 发送开启命令
    if (ble_send_command("AT+RESET", NULL, 0, 5000) != BLE_SUCCESS)
    {
        return BLE_ERROR_HARDWARE;
    }

    // 等待初始化完成
    timer_delay_ms(2000);

    // 设置设备名称
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+NAME%s", ble_ctrl.config.device_name);
    ble_send_command(cmd, NULL, 0, 3000);

    // 设置角色
    if (ble_ctrl.config.role == BLE_ROLE_PERIPHERAL)
    {
        ble_send_command("AT+ROLE0", NULL, 0, 3000);
    }
    else if (ble_ctrl.config.role == BLE_ROLE_CENTRAL)
    {
        ble_send_command("AT+ROLE1", NULL, 0, 3000);
    }

    ble_ctrl.state = BLE_STATE_IDLE;

    return BLE_SUCCESS;
}

/**
 * @brief 关闭蓝牙
 */
ble_error_t ble_power_off(void)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    // 断开所有连接
    ble_disconnect_all();

    // 停止广播和扫描
    ble_stop_advertising();
    ble_stop_scan();

    ble_ctrl.state = BLE_STATE_OFF;

    return BLE_SUCCESS;
}

/**
 * @brief 复位蓝牙模块
 */
ble_error_t ble_reset(void)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    ble_power_off();
    timer_delay_ms(1000);

    return ble_power_on();
}

/**
 * @brief 获取蓝牙状态
 */
ble_state_t ble_get_state(void)
{
    return ble_ctrl.state;
}

/**
 * @brief 获取详细状态信息
 */
ble_error_t ble_get_status(ble_status_t *status)
{
    if (!status)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    memcpy(status, &ble_ctrl.status, sizeof(ble_status_t));

    return BLE_SUCCESS;
}

/**
 * @brief 开始广播
 */
ble_error_t ble_start_advertising(const ble_adv_config_t *config)
{
    if (!config)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (!ble_ctrl.initialized || ble_ctrl.state < BLE_STATE_IDLE)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    // 保存广播配置
    memcpy(&ble_ctrl.adv_config, config, sizeof(ble_adv_config_t));

    // 设置广播间隔
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+ADVI%d", config->adv_interval_min);
    if (ble_send_command(cmd, NULL, 0, 3000) != BLE_SUCCESS)
    {
        return BLE_ERROR_HARDWARE;
    }

    // 开始广播
    if (ble_send_command("AT+ADTY3", NULL, 0, 3000) != BLE_SUCCESS)
    {
        return BLE_ERROR_HARDWARE;
    }

    ble_ctrl.is_advertising = true;
    ble_ctrl.state = BLE_STATE_ADVERTISING;
    ble_ctrl.status.is_advertising = true;

    return BLE_SUCCESS;
}

/**
 * @brief 停止广播
 */
ble_error_t ble_stop_advertising(void)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    if (!ble_ctrl.is_advertising)
    {
        return BLE_SUCCESS; // 已经停止
    }

    // 停止广播
    ble_send_command("AT+ADTY0", NULL, 0, 3000);

    ble_ctrl.is_advertising = false;
    ble_ctrl.status.is_advertising = false;

    if (ble_ctrl.status.connected_devices == 0)
    {
        ble_ctrl.state = BLE_STATE_IDLE;
    }

    return BLE_SUCCESS;
}

/**
 * @brief 开始扫描
 */
ble_error_t ble_start_scan(const ble_scan_config_t *config)
{
    if (!config)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (!ble_ctrl.initialized || ble_ctrl.state < BLE_STATE_IDLE)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    // 清空扫描结果
    memset(&ble_ctrl.scan_result, 0, sizeof(ble_scan_result_t));

    // 设置扫描参数
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+DISC?");
    if (ble_send_command(cmd, NULL, 0, config->scan_duration_ms) != BLE_SUCCESS)
    {
        return BLE_ERROR_HARDWARE;
    }

    ble_ctrl.scan_result.is_scanning = true;
    ble_ctrl.scan_result.scan_start_time = timer_get_tick();
    ble_ctrl.state = BLE_STATE_SCANNING;
    ble_ctrl.status.is_scanning = true;

    return BLE_SUCCESS;
}

/**
 * @brief 停止扫描
 */
ble_error_t ble_stop_scan(void)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    if (!ble_ctrl.scan_result.is_scanning)
    {
        return BLE_SUCCESS; // 已经停止
    }

    ble_ctrl.scan_result.is_scanning = false;
    ble_ctrl.status.is_scanning = false;

    if (ble_ctrl.status.connected_devices == 0)
    {
        ble_ctrl.state = BLE_STATE_IDLE;
    }

    return BLE_SUCCESS;
}

/**
 * @brief 连接设备
 */
ble_error_t ble_connect(const char *mac_addr, uint8_t addr_type)
{
    if (!mac_addr)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (!ble_ctrl.initialized || ble_ctrl.state < BLE_STATE_IDLE)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    // 验证MAC地址格式
    if (ble_validate_mac_address(mac_addr) != BLE_SUCCESS)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    // 分配连接句柄
    uint16_t conn_handle = ble_allocate_connection();
    if (conn_handle == 0xFFFF)
    {
        return BLE_ERROR_MEMORY;
    }

    // 发送连接命令
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CON%s", mac_addr);
    if (ble_send_command(cmd, NULL, 0, BLE_CONNECT_TIMEOUT_MS) != BLE_SUCCESS)
    {
        ble_free_connection(conn_handle);
        return BLE_ERROR_CONNECTION_FAILED;
    }

    // 保存连接信息
    ble_connection_t *conn = ble_find_connection(conn_handle);
    if (conn)
    {
        strncpy(conn->device_info.mac_addr, mac_addr, sizeof(conn->device_info.mac_addr) - 1);
        conn->device_info.addr_type = addr_type;
        conn->device_info.is_connected = true;
        conn->last_activity = timer_get_tick();
    }

    ble_ctrl.state = BLE_STATE_CONNECTED;
    ble_ctrl.status.connected_devices++;

    // 触发连接事件
    ble_trigger_event(BLE_EVENT_CONNECTED, conn_handle, &conn->device_info, NULL, 0);

    return BLE_SUCCESS;
}

/**
 * @brief 断开连接
 */
ble_error_t ble_disconnect(uint16_t conn_handle)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    ble_connection_t *conn = ble_find_connection(conn_handle);
    if (!conn || !conn->is_used)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    // 发送断开命令
    ble_send_command("AT", NULL, 0, 3000);

    // 触发断开事件
    ble_trigger_event(BLE_EVENT_DISCONNECTED, conn_handle, &conn->device_info, NULL, 0);

    // 释放连接
    ble_free_connection(conn_handle);
    ble_ctrl.status.connected_devices--;

    if (ble_ctrl.status.connected_devices == 0)
    {
        ble_ctrl.state = BLE_STATE_IDLE;
    }

    return BLE_SUCCESS;
}

/**
 * @brief 断开所有连接
 */
ble_error_t ble_disconnect_all(void)
{
    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (ble_ctrl.connections[i].is_used)
        {
            ble_disconnect(ble_ctrl.connections[i].conn_handle);
        }
    }

    return BLE_SUCCESS;
}

/**
 * @brief 发送数据
 */
ble_error_t ble_send_data(uint16_t conn_handle, const uint8_t *data, uint16_t data_len)
{
    if (!data || data_len == 0 || data_len > BLE_MAX_DATA_LEN)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (!ble_ctrl.initialized)
    {
        return BLE_ERROR_NOT_INITIALIZED;
    }

    ble_connection_t *conn = ble_find_connection(conn_handle);
    if (!conn || !conn->is_used || !conn->device_info.is_connected)
    {
        return BLE_ERROR_NOT_CONNECTED;
    }

    // 发送数据
    if (uart_send_data(1, data, data_len) != UART_SUCCESS)
    {
        return BLE_ERROR_HARDWARE;
    }

    conn->last_activity = timer_get_tick();
    ble_ctrl.status.data_sent_bytes += data_len;

    // 触发数据发送事件
    ble_trigger_event(BLE_EVENT_DATA_SENT, conn_handle, &conn->device_info, data, data_len);

    return BLE_SUCCESS;
}

/**
 * @brief 获取状态名称
 */
const char *ble_get_state_name(ble_state_t state)
{
    if (state < sizeof(ble_state_names) / sizeof(ble_state_names[0]))
    {
        return ble_state_names[state];
    }
    return "UNKNOWN";
}

/**
 * @brief 获取错误码描述
 */
const char *ble_get_error_string(ble_error_t error)
{
    if (error < sizeof(ble_error_strings) / sizeof(ble_error_strings[0]))
    {
        return ble_error_strings[error];
    }
    return "Unknown error";
}

//==============================================================================
// 私有函数实现
//==============================================================================

/**
 * @brief 发送命令
 */
static ble_error_t ble_send_command(const char *cmd, char *response, uint16_t response_len, uint32_t timeout_ms)
{
    if (!cmd)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    // 清空接收缓冲区
    ble_ctrl.rx_index = 0;
    memset(ble_ctrl.rx_buffer, 0, sizeof(ble_ctrl.rx_buffer));

    // 发送命令
    snprintf(ble_ctrl.cmd_buffer, sizeof(ble_ctrl.cmd_buffer), "%s\r\n", cmd);
    if (uart_send_string(1, ble_ctrl.cmd_buffer) != UART_SUCCESS)
    {
        return BLE_ERROR_HARDWARE;
    }

    ble_ctrl.commands_sent++;

    // 等待响应
    uint32_t start_time = timer_get_tick();
    while (timer_get_tick() - start_time < timeout_ms)
    {
        ble_process_received_data();

        if (strstr(ble_ctrl.rx_buffer, "OK") || strstr(ble_ctrl.rx_buffer, "ERROR"))
        {
            if (response && response_len > 0)
            {
                strncpy(response, ble_ctrl.rx_buffer, response_len - 1);
                response[response_len - 1] = '\0';
            }

            if (strstr(ble_ctrl.rx_buffer, "OK"))
            {
                return BLE_SUCCESS;
            }
            else
            {
                ble_ctrl.connection_errors++;
                return BLE_ERROR_HARDWARE;
            }
        }

        timer_delay_ms(10);
    }

    return BLE_ERROR_TIMEOUT;
}

/**
 * @brief 处理接收数据
 */
static void ble_process_received_data(void)
{
    uint8_t byte;
    while (uart_receive_byte(1, &byte) == UART_SUCCESS)
    {
        if (ble_ctrl.rx_index < BLE_RX_BUFFER_SIZE - 1)
        {
            ble_ctrl.rx_buffer[ble_ctrl.rx_index++] = byte;
            ble_ctrl.rx_buffer[ble_ctrl.rx_index] = '\0';
        }

        ble_ctrl.status.data_received_bytes++;
    }
}

/**
 * @brief 处理事件
 */
static void ble_process_events(void)
{
    // 检查扫描超时
    if (ble_ctrl.scan_result.is_scanning)
    {
        uint32_t current_time = timer_get_tick();
        if (current_time - ble_ctrl.scan_result.scan_start_time > BLE_SCAN_TIMEOUT_MS)
        {
            ble_stop_scan();
        }
    }

    // 检查连接超时
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (ble_ctrl.connections[i].is_used)
        {
            uint32_t current_time = timer_get_tick();
            if (current_time - ble_ctrl.connections[i].last_activity > 60000)
            { // 60秒超时
                ble_disconnect(ble_ctrl.connections[i].conn_handle);
            }
        }
    }
}

/**
 * @brief 更新状态信息
 */
static void ble_update_status(void)
{
    ble_ctrl.status.state = ble_ctrl.state;
    ble_ctrl.status.uptime_seconds = (timer_get_tick() - ble_ctrl.init_time) / 1000;

    // 统计连接设备数
    uint8_t connected_count = 0;
    uint8_t paired_count = 0;

    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (ble_ctrl.connections[i].is_used)
        {
            if (ble_ctrl.connections[i].device_info.is_connected)
            {
                connected_count++;
            }
            if (ble_ctrl.connections[i].is_paired)
            {
                paired_count++;
            }
        }
    }

    ble_ctrl.status.connected_devices = connected_count;
    ble_ctrl.status.paired_devices = paired_count;
}

/**
 * @brief 分配连接句柄
 */
static uint16_t ble_allocate_connection(void)
{
    for (uint16_t i = 0; i < BLE_MAX_CONNECTIONS; i++)
    {
        if (!ble_ctrl.connections[i].is_used)
        {
            ble_ctrl.connections[i].is_used = true;
            ble_ctrl.connections[i].conn_handle = i;
            return i;
        }
    }
    return 0xFFFF; // 无可用连接
}

/**
 * @brief 释放连接
 */
static void ble_free_connection(uint16_t conn_handle)
{
    if (conn_handle < BLE_MAX_CONNECTIONS)
    {
        memset(&ble_ctrl.connections[conn_handle], 0, sizeof(ble_connection_t));
    }
}

/**
 * @brief 查找连接
 */
static ble_connection_t *ble_find_connection(uint16_t conn_handle)
{
    if (conn_handle < BLE_MAX_CONNECTIONS && ble_ctrl.connections[conn_handle].is_used)
    {
        return &ble_ctrl.connections[conn_handle];
    }
    return NULL;
}

/**
 * @brief 触发事件
 */
static void ble_trigger_event(ble_event_type_t event_type, uint16_t conn_handle,
                              const ble_device_info_t *device_info, const uint8_t *data, uint16_t data_len)
{
    if (ble_ctrl.event_callback)
    {
        ble_event_t event = {0};
        event.event_type = event_type;
        event.conn_handle = conn_handle;

        if (device_info)
        {
            memcpy(&event.device_info, device_info, sizeof(ble_device_info_t));
        }

        event.data = (uint8_t *)data;
        event.data_len = data_len;
        event.error_code = BLE_SUCCESS;

        ble_ctrl.event_callback(&event);
        ble_ctrl.events_received++;
    }
}

/**
 * @brief 验证MAC地址
 */
static ble_error_t ble_validate_mac_address(const char *mac_addr)
{
    if (!mac_addr || strlen(mac_addr) != 17)
    {
        return BLE_ERROR_INVALID_PARAM;
    }

    // 检查格式: XX:XX:XX:XX:XX:XX
    for (int i = 0; i < 17; i++)
    {
        if (i % 3 == 2)
        {
            if (mac_addr[i] != ':')
            {
                return BLE_ERROR_INVALID_PARAM;
            }
        }
        else
        {
            if (!((mac_addr[i] >= '0' && mac_addr[i] <= '9') ||
                  (mac_addr[i] >= 'A' && mac_addr[i] <= 'F') ||
                  (mac_addr[i] >= 'a' && mac_addr[i] <= 'f')))
            {
                return BLE_ERROR_INVALID_PARAM;
            }
        }
    }

    return BLE_SUCCESS;
}