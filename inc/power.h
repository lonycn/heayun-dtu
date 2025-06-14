/**
 * @file power.h
 * @brief 功耗管理模块头文件
 * @version 1.0
 * @date 2025-01-14
 *
 * 憨云DTU - 功耗管理系统
 * 针对NANO100B微控制器优化的功耗管理方案
 *
 * 功能特性:
 * - 多级低功耗模式
 * - 智能唤醒控制
 * - 电源状态监控
 * - 功耗统计分析
 * - 电池管理
 *
 * 功耗性能:
 * - 运行模式: 8mA @ 32MHz
 * - 空闲模式: 2mA
 * - 深度睡眠: 10μA
 * - 待机模式: 1μA
 */

#ifndef __POWER_H__
#define __POWER_H__

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// 功耗配置宏定义
//==============================================================================

// 电源电压阈值 (mV)
#define POWER_VOLTAGE_MAX 3600      // 最大工作电压 3.6V
#define POWER_VOLTAGE_NORMAL 3300   // 正常工作电压 3.3V
#define POWER_VOLTAGE_LOW 3000      // 低电压告警 3.0V
#define POWER_VOLTAGE_CRITICAL 2800 // 临界电压 2.8V
#define POWER_VOLTAGE_SHUTDOWN 2500 // 关机电压 2.5V

// 功耗测量参数
#define POWER_MEASURE_INTERVAL_MS 1000 // 功耗测量间隔 1秒
#define POWER_AVERAGE_SAMPLES 60       // 平均功耗样本数 (1分钟)
#define POWER_HISTORY_SIZE 144         // 功耗历史记录 (2.4小时)

// 唤醒源配置
#define POWER_WAKEUP_RTC 0x01    // RTC定时唤醒
#define POWER_WAKEUP_GPIO 0x02   // GPIO中断唤醒
#define POWER_WAKEUP_UART 0x04   // UART数据唤醒
#define POWER_WAKEUP_ADC 0x08    // ADC阈值唤醒
#define POWER_WAKEUP_LORA 0x10   // LoRa中断唤醒
#define POWER_WAKEUP_WDT 0x20    // 看门狗唤醒
#define POWER_WAKEUP_BUTTON 0x40 // 按键唤醒
#define POWER_WAKEUP_ALL 0xFF    // 所有唤醒源

// 睡眠持续时间预设 (秒)
#define POWER_SLEEP_SHORT 5   // 短睡眠 5秒
#define POWER_SLEEP_MEDIUM 30 // 中等睡眠 30秒
#define POWER_SLEEP_LONG 300  // 长睡眠 5分钟
#define POWER_SLEEP_DEEP 3600 // 深度睡眠 1小时

//==============================================================================
// 枚举定义
//==============================================================================

/**
 * @brief 功耗模式
 */
typedef enum
{
    POWER_MODE_RUN = 0,        // 运行模式 (全速运行)
    POWER_MODE_IDLE = 1,       // 空闲模式 (CPU停止，外设运行)
    POWER_MODE_SLEEP = 2,      // 睡眠模式 (低功耗，快速唤醒)
    POWER_MODE_DEEP_SLEEP = 3, // 深度睡眠 (超低功耗，慢速唤醒)
    POWER_MODE_STANDBY = 4,    // 待机模式 (最低功耗，保持RAM)
    POWER_MODE_SHUTDOWN = 5    // 关机模式 (断电状态)
} power_mode_t;

/**
 * @brief 电源状态
 */
typedef enum
{
    POWER_STATE_NORMAL = 0,   // 正常状态
    POWER_STATE_LOW = 1,      // 低电压状态
    POWER_STATE_CRITICAL = 2, // 临界状态
    POWER_STATE_SHUTDOWN = 3, // 关机状态
    POWER_STATE_CHARGING = 4, // 充电状态
    POWER_STATE_FULL = 5      // 电量充满
} power_state_t;

/**
 * @brief 唤醒原因
 */
