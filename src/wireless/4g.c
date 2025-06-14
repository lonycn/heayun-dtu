/**
 * @file 4g.c
 * @brief 憨云DTU - 4G无线通信模块实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 4G无线通信模块实现，支持EC20/SIM7600等主流4G模组
 * 提供TCP/UDP网络通信、HTTP/HTTPS客户端功能
 */

#include "4g.h"
#include "uart.h"
#include "gpio.h"
#include "timer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//==============================================================================
// 私有宏定义
//==============================================================================

#define G4_AT_BUFFER_SIZE 512       // AT命令缓冲区大小
#define G4_RX_BUFFER_SIZE 1024      // 接收缓冲区大小
#define G4_MAX_SOCKETS 4            // 最大Socket连接数
#define G4_HEARTBEAT_INTERVAL 30000 // 心跳间隔(ms)

//==============================================================================
// 私有类型定义
//==============================================================================

/**
 * @brief Socket连接信息
 */
typedef struct
{
    bool is_used;           // 是否使用
    bool is_tcp;            // TCP/UDP类型
    char remote_host[64];   // 远程主机
    uint16_t remote_port;   // 远程端口
    uint16_t local_port;    // 本地端口
    bool is_connected;      // 连接状态
    uint32_t last_activity; // 最后活动时间
} g4_socket_info_t;

/**
 * @brief 4G模块控制结构
 */
typedef struct
{
    bool initialized;   // 初始化标志
    g4_config_t config; // 配置信息
    g4_state_t state;   // 当前状态
    g4_status_t status; // 状态信息

    // 通信缓冲区
    char at_buffer[G4_AT_BUFFER_SIZE]; // AT命令缓冲区
    char rx_buffer[G4_RX_BUFFER_SIZE]; // 接收缓冲区
    uint16_t rx_index;                 // 接收索引

    // Socket管理
    g4_socket_info_t sockets[G4_MAX_SOCKETS]; // Socket信息

    // 时间管理
    uint32_t last_heartbeat; // 最后心跳时间
    uint32_t init_time;      // 初始化时间

    // 统计信息
    uint32_t at_commands_sent;      // 发送的AT命令数
    uint32_t at_responses_received; // 接收的响应数
    uint32_t network_errors;        // 网络错误数
} g4_control_t;

//==============================================================================
// 私有变量
//==============================================================================

static g4_control_t g4_ctrl = {0};

//==============================================================================
// 私有函数声明
//==============================================================================

static g4_error_t g4_send_at_cmd(const char *cmd, char *response, uint16_t response_len, uint32_t timeout_ms);
static g4_error_t g4_wait_response(const char *expected, uint32_t timeout_ms);
static g4_error_t g4_parse_response(const char *response, const char *prefix, char *value, uint16_t value_len);
static void g4_process_received_data(void);
static void g4_update_status(void);
static uint8_t g4_allocate_socket(void);
static void g4_free_socket(uint8_t socket_id);
static g4_signal_level_t g4_rssi_to_level(int8_t rssi);

//==============================================================================
// 状态名称映射
//==============================================================================

static const char *g4_state_names[] = {
    "POWER_OFF",
    "POWER_ON",
    "SIM_READY",
    "NETWORK_SEARCHING",
    "NETWORK_REGISTERED",
    "PDP_ACTIVATED",
    "CONNECTED",
    "ERROR"};

static const char *g4_net_type_names[] = {
    "UNKNOWN",
    "2G",
    "3G",
    "4G",
    "5G"};

static const char *g4_error_strings[] = {
    "Success",
    "Invalid parameter",
    "Not initialized",
    "Timeout",
    "Network error",
    "SIM not ready",
    "No signal",
    "AT command error",
    "HTTP error",
    "Memory error",
    "Unknown error"};

//==============================================================================
// 公共函数实现
//==============================================================================

/**
 * @brief 初始化4G模块
 */
