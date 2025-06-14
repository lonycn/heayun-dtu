/**
 * @file power.c
 * @brief 功耗管理模块实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 憨云DTU - 功耗管理系统实现
 * 针对NANO100B微控制器的功耗优化方案
 */

#include "power.h"
#include "gpio.h"
#include "adc.h"
#include "system.h"
#include <string.h>
#include <stdio.h>

//==============================================================================
// 全局变量
//==============================================================================

static power_config_t g_power_config;
static power_status_t g_power_status;
static power_statistics_t g_power_stats;

static power_history_record_t g_power_history[POWER_HISTORY_SIZE];
static uint16_t g_history_index = 0;
static uint16_t g_history_count = 0;

static bool g_power_initialized = false;
static uint32_t g_last_measure_time = 0;
static uint32_t g_sleep_start_time = 0;
static uint16_t g_power_samples[POWER_AVERAGE_SAMPLES];
static uint8_t g_sample_index = 0;

//==============================================================================
// 内部函数声明
//==============================================================================

static void power_measure_consumption(void);
static void power_update_battery_level(void);
static void power_check_voltage_thresholds(void);
static void power_optimize_for_mode(power_mode_t mode);
static uint16_t power_calculate_average_power(void);
static void power_add_sample(uint16_t power_mw);

//==============================================================================
// 核心API实现
//==============================================================================

int power_init(const power_config_t *config)
{
    // 配置参数初始化
    if (config != NULL)
    {
        g_power_config = *config;
    }
    else
    {
        g_power_config = (power_config_t)POWER_DEFAULT_CONFIG();
    }

    // 状态初始化
    memset(&g_power_status, 0, sizeof(g_power_status));
    memset(&g_power_stats, 0, sizeof(g_power_stats));
    memset(g_power_history, 0, sizeof(g_power_history));
    memset(g_power_samples, 0, sizeof(g_power_samples));

    g_power_status.current_mode = POWER_MODE_RUN;
    g_power_status.power_state = POWER_STATE_NORMAL;
    g_power_status.last_wakeup = POWER_WAKEUP_POWER_ON;

    // 初始化ADC用于电压监控
    if (g_power_config.voltage_monitor_enable)
    {
        adc_init();
        adc_config_t adc_config = {
            .channel = ADC_VOLTAGE_MONITOR_CHANNEL,
            .resolution = ADC_RESOLUTION_12BIT,
            .sample_time = ADC_SAMPLE_TIME_8,
            .trigger_mode = ADC_TRIGGER_SOFTWARE,
            .enable_interrupt = false};
        adc_config_channel(&adc_config);
    }

    // 初始功耗优化
    if (g_power_config.peripheral_clock_gate)
    {
        power_disable_unused_peripherals();
    }

    if (g_power_config.unused_gpio_pulldown)
    {
        power_optimize_gpio_config();
    }

    // 设置CPU频率
    power_set_cpu_frequency(g_power_config.cpu_freq_hz);

    // 初始测量
    power_measure_consumption();
    power_update_battery_level();

    g_power_initialized = true;

    printf("功耗管理: 初始化成功 (等级: %d, CPU: %lu Hz)\n",
           g_power_config.level, g_power_config.cpu_freq_hz);

    return 0;
}

int power_deinit(void)
{
    g_power_initialized = false;
    return 0;
}