typedef enum
{
    POWER_WAKEUP_NONE = 0,         // 无唤醒 (正常运行)
    POWER_WAKEUP_RTC_ALARM = 1,    // RTC闹钟唤醒
    POWER_WAKEUP_GPIO_INT = 2,     // GPIO中断唤醒
    POWER_WAKEUP_UART_RX = 3,      // UART接收唤醒
    POWER_WAKEUP_ADC_THRESH = 4,   // ADC阈值唤醒
    POWER_WAKEUP_LORA_INT = 5,     // LoRa中断唤醒
    POWER_WAKEUP_WDT_RESET = 6,    // 看门狗复位唤醒
    POWER_WAKEUP_BUTTON_PRESS = 7, // 按键按下唤醒
    POWER_WAKEUP_POWER_ON = 8,     // 上电唤醒
    POWER_WAKEUP_RESET = 9         // 复位唤醒
} power_wakeup_reason_t;

/**
 * @brief 功耗等级
 */
typedef enum
{
    POWER_LEVEL_HIGH = 0,     // 高功耗 (全性能模式)
    POWER_LEVEL_MEDIUM = 1,   // 中功耗 (平衡模式)
    POWER_LEVEL_LOW = 2,      // 低功耗 (节能模式)
    POWER_LEVEL_ULTRA_LOW = 3 // 超低功耗 (极限节能)
} power_level_t;

//==============================================================================
// 数据结构定义
//==============================================================================

/**
 * @brief 功耗配置参数
 */
typedef struct
{
    power_level_t level;             // 功耗等级
    uint32_t cpu_freq_hz;            // CPU频率 (Hz)
    bool peripheral_clock_gate;      // 外设时钟门控使能
    bool unused_gpio_pulldown;       // 未用GPIO下拉使能
    uint8_t sleep_wakeup_sources;    // 睡眠唤醒源掩码
    uint32_t auto_sleep_timeout;     // 自动睡眠超时 (秒)
    uint16_t voltage_monitor_enable; // 电压监控使能
    uint16_t battery_capacity_mah;   // 电池容量 (mAh)
} power_config_t;

/**
 * @brief 电源状态信息
 */
typedef struct
{
    power_mode_t current_mode;         // 当前功耗模式
    power_state_t power_state;         // 电源状态
    uint16_t voltage_mv;               // 当前电压 (mV)
    uint16_t current_ma;               // 当前电流 (mA)
    uint16_t power_mw;                 // 当前功耗 (mW)
    uint8_t battery_percentage;        // 电池电量百分比
    uint32_t uptime_seconds;           // 运行时间 (秒)
    uint32_t sleep_time_seconds;       // 睡眠时间 (秒)
    power_wakeup_reason_t last_wakeup; // 最后唤醒原因
    bool is_charging;                  // 充电状态
    bool low_power_warning;            // 低电量警告
} power_status_t;

/**
 * @brief 功耗统计信息
 */
typedef struct
{
    uint32_t total_energy_mwh;   // 总能耗 (mWh)
    uint16_t avg_power_mw;       // 平均功耗 (mW)
    uint16_t peak_power_mw;      // 峰值功耗 (mW)
    uint16_t min_power_mw;       // 最小功耗 (mW)
    uint32_t run_time_seconds;   // 运行时间 (秒)
    uint32_t sleep_time_seconds; // 睡眠时间 (秒)
    uint16_t sleep_efficiency;   // 睡眠效率 (%)
    uint32_t wakeup_count;       // 唤醒次数
    uint16_t power_cycles;       // 电源循环次数
    uint32_t battery_cycles;     // 电池循环次数
} power_statistics_t;

/**
 * @brief 功耗历史记录
 */
typedef struct
{
    uint32_t timestamp;    // 时间戳
    uint16_t voltage_mv;   // 电压 (mV)
    uint16_t current_ma;   // 电流 (mA)
    uint16_t power_mw;     // 功耗 (mW)
    power_mode_t mode;     // 功耗模式
    uint8_t battery_level; // 电池电量
} power_history_record_t;

