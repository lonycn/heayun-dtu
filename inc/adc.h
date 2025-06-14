/**
 * @file adc.h
 * @brief 憨云DTU ADC驱动接口
 * @version 1.0.0
 * @date 2025-03-28
 *
 * NANO100B ADC驱动，专为传感器数据采集优化
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ADC配置定义
// ============================================================================

// ADC通道定义
typedef enum
{
    ADC_CHANNEL_0 = 0, // ADC通道0
    ADC_CHANNEL_1 = 1, // ADC通道1
    ADC_CHANNEL_2 = 2, // ADC通道2
    ADC_CHANNEL_3 = 3, // ADC通道3
    ADC_CHANNEL_4 = 4, // ADC通道4
    ADC_CHANNEL_5 = 5, // ADC通道5
    ADC_CHANNEL_6 = 6, // ADC通道6
    ADC_CHANNEL_7 = 7, // ADC通道7
    ADC_CHANNEL_COUNT = 8
} adc_channel_t;

// ADC分辨率
typedef enum
{
    ADC_RESOLUTION_12BIT = 12 // 12位分辨率 (NANO100B支持)
} adc_resolution_t;

// ADC采样时间
typedef enum
{
    ADC_SAMPLE_TIME_1 = 1,  // 1个ADC时钟周期
    ADC_SAMPLE_TIME_2 = 2,  // 2个ADC时钟周期
    ADC_SAMPLE_TIME_4 = 4,  // 4个ADC时钟周期
    ADC_SAMPLE_TIME_8 = 8,  // 8个ADC时钟周期
    ADC_SAMPLE_TIME_16 = 16 // 16个ADC时钟周期
} adc_sample_time_t;

// ADC触发模式
typedef enum
{
    ADC_TRIGGER_SOFTWARE = 0, // 软件触发
    ADC_TRIGGER_TIMER = 1,    // 定时器触发
    ADC_TRIGGER_EXTERNAL = 2  // 外部触发
} adc_trigger_mode_t;

// ADC配置结构体
typedef struct
{
    adc_channel_t channel;           // ADC通道
    adc_resolution_t resolution;     // 分辨率
    adc_sample_time_t sample_time;   // 采样时间
    adc_trigger_mode_t trigger_mode; // 触发模式
    bool enable_interrupt;           // 中断使能
} adc_config_t;

// ADC状态
typedef enum
{
    ADC_STATUS_OK = 0,      // 正常
    ADC_STATUS_BUSY = 1,    // 忙碌
    ADC_STATUS_ERROR = 2,   // 错误
    ADC_STATUS_TIMEOUT = 3, // 超时
    ADC_STATUS_OVERRUN = 4  // 数据溢出
} adc_status_t;

// ADC转换完成回调函数类型
typedef void (*adc_callback_t)(adc_channel_t channel, uint16_t value);

// ============================================================================
// 传感器通道映射 (为应用层提供语义化接口)
// ============================================================================

#define ADC_TEMP_SENSOR_CHANNEL ADC_CHANNEL_0     // 温度传感器通道
#define ADC_HUMIDITY_SENSOR_CHANNEL ADC_CHANNEL_1 // 湿度传感器通道
#define ADC_VOLTAGE_MONITOR_CHANNEL ADC_CHANNEL_2 // 电压监测通道
#define ADC_CURRENT_MONITOR_CHANNEL ADC_CHANNEL_3 // 电流监测通道

// ADC采样缓冲区大小 (为8KB RAM优化)
#define ADC_SAMPLE_BUFFER_SIZE 32 // 每通道32个采样

// ============================================================================
// ADC驱动接口
// ============================================================================

/**
 * @brief ADC模块初始化
 * @return true: 成功, false: 失败
 */
bool adc_init(void);

/**
 * @brief 配置ADC通道
 * @param config ADC配置结构体
 * @return true: 成功, false: 失败
 */
bool adc_config_channel(const adc_config_t *config);

/**
 * @brief 启用ADC通道
 * @param channel ADC通道
 * @param enable true: 启用, false: 禁用
 * @return true: 成功, false: 失败
 */
bool adc_enable_channel(adc_channel_t channel, bool enable);

/**
 * @brief 单次ADC转换 (阻塞方式)
 * @param channel ADC通道
 * @param value 转换结果指针
 * @param timeout_ms 超时时间(毫秒)
 * @return true: 成功, false: 失败
 */
bool adc_read_single(adc_channel_t channel, uint16_t *value, uint32_t timeout_ms);

/**
 * @brief 启动连续ADC转换 (非阻塞方式)
 * @param channel ADC通道
 * @param callback 转换完成回调函数
 * @return true: 成功, false: 失败
 */
bool adc_start_continuous(adc_channel_t channel, adc_callback_t callback);

/**
 * @brief 停止连续ADC转换
 * @param channel ADC通道
 * @return true: 成功, false: 失败
 */