void power_task(void)
{
    if (!g_power_initialized)
        return;

    uint32_t current_time = system_get_tick();

    // 定期功耗测量
    if ((current_time - g_last_measure_time) >= POWER_MEASURE_INTERVAL_MS)
    {
        power_measure_consumption();
        power_update_battery_level();
        power_check_voltage_thresholds();

        // 添加历史记录
        power_history_record_t record = {
            .timestamp = current_time / 1000,
            .voltage_mv = g_power_status.voltage_mv,
            .current_ma = g_power_status.current_ma,
            .power_mw = g_power_status.power_mw,
            .mode = g_power_status.current_mode,
            .battery_level = g_power_status.battery_percentage};
        power_add_history_record(&record);

        g_last_measure_time = current_time;
    }

    // 更新运行时间
    g_power_status.uptime_seconds = current_time / 1000;
    g_power_stats.run_time_seconds = g_power_status.uptime_seconds - g_power_status.sleep_time_seconds;

    // 自动睡眠检查
    if (g_power_config.auto_sleep_timeout > 0)
    {
        static uint32_t last_activity_time = 0;
        if (last_activity_time == 0)
            last_activity_time = current_time;

        if ((current_time - last_activity_time) > (g_power_config.auto_sleep_timeout * 1000))
        {
            power_sleep_config_t sleep_config = {
                .duration_seconds = POWER_SLEEP_MEDIUM,
                .wakeup_sources = g_power_config.sleep_wakeup_sources,
                .retain_ram = true,
                .retain_registers = true,
                .wakeup_gpio_level = true,
                .adc_wakeup_threshold = 2048};
            power_enter_sleep(&sleep_config);
            last_activity_time = system_get_tick(); // 唤醒后重置
        }
    }
}

//==============================================================================
// 功耗模式控制函数实现
//==============================================================================

int power_set_mode(power_mode_t mode)
{
    if (!g_power_initialized)
        return -1;

    power_mode_t old_mode = g_power_status.current_mode;
    g_power_status.current_mode = mode;

    // 模式切换优化
    power_optimize_for_mode(mode);

    printf("功耗管理: 模式切换 %s -> %s\n",
           power_get_mode_name(old_mode), power_get_mode_name(mode));

    return 0;
}

power_mode_t power_get_mode(void)
{
    return g_power_status.current_mode;
}

int power_set_level(power_level_t level)
{
    if (!g_power_initialized)
        return -1;

    g_power_config.level = level;

    // 根据功耗等级调整CPU频率
    switch (level)
    {
    case POWER_LEVEL_HIGH:
        power_set_cpu_frequency(32000000); // 32MHz
        break;
    case POWER_LEVEL_MEDIUM:
        power_set_cpu_frequency(16000000); // 16MHz
        break;
    case POWER_LEVEL_LOW:
        power_set_cpu_frequency(8000000); // 8MHz
        break;
    case POWER_LEVEL_ULTRA_LOW:
        power_set_cpu_frequency(4000000); // 4MHz
        break;
    }

    return 0;
}

int power_enter_sleep(const power_sleep_config_t *config)
{
    if (!g_power_initialized || config == NULL)
        return -1;

    printf("功耗管理: 进入睡眠模式 (%lu秒)\n", config->duration_seconds);

    // 记录睡眠开始时间
    g_sleep_start_time = system_get_tick();

    // 配置唤醒源
    power_configure_wakeup_sources(config->wakeup_sources);

    // 设置RTC唤醒
    if (config->wakeup_sources & POWER_WAKEUP_RTC)
    {
        power_set_rtc_wakeup(config->duration_seconds);
    }

    // 模拟睡眠 (实际应用中会进入真正的低功耗模式)
    power_set_mode(POWER_MODE_SLEEP);

    // 模拟睡眠延迟
    system_delay_ms(100);

    // 模拟唤醒
    g_power_status.last_wakeup = POWER_WAKEUP_RTC_ALARM;
    power_set_mode(POWER_MODE_RUN);

    // 更新睡眠时间统计
    uint32_t sleep_duration = (system_get_tick() - g_sleep_start_time) / 1000;
    g_power_status.sleep_time_seconds += sleep_duration;
    g_power_stats.sleep_time_seconds += sleep_duration;
    g_power_stats.wakeup_count++;

    printf("功耗管理: 从睡眠唤醒 (原因: %s)\n",
           power_get_wakeup_reason_name(g_power_status.last_wakeup));

    return 0;
}

int power_enter_deep_sleep(uint32_t duration_seconds, uint8_t wakeup_sources)
{
    power_sleep_config_t config = {
        .duration_seconds = duration_seconds,
        .wakeup_sources = wakeup_sources,
        .retain_ram = true,
        .retain_registers = false,
        .wakeup_gpio_level = true,
        .adc_wakeup_threshold = 2048};

    power_set_mode(POWER_MODE_DEEP_SLEEP);
    return power_enter_sleep(&config);
}