/**
 * @brief 睡眠配置
 */
typedef struct
{
    uint32_t duration_seconds;     // 睡眠持续时间 (秒)
    uint8_t wakeup_sources;        // 唤醒源掩码
    bool retain_ram;               // 保持RAM数据
    bool retain_registers;         // 保持寄存器状态
    bool wakeup_gpio_level;        // GPIO唤醒电平 (true=高电平)
    uint16_t adc_wakeup_threshold; // ADC唤醒阈值
} power_sleep_config_t;

//==============================================================================
// 核心API函数声明
//==============================================================================

/**
 * @brief 初始化功耗管理模块
 * @param config 配置参数指针，NULL使用默认配置
 * @return 0=成功，负值=错误代码
 */
int power_init(const power_config_t *config);

/**
 * @brief 反初始化功耗管理模块
 * @return 0=成功，负值=错误代码
 */
int power_deinit(void);

/**
 * @brief 功耗管理任务 (定期调用)
 */
void power_task(void);

//==============================================================================
// 功耗模式控制函数
//==============================================================================

/**
 * @brief 设置功耗模式
 * @param mode 目标功耗模式
 * @return 0=成功，负值=错误代码
 */
int power_set_mode(power_mode_t mode);

/**
 * @brief 获取当前功耗模式
 * @return power_mode_t 当前功耗模式
 */
power_mode_t power_get_mode(void);

/**
 * @brief 设置功耗等级
 * @param level 功耗等级
 * @return 0=成功，负值=错误代码
 */
int power_set_level(power_level_t level);

/**
 * @brief 进入睡眠模式
 * @param config 睡眠配置指针
 * @return 0=成功，负值=错误代码
 */
int power_enter_sleep(const power_sleep_config_t *config);

/**
 * @brief 进入深度睡眠模式
 * @param duration_seconds 睡眠持续时间 (秒)
 * @param wakeup_sources 唤醒源掩码
 * @return 0=成功，负值=错误代码
 */
int power_enter_deep_sleep(uint32_t duration_seconds, uint8_t wakeup_sources);

/**
 * @brief 立即唤醒系统
 * @return 0=成功，负值=错误代码
 */
int power_wakeup(void);

//==============================================================================
// 电源监控函数
//==============================================================================

/**
 * @brief 获取电源状态
 * @return 状态信息结构体
 */
power_status_t power_get_status(void);

/**
 * @brief 获取电池电量百分比
 * @return 电池电量 (0-100%)
 */
uint8_t power_get_battery_level(void);

/**
 * @brief 获取电源电压
 * @return 电压值 (mV)
 */
uint16_t power_get_voltage(void);

/**
 * @brief 获取当前功耗
 * @return 功耗值 (mW)
 */
uint16_t power_get_power_consumption(void);

/**
 * @brief 检查低电量状态
 * @return true=低电量，false=正常
 */
bool power_is_low_battery(void);

/**
 * @brief 检查充电状态
 * @return true=正在充电，false=未充电
 */
bool power_is_charging(void);

//==============================================================================
// 唤醒控制函数
//==============================================================================

/**
 * @brief 配置唤醒源
 * @param sources 唤醒源掩码
 * @return 0=成功，负值=错误代码
 */
int power_configure_wakeup_sources(uint8_t sources);

/**
 * @brief 设置RTC唤醒
 * @param seconds 唤醒间隔 (秒)
 * @return 0=成功，负值=错误代码
 */
int power_set_rtc_wakeup(uint32_t seconds);

/**
 * @brief 设置GPIO唤醒
 * @param gpio_mask GPIO掩码
 * @param level 唤醒电平 (true=高电平)
 * @return 0=成功，负值=错误代码
 */
int power_set_gpio_wakeup(uint32_t gpio_mask, bool level);

/**
 * @brief 设置ADC阈值唤醒
 * @param channel ADC通道
 * @param threshold 阈值
 * @return 0=成功，负值=错误代码
 */
