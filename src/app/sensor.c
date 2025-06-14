/**
 * @file sensor.c
 * @brief 憨云DTU传感器模块实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 传感器数据采集、处理和状态管理实现
 * 支持多通道ADC采集、数据滤波、温湿度转换等功能
 */

#include "sensor.h"
#include "system.h"
#include "adc.h"
#include "gpio.h"
#include <string.h>
#include <math.h>

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 传感器通道控制块
 */
typedef struct
{
    sensor_config_t config;                     // 配置参数
    sensor_data_t data;                         // 当前数据
    sensor_stats_t stats;                       // 统计信息
    uint16_t filter_buffer[SENSOR_FILTER_SIZE]; // 滤波缓冲区
    uint8_t filter_index;                       // 滤波缓冲区索引
    uint8_t filter_count;                       // 有效滤波数据个数
    uint32_t last_sample_time;                  // 上次采样时间
    float min_threshold;                        // 最小报警阈值
    float max_threshold;                        // 最大报警阈值
    bool threshold_enabled;                     // 阈值报警使能
} sensor_channel_t;

/**
 * @brief 传感器模块控制块
 */
typedef struct
{
    sensor_channel_t channels[SENSOR_MAX_CHANNELS]; // 传感器通道
    bool initialized;                               // 初始化标志
    bool scan_enabled;                              // 扫描使能
    uint32_t scan_count;                            // 扫描计数
    uint32_t last_scan_time;                        // 上次扫描时间
} sensor_control_t;

// 全局控制块
static sensor_control_t g_sensor = {0};

// ============================================================================
// 内部函数声明
// ============================================================================

static float sensor_convert_to_physical(uint8_t channel, uint16_t raw_value);
static uint16_t sensor_apply_filter(uint8_t channel, uint16_t raw_value);
static void sensor_update_statistics(uint8_t channel, float value);
static bool sensor_check_threshold(uint8_t channel, float value);

// ============================================================================
// 公共接口实现
// ============================================================================

/**
 * @brief 传感器模块初始化
 */
bool sensor_init(void)
{
    if (g_sensor.initialized)
    {
        sensor_deinit();
    }

    // 清空控制块
    memset(&g_sensor, 0, sizeof(sensor_control_t));

    // 初始化默认配置
    for (uint8_t i = 0; i < SENSOR_MAX_CHANNELS; i++)
    {
        sensor_channel_t *ch = &g_sensor.channels[i];

        // 默认配置
        ch->config.channel = i;
        ch->config.type = SENSOR_TYPE_ANALOG;
        ch->config.enabled = false;
        ch->config.sample_period = 1000; // 1秒
        ch->config.scale_factor = 1.0f;
        ch->config.offset = 0.0f;
        ch->config.min_value = 0.0f;
        ch->config.max_value = 4095.0f;
        ch->config.filter_size = SENSOR_FILTER_SIZE;

        // 初始化数据
        ch->data.raw_value = 0;
        ch->data.physical_value = 0.0f;
        ch->data.status = SENSOR_STATUS_OFFLINE;
        ch->data.timestamp = 0;
        ch->data.sample_count = 0;
        ch->data.data_valid = false;

        // 初始化统计信息
        ch->stats.total_samples = 0;
        ch->stats.valid_samples = 0;
        ch->stats.error_count = 0;
        ch->stats.overflow_count = 0;
        ch->stats.underflow_count = 0;
        ch->stats.min_value = INFINITY;
        ch->stats.max_value = -INFINITY;
        ch->stats.average_value = 0.0f;

        // 初始化滤波器
        ch->filter_index = 0;
        ch->filter_count = 0;
        memset(ch->filter_buffer, 0, sizeof(ch->filter_buffer));

        // 阈值设置
        ch->min_threshold = -INFINITY;
        ch->max_threshold = INFINITY;
        ch->threshold_enabled = false;

        ch->last_sample_time = 0;
    }

    // 配置专用传感器通道
    // 温度传感器 (通道0)
    sensor_config_t temp_config = {
        .channel = SENSOR_TEMP_CHANNEL,
        .type = SENSOR_TYPE_TEMPERATURE,
        .enabled = true,
        .sample_period = 1000,
        .scale_factor = 0.1f, // 0.1°C精度
        .offset = -400.0f,    // 偏移到-40°C起点
        .min_value = -40.0f,  // -40°C
        .max_value = 100.0f,  // 100°C
        .filter_size = 4};
    sensor_config(SENSOR_TEMP_CHANNEL, &temp_config);

    // 湿度传感器 (通道1)
    sensor_config_t humidity_config = {
        .channel = SENSOR_HUMIDITY_CHANNEL,
        .type = SENSOR_TYPE_HUMIDITY,
        .enabled = true,
        .sample_period = 1000,
        .scale_factor = 0.1f, // 0.1%RH精度
        .offset = 0.0f,
        .min_value = 0.0f,   // 0%RH
        .max_value = 100.0f, // 100%RH
        .filter_size = 4};
    sensor_config(SENSOR_HUMIDITY_CHANNEL, &humidity_config);

    // 电压监测 (通道2)
    sensor_config_t voltage_config = {
        .channel = SENSOR_VOLTAGE_CHANNEL,
        .type = SENSOR_TYPE_VOLTAGE,
        .enabled = true,
        .sample_period = 2000, // 2秒采样
        .scale_factor = 0.01f, // 0.01V精度
        .offset = 0.0f,
        .min_value = 0.0f, // 0V
        .max_value = 5.0f, // 5V
        .filter_size = 8};
    sensor_config(SENSOR_VOLTAGE_CHANNEL, &voltage_config);

    g_sensor.initialized = true;
    g_sensor.scan_enabled = false;
    g_sensor.scan_count = 0;
    g_sensor.last_scan_time = system_get_tick();

    debug_printf("[SENSOR] Module initialized successfully\n");
    return true;
}

