/**
 * @file alarm.h
 * @brief 憨云DTU报警监控模块接口
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 报警监控系统接口，支持阈值监控、报警输出控制、多级报警管理
 * 设计目标：实时监控、快速响应、可配置报警策略
 */

#ifndef __ALARM_H__
#define __ALARM_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // ============================================================================
    // 宏定义
    // ============================================================================

#define ALARM_MAX_RULES 16          // 最大报警规则数
#define ALARM_MAX_OUTPUTS 8         // 最大报警输出数
#define ALARM_MAX_HISTORY 50        // 最大报警历史记录数
#define ALARM_DEBOUNCE_TIME 3000    // 去抖时间 (ms)
#define ALARM_AUTO_RESET_TIME 30000 // 自动复位时间 (ms)

// 报警类型定义
#define ALARM_TYPE_TEMPERATURE 0x01   // 温度报警
#define ALARM_TYPE_HUMIDITY 0x02      // 湿度报警
#define ALARM_TYPE_VOLTAGE 0x03       // 电压报警
#define ALARM_TYPE_SENSOR_FAULT 0x04  // 传感器故障
#define ALARM_TYPE_COMMUNICATION 0x05 // 通信故障
#define ALARM_TYPE_SYSTEM 0x06        // 系统故障
#define ALARM_TYPE_CUSTOM 0xFF        // 自定义报警

// 报警级别定义
#define ALARM_LEVEL_INFO 0     // 信息级
#define ALARM_LEVEL_WARNING 1  // 警告级
#define ALARM_LEVEL_ERROR 2    // 错误级
#define ALARM_LEVEL_CRITICAL 3 // 严重级

// 报警条件定义
#define ALARM_CONDITION_NONE 0      // 无条件
#define ALARM_CONDITION_GT 1        // 大于
#define ALARM_CONDITION_LT 2        // 小于
#define ALARM_CONDITION_GE 3        // 大于等于
#define ALARM_CONDITION_LE 4        // 小于等于
#define ALARM_CONDITION_EQ 5        // 等于
#define ALARM_CONDITION_NE 6        // 不等于
#define ALARM_CONDITION_RANGE 7     // 范围内
#define ALARM_CONDITION_OUT_RANGE 8 // 范围外

