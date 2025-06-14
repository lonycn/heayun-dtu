#include "mqtt.h"
#include "system.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//==============================================================================
// 全局变量
//==============================================================================

static mqtt_config_t g_mqtt_config;
static mqtt_state_t g_mqtt_state = MQTT_STATE_DISCONNECTED;
static bool g_mqtt_initialized = false;
static mqtt_statistics_t g_mqtt_stats = {0};
static mqtt_event_callback_t g_event_callback = NULL;
static uint16_t g_message_id_counter = 1;

//==============================================================================
// 核心API实现
//==============================================================================

int mqtt_init(const mqtt_config_t *config)
{
    if (g_mqtt_initialized)
    {
        return MQTT_SUCCESS; // 已经初始化
    }

    if (!config)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    // 复制配置
    g_mqtt_config = *config;

    // 初始化状态
    g_mqtt_state = MQTT_STATE_DISCONNECTED;
    g_mqtt_initialized = true;

    // 重置统计信息
    memset(&g_mqtt_stats, 0, sizeof(g_mqtt_stats));

    printf("MQTT: 模块初始化完成，服务器 %s:%d\n",
           g_mqtt_config.broker_host, g_mqtt_config.broker_port);
    return MQTT_SUCCESS;
}

int mqtt_deinit(void)
{
    if (!g_mqtt_initialized)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    // 断开连接
    if (g_mqtt_state == MQTT_STATE_CONNECTED)
    {
        mqtt_disconnect();
    }

    g_mqtt_initialized = false;
    g_mqtt_state = MQTT_STATE_DISCONNECTED;

    return MQTT_SUCCESS;
}

int mqtt_set_event_callback(mqtt_event_callback_t callback)
{
    if (!g_mqtt_initialized)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    g_event_callback = callback;
    return MQTT_SUCCESS;
}

//==============================================================================
// 连接管理
//==============================================================================

int mqtt_connect(void)
{
    if (!g_mqtt_initialized)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    if (g_mqtt_state == MQTT_STATE_CONNECTED)
    {
        return MQTT_SUCCESS; // 已经连接
    }

    g_mqtt_state = MQTT_STATE_CONNECTING;

    // 模拟连接过程
    printf("MQTT: 连接到服务器 %s:%d\n",
           g_mqtt_config.broker_host, g_mqtt_config.broker_port);

    // 简化实现：直接设置为已连接
    g_mqtt_state = MQTT_STATE_CONNECTED;
    g_mqtt_stats.last_ping_time = system_get_tick();

    // 触发连接事件
    if (g_event_callback)
    {
        mqtt_event_t event = {
            .event = MQTT_EVENT_CONNECTED,
            .data = NULL,
            .data_len = 0,
            .error_code = 0};
        g_event_callback(&event);
    }

    return MQTT_SUCCESS;
}

int mqtt_disconnect(void)
{
    if (!g_mqtt_initialized)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    if (g_mqtt_state == MQTT_STATE_DISCONNECTED)
    {
        return MQTT_SUCCESS; // 已经断开
    }

    g_mqtt_state = MQTT_STATE_DISCONNECTING;

    printf("MQTT: 断开连接\n");

    g_mqtt_state = MQTT_STATE_DISCONNECTED;

    // 触发断开事件
    if (g_event_callback)
    {
        mqtt_event_t event = {
            .event = MQTT_EVENT_DISCONNECTED,
            .data = NULL,
            .data_len = 0,
            .error_code = 0};
        g_event_callback(&event);
    }

    return MQTT_SUCCESS;
}

mqtt_state_t mqtt_get_state(void)
{
    return g_mqtt_state;
}

bool mqtt_is_connected(void)
{
    return (g_mqtt_state == MQTT_STATE_CONNECTED);
}

//==============================================================================
// 消息发布
//==============================================================================

int mqtt_publish(const char *topic, const void *payload, uint16_t len,
                 uint8_t qos, bool retain)
{
    if (!g_mqtt_initialized || !topic || !payload)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    if (g_mqtt_state != MQTT_STATE_CONNECTED)
    {
        return MQTT_ERROR_NOT_CONNECTED;
    }

    printf("MQTT: 发布消息到主题 '%s'，长度 %d\n", topic, len);

    g_mqtt_stats.tx_count++;
    g_mqtt_stats.bytes_sent += len;
    g_mqtt_stats.last_message_time = system_get_tick();

    return MQTT_SUCCESS;
}

int mqtt_publish_json(const char *topic, const char *json_str, uint8_t qos)
{
    if (!json_str)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    return mqtt_publish(topic, json_str, strlen(json_str), qos, false);
}

//==============================================================================
// 订阅管理
//==============================================================================