/**
 * @brief 传感器模块反初始化
 */
void sensor_deinit(void)
{
    if (!g_sensor.initialized)
    {
        return;
    }

    // 停止扫描
    sensor_stop_scan();

    // 清空控制块
    memset(&g_sensor, 0, sizeof(sensor_control_t));

    debug_printf("[SENSOR] Module deinitialized\n");
}

/**
 * @brief 传感器配置
 */
bool sensor_config(uint8_t channel, const sensor_config_t *config)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel) || !config)
    {
        return false;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    // 复制配置
    memcpy(&ch->config, config, sizeof(sensor_config_t));

    // 重置滤波器
    ch->filter_index = 0;
    ch->filter_count = 0;
    memset(ch->filter_buffer, 0, sizeof(ch->filter_buffer));

    // 更新状态
    if (config->enabled)
    {
        ch->data.status = SENSOR_STATUS_OK;
    }
    else
    {
        ch->data.status = SENSOR_STATUS_OFFLINE;
    }

    debug_printf("[SENSOR] Channel %d configured: type=%d, enabled=%d\n",
                 channel, config->type, config->enabled);

    return true;
}

/**
 * @brief 使能/禁用传感器通道
 */
bool sensor_enable(uint8_t channel, bool enable)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel))
    {
        return false;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    ch->config.enabled = enable;
    ch->data.status = enable ? SENSOR_STATUS_OK : SENSOR_STATUS_OFFLINE;

    if (!enable)
    {
        ch->data.data_valid = false;
        ch->filter_count = 0;
    }

    debug_printf("[SENSOR] Channel %d %s\n", channel, enable ? "enabled" : "disabled");
    return true;
}

/**
 * @brief 传感器任务函数 (周期性调用)
 */
void sensor_task(void)
{
    if (!g_sensor.initialized || !g_sensor.scan_enabled)
    {
        return;
    }

    uint32_t current_time = system_get_tick();

    // 扫描所有使能的通道
    for (uint8_t i = 0; i < SENSOR_MAX_CHANNELS; i++)
    {
        sensor_channel_t *ch = &g_sensor.channels[i];

        if (!ch->config.enabled)
        {
            continue;
        }

        // 检查采样周期
        if ((current_time - ch->last_sample_time) >= ch->config.sample_period)
        {
            ch->last_sample_time = current_time;

            // 读取原始值
            uint16_t raw_value = sensor_read_raw(i);

            if (raw_value != SENSOR_INVALID_VALUE)
            {
                // 应用滤波
                uint16_t filtered_value = sensor_apply_filter(i, raw_value);

                // 转换为物理量
                float physical_value = sensor_convert_to_physical(i, filtered_value);

                // 更新数据
                ch->data.raw_value = filtered_value;
                ch->data.physical_value = physical_value;
                ch->data.timestamp = current_time;
                ch->data.sample_count++;
                ch->data.data_valid = true;
                ch->data.status = SENSOR_STATUS_OK;

                // 更新统计信息
                sensor_update_statistics(i, physical_value);

                // 检查阈值报警
                if (ch->threshold_enabled)
                {
                    if (!sensor_check_threshold(i, physical_value))
                    {
                        ch->data.status = SENSOR_STATUS_OVERRANGE;
                    }
                }
            }
            else
            {
                // 读取失败
                ch->data.data_valid = false;
                ch->data.status = SENSOR_STATUS_ERROR;
                ch->stats.error_count++;
            }
        }
    }

    g_sensor.scan_count++;
}