int power_set_adc_wakeup(uint8_t channel, uint16_t threshold);

/**
 * @brief 获取唤醒原因
 * @return power_wakeup_reason_t 唤醒原因
 */
power_wakeup_reason_t power_get_wakeup_reason(void);

//==============================================================================
// 功耗优化函数
//==============================================================================

/**
 * @brief 启用外设时钟门控
 * @param peripheral_mask 外设掩码
 * @return 0=成功，负值=错误代码
 */
int power_enable_clock_gating(uint32_t peripheral_mask);

/**
 * @brief 禁用未使用的外设
 * @return 0=成功，负值=错误代码
 */
int power_disable_unused_peripherals(void);

/**
 * @brief 优化GPIO配置
 * @return 0=成功，负值=错误代码
 */
int power_optimize_gpio_config(void);

/**
 * @brief 设置CPU频率
 * @param freq_hz 目标频率 (Hz)
 * @return 0=成功，负值=错误代码
 */
int power_set_cpu_frequency(uint32_t freq_hz);

/**
 * @brief 自动功耗优化
 * @return 0=成功，负值=错误代码
 */
int power_auto_optimize(void);

//==============================================================================
// 统计和历史函数
//==============================================================================

/**
 * @brief 获取功耗统计信息
 * @param stats 统计信息指针
 * @return 0=成功，负值=错误代码
 */
int power_get_statistics(power_statistics_t *stats);

/**
 * @brief 清除功耗统计
 * @return 0=成功，负值=错误代码
 */
int power_clear_statistics(void);

/**
 * @brief 获取功耗历史记录
 * @param records 记录数组指针
 * @param max_count 最大记录数
 * @param actual_count 实际记录数指针
 * @return 0=成功，负值=错误代码
 */
int power_get_history(power_history_record_t *records, uint16_t max_count,
                      uint16_t *actual_count);

/**
 * @brief 添加功耗历史记录
 * @param record 记录指针
 * @return 0=成功，负值=错误代码
 */
int power_add_history_record(const power_history_record_t *record);

//==============================================================================
// 电池管理函数
//==============================================================================

/**
 * @brief 校准电池电量
 * @param actual_voltage_mv 实际电压 (mV)
 * @return 0=成功，负值=错误代码
 */
int power_calibrate_battery(uint16_t actual_voltage_mv);

/**
 * @brief 估算剩余使用时间
 * @return 预计剩余时间 (分钟)
 */
uint32_t power_estimate_remaining_time(void);

/**
 * @brief 设置电池容量
 * @param capacity_mah 电池容量 (mAh)
 * @return 0=成功，负值=错误代码
 */
int power_set_battery_capacity(uint16_t capacity_mah);

//==============================================================================
// 工具函数
//==============================================================================

/**
 * @brief 获取功耗模式名称
 * @param mode 功耗模式
 * @return const char* 模式名称字符串
 */
const char *power_get_mode_name(power_mode_t mode);

/**
 * @brief 获取唤醒原因名称
 * @param reason 唤醒原因
 * @return const char* 原因名称字符串
 */
const char *power_get_wakeup_reason_name(power_wakeup_reason_t reason);

/**
 * @brief 计算功耗效率
 * @return 功耗效率百分比 (0-100%)
 */
uint8_t power_calculate_efficiency(void);

/**
 * @brief 获取版本信息
 * @return const char* 版本字符串
 */
const char *power_get_version(void);

//==============================================================================
// 默认配置宏
//==============================================================================

#define POWER_DEFAULT_CONFIG() {              \
    .level = POWER_LEVEL_MEDIUM,              \
    .cpu_freq_hz = 32000000,                  \
    .peripheral_clock_gate = true,            \
    .unused_gpio_pulldown = true,             \
    .sleep_wakeup_sources = POWER_WAKEUP_ALL, \
    .auto_sleep_timeout = 300,                \
    .voltage_monitor_enable = true,           \
    .battery_capacity_mah = 2000}

#endif // __POWER_H__