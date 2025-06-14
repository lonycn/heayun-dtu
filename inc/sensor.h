/**
 * @file sensor.h
 * @brief 憨云DTU传感器模块接口
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 传感器数据采集、处理和状态管理接口
 * 支持多通道ADC采集、数据滤波、温湿度转换等功能
 */

#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // ============================================================================
    // 宏定义
    // ============================================================================

#define SENSOR_MAX_CHANNELS 8       // 最大传感器通道数
#define SENSOR_FILTER_SIZE 8        // 滤波缓冲区大小
#define SENSOR_INVALID_VALUE 0xFFFF // 无效数据值

// ADC通道定义
#define SENSOR_TEMP_CHANNEL 0     // 温度传感器通道
#define SENSOR_HUMIDITY_CHANNEL 1 // 湿度传感器通道
#define SENSOR_VOLTAGE_CHANNEL 2  // 电压监测通道
#define SENSOR_CURRENT_CHANNEL 3  // 电流监测通道

// 物理量范围定义
#define SENSOR_TEMP_MIN -400     // 最低温度 (-40.0°C)
#define SENSOR_TEMP_MAX 1000     // 最高温度 (100.0°C)
#define SENSOR_HUMIDITY_MIN 0    // 最低湿度 (0.0%RH)
#define SENSOR_HUMIDITY_MAX 1000 // 最高湿度 (100.0%RH)

    // ============================================================================
    // 数据类型定义
    // ============================================================================

    /**
     * @brief 传感器类型枚举
     */
    typedef enum
    {
        SENSOR_TYPE_TEMPERATURE = 0, // 温度传感器
        SENSOR_TYPE_HUMIDITY = 1,    // 湿度传感器
        SENSOR_TYPE_VOLTAGE = 2,     // 电压传感器
        SENSOR_TYPE_CURRENT = 3,     // 电流传感器
        SENSOR_TYPE_ANALOG = 4,      // 通用模拟量
        SENSOR_TYPE_DIGITAL = 5,     // 数字量
        SENSOR_TYPE_COUNT
    } sensor_type_t;

    /**
     * @brief 传感器状态枚举
     */
    typedef enum
    {
        SENSOR_STATUS_OK = 0,         // 正常
        SENSOR_STATUS_OFFLINE = 1,    // 离线
        SENSOR_STATUS_ERROR = 2,      // 错误
        SENSOR_STATUS_OVERRANGE = 3,  // 超量程
        SENSOR_STATUS_UNDERRANGE = 4, // 欠量程
        SENSOR_STATUS_FAULT = 5,      // 故障
        SENSOR_STATUS_COUNT
    } sensor_status_t;

    /**
     * @brief 传感器配置结构体
     */
    typedef struct
    {
        uint8_t channel;        // ADC通道号
        sensor_type_t type;     // 传感器类型
        bool enabled;           // 使能状态
        uint16_t sample_period; // 采样周期 (ms)
        float scale_factor;     // 比例因子
        float offset;           // 偏移量
        float min_value;        // 最小值
        float max_value;        // 最大值
        uint8_t filter_size;    // 滤波器大小
    } sensor_config_t;

    /**
     * @brief 传感器数据结构体
     */
    typedef struct
    {
        uint16_t raw_value;     // 原始ADC值
        float physical_value;   // 物理量值
        sensor_status_t status; // 传感器状态
        uint32_t timestamp;     // 时间戳
        uint16_t sample_count;  // 采样计数
        bool data_valid;        // 数据有效标志
    } sensor_data_t;

    /**
     * @brief 传感器统计信息结构体
     */
    typedef struct
    {
        uint32_t total_samples;   // 总采样次数
        uint32_t valid_samples;   // 有效采样次数
        uint32_t error_count;     // 错误次数
        uint32_t overflow_count;  // 溢出次数
        uint32_t underflow_count; // 欠量程次数
        float min_value;          // 最小值
        float max_value;          // 最大值
        float average_value;      // 平均值
    } sensor_stats_t;

    // ============================================================================
    // 函数声明
    // ============================================================================

    /**
     * @brief 传感器模块初始化
     * @return true: 成功, false: 失败
     */
    bool sensor_init(void);

    /**
     * @brief 传感器模块反初始化
     */
    void sensor_deinit(void);

    /**
     * @brief 传感器配置
     * @param channel 传感器通道号 (0-7)
     * @param config 配置参数
     * @return true: 成功, false: 失败
     */
    bool sensor_config(uint8_t channel, const sensor_config_t *config);

    /**
     * @brief 使能/禁用传感器通道
     * @param channel 传感器通道号 (0-7)
     * @param enable true: 使能, false: 禁用
     * @return true: 成功, false: 失败
     */
    bool sensor_enable(uint8_t channel, bool enable);

    /**
     * @brief 传感器任务函数 (周期性调用)
     */
    void sensor_task(void);

    /**
     * @brief 启动传感器扫描
     * @return true: 成功, false: 失败
     */
    bool sensor_start_scan(void);

    /**
     * @brief 停止传感器扫描
     * @return true: 成功, false: 失败
     */
    bool sensor_stop_scan(void);

    /**
     * @brief 读取传感器原始值
     * @param channel 传感器通道号 (0-7)
     * @return 原始ADC值，失败返回SENSOR_INVALID_VALUE
     */
    uint16_t sensor_read_raw(uint8_t channel);

    /**
     * @brief 读取传感器物理量值
     * @param channel 传感器通道号 (0-7)
     * @return 物理量值，失败返回NAN
     */
    float sensor_read_value(uint8_t channel);

    /**
     * @brief 读取温度值
     * @param channel 温度传感器通道号
     * @return 温度值 (°C)，失败返回NAN
     */
    float sensor_read_temperature(uint8_t channel);

    /**
     * @brief 读取湿度值
     * @param channel 湿度传感器通道号
     * @return 湿度值 (%RH)，失败返回NAN
     */
    float sensor_read_humidity(uint8_t channel);

    /**
     * @brief 读取电压值
     * @param channel 电压传感器通道号
     * @return 电压值 (V)，失败返回NAN
     */
    float sensor_read_voltage(uint8_t channel);

    /**
     * @brief 读取电流值
     * @param channel 电流传感器通道号
     * @return 电流值 (A)，失败返回NAN
     */
    float sensor_read_current(uint8_t channel);

    /**
     * @brief 获取传感器数据
     * @param channel 传感器通道号 (0-7)
     * @param data 输出数据结构体指针
     * @return true: 成功, false: 失败
     */
    bool sensor_get_data(uint8_t channel, sensor_data_t *data);

    /**
     * @brief 获取传感器状态
     * @param channel 传感器通道号 (0-7)
     * @return 传感器状态
     */
    sensor_status_t sensor_get_status(uint8_t channel);

    /**
     * @brief 检查传感器是否在线
     * @param channel 传感器通道号 (0-7)
     * @return true: 在线, false: 离线
     */
    bool sensor_is_online(uint8_t channel);

    /**
     * @brief 获取传感器统计信息
     * @param channel 传感器通道号 (0-7)
     * @param stats 输出统计信息结构体指针
     * @return true: 成功, false: 失败
     */
    bool sensor_get_stats(uint8_t channel, sensor_stats_t *stats);

    /**
     * @brief 清除传感器统计信息
     * @param channel 传感器通道号 (0-7)，0xFF为清除所有通道
     * @return true: 成功, false: 失败
     */
    bool sensor_clear_stats(uint8_t channel);

    /**
     * @brief 设置传感器报警阈值
     * @param channel 传感器通道号 (0-7)
     * @param min_threshold 最小阈值
     * @param max_threshold 最大阈值
     * @return true: 成功, false: 失败
     */
    bool sensor_set_threshold(uint8_t channel, float min_threshold, float max_threshold);

    /**
     * @brief 校准传感器
     * @param channel 传感器通道号 (0-7)
     * @param reference_value 参考值
     * @return true: 成功, false: 失败
     */
    bool sensor_calibrate(uint8_t channel, float reference_value);

    /**
     * @brief 打印传感器状态信息 (调试用)
     */
    void sensor_print_status(void);

    /**
     * @brief 打印传感器统计信息 (调试用)
     */
    void sensor_print_stats(void);

    /**
     * @brief 传感器处理函数 (供main.c调用，避免函数名冲突)
     */
    void sensor_process(void);

    // ============================================================================
    // 内联函数
    // ============================================================================

    /**
     * @brief 检查传感器通道号是否有效
     * @param channel 传感器通道号
     * @return true: 有效, false: 无效
     */
    static inline bool sensor_is_channel_valid(uint8_t channel)
    {
        return (channel < SENSOR_MAX_CHANNELS);
    }

    /**
     * @brief 将整数温度值转换为浮点数 (°C)
     * @param temp_int 整数温度值 (0.1°C精度)
     * @return 浮点温度值 (°C)
     */
    static inline float sensor_temp_int_to_float(int16_t temp_int)
    {
        return (float)temp_int / 10.0f;
    }

    /**
     * @brief 将浮点温度值转换为整数 (0.1°C精度)
     * @param temp_float 浮点温度值 (°C)
     * @return 整数温度值 (0.1°C精度)
     */
    static inline int16_t sensor_temp_float_to_int(float temp_float)
    {
        return (int16_t)(temp_float * 10.0f);
    }

    /**
     * @brief 将整数湿度值转换为浮点数 (%RH)
     * @param humidity_int 整数湿度值 (0.1%RH精度)
     * @return 浮点湿度值 (%RH)
     */
    static inline float sensor_humidity_int_to_float(uint16_t humidity_int)
    {
        return (float)humidity_int / 10.0f;
    }

    /**
     * @brief 将浮点湿度值转换为整数 (0.1%RH精度)
     * @param humidity_float 浮点湿度值 (%RH)
     * @return 整数湿度值 (0.1%RH精度)
     */
    static inline uint16_t sensor_humidity_float_to_int(float humidity_float)
    {
        return (uint16_t)(humidity_float * 10.0f);
    }

#ifdef __cplusplus
}
#endif

#endif // __SENSOR_H__