int mqtt_subscribe(const char *topic, uint8_t qos)
{
    if (!g_mqtt_initialized || !topic)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    if (g_mqtt_state != MQTT_STATE_CONNECTED)
    {
        return MQTT_ERROR_NOT_CONNECTED;
    }

    printf("MQTT: 订阅主题 '%s'，QoS %d\n", topic, qos);
    return MQTT_SUCCESS;
}

int mqtt_unsubscribe(const char *topic)
{
    if (!g_mqtt_initialized || !topic)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    if (g_mqtt_state != MQTT_STATE_CONNECTED)
    {
        return MQTT_ERROR_NOT_CONNECTED;
    }

    printf("MQTT: 取消订阅主题 '%s'\n", topic);
    return MQTT_SUCCESS;
}

//==============================================================================
// 任务处理
//==============================================================================

void mqtt_task(void)
{
    if (!g_mqtt_initialized)
    {
        return;
    }

    uint32_t current_time = system_get_tick();

    // 心跳检查
    if (g_mqtt_state == MQTT_STATE_CONNECTED)
    {
        if (current_time - g_mqtt_stats.last_ping_time > (g_mqtt_config.keep_alive * 1000))
        {
            mqtt_ping();
        }
    }

    // 模拟接收消息
    static uint32_t last_rx_time = 0;
    if (g_mqtt_state == MQTT_STATE_CONNECTED &&
        current_time - last_rx_time > 60000) // 每分钟模拟接收一次
    {
        if (g_event_callback)
        {
            mqtt_event_t event = {
                .event = MQTT_EVENT_MESSAGE_RECEIVED,
                .data = (void *)"test message",
                .data_len = 12,
                .error_code = 0};
            g_event_callback(&event);
        }

        g_mqtt_stats.rx_count++;
        g_mqtt_stats.bytes_received += 12;
        last_rx_time = current_time;
    }
}

//==============================================================================
// 工具函数
//==============================================================================

int mqtt_ping(void)
{
    if (!g_mqtt_initialized)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    if (g_mqtt_state != MQTT_STATE_CONNECTED)
    {
        return MQTT_ERROR_NOT_CONNECTED;
    }

    printf("MQTT: 发送心跳\n");
    g_mqtt_stats.last_ping_time = system_get_tick();
    return MQTT_SUCCESS;
}

uint16_t mqtt_get_next_message_id(void)
{
    return g_message_id_counter++;
}

int mqtt_get_statistics(mqtt_statistics_t *stats)
{
    if (!g_mqtt_initialized || !stats)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    *stats = g_mqtt_stats;
    return MQTT_SUCCESS;
}

//==============================================================================
// JSON编码函数
//==============================================================================

int mqtt_encode_sensor_data(char *buffer, uint16_t size, const mqtt_sensor_data_t *data)
{
    if (!buffer || size == 0 || !data)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    int len = snprintf(buffer, size,
                       "{"
                       "\"temperature\":%.1f,"
                       "\"humidity\":%.1f,"
                       "\"voltage\":%.2f,"
                       "\"current\":%.2f,"
                       "\"power\":%.2f,"
                       "\"timestamp\":%u"
                       "}",
                       data->temperature,
                       data->humidity,
                       data->voltage,
                       data->current,
                       data->power,
                       system_get_tick());

    return (len > 0 && len < (int)size) ? len : MQTT_ERROR_NO_MEMORY;
}

int mqtt_encode_device_status(char *buffer, uint16_t size, const device_status_t *status)
{
    if (!buffer || size == 0 || !status)
    {
        return MQTT_ERROR_INVALID_PARAM;
    }

    int len = snprintf(buffer, size,
                       "{"
                       "\"modbus_online\":%s,"
                       "\"lora_connected\":%s,"
                       "\"storage_normal\":%s,"
                       "\"alarm_active\":%s,"
                       "\"uptime\":%u,"
                       "\"free_memory\":%u"
                       "}",
                       status->modbus_online ? "true" : "false",
                       status->lora_connected ? "true" : "false",
                       status->storage_normal ? "true" : "false",
                       status->alarm_active ? "true" : "false",
                       status->uptime,
                       status->free_memory);

    return (len > 0 && len < (int)size) ? len : MQTT_ERROR_NO_MEMORY;
}

//==============================================================================
// 状态名称函数
//==============================================================================

const char *mqtt_get_state_name(mqtt_state_t state)
{
    switch (state)
    {
    case MQTT_STATE_DISCONNECTED:
        return "未连接";
    case MQTT_STATE_CONNECTING:
        return "连接中";
    case MQTT_STATE_CONNECTED:
        return "已连接";
    case MQTT_STATE_DISCONNECTING:
        return "断开中";
    case MQTT_STATE_ERROR:
        return "错误";
    default:
        return "未知";
    }
}