g4_error_t g4_init(const g4_config_t *config)
{
    if (!config)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    // 保存配置
    memcpy(&g4_ctrl.config, config, sizeof(g4_config_t));

    // 初始化状态
    g4_ctrl.state = G4_STATE_POWER_OFF;
    g4_ctrl.rx_index = 0;
    g4_ctrl.last_heartbeat = 0;
    g4_ctrl.init_time = timer_get_tick();

    // 清空Socket信息
    memset(g4_ctrl.sockets, 0, sizeof(g4_ctrl.sockets));

    // 初始化状态信息
    memset(&g4_ctrl.status, 0, sizeof(g4_status_t));
    g4_ctrl.status.state = G4_STATE_POWER_OFF;

    // 初始化UART
    uart_config_t uart_cfg = {
        .baudrate = config->baudrate,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = UART_PARITY_NONE,
        .flow_control = false};

    if (uart_init(config->uart_port, &uart_cfg) != UART_SUCCESS)
    {
        return G4_ERROR_HARDWARE;
    }

    // 初始化GPIO
    gpio_config_t gpio_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pull = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_LOW};

    gpio_init(config->power_pin, &gpio_cfg);
    gpio_init(config->reset_pin, &gpio_cfg);

    // 设置初始状态
    gpio_write(config->power_pin, false);
    gpio_write(config->reset_pin, false);

    g4_ctrl.initialized = true;

    return G4_SUCCESS;
}

/**
 * @brief 反初始化4G模块
 */
g4_error_t g4_deinit(void)
{
    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    // 关机
    g4_power_off();

    // 反初始化UART
    uart_deinit(g4_ctrl.config.uart_port);

    // 清空控制结构
    memset(&g4_ctrl, 0, sizeof(g4_control_t));

    return G4_SUCCESS;
}

/**
 * @brief 4G模块任务处理
 */
void g4_task(void)
{
    if (!g4_ctrl.initialized)
    {
        return;
    }

    uint32_t current_time = timer_get_tick();

    // 处理接收数据
    g4_process_received_data();

    // 更新状态
    g4_update_status();

    // 心跳处理
    if (g4_ctrl.state == G4_STATE_CONNECTED)
    {
        if (current_time - g4_ctrl.last_heartbeat > G4_HEARTBEAT_INTERVAL)
        {
            // 发送心跳
            g4_send_at_cmd("AT", NULL, 0, 1000);
            g4_ctrl.last_heartbeat = current_time;
        }
    }

    // 自动连接处理
    if (g4_ctrl.config.auto_connect && g4_ctrl.state == G4_STATE_PDP_ACTIVATED)
    {
        g4_connect_network();
    }
}

/**
 * @brief 开机
 */
g4_error_t g4_power_on(void)
{
    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    if (g4_ctrl.state != G4_STATE_POWER_OFF)
    {
        return G4_SUCCESS; // 已经开机
    }

    // 拉高电源引脚
    gpio_write(g4_ctrl.config.power_pin, true);
    timer_delay_ms(g4_ctrl.config.power_on_delay_ms);
    gpio_write(g4_ctrl.config.power_pin, false);

    // 等待模块启动
    timer_delay_ms(3000);

    // 检查模块响应
    if (g4_send_at_cmd("AT", NULL, 0, 5000) == G4_SUCCESS)
    {
        g4_ctrl.state = G4_STATE_POWER_ON;
        return G4_SUCCESS;
    }

    return G4_ERROR_HARDWARE;
}

/**
 * @brief 关机
 */
g4_error_t g4_power_off(void)
{
    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    // 发送关机命令
    g4_send_at_cmd("AT+QPOWD=1", NULL, 0, 5000);

    // 强制关机
    gpio_write(g4_ctrl.config.power_pin, true);
    timer_delay_ms(2000);
    gpio_write(g4_ctrl.config.power_pin, false);

    g4_ctrl.state = G4_STATE_POWER_OFF;

    return G4_SUCCESS;
}

/**
 * @brief 复位模块
 */
g4_error_t g4_reset(void)
{
    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    // 硬件复位
    gpio_write(g4_ctrl.config.reset_pin, true);
    timer_delay_ms(g4_ctrl.config.reset_delay_ms);
    gpio_write(g4_ctrl.config.reset_pin, false);

    // 等待复位完成
    timer_delay_ms(5000);

    g4_ctrl.state = G4_STATE_POWER_OFF;

    return g4_power_on();
}

/**
 * @brief 检查模块是否就绪
 */
bool g4_is_ready(void)
{
    return (g4_ctrl.initialized && g4_ctrl.state >= G4_STATE_POWER_ON);
}

/**
 * @brief 获取模块状态
 */
g4_state_t g4_get_state(void)
{
    return g4_ctrl.state;
}

/**
 * @brief 获取详细状态信息
 */