//==============================================================================
// 电源监控函数实现
//==============================================================================

power_status_t power_get_status(void)
{
    return g_power_status;
}

uint8_t power_get_battery_level(void)
{
    return g_power_status.battery_percentage;
}

uint16_t power_get_voltage(void)
{
    return g_power_status.voltage_mv;
}

uint16_t power_get_power_consumption(void)
{
    return g_power_status.power_mw;
}

bool power_is_low_battery(void)
{
    return g_power_status.low_power_warning;
}

bool power_is_charging(void)
{
    return g_power_status.is_charging;
}

//==============================================================================
// 唤醒控制函数实现
//==============================================================================

int power_configure_wakeup_sources(uint8_t sources)
{
    // 模拟配置唤醒源
    printf("功耗管理: 配置唤醒源 0x%02X\n", sources);
    return 0;
}

int power_set_rtc_wakeup(uint32_t seconds)
{
    // 模拟设置RTC唤醒
    printf("功耗管理: 设置RTC唤醒 %lu秒\n", seconds);
    return 0;
}

int power_set_gpio_wakeup(uint32_t gpio_mask, bool level)
{
    // 模拟设置GPIO唤醒
    printf("功耗管理: 设置GPIO唤醒 0x%08lX, 电平: %d\n", gpio_mask, level);
    return 0;
}

power_wakeup_reason_t power_get_wakeup_reason(void)
{
    return g_power_status.last_wakeup;
}

//==============================================================================
// 功耗优化函数实现
//==============================================================================

int power_disable_unused_peripherals(void)
{
    // 模拟禁用未使用的外设
    printf("功耗管理: 禁用未使用外设\n");
    return 0;
}

int power_optimize_gpio_config(void)
{
    // 模拟优化GPIO配置
    printf("功耗管理: 优化GPIO配置\n");
    return 0;
}

int power_set_cpu_frequency(uint32_t freq_hz)
{
    g_power_config.cpu_freq_hz = freq_hz;
    printf("功耗管理: 设置CPU频率 %lu Hz\n", freq_hz);
    return 0;
}

int power_auto_optimize(void)
{
    // 根据当前状态自动优化
    if (g_power_status.battery_percentage < 20)
    {
        power_set_level(POWER_LEVEL_ULTRA_LOW);
    }
    else if (g_power_status.battery_percentage < 50)
    {
        power_set_level(POWER_LEVEL_LOW);
    }
    else
    {
        power_set_level(POWER_LEVEL_MEDIUM);
    }

    return 0;
}

//==============================================================================
// 统计和历史函数实现
//==============================================================================

int power_get_statistics(power_statistics_t *stats)
{
    if (!g_power_initialized || stats == NULL)
        return -1;

    // 更新统计信息
    g_power_stats.avg_power_mw = power_calculate_average_power();
    g_power_stats.sleep_efficiency = (g_power_stats.sleep_time_seconds * 100) /
                                     (g_power_stats.run_time_seconds + g_power_stats.sleep_time_seconds);

    *stats = g_power_stats;
    return 0;
}

int power_clear_statistics(void)
{
    memset(&g_power_stats, 0, sizeof(g_power_stats));
    return 0;
}

int power_add_history_record(const power_history_record_t *record)
{
    if (record == NULL)
        return -1;

    g_power_history[g_history_index] = *record;
    g_history_index = (g_history_index + 1) % POWER_HISTORY_SIZE;

    if (g_history_count < POWER_HISTORY_SIZE)
    {
        g_history_count++;
    }

    return 0;
}

//==============================================================================
// 电池管理函数实现
//==============================================================================

int power_set_battery_capacity(uint16_t capacity_mah)
{
    g_power_config.battery_capacity_mah = capacity_mah;
    return 0;
}