bool adc_stop_continuous(adc_channel_t channel);

/**
 * @brief 多通道ADC转换
 * @param channels 通道数组
 * @param channel_count 通道数量
 * @param values 结果数组
 * @param timeout_ms 超时时间(毫秒)
 * @return 成功转换的通道数量
 */
uint8_t adc_read_multiple(const adc_channel_t *channels, uint8_t channel_count,
                          uint16_t *values, uint32_t timeout_ms);

/**
 * @brief 获取ADC原始值对应的电压值 (mV)
 * @param raw_value ADC原始值
 * @param vref_mv 参考电压(mV)
 * @return 电压值(mV)
 */
uint32_t adc_raw_to_voltage(uint16_t raw_value, uint32_t vref_mv);

/**
 * @brief 校准ADC (减少误差)
 * @param channel ADC通道
 * @return true: 成功, false: 失败
 */
bool adc_calibrate(adc_channel_t channel);

// ============================================================================
// 传感器专用接口 (高级功能)
// ============================================================================

/**
 * @brief 读取温度传感器 (摄氏度 * 10)
 * @param temperature 温度值指针 (单位: 0.1°C)
 * @return true: 成功, false: 失败
 */
bool adc_read_temperature(int16_t *temperature);

/**
 * @brief 读取湿度传感器 (相对湿度 * 10)
 * @param humidity 湿度值指针 (单位: 0.1%RH)
 * @return true: 成功, false: 失败
 */
bool adc_read_humidity(uint16_t *humidity);

/**
 * @brief 读取电源电压 (mV)
 * @param voltage 电压值指针 (单位: mV)
 * @return true: 成功, false: 失败
 */
bool adc_read_supply_voltage(uint32_t *voltage);

/**
 * @brief 读取工作电流 (mA)
 * @param current 电流值指针 (单位: mA)
 * @return true: 成功, false: 失败
 */
bool adc_read_supply_current(uint32_t *current);

/**
 * @brief 启动环境监测 (温湿度定时采集)
 * @param interval_ms 采集间隔(毫秒)
 * @param callback 数据就绪回调
 * @return true: 成功, false: 失败
 */
bool adc_start_environment_monitoring(uint32_t interval_ms,
                                      void (*callback)(int16_t temp, uint16_t humidity));

/**
 * @brief 停止环境监测
 * @return true: 成功, false: 失败
 */
bool adc_stop_environment_monitoring(void);

// ============================================================================
// 滤波和统计接口
// ============================================================================

/**
 * @brief 获取ADC通道的平均值 (基于历史采样)
 * @param channel ADC通道
 * @param sample_count 采样次数
 * @return 平均值
 */
uint16_t adc_get_average(adc_channel_t channel, uint8_t sample_count);

/**
 * @brief 获取ADC通道的最大最小值
 * @param channel ADC通道
 * @param min_value 最小值指针
 * @param max_value 最大值指针
 * @param sample_count 采样次数
 * @return true: 成功, false: 失败
 */
bool adc_get_min_max(adc_channel_t channel, uint16_t *min_value, uint16_t *max_value,
                     uint8_t sample_count);

/**
 * @brief 数字滤波 (简单移动平均)
 * @param raw_value 原始值
 * @param channel ADC通道 (用于存储滤波历史)
 * @return 滤波后的值
 */
uint16_t adc_digital_filter(uint16_t raw_value, adc_channel_t channel);

// ============================================================================
// 中断和回调接口
// ============================================================================

/**
 * @brief 设置ADC转换完成回调函数
 * @param channel ADC通道
 * @param callback 回调函数
 * @return true: 成功, false: 失败
 */
bool adc_set_callback(adc_channel_t channel, adc_callback_t callback);

/**
 * @brief ADC中断处理函数 (在中断服务程序中调用)
 */
void adc_interrupt_handler(void);

// ============================================================================
// 状态和调试接口
// ============================================================================

/**
 * @brief 获取ADC通道状态
 * @param channel ADC通道
 * @return ADC状态
 */
adc_status_t adc_get_status(adc_channel_t channel);

/**
 * @brief 获取ADC统计信息
 * @param channel ADC通道
 * @param conversion_count 转换次数
 * @param error_count 错误次数
 * @param last_value 最后一次转换值
 */
void adc_get_stats(adc_channel_t channel, uint32_t *conversion_count,
                   uint32_t *error_count, uint16_t *last_value);

/**
 * @brief 打印ADC状态信息 (调试用)
 * @param channel ADC通道
 */
void adc_print_status(adc_channel_t channel);

/**
 * @brief 打印所有ADC状态 (调试用)
 */
void adc_print_all_status(void);

/**
 * @brief ADC自检 (检查硬件是否正常)
 * @return true: 正常, false: 异常
 */
bool adc_self_test(void);

#endif // ADC_H