g4_error_t g4_get_status(g4_status_t *status)
{
    if (!status)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    memcpy(status, &g4_ctrl.status, sizeof(g4_status_t));

    return G4_SUCCESS;
}

/**
 * @brief 获取信号强度
 */
g4_error_t g4_get_signal_strength(int8_t *rssi, uint8_t *quality)
{
    if (!rssi || !quality)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    char response[64];
    if (g4_send_at_cmd("AT+CSQ", response, sizeof(response), 3000) != G4_SUCCESS)
    {
        return G4_ERROR_AT_COMMAND;
    }

    // 解析响应: +CSQ: <rssi>,<ber>
    char rssi_str[8], ber_str[8];
    if (sscanf(response, "+CSQ: %[^,],%s", rssi_str, ber_str) == 2)
    {
        int rssi_val = atoi(rssi_str);
        if (rssi_val == 99)
        {
            *rssi = -113; // 无信号
            *quality = 0;
        }
        else
        {
            *rssi = -113 + rssi_val * 2; // 转换为dBm
            *quality = rssi_val;
        }

        g4_ctrl.status.rssi = *rssi;
        g4_ctrl.status.signal_quality = *quality;
        g4_ctrl.status.signal_level = g4_rssi_to_level(*rssi);

        return G4_SUCCESS;
    }

    return G4_ERROR_AT_COMMAND;
}

/**
 * @brief 连接网络
 */
g4_error_t g4_connect_network(void)
{
    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    g4_error_t result;
    char cmd[128];

    // 1. 检查SIM卡状态
    if (g4_send_at_cmd("AT+CPIN?", NULL, 0, 3000) != G4_SUCCESS)
    {
        return G4_ERROR_SIM_NOT_READY;
    }

    g4_ctrl.state = G4_STATE_SIM_READY;

    // 2. 设置APN
    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", g4_ctrl.config.apn);
    if (g4_send_at_cmd(cmd, NULL, 0, 5000) != G4_SUCCESS)
    {
        return G4_ERROR_AT_COMMAND;
    }

    // 3. 激活PDP上下文
    if (g4_send_at_cmd("AT+CGACT=1,1", NULL, 0, 30000) != G4_SUCCESS)
    {
        return G4_ERROR_NETWORK;
    }

    g4_ctrl.state = G4_STATE_PDP_ACTIVATED;

    // 4. 获取IP地址
    char response[64];
    if (g4_send_at_cmd("AT+CGPADDR=1", response, sizeof(response), 5000) == G4_SUCCESS)
    {
        // 解析IP地址
        char ip_str[16];
        if (sscanf(response, "+CGPADDR: 1,\"%[^\"]\"", ip_str) == 1)
        {
            strncpy(g4_ctrl.status.local_ip, ip_str, sizeof(g4_ctrl.status.local_ip) - 1);
        }
    }

    g4_ctrl.state = G4_STATE_CONNECTED;

    return G4_SUCCESS;
}

/**
 * @brief 断开网络连接
 */
g4_error_t g4_disconnect_network(void)
{
    if (!g4_ctrl.initialized)
    {
        return G4_ERROR_NOT_INITIALIZED;
    }

    // 断开所有Socket连接
    for (int i = 0; i < G4_MAX_SOCKETS; i++)
    {
        if (g4_ctrl.sockets[i].is_used)
        {
            g4_socket_close(i);
        }
    }

    // 去激活PDP上下文
    g4_send_at_cmd("AT+CGACT=0,1", NULL, 0, 10000);

    g4_ctrl.state = G4_STATE_POWER_ON;
    memset(g4_ctrl.status.local_ip, 0, sizeof(g4_ctrl.status.local_ip));

    return G4_SUCCESS;
}

/**
 * @brief 检查网络连接状态
 */
bool g4_is_network_connected(void)
{
    return (g4_ctrl.state == G4_STATE_CONNECTED);
}

/**
 * @brief HTTP GET请求
 */