uint32_t power_estimate_remaining_time(void)
{
    if (g_power_status.current_ma == 0)
        return 0;

    uint32_t remaining_capacity = (g_power_status.battery_percentage * g_power_config.battery_capacity_mah) / 100;
    uint32_t remaining_minutes = (remaining_capacity * 60) / g_power_status.current_ma;

    return remaining_minutes;
}

//==============================================================================
// 工具函数实现
//==============================================================================

const char *power_get_mode_name(power_mode_t mode)
{
    switch (mode)
    {
    case POWER_MODE_RUN:
        return "运行模式";
    case POWER_MODE_IDLE:
        return "空闲模式";
    case POWER_MODE_SLEEP:
        return "睡眠模式";
    case POWER_MODE_DEEP_SLEEP:
        return "深度睡眠";
    case POWER_MODE_STANDBY:
        return "待机模式";
    case POWER_MODE_SHUTDOWN:
        return "关机模式";
    default:
        return "未知模式";
    }
}

const char *power_get_wakeup_reason_name(power_wakeup_reason_t reason)
{
    switch (reason)
    {
    case POWER_WAKEUP_NONE:
        return "无唤醒";
    case POWER_WAKEUP_RTC_ALARM:
        return "RTC闹钟";
    case POWER_WAKEUP_GPIO_INT:
        return "GPIO中断";
    case POWER_WAKEUP_UART_RX:
        return "UART接收";
    case POWER_WAKEUP_ADC_THRESH:
        return "ADC阈值";
    case POWER_WAKEUP_LORA_INT:
        return "LoRa中断";
    case POWER_WAKEUP_WDT_RESET:
        return "看门狗复位";
    case POWER_WAKEUP_BUTTON_PRESS:
        return "按键按下";
    case POWER_WAKEUP_POWER_ON:
        return "上电唤醒";
    case POWER_WAKEUP_RESET:
        return "复位唤醒";
    default:
        return "未知原因";
    }
}

uint8_t power_calculate_efficiency(void)
{
    uint32_t total_time = g_power_stats.run_time_seconds + g_power_stats.sleep_time_seconds;
    if (total_time == 0)
        return 100;

    return (g_power_stats.sleep_time_seconds * 100) / total_time;
}

const char *power_get_version(void)
{
    return "功耗管理 v1.0.0 - 憨云DTU";
}

//==============================================================================
// 内部函数实现
//==============================================================================

static void power_measure_consumption(void)
{
    // 模拟电压测量 (实际应用中通过ADC读取)
    if (g_power_config.voltage_monitor_enable)
    {
        uint16_t adc_value;
        if (adc_read_single(ADC_VOLTAGE_MONITOR_CHANNEL, &adc_value, 100))
        {
            g_power_status.voltage_mv = (adc_value * 3300) / 4095; // 假设3.3V参考电压
        }
        else
        {
            g_power_status.voltage_mv = 3300; // 读取失败时使用默认值
        }
    }
    else
    {
        g_power_status.voltage_mv = 3300; // 默认3.3V
    }

    // 根据模式估算电流消耗
    switch (g_power_status.current_mode)
    {
    case POWER_MODE_RUN:
        g_power_status.current_ma = 8 + (g_power_config.cpu_freq_hz / 4000000); // 基础8mA + 频率相关
        break;
    case POWER_MODE_IDLE:
        g_power_status.current_ma = 2;
        break;
    case POWER_MODE_SLEEP:
        g_power_status.current_ma = 1; // 1mA睡眠电流
        break;
    case POWER_MODE_DEEP_SLEEP:
        g_power_status.current_ma = 0; // 10μA，近似为0
        break;
    case POWER_MODE_STANDBY:
        g_power_status.current_ma = 0; // 1μA，近似为0
        break;
    default:
        g_power_status.current_ma = 10;
        break;
    }

    // 计算功耗
    g_power_status.power_mw = (g_power_status.voltage_mv * g_power_status.current_ma) / 1000;

    // 添加功耗样本
    power_add_sample(g_power_status.power_mw);

    // 更新统计
    if (g_power_status.power_mw > g_power_stats.peak_power_mw)
    {
        g_power_stats.peak_power_mw = g_power_status.power_mw;
    }

    if (g_power_stats.min_power_mw == 0 || g_power_status.power_mw < g_power_stats.min_power_mw)
    {
        g_power_stats.min_power_mw = g_power_status.power_mw;
    }

    g_power_stats.total_energy_mwh += g_power_status.power_mw / 3600; // 累计能耗
}