/**
 * @brief 启动传感器扫描
 */
bool sensor_start_scan(void)
{
    if (!g_sensor.initialized)
    {
        return false;
    }

    g_sensor.scan_enabled = true;
    g_sensor.last_scan_time = system_get_tick();

    debug_printf("[SENSOR] Scan started\n");
    return true;
}

/**
 * @brief 停止传感器扫描
 */
bool sensor_stop_scan(void)
{
    if (!g_sensor.initialized)
    {
        return false;
    }

    g_sensor.scan_enabled = false;

    debug_printf("[SENSOR] Scan stopped\n");
    return true;
}

/**
 * @brief 读取传感器原始值
 */
uint16_t sensor_read_raw(uint8_t channel)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel))
    {
        return SENSOR_INVALID_VALUE;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    if (!ch->config.enabled)
    {
        return SENSOR_INVALID_VALUE;
    }

    // 根据传感器类型读取不同的ADC通道
    uint16_t adc_value = 0;
    bool read_success = false;

    switch (channel)
    {
    case SENSOR_TEMP_CHANNEL:
        read_success = adc_read_single(ADC_TEMP_SENSOR_CHANNEL, &adc_value, 100);
        break;
    case SENSOR_HUMIDITY_CHANNEL:
        read_success = adc_read_single(ADC_HUMIDITY_SENSOR_CHANNEL, &adc_value, 100);
        break;
    case SENSOR_VOLTAGE_CHANNEL:
        read_success = adc_read_single(ADC_VOLTAGE_MONITOR_CHANNEL, &adc_value, 100);
        break;
    default:
        read_success = adc_read_single(channel, &adc_value, 100);
        break;
    }

    if (read_success)
    {
        ch->stats.total_samples++;
        return adc_value;
    }
    else
    {
        ch->stats.error_count++;
        return SENSOR_INVALID_VALUE;
    }
}

/**
 * @brief 读取传感器物理量值
 */
float sensor_read_value(uint8_t channel)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel))
    {
        return NAN;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    if (!ch->config.enabled || !ch->data.data_valid)
    {
        return NAN;
    }

    return ch->data.physical_value;
}

/**
 * @brief 读取温度值
 */
float sensor_read_temperature(uint8_t channel)
{
    float value = sensor_read_value(channel);

    if (isnan(value))
    {
        return NAN;
    }

    // 如果是温度传感器，直接返回
    if (g_sensor.channels[channel].config.type == SENSOR_TYPE_TEMPERATURE)
    {
        return value;
    }

    // 如果是通用ADC，按温度传感器公式转换
    // 假设使用NTC热敏电阻或其他温度传感器
    // 这里使用简化的线性转换
    return value * 0.1f - 40.0f; // 0.1°C精度，-40°C偏移
}

/**
 * @brief 读取湿度值
 */
float sensor_read_humidity(uint8_t channel)
{
    float value = sensor_read_value(channel);

    if (isnan(value))
    {
        return NAN;
    }

    // 如果是湿度传感器，直接返回
    if (g_sensor.channels[channel].config.type == SENSOR_TYPE_HUMIDITY)
    {
        return value;
    }

    // 通用ADC按湿度传感器公式转换
    return value * 0.1f; // 0.1%RH精度
}

/**
 * @brief 读取电压值
 */
float sensor_read_voltage(uint8_t channel)
{
    float value = sensor_read_value(channel);

    if (isnan(value))
    {
        return NAN;
    }

    // 如果是电压传感器，直接返回
    if (g_sensor.channels[channel].config.type == SENSOR_TYPE_VOLTAGE)
    {
        return value;
    }

    // 通用ADC按电压转换 (假设3.3V参考电压，12位ADC)
    return (value / 4095.0f) * 3.3f;
}

/**
 * @brief 读取电流值
 */
float sensor_read_current(uint8_t channel)
{
    float value = sensor_read_value(channel);

    if (isnan(value))
    {
        return NAN;
    }

    // 如果是电流传感器，直接返回
    if (g_sensor.channels[channel].config.type == SENSOR_TYPE_CURRENT)
    {
        return value;
    }

    // 通用ADC按电流转换 (假设使用分流电阻采样)
    return value * 0.001f; // 1mA精度
}