// 报警输出类型
#define ALARM_OUTPUT_LED 0x01    // LED指示
#define ALARM_OUTPUT_BUZZER 0x02 // 蜂鸣器
#define ALARM_OUTPUT_RELAY 0x04  // 继电器
#define ALARM_OUTPUT_MODBUS 0x08 // Modbus通知
#define ALARM_OUTPUT_UART 0x10   // 串口输出

    // 报警状态
    typedef enum
    {
        ALARM_STATUS_OK = 0,       // 正常
        ALARM_STATUS_INIT_FAILED,  // 初始化失败
        ALARM_STATUS_RULE_ERROR,   // 规则错误
        ALARM_STATUS_OUTPUT_ERROR, // 输出错误
        ALARM_STATUS_MEMORY_ERROR, // 内存错误
        ALARM_STATUS_CONFIG_ERROR, // 配置错误
        ALARM_STATUS_COUNT
    } alarm_status_t;

    // 报警状态机
    typedef enum
    {
        ALARM_STATE_IDLE = 0,     // 空闲
        ALARM_STATE_PENDING,      // 待定
        ALARM_STATE_ACTIVE,       // 激活
        ALARM_STATE_ACKNOWLEDGED, // 已确认
        ALARM_STATE_RESOLVED,     // 已解决
        ALARM_STATE_COUNT
    } alarm_state_t;

    // ============================================================================
    // 数据结构定义
    // ============================================================================

    /**
     * @brief 报警规则配置
     */
    typedef struct
    {
        uint8_t id;        // 规则ID
        uint8_t type;      // 报警类型
        uint8_t level;     // 报警级别
        uint8_t condition; // 报警条件
        bool enabled;      // 是否启用

        int32_t threshold_low;    // 下阈值
        int32_t threshold_high;   // 上阈值
        uint32_t debounce_time;   // 去抖时间 (ms)
        uint32_t auto_reset_time; // 自动复位时间 (ms)

        uint8_t output_mask; // 输出掩码
        uint8_t priority;    // 优先级 (0-7)

        char description[32]; // 描述信息
    } alarm_rule_t;

    /**
     * @brief 报警状态信息
     */
    typedef struct
    {
        uint8_t rule_id;     // 规则ID
        alarm_state_t state; // 当前状态
        uint8_t level;       // 报警级别
        uint8_t type;        // 报警类型

        uint32_t trigger_time;     // 触发时间
        uint32_t acknowledge_time; // 确认时间
        uint32_t resolve_time;     // 解决时间
        uint32_t duration;         // 持续时间

        int32_t trigger_value; // 触发值
        uint8_t trigger_count; // 触发次数

        bool auto_acknowledged; // 自动确认
        bool output_active;     // 输出是否激活
    } alarm_info_t;

    /**
     * @brief 报警输出配置
     */
    typedef struct
    {
        uint8_t type;          // 输出类型
        uint8_t gpio_pin;      // GPIO引脚
        bool active_high;      // 高电平有效
        uint32_t pulse_period; // 脉冲周期 (ms)
        uint32_t pulse_duty;   // 脉冲占空比 (%)
        bool enabled;          // 是否启用
    } alarm_output_config_t;

    /**
     * @brief 报警历史记录
     */
    typedef struct
    {
        uint32_t timestamp;  // 时间戳
        uint8_t rule_id;     // 规则ID
        uint8_t type;        // 报警类型
        uint8_t level;       // 报警级别
        alarm_state_t state; // 状态变化

        int32_t value;        // 相关值
        uint32_t duration;    // 持续时间
        char description[32]; // 描述
    } alarm_history_t;

    /**
     * @brief 报警统计信息
     */
    typedef struct
    {
        uint32_t total_alarms;    // 总报警次数
        uint32_t active_alarms;   // 当前激活报警数
        uint32_t critical_alarms; // 严重报警次数
        uint32_t error_alarms;    // 错误报警次数
        uint32_t warning_alarms;  // 警告报警次数
        uint32_t info_alarms;     // 信息报警次数

        uint32_t auto_acknowledged;   // 自动确认次数
        uint32_t manual_acknowledged; // 手动确认次数
        uint32_t auto_resolved;       // 自动解决次数
        uint32_t manual_resolved;     // 手动解决次数

        uint32_t false_alarms;       // 误报次数
        uint32_t output_activations; // 输出激活次数
    } alarm_stats_t;

    /**
     * @brief 报警系统配置
     */
    typedef struct
    {
        bool global_enable;         // 全局使能
        uint8_t default_level;      // 默认级别
        uint32_t max_active_alarms; // 最大激活报警数

        bool auto_acknowledge;    // 自动确认
        bool auto_resolve;        // 自动解决
        uint32_t global_debounce; // 全局去抖时间

        alarm_output_config_t outputs[ALARM_MAX_OUTPUTS]; // 输出配置
    } alarm_config_t;

    // ============================================================================
    // 报警管理接口
    // ============================================================================

    /**
     * @brief 报警模块初始化
     * @return true: 成功, false: 失败
     */
    bool alarm_init(void);

    /**
     * @brief 报警模块反初始化
     */
    void alarm_deinit(void);

    /**
     * @brief 报警模块处理函数 (主循环调用)
     */
    void alarm_process(void);

    /**
     * @brief 获取报警模块状态
     * @return 模块状态
     */
    alarm_status_t alarm_get_status(void);

    /**
     * @brief 获取报警统计信息
     * @param stats 统计信息结构体指针
     * @return true: 成功, false: 失败
     */
    bool alarm_get_stats(alarm_stats_t *stats);

    /**
     * @brief 重置报警统计信息
     * @return true: 成功, false: 失败
     */
    bool alarm_reset_stats(void);

    // ============================================================================
    // 报警规则管理接口
    // ============================================================================

    /**
     * @brief 添加报警规则
     * @param rule 报警规则指针
     * @return true: 成功, false: 失败
     */
    bool alarm_add_rule(const alarm_rule_t *rule);

    /**
     * @brief 删除报警规则
     * @param rule_id 规则ID
     * @return true: 成功, false: 失败
     */
    bool alarm_remove_rule(uint8_t rule_id);

    /**
     * @brief 更新报警规则
     * @param rule 报警规则指针
     * @return true: 成功, false: 失败
     */
    bool alarm_update_rule(const alarm_rule_t *rule);

    /**
     * @brief 获取报警规则
     * @param rule_id 规则ID
     * @param rule 报警规则指针
     * @return true: 成功, false: 失败
     */
    bool alarm_get_rule(uint8_t rule_id, alarm_rule_t *rule);

    /**
     * @brief 启用/禁用报警规则
     * @param rule_id 规则ID
     * @param enabled 是否启用
     * @return true: 成功, false: 失败
     */
    bool alarm_enable_rule(uint8_t rule_id, bool enabled);

    /**
     * @brief 获取所有报警规则
     * @param rules 规则数组指针
     * @param max_count 最大数量
     * @return 实际获取的规则数量
     */
    uint8_t alarm_get_all_rules(alarm_rule_t *rules, uint8_t max_count);

    // ============================================================================
    // 报警触发和处理接口
    // ============================================================================

    /**
     * @brief 检查报警条件 (传感器数据检查)
     * @param type 数据类型
     * @param value 当前值
     * @return true: 有报警触发, false: 无报警
     */
    bool alarm_check_condition(uint8_t type, int32_t value);

    /**
     * @brief 手动触发报警
     * @param rule_id 规则ID
     * @param value 触发值
     * @return true: 成功, false: 失败
     */
    bool alarm_trigger(uint8_t rule_id, int32_t value);

    /**
     * @brief 确认报警
     * @param rule_id 规则ID
     * @return true: 成功, false: 失败
     */
    bool alarm_acknowledge(uint8_t rule_id);

    /**
     * @brief 解决报警
     * @param rule_id 规则ID
     * @return true: 成功, false: 失败
     */
    bool alarm_resolve(uint8_t rule_id);

    /**
     * @brief 确认所有报警
     * @return 确认的报警数量
     */
    uint8_t alarm_acknowledge_all(void);

    /**
     * @brief 解决所有报警
     * @return 解决的报警数量
     */
    uint8_t alarm_resolve_all(void);

    /**
     * @brief 静音报警输出
     * @param duration 静音时长 (ms, 0表示永久)
     * @return true: 成功, false: 失败
     */
    bool alarm_silence(uint32_t duration);

    // ============================================================================
    // 报警状态查询接口
    // ============================================================================

    /**
     * @brief 获取激活的报警数量
     * @return 激活报警数量
     */
    uint8_t alarm_get_active_count(void);

    /**
     * @brief 获取指定级别的报警数量
     * @param level 报警级别
     * @return 报警数量
     */
    uint8_t alarm_get_level_count(uint8_t level);

    /**
     * @brief 获取报警信息
     * @param rule_id 规则ID
     * @param info 报警信息指针
     * @return true: 成功, false: 失败
     */
    bool alarm_get_info(uint8_t rule_id, alarm_info_t *info);

    /**
     * @brief 获取所有激活的报警信息
     * @param infos 报警信息数组指针
     * @param max_count 最大数量
     * @return 实际获取的报警数量
     */
    uint8_t alarm_get_active_alarms(alarm_info_t *infos, uint8_t max_count);

    /**
     * @brief 检查是否有指定类型的报警
     * @param type 报警类型
     * @return true: 有, false: 无
     */
    bool alarm_has_type(uint8_t type);

    /**
     * @brief 检查是否有指定级别的报警
     * @param level 报警级别
     * @return true: 有, false: 无
     */
    bool alarm_has_level(uint8_t level);

    // ============================================================================
    // 报警输出控制接口
    // ============================================================================

    /**
     * @brief 配置报警输出
     * @param output_type 输出类型
     * @param config 输出配置指针
     * @return true: 成功, false: 失败
     */
    bool alarm_config_output(uint8_t output_type, const alarm_output_config_t *config);

    /**
     * @brief 启用/禁用报警输出
     * @param output_type 输出类型
     * @param enabled 是否启用
     * @return true: 成功, false: 失败
     */
    bool alarm_enable_output(uint8_t output_type, bool enabled);

    /**
     * @brief 测试报警输出
     * @param output_type 输出类型
     * @param duration 测试时长 (ms)
     * @return true: 成功, false: 失败
     */
    bool alarm_test_output(uint8_t output_type, uint32_t duration);

    /**
     * @brief 强制激活报警输出
     * @param output_mask 输出掩码
     * @param duration 激活时长 (ms)
     * @return true: 成功, false: 失败
     */
    bool alarm_force_output(uint8_t output_mask, uint32_t duration);

    // ============================================================================
    // 报警历史管理接口
    // ============================================================================

    /**
     * @brief 获取报警历史记录
     * @param history 历史记录数组指针
     * @param max_count 最大数量
     * @return 实际获取的记录数量
     */
    uint16_t alarm_get_history(alarm_history_t *history, uint16_t max_count);

    /**
     * @brief 清除报警历史记录
     * @param type 报警类型 (0xFF: 清除所有)
     * @return true: 成功, false: 失败
     */
    bool alarm_clear_history(uint8_t type);

    /**
     * @brief 导出报警历史记录 (通过串口)
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 导出的记录数量
     */
    uint16_t alarm_export_history(uint32_t start_time, uint32_t end_time);

    // ============================================================================
    // 配置管理接口
    // ============================================================================

    /**
     * @brief 加载报警配置
     * @return true: 成功, false: 失败
     */
    bool alarm_load_config(void);

    /**
     * @brief 保存报警配置
     * @return true: 成功, false: 失败
     */
    bool alarm_save_config(void);

    /**
     * @brief 恢复默认配置
     * @return true: 成功, false: 失败
     */
    bool alarm_reset_config(void);

    /**
     * @brief 获取当前配置
     * @param config 配置结构体指针
     * @return true: 成功, false: 失败
     */
    bool alarm_get_config(alarm_config_t *config);

    /**
     * @brief 设置当前配置
     * @param config 配置结构体指针
     * @return true: 成功, false: 失败
     */
    bool alarm_set_config(const alarm_config_t *config);

    // ============================================================================
    // 调试和诊断接口
    // ============================================================================

    /**
     * @brief 打印报警状态信息 (调试用)
     */
    void alarm_print_status(void);

    /**
     * @brief 打印报警统计信息 (调试用)
     */
    void alarm_print_stats(void);

    /**
     * @brief 打印所有报警规则 (调试用)
     */
    void alarm_print_rules(void);

    /**
     * @brief 打印激活的报警 (调试用)
     */
    void alarm_print_active_alarms(void);

    /**
     * @brief 自检报警系统
     * @return true: 正常, false: 异常
     */
    bool alarm_self_test(void);

    // ============================================================================
    // 内联函数
    // ============================================================================

    /**
     * @brief 检查报警模块是否已初始化
     */
    static inline bool alarm_is_initialized(void)
    {
        extern bool g_alarm_initialized;
        return g_alarm_initialized;
    }

    /**
     * @brief 检查报警级别是否有效
     */
    static inline bool alarm_is_valid_level(uint8_t level)
    {
        return (level <= ALARM_LEVEL_CRITICAL);
    }

    /**
     * @brief 检查报警类型是否有效
     */
    static inline bool alarm_is_valid_type(uint8_t type)
    {
        return (type >= ALARM_TYPE_TEMPERATURE && type <= ALARM_TYPE_CUSTOM);
    }

    /**
     * @brief 获取报警级别名称
     */
    static inline const char *alarm_get_level_name(uint8_t level)
    {
        static const char *level_names[] = {"INFO", "WARNING", "ERROR", "CRITICAL"};
        return (level <= ALARM_LEVEL_CRITICAL) ? level_names[level] : "UNKNOWN";
    }