static void power_update_battery_level(void)
{
    // 简单的电压-电量映射 (实际应用中需要更复杂的算法)
    if (g_power_status.voltage_mv >= POWER_VOLTAGE_NORMAL)
    {
        g_power_status.battery_percentage = 100;
        g_power_status.power_state = POWER_STATE_NORMAL;
    }
    else if (g_power_status.voltage_mv >= POWER_VOLTAGE_LOW)
    {
        g_power_status.battery_percentage = 50 + ((g_power_status.voltage_mv - POWER_VOLTAGE_LOW) * 50) /
                                                     (POWER_VOLTAGE_NORMAL - POWER_VOLTAGE_LOW);
        g_power_status.power_state = POWER_STATE_NORMAL;
    }
    else if (g_power_status.voltage_mv >= POWER_VOLTAGE_CRITICAL)
    {
        g_power_status.battery_percentage = 20 + ((g_power_status.voltage_mv - POWER_VOLTAGE_CRITICAL) * 30) /
                                                     (POWER_VOLTAGE_LOW - POWER_VOLTAGE_CRITICAL);
        g_power_status.power_state = POWER_STATE_LOW;
    }
    else
    {
        g_power_status.battery_percentage = (g_power_status.voltage_mv - POWER_VOLTAGE_SHUTDOWN) * 20 /
                                            (POWER_VOLTAGE_CRITICAL - POWER_VOLTAGE_SHUTDOWN);
        g_power_status.power_state = POWER_STATE_CRITICAL;
    }

    // 限制范围
    if (g_power_status.battery_percentage > 100)
        g_power_status.battery_percentage = 100;
    if (g_power_status.battery_percentage < 0)
        g_power_status.battery_percentage = 0;
}

static void power_check_voltage_thresholds(void)
{
    static bool low_voltage_warned = false;

    if (g_power_status.voltage_mv <= POWER_VOLTAGE_LOW && !low_voltage_warned)
    {
        g_power_status.low_power_warning = true;
        low_voltage_warned = true;
        printf("功耗管理: 低电压告警 %u mV\n", g_power_status.voltage_mv);
    }
    else if (g_power_status.voltage_mv > POWER_VOLTAGE_LOW)
    {
        g_power_status.low_power_warning = false;
        low_voltage_warned = false;
    }

    if (g_power_status.voltage_mv <= POWER_VOLTAGE_SHUTDOWN)
    {
        printf("功耗管理: 电压过低，系统即将关机 %u mV\n", g_power_status.voltage_mv);
        power_set_mode(POWER_MODE_SHUTDOWN);
    }
}

static void power_optimize_for_mode(power_mode_t mode)
{
    // 根据模式进行功耗优化
    switch (mode)
    {
    case POWER_MODE_SLEEP:
    case POWER_MODE_DEEP_SLEEP:
        // 睡眠模式优化
        power_disable_unused_peripherals();
        break;
    case POWER_MODE_RUN:
        // 运行模式恢复
        break;
    default:
        break;
    }
}

static uint16_t power_calculate_average_power(void)
{
    uint32_t sum = 0;
    uint8_t count = 0;

    for (uint8_t i = 0; i < POWER_AVERAGE_SAMPLES; i++)
    {
        if (g_power_samples[i] > 0)
        {
            sum += g_power_samples[i];
            count++;
        }
    }

    return count > 0 ? (sum / count) : 0;
}

static void power_add_sample(uint16_t power_mw)
{
    g_power_samples[g_sample_index] = power_mw;
    g_sample_index = (g_sample_index + 1) % POWER_AVERAGE_SAMPLES;
}