/**
 * @brief 获取传感器数据
 */
bool sensor_get_data(uint8_t channel, sensor_data_t *data)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel) || !data)
    {
        return false;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    // 复制数据
    memcpy(data, &ch->data, sizeof(sensor_data_t));

    return true;
}

/**
 * @brief 获取传感器状态
 */
sensor_status_t sensor_get_status(uint8_t channel)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel))
    {
        return SENSOR_STATUS_FAULT;
    }

    return g_sensor.channels[channel].data.status;
}

/**
 * @brief 检查传感器是否在线
 */
bool sensor_is_online(uint8_t channel)
{
    sensor_status_t status = sensor_get_status(channel);
    return (status == SENSOR_STATUS_OK || status == SENSOR_STATUS_OVERRANGE);
}

/**
 * @brief 获取传感器统计信息
 */
bool sensor_get_stats(uint8_t channel, sensor_stats_t *stats)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel) || !stats)
    {
        return false;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    // 复制统计信息
    memcpy(stats, &ch->stats, sizeof(sensor_stats_t));

    return true;
}

/**
 * @brief 清除传感器统计信息
 */
bool sensor_clear_stats(uint8_t channel)
{
    if (!g_sensor.initialized)
    {
        return false;
    }

    if (channel == 0xFF)
    {
        // 清除所有通道
        for (uint8_t i = 0; i < SENSOR_MAX_CHANNELS; i++)
        {
            memset(&g_sensor.channels[i].stats, 0, sizeof(sensor_stats_t));
            g_sensor.channels[i].stats.min_value = INFINITY;
            g_sensor.channels[i].stats.max_value = -INFINITY;
        }
        debug_printf("[SENSOR] All statistics cleared\n");
    }
    else if (sensor_is_channel_valid(channel))
    {
        // 清除指定通道
        memset(&g_sensor.channels[channel].stats, 0, sizeof(sensor_stats_t));
        g_sensor.channels[channel].stats.min_value = INFINITY;
        g_sensor.channels[channel].stats.max_value = -INFINITY;
        debug_printf("[SENSOR] Channel %d statistics cleared\n", channel);
    }
    else
    {
        return false;
    }

    return true;
}

/**
 * @brief 设置传感器报警阈值
 */
bool sensor_set_threshold(uint8_t channel, float min_threshold, float max_threshold)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel))
    {
        return false;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    ch->min_threshold = min_threshold;
    ch->max_threshold = max_threshold;
    ch->threshold_enabled = true;

    debug_printf("[SENSOR] Channel %d threshold set: %.2f to %.2f\n",
                 channel, min_threshold, max_threshold);

    return true;
}

/**
 * @brief 校准传感器
 */
bool sensor_calibrate(uint8_t channel, float reference_value)
{
    if (!g_sensor.initialized || !sensor_is_channel_valid(channel))
    {
        return false;
    }

    sensor_channel_t *ch = &g_sensor.channels[channel];

    // 读取当前原始值
    uint16_t raw_value = sensor_read_raw(channel);
    if (raw_value == SENSOR_INVALID_VALUE)
    {
        return false;
    }

    // 计算新的偏移量 (简化的一点校准)
    float current_physical = sensor_convert_to_physical(channel, raw_value);
    ch->config.offset += (reference_value - current_physical);

    debug_printf("[SENSOR] Channel %d calibrated: offset=%.3f\n",
                 channel, ch->config.offset);

    return true;
}

/**
 * @brief 打印传感器状态信息 (调试用)
 */
void sensor_print_status(void)
{
    if (!g_sensor.initialized)
    {
        debug_printf("[SENSOR] Module not initialized\n");
        return;
    }

    debug_printf("\n[SENSOR] Module Status:\n");
    debug_printf("  - Initialized: %s\n", g_sensor.initialized ? "Yes" : "No");
    debug_printf("  - Scan Enabled: %s\n", g_sensor.scan_enabled ? "Yes" : "No");
    debug_printf("  - Scan Count: %lu\n", g_sensor.scan_count);

    debug_printf("\n[SENSOR] Channel Status:\n");
    for (uint8_t i = 0; i < SENSOR_MAX_CHANNELS; i++)
    {
        sensor_channel_t *ch = &g_sensor.channels[i];

        if (!ch->config.enabled)
        {
            continue;
        }

        const char *type_names[] = {"TEMP", "HUMID", "VOLT", "CURR", "ANALOG", "DIGITAL"};
        const char *status_names[] = {"OK", "OFFLINE", "ERROR", "OVER", "UNDER", "FAULT"};

        debug_printf("  Ch%d: %s, Status=%s, Value=%.2f, Samples=%d\n",
                     i,
                     (ch->config.type < SENSOR_TYPE_COUNT) ? type_names[ch->config.type] : "UNKNOWN",
                     (ch->data.status < SENSOR_STATUS_COUNT) ? status_names[ch->data.status] : "UNKNOWN",
                     ch->data.physical_value,
                     ch->data.sample_count);
    }
    debug_printf("\n");
}