#ifdef __cplusplus
}
#endif

#endif /* __ALARM_H__ */

// ============================================================================
// 使用示例
// ============================================================================

#if 0
// 初始化报警模块
if (!alarm_init()) {
    debug_printf("Alarm init failed\n");
    return false;
}

// 添加温度报警规则
alarm_rule_t temp_rule = {
    .id = 1,
    .type = ALARM_TYPE_TEMPERATURE,
    .level = ALARM_LEVEL_WARNING,
    .condition = ALARM_CONDITION_GT,
    .enabled = true,
    .threshold_high = 600,  // 60.0°C
    .debounce_time = 5000,  // 5秒
    .output_mask = ALARM_OUTPUT_LED | ALARM_OUTPUT_BUZZER,
    .priority = 3,
    .description = "High Temperature"
};
alarm_add_rule(&temp_rule);

// 主循环中处理报警
while (1) {
    // 检查传感器数据
    int32_t temp = sensor_get_temperature(); // 获取温度
    alarm_check_condition(ALARM_TYPE_TEMPERATURE, temp);
    
    // 处理报警逻辑
    alarm_process();
    
    // 其他任务...
    delay_ms(100);
}

// 确认所有报警
alarm_acknowledge_all();

// 打印报警状态
alarm_print_status();
#endif