g4_error_t g4_http_get(const char *url, char *response, uint16_t response_len)
{
    if (!url || !response || response_len == 0)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    if (!g4_is_network_connected())
    {
        return G4_ERROR_NOT_CONNECTED;
    }

    char cmd[256];

    // 配置HTTP参数
    snprintf(cmd, sizeof(cmd), "AT+QHTTPURL=%d,80", (int)strlen(url));
    if (g4_send_at_cmd(cmd, NULL, 0, 5000) != G4_SUCCESS)
    {
        return G4_ERROR_HTTP;
    }

    // 发送URL
    if (uart_send_string(g4_ctrl.config.uart_port, url) != UART_SUCCESS)
    {
        return G4_ERROR_HTTP;
    }

    // 发送GET请求
    if (g4_send_at_cmd("AT+QHTTPGET=80", NULL, 0, 80000) != G4_SUCCESS)
    {
        return G4_ERROR_HTTP;
    }

    // 读取响应
    snprintf(cmd, sizeof(cmd), "AT+QHTTPREAD=80");
    if (g4_send_at_cmd(cmd, response, response_len, 5000) != G4_SUCCESS)
    {
        return G4_ERROR_HTTP;
    }

    return G4_SUCCESS;
}

/**
 * @brief 创建Socket连接
 */
g4_error_t g4_socket_create(const g4_socket_config_t *config, uint8_t *socket_id)
{
    if (!config || !socket_id)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    if (!g4_is_network_connected())
    {
        return G4_ERROR_NOT_CONNECTED;
    }

    // 分配Socket ID
    uint8_t id = g4_allocate_socket();
    if (id >= G4_MAX_SOCKETS)
    {
        return G4_ERROR_MEMORY;
    }

    char cmd[128];
    const char *protocol = config->is_tcp ? "TCP" : "UDP";

    // 创建Socket
    snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,%d,0",
             id, protocol, config->remote_host, config->remote_port, config->local_port);

    if (g4_send_at_cmd(cmd, NULL, 0, config->timeout_ms) != G4_SUCCESS)
    {
        g4_free_socket(id);
        return G4_ERROR_CONNECTION_FAILED;
    }

    // 保存连接信息
    g4_ctrl.sockets[id].is_used = true;
    g4_ctrl.sockets[id].is_tcp = config->is_tcp;
    strncpy(g4_ctrl.sockets[id].remote_host, config->remote_host, sizeof(g4_ctrl.sockets[id].remote_host) - 1);
    g4_ctrl.sockets[id].remote_port = config->remote_port;
    g4_ctrl.sockets[id].local_port = config->local_port;
    g4_ctrl.sockets[id].is_connected = true;
    g4_ctrl.sockets[id].last_activity = timer_get_tick();

    *socket_id = id;

    return G4_SUCCESS;
}

/**
 * @brief 关闭Socket连接
 */
g4_error_t g4_socket_close(uint8_t socket_id)
{
    if (socket_id >= G4_MAX_SOCKETS)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    if (!g4_ctrl.sockets[socket_id].is_used)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QICLOSE=%d", socket_id);
    g4_send_at_cmd(cmd, NULL, 0, 5000);

    g4_free_socket(socket_id);

    return G4_SUCCESS;
}

/**
 * @brief 发送Socket数据
 */
g4_error_t g4_socket_send(uint8_t socket_id, const uint8_t *data, uint16_t data_len)
{
    if (socket_id >= G4_MAX_SOCKETS || !data || data_len == 0)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    if (!g4_ctrl.sockets[socket_id].is_used || !g4_ctrl.sockets[socket_id].is_connected)
    {
        return G4_ERROR_NOT_CONNECTED;
    }

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+QISEND=%d,%d", socket_id, data_len);

    if (g4_send_at_cmd(cmd, NULL, 0, 5000) != G4_SUCCESS)
    {
        return G4_ERROR_NETWORK;
    }

    // 发送数据
    if (uart_send_data(g4_ctrl.config.uart_port, data, data_len) != UART_SUCCESS)
    {
        return G4_ERROR_NETWORK;
    }

    g4_ctrl.sockets[socket_id].last_activity = timer_get_tick();
    g4_ctrl.status.data_sent_bytes += data_len;

    return G4_SUCCESS;
}

/**
 * @brief 发送AT命令
 */
g4_error_t g4_send_at_command(const char *command, char *response, uint16_t response_len, uint32_t timeout_ms)
{
    return g4_send_at_cmd(command, response, response_len, timeout_ms);
}

/**
 * @brief 获取状态名称
 */
const char *g4_get_state_name(g4_state_t state)
{
    if (state < sizeof(g4_state_names) / sizeof(g4_state_names[0]))
    {
        return g4_state_names[state];
    }
    return "UNKNOWN";
}