/**
 * @brief 打印传感器统计信息 (调试用)
 */
void sensor_print_stats(void)
{
    if (!g_sensor.initialized)
    {
        debug_printf("[SENSOR] Module not initialized\n");
        return;
    }

    debug_printf("\n[SENSOR] Statistics:\n");
    for (uint8_t i = 0; i < SENSOR_MAX_CHANNELS; i++)
    {
        sensor_channel_t *ch = &g_sensor.channels[i];

        if (!ch->config.enabled || ch->stats.total_samples == 0)
        {
            continue;
        }

        debug_printf("  Ch%d: Total=%lu, Valid=%lu, Errors=%lu\n",
                     i, ch->stats.total_samples, ch->stats.valid_samples, ch->stats.error_count);
        debug_printf("        Min=%.2f, Max=%.2f, Avg=%.2f\n",
                     ch->stats.min_value, ch->stats.max_value, ch->stats.average_value);
    }
    debug_printf("\n");
}

/**
 * @brief 传感器处理函数 (供main.c调用，避免函数名冲突)
 */
void sensor_process(void)
{
    // 直接调用内部的sensor_task函数
    sensor_task();
}

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 转换原始值为物理量
 */
static float sensor_convert_to_physical(uint8_t channel, uint16_t raw_value)
{
    sensor_channel_t *ch = &g_sensor.channels[channel];

    // 基本线性转换: physical = (raw * scale) + offset
    float physical = (float)raw_value * ch->config.scale_factor + ch->config.offset;

    // 限制在有效范围内
    if (physical < ch->config.min_value)
    {
        physical = ch->config.min_value;
        ch->stats.underflow_count++;
    }
    else if (physical > ch->config.max_value)
    {
        physical = ch->config.max_value;
        ch->stats.overflow_count++;
    }

    return physical;
}

/**
 * @brief 应用滤波器
 */
static uint16_t sensor_apply_filter(uint8_t channel, uint16_t raw_value)
{
    sensor_channel_t *ch = &g_sensor.channels[channel];

    // 添加新数据到滤波缓冲区
    ch->filter_buffer[ch->filter_index] = raw_value;
    ch->filter_index = (ch->filter_index + 1) % ch->config.filter_size;

    if (ch->filter_count < ch->config.filter_size)
    {
        ch->filter_count++;
    }

    // 计算移动平均值
    uint32_t sum = 0;
    for (uint8_t i = 0; i < ch->filter_count; i++)
    {
        sum += ch->filter_buffer[i];
    }

    return (uint16_t)(sum / ch->filter_count);
}

/**
 * @brief 更新统计信息
 */
static void sensor_update_statistics(uint8_t channel, float value)
{
    sensor_channel_t *ch = &g_sensor.channels[channel];

    ch->stats.valid_samples++;

    // 更新最值
    if (value < ch->stats.min_value)
    {
        ch->stats.min_value = value;
    }
    if (value > ch->stats.max_value)
    {
        ch->stats.max_value = value;
    }

    // 更新平均值 (滑动平均)
    if (ch->stats.valid_samples == 1)
    {
        ch->stats.average_value = value;
    }
    else
    {
        // 简单的指数移动平均
        float alpha = 0.1f; // 平滑因子
        ch->stats.average_value = alpha * value + (1.0f - alpha) * ch->stats.average_value;
    }
}

/**
 * @brief 检查阈值报警
 */
static bool sensor_check_threshold(uint8_t channel, float value)
{
    sensor_channel_t *ch = &g_sensor.channels[channel];

    if (!ch->threshold_enabled)
    {
        return true;
    }

    if (value < ch->min_threshold || value > ch->max_threshold)
    {
        debug_printf("[SENSOR] Ch%d threshold alarm: %.2f (%.2f~%.2f)\n",
                     channel, value, ch->min_threshold, ch->max_threshold);
        return false;
    }

    return true;
}