/**
 * @brief 获取网络类型名称
 */
const char *g4_get_net_type_name(g4_net_type_t net_type)
{
    if (net_type < sizeof(g4_net_type_names) / sizeof(g4_net_type_names[0]))
    {
        return g4_net_type_names[net_type];
    }
    return "UNKNOWN";
}

/**
 * @brief 获取错误码描述
 */
const char *g4_get_error_string(g4_error_t error)
{
    if (error < sizeof(g4_error_strings) / sizeof(g4_error_strings[0]))
    {
        return g4_error_strings[error];
    }
    return "Unknown error";
}

//==============================================================================
// 私有函数实现
//==============================================================================

/**
 * @brief 发送AT命令
 */
static g4_error_t g4_send_at_cmd(const char *cmd, char *response, uint16_t response_len, uint32_t timeout_ms)
{
    if (!cmd)
    {
        return G4_ERROR_INVALID_PARAM;
    }

    // 清空接收缓冲区
    g4_ctrl.rx_index = 0;
    memset(g4_ctrl.rx_buffer, 0, sizeof(g4_ctrl.rx_buffer));

    // 发送命令
    snprintf(g4_ctrl.at_buffer, sizeof(g4_ctrl.at_buffer), "%s\r\n", cmd);
    if (uart_send_string(g4_ctrl.config.uart_port, g4_ctrl.at_buffer) != UART_SUCCESS)
    {
        return G4_ERROR_AT_COMMAND;
    }

    g4_ctrl.at_commands_sent++;

    // 等待响应
    uint32_t start_time = timer_get_tick();
    while (timer_get_tick() - start_time < timeout_ms)
    {
        g4_process_received_data();

        if (strstr(g4_ctrl.rx_buffer, "OK") || strstr(g4_ctrl.rx_buffer, "ERROR"))
        {
            g4_ctrl.at_responses_received++;

            if (response && response_len > 0)
            {
                strncpy(response, g4_ctrl.rx_buffer, response_len - 1);
                response[response_len - 1] = '\0';
            }

            if (strstr(g4_ctrl.rx_buffer, "OK"))
            {
                return G4_SUCCESS;
            }
            else
            {
                g4_ctrl.network_errors++;
                return G4_ERROR_AT_COMMAND;
            }
        }

        timer_delay_ms(10);
    }

    return G4_ERROR_TIMEOUT;
}

/**
 * @brief 处理接收数据
 */
static void g4_process_received_data(void)
{
    uint8_t byte;
    while (uart_receive_byte(g4_ctrl.config.uart_port, &byte) == UART_SUCCESS)
    {
        if (g4_ctrl.rx_index < G4_RX_BUFFER_SIZE - 1)
        {
            g4_ctrl.rx_buffer[g4_ctrl.rx_index++] = byte;
            g4_ctrl.rx_buffer[g4_ctrl.rx_index] = '\0';
        }

        g4_ctrl.status.data_received_bytes++;
    }
}

/**
 * @brief 更新状态信息
 */
static void g4_update_status(void)
{
    g4_ctrl.status.state = g4_ctrl.state;
    g4_ctrl.status.uptime_seconds = (timer_get_tick() - g4_ctrl.init_time) / 1000;
}

/**
 * @brief 分配Socket ID
 */
static uint8_t g4_allocate_socket(void)
{
    for (uint8_t i = 0; i < G4_MAX_SOCKETS; i++)
    {
        if (!g4_ctrl.sockets[i].is_used)
        {
            return i;
        }
    }
    return G4_MAX_SOCKETS; // 无可用Socket
}

/**
 * @brief 释放Socket
 */
static void g4_free_socket(uint8_t socket_id)
{
    if (socket_id < G4_MAX_SOCKETS)
    {
        memset(&g4_ctrl.sockets[socket_id], 0, sizeof(g4_socket_info_t));
    }
}

/**
 * @brief RSSI转信号等级
 */
static g4_signal_level_t g4_rssi_to_level(int8_t rssi)
{
    if (rssi >= -70)
        return G4_SIGNAL_EXCELLENT;
    if (rssi >= -85)
        return G4_SIGNAL_GOOD;
    if (rssi >= -100)
        return G4_SIGNAL_FAIR;
    if (rssi >= -110)
        return G4_SIGNAL_POOR;
    return G4_SIGNAL_NONE;
}