/**
 * @file alarm.c
 * @brief 憨云DTU报警监控模块实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 报警监控系统实现，支持阈值监控、报警输出控制、多级报警管理
 * 设计目标：实时监控、快速响应、可配置报警策略
 */

#include "alarm.h"
#include "system.h"
#include "gpio.h"
#include "storage.h"
#include <string.h>

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 报警模块控制块
 */
typedef struct
{
    bool initialized;      // 初始化标志
    alarm_status_t status; // 当前状态
    alarm_config_t config; // 系统配置
    alarm_stats_t stats;   // 统计信息

    alarm_rule_t rules[ALARM_MAX_RULES];        // 报警规则
    alarm_info_t infos[ALARM_MAX_RULES];        // 报警状态信息
    alarm_history_t history[ALARM_MAX_HISTORY]; // 历史记录

    uint8_t rule_count;     // 规则数量
    uint8_t active_count;   // 激活报警数量
    uint16_t history_count; // 历史记录数量
    uint16_t history_index; // 历史记录写入索引

    uint32_t silence_start_time; // 静音开始时间
    uint32_t silence_duration;   // 静音持续时间
    bool silenced;               // 是否静音

    uint32_t last_process_time; // 上次处理时间
} alarm_control_t;

// 全局控制块
static alarm_control_t g_alarm = {0};
bool g_alarm_initialized = false;

// ============================================================================
// 内部函数声明
// ============================================================================

static bool alarm_evaluate_condition(const alarm_rule_t *rule, int32_t value);
static void alarm_update_outputs(void);
static void alarm_add_history(uint8_t rule_id, alarm_state_t state, int32_t value, const char *description);
static uint8_t alarm_find_rule_index(uint8_t rule_id);
static void alarm_setup_default_rules(void);
static void alarm_setup_default_config(void);

// ============================================================================
// 报警管理接口实现
// ============================================================================

/**
 * @brief 报警模块初始化
 */
bool alarm_init(void)
{
    if (g_alarm.initialized)
    {
        alarm_deinit();
    }

    // 清空控制块
    memset(&g_alarm, 0, sizeof(alarm_control_t));

    // 设置默认配置
    alarm_setup_default_config();

    // 设置默认报警规则
    alarm_setup_default_rules();

    // 加载配置 (如果存储模块可用)
    if (storage_is_initialized())
    {
        alarm_load_config();
    }

    g_alarm.initialized = true;
    g_alarm_initialized = true;
    g_alarm.status = ALARM_STATUS_OK;
    g_alarm.last_process_time = system_get_tick();

    // 初始化输出GPIO
    if (g_alarm.config.outputs[0].enabled && g_alarm.config.outputs[0].type == ALARM_OUTPUT_LED)
    {
        gpio_config_t led_config = {
            .port = GPIO_PORT_B,
            .pin = 5,
            .mode = GPIO_MODE_OUTPUT,
            .initial_state = false};
        gpio_config_pin(&led_config);
    }

    if (g_alarm.config.outputs[1].enabled && g_alarm.config.outputs[1].type == ALARM_OUTPUT_BUZZER)
    {
        gpio_config_t buzzer_config = {
            .port = GPIO_PORT_B,
            .pin = 6,
            .mode = GPIO_MODE_OUTPUT,
            .initial_state = false};
        gpio_config_pin(&buzzer_config);
    }

    return true;
}

/**
 * @brief 报警模块反初始化
 */
void alarm_deinit(void)
{
    if (!g_alarm.initialized)
    {
        return;
    }

    // 关闭所有输出
    for (uint8_t i = 0; i < ALARM_MAX_OUTPUTS; i++)
    {
        if (g_alarm.config.outputs[i].enabled)
        {
            if (g_alarm.config.outputs[i].type == ALARM_OUTPUT_LED ||
                g_alarm.config.outputs[i].type == ALARM_OUTPUT_BUZZER)
            {
                gpio_write_pin(GPIO_PORT_B, (i == 0) ? 5 : 6, !g_alarm.config.outputs[i].active_high);
            }
        }
    }

    // 清空控制块
    memset(&g_alarm, 0, sizeof(alarm_control_t));
    g_alarm_initialized = false;
}

/**
 * @brief 报警模块处理函数
 */
void alarm_process(void)
{
    if (!g_alarm.initialized)
    {
        return;
    }

    uint32_t current_time = system_get_tick();

    // 检查静音状态
    if (g_alarm.silenced)
    {
        if (g_alarm.silence_duration > 0 &&
            (current_time - g_alarm.silence_start_time) >= g_alarm.silence_duration)
        {
            g_alarm.silenced = false;
        }
    }

    // 处理每个激活的报警
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        alarm_rule_t *rule = &g_alarm.rules[i];
        alarm_info_t *info = &g_alarm.infos[i];

        if (!rule->enabled || info->state == ALARM_STATE_IDLE)
        {
            continue;
        }

        // 检查自动复位
        if (rule->auto_reset_time > 0 && info->state == ALARM_STATE_ACTIVE)
        {
            if ((current_time - info->trigger_time) >= rule->auto_reset_time)
            {
                info->state = ALARM_STATE_RESOLVED;
                info->resolve_time = current_time;
                info->output_active = false;
                g_alarm.stats.auto_resolved++;

                alarm_add_history(rule->id, ALARM_STATE_RESOLVED, 0, "Auto resolved");
            }
        }

        // 检查自动确认
        if (g_alarm.config.auto_acknowledge && info->state == ALARM_STATE_ACTIVE)
        {
            if ((current_time - info->trigger_time) >= ALARM_AUTO_RESET_TIME / 2)
            {
                info->state = ALARM_STATE_ACKNOWLEDGED;
                info->acknowledge_time = current_time;
                info->auto_acknowledged = true;
                g_alarm.stats.auto_acknowledged++;

                alarm_add_history(rule->id, ALARM_STATE_ACKNOWLEDGED, 0, "Auto acknowledged");
            }
        }

        // 更新持续时间
        info->duration = current_time - info->trigger_time;
    }

    // 更新输出状态
    alarm_update_outputs();

    g_alarm.last_process_time = current_time;
}

/**
 * @brief 获取报警模块状态
 */
alarm_status_t alarm_get_status(void)
{
    return g_alarm.status;
}

/**
 * @brief 获取报警统计信息
 */
bool alarm_get_stats(alarm_stats_t *stats)
{
    if (!g_alarm.initialized || !stats)
    {
        return false;
    }

    // 更新当前激活报警数
    g_alarm.stats.active_alarms = 0;
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        if (g_alarm.infos[i].state == ALARM_STATE_ACTIVE)
        {
            g_alarm.stats.active_alarms++;
        }
    }

    memcpy(stats, &g_alarm.stats, sizeof(alarm_stats_t));
    return true;
}

/**
 * @brief 重置报警统计信息
 */
bool alarm_reset_stats(void)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    memset(&g_alarm.stats, 0, sizeof(alarm_stats_t));
    return true;
}

// ============================================================================
// 报警规则管理接口实现
// ============================================================================

/**
 * @brief 添加报警规则
 */
bool alarm_add_rule(const alarm_rule_t *rule)
{
    if (!g_alarm.initialized || !rule || g_alarm.rule_count >= ALARM_MAX_RULES)
    {
        return false;
    }

    // 检查规则ID是否已存在
    if (alarm_find_rule_index(rule->id) < ALARM_MAX_RULES)
    {
        return false; // 规则ID已存在
    }

    // 添加规则
    memcpy(&g_alarm.rules[g_alarm.rule_count], rule, sizeof(alarm_rule_t));

    // 初始化报警信息
    alarm_info_t *info = &g_alarm.infos[g_alarm.rule_count];
    memset(info, 0, sizeof(alarm_info_t));
    info->rule_id = rule->id;
    info->state = ALARM_STATE_IDLE;
    info->level = rule->level;
    info->type = rule->type;

    g_alarm.rule_count++;

    return true;
}

/**
 * @brief 删除报警规则
 */
bool alarm_remove_rule(uint8_t rule_id)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    uint8_t index = alarm_find_rule_index(rule_id);
    if (index >= ALARM_MAX_RULES)
    {
        return false; // 规则不存在
    }

    // 如果报警处于激活状态，先解决它
    if (g_alarm.infos[index].state == ALARM_STATE_ACTIVE)
    {
        alarm_resolve(rule_id);
    }

    // 移动后续元素
    for (uint8_t i = index; i < g_alarm.rule_count - 1; i++)
    {
        memcpy(&g_alarm.rules[i], &g_alarm.rules[i + 1], sizeof(alarm_rule_t));
        memcpy(&g_alarm.infos[i], &g_alarm.infos[i + 1], sizeof(alarm_info_t));
    }

    g_alarm.rule_count--;
    return true;
}

/**
 * @brief 获取报警规则
 */
bool alarm_get_rule(uint8_t rule_id, alarm_rule_t *rule)
{
    if (!g_alarm.initialized || !rule)
    {
        return false;
    }

    uint8_t index = alarm_find_rule_index(rule_id);
    if (index >= ALARM_MAX_RULES)
    {
        return false;
    }

    memcpy(rule, &g_alarm.rules[index], sizeof(alarm_rule_t));
    return true;
}

/**
 * @brief 启用/禁用报警规则
 */
bool alarm_enable_rule(uint8_t rule_id, bool enabled)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    uint8_t index = alarm_find_rule_index(rule_id);
    if (index >= ALARM_MAX_RULES)
    {
        return false;
    }

    g_alarm.rules[index].enabled = enabled;

    // 如果禁用规则且当前有激活报警，解决它
    if (!enabled && g_alarm.infos[index].state == ALARM_STATE_ACTIVE)
    {
        alarm_resolve(rule_id);
    }

    return true;
}

// ============================================================================
// 报警触发和处理接口实现
// ============================================================================

/**
 * @brief 检查报警条件
 */
bool alarm_check_condition(uint8_t type, int32_t value)
{
    if (!g_alarm.initialized || !g_alarm.config.global_enable)
    {
        return false;
    }

    bool alarm_triggered = false;

    // 遍历所有规则，检查匹配的类型
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        alarm_rule_t *rule = &g_alarm.rules[i];
        alarm_info_t *info = &g_alarm.infos[i];

        if (!rule->enabled || rule->type != type)
        {
            continue;
        }

        // 评估报警条件
        bool condition_met = alarm_evaluate_condition(rule, value);

        if (condition_met && info->state == ALARM_STATE_IDLE)
        {
            // 触发报警
            uint32_t current_time = system_get_tick();

            info->state = ALARM_STATE_PENDING;
            info->trigger_time = current_time;
            info->trigger_value = value;
            info->trigger_count++;

            // 检查去抖时间
            if (rule->debounce_time == 0)
            {
                // 立即激活
                info->state = ALARM_STATE_ACTIVE;
                info->output_active = true;
                g_alarm.stats.total_alarms++;

                // 按级别统计
                switch (rule->level)
                {
                case ALARM_LEVEL_INFO:
                    g_alarm.stats.info_alarms++;
                    break;
                case ALARM_LEVEL_WARNING:
                    g_alarm.stats.warning_alarms++;
                    break;
                case ALARM_LEVEL_ERROR:
                    g_alarm.stats.error_alarms++;
                    break;
                case ALARM_LEVEL_CRITICAL:
                    g_alarm.stats.critical_alarms++;
                    break;
                }

                alarm_add_history(rule->id, ALARM_STATE_ACTIVE, value, rule->description);
                alarm_triggered = true;
            }
        }
        else if (!condition_met && (info->state == ALARM_STATE_PENDING || info->state == ALARM_STATE_ACTIVE))
        {
            // 条件不满足，解决报警
            info->state = ALARM_STATE_RESOLVED;
            info->resolve_time = system_get_tick();
            info->output_active = false;
            g_alarm.stats.auto_resolved++;

            alarm_add_history(rule->id, ALARM_STATE_RESOLVED, value, "Condition cleared");
        }
    }

    return alarm_triggered;
}

/**
 * @brief 手动触发报警
 */
bool alarm_trigger(uint8_t rule_id, int32_t value)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    uint8_t index = alarm_find_rule_index(rule_id);
    if (index >= ALARM_MAX_RULES)
    {
        return false;
    }

    alarm_rule_t *rule = &g_alarm.rules[index];
    alarm_info_t *info = &g_alarm.infos[index];

    if (!rule->enabled || info->state != ALARM_STATE_IDLE)
    {
        return false;
    }

    // 手动触发报警
    uint32_t current_time = system_get_tick();

    info->state = ALARM_STATE_ACTIVE;
    info->trigger_time = current_time;
    info->trigger_value = value;
    info->trigger_count++;
    info->output_active = true;

    g_alarm.stats.total_alarms++;

    // 按级别统计
    switch (rule->level)
    {
    case ALARM_LEVEL_INFO:
        g_alarm.stats.info_alarms++;
        break;
    case ALARM_LEVEL_WARNING:
        g_alarm.stats.warning_alarms++;
        break;
    case ALARM_LEVEL_ERROR:
        g_alarm.stats.error_alarms++;
        break;
    case ALARM_LEVEL_CRITICAL:
        g_alarm.stats.critical_alarms++;
        break;
    }

    alarm_add_history(rule->id, ALARM_STATE_ACTIVE, value, "Manual trigger");
    return true;
}

/**
 * @brief 确认报警
 */
bool alarm_acknowledge(uint8_t rule_id)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    uint8_t index = alarm_find_rule_index(rule_id);
    if (index >= ALARM_MAX_RULES)
    {
        return false;
    }

    alarm_info_t *info = &g_alarm.infos[index];

    if (info->state != ALARM_STATE_ACTIVE)
    {
        return false;
    }

    info->state = ALARM_STATE_ACKNOWLEDGED;
    info->acknowledge_time = system_get_tick();
    info->auto_acknowledged = false;
    g_alarm.stats.manual_acknowledged++;

    alarm_add_history(rule_id, ALARM_STATE_ACKNOWLEDGED, 0, "Manual acknowledge");
    return true;
}

/**
 * @brief 解决报警
 */
bool alarm_resolve(uint8_t rule_id)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    uint8_t index = alarm_find_rule_index(rule_id);
    if (index >= ALARM_MAX_RULES)
    {
        return false;
    }

    alarm_info_t *info = &g_alarm.infos[index];

    if (info->state == ALARM_STATE_IDLE || info->state == ALARM_STATE_RESOLVED)
    {
        return false;
    }

    info->state = ALARM_STATE_RESOLVED;
    info->resolve_time = system_get_tick();
    info->output_active = false;
    g_alarm.stats.manual_resolved++;

    alarm_add_history(rule_id, ALARM_STATE_RESOLVED, 0, "Manual resolve");
    return true;
}

/**
 * @brief 确认所有报警
 */
uint8_t alarm_acknowledge_all(void)
{
    if (!g_alarm.initialized)
    {
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        if (g_alarm.infos[i].state == ALARM_STATE_ACTIVE)
        {
            if (alarm_acknowledge(g_alarm.rules[i].id))
            {
                count++;
            }
        }
    }

    return count;
}

/**
 * @brief 静音报警输出
 */
bool alarm_silence(uint32_t duration)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    g_alarm.silenced = true;
    g_alarm.silence_start_time = system_get_tick();
    g_alarm.silence_duration = duration;

    return true;
}

// ============================================================================
// 报警状态查询接口实现
// ============================================================================

/**
 * @brief 获取激活的报警数量
 */
uint8_t alarm_get_active_count(void)
{
    if (!g_alarm.initialized)
    {
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        if (g_alarm.infos[i].state == ALARM_STATE_ACTIVE)
        {
            count++;
        }
    }

    return count;
}

/**
 * @brief 获取指定级别的报警数量
 */
uint8_t alarm_get_level_count(uint8_t level)
{
    if (!g_alarm.initialized)
    {
        return 0;
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        if (g_alarm.infos[i].state == ALARM_STATE_ACTIVE && g_alarm.rules[i].level == level)
        {
            count++;
        }
    }

    return count;
}

/**
 * @brief 获取报警信息
 */
bool alarm_get_info(uint8_t rule_id, alarm_info_t *info)
{
    if (!g_alarm.initialized || !info)
    {
        return false;
    }

    uint8_t index = alarm_find_rule_index(rule_id);
    if (index >= ALARM_MAX_RULES)
    {
        return false;
    }

    memcpy(info, &g_alarm.infos[index], sizeof(alarm_info_t));
    return true;
}

// ============================================================================
// 配置管理接口实现
// ============================================================================

/**
 * @brief 加载报警配置
 */
bool alarm_load_config(void)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    // 简化实现：使用默认配置
    // 实际实现应该从存储模块读取配置
    return true;
}

/**
 * @brief 保存报警配置
 */
bool alarm_save_config(void)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    // 简化实现：直接返回成功
    // 实际实现应该将配置保存到存储模块
    return true;
}

/**
 * @brief 恢复默认配置
 */
bool alarm_reset_config(void)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    alarm_setup_default_config();
    return true;
}

// ============================================================================
// 调试和诊断接口实现
// ============================================================================

/**
 * @brief 打印报警状态信息
 */
void alarm_print_status(void)
{
    if (!g_alarm.initialized)
    {
        return;
    }

    // 模拟调试输出
}

/**
 * @brief 自检报警系统
 */
bool alarm_self_test(void)
{
    if (!g_alarm.initialized)
    {
        return false;
    }

    // 简化实现：直接返回成功
    return true;
}

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 评估报警条件
 */
static bool alarm_evaluate_condition(const alarm_rule_t *rule, int32_t value)
{
    if (!rule)
    {
        return false;
    }

    switch (rule->condition)
    {
    case ALARM_CONDITION_GT:
        return (value > rule->threshold_high);
    case ALARM_CONDITION_LT:
        return (value < rule->threshold_low);
    case ALARM_CONDITION_GE:
        return (value >= rule->threshold_high);
    case ALARM_CONDITION_LE:
        return (value <= rule->threshold_low);
    case ALARM_CONDITION_EQ:
        return (value == rule->threshold_high);
    case ALARM_CONDITION_NE:
        return (value != rule->threshold_high);
    case ALARM_CONDITION_RANGE:
        return (value >= rule->threshold_low && value <= rule->threshold_high);
    case ALARM_CONDITION_OUT_RANGE:
        return (value < rule->threshold_low || value > rule->threshold_high);
    default:
        return false;
    }
}

/**
 * @brief 更新输出状态
 */
static void alarm_update_outputs(void)
{
    if (g_alarm.silenced)
    {
        return; // 静音状态下不更新输出
    }

    bool led_active = false;
    bool buzzer_active = false;

    // 检查是否有激活的报警需要输出
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        if (g_alarm.infos[i].state == ALARM_STATE_ACTIVE && g_alarm.infos[i].output_active)
        {
            uint8_t output_mask = g_alarm.rules[i].output_mask;

            if (output_mask & ALARM_OUTPUT_LED)
            {
                led_active = true;
            }
            if (output_mask & ALARM_OUTPUT_BUZZER)
            {
                buzzer_active = true;
            }
        }
    }

    // 更新LED输出
    if (g_alarm.config.outputs[0].enabled && g_alarm.config.outputs[0].type == ALARM_OUTPUT_LED)
    {
        bool output_level = led_active ? g_alarm.config.outputs[0].active_high : !g_alarm.config.outputs[0].active_high;
        gpio_write_pin(GPIO_PORT_B, 5, output_level);
    }

    // 更新蜂鸣器输出
    if (g_alarm.config.outputs[1].enabled && g_alarm.config.outputs[1].type == ALARM_OUTPUT_BUZZER)
    {
        bool output_level = buzzer_active ? g_alarm.config.outputs[1].active_high : !g_alarm.config.outputs[1].active_high;
        gpio_write_pin(GPIO_PORT_B, 6, output_level);
    }

    if (led_active || buzzer_active)
    {
        g_alarm.stats.output_activations++;
    }
}

/**
 * @brief 添加历史记录
 */
static void alarm_add_history(uint8_t rule_id, alarm_state_t state, int32_t value, const char *description)
{
    if (g_alarm.history_count >= ALARM_MAX_HISTORY)
    {
        // 循环覆盖
        g_alarm.history_index = 0;
    }

    alarm_history_t *record = &g_alarm.history[g_alarm.history_index];

    record->timestamp = system_get_tick();
    record->rule_id = rule_id;
    record->state = state;
    record->value = value;

    // 查找规则信息
    uint8_t rule_index = alarm_find_rule_index(rule_id);
    if (rule_index < ALARM_MAX_RULES)
    {
        record->type = g_alarm.rules[rule_index].type;
        record->level = g_alarm.rules[rule_index].level;
        record->duration = g_alarm.infos[rule_index].duration;
    }

    if (description)
    {
        strncpy(record->description, description, sizeof(record->description) - 1);
        record->description[sizeof(record->description) - 1] = '\0';
    }

    g_alarm.history_index++;
    if (g_alarm.history_count < ALARM_MAX_HISTORY)
    {
        g_alarm.history_count++;
    }
}

/**
 * @brief 查找规则索引
 */
static uint8_t alarm_find_rule_index(uint8_t rule_id)
{
    for (uint8_t i = 0; i < g_alarm.rule_count; i++)
    {
        if (g_alarm.rules[i].id == rule_id)
        {
            return i;
        }
    }
    return ALARM_MAX_RULES; // 未找到
}

/**
 * @brief 设置默认报警规则
 */
static void alarm_setup_default_rules(void)
{
    // 温度高报警
    alarm_rule_t temp_high_rule = {
        .id = 1,
        .type = ALARM_TYPE_TEMPERATURE,
        .level = ALARM_LEVEL_WARNING,
        .condition = ALARM_CONDITION_GT,
        .enabled = true,
        .threshold_high = 600,    // 60.0°C
        .debounce_time = 5000,    // 5秒
        .auto_reset_time = 30000, // 30秒
        .output_mask = ALARM_OUTPUT_LED,
        .priority = 3,
        .description = "High Temperature"};
    strcpy(temp_high_rule.description, "High Temperature");
    alarm_add_rule(&temp_high_rule);

    // 温度低报警
    alarm_rule_t temp_low_rule = {
        .id = 2,
        .type = ALARM_TYPE_TEMPERATURE,
        .level = ALARM_LEVEL_WARNING,
        .condition = ALARM_CONDITION_LT,
        .enabled = true,
        .threshold_low = -100,    // -10.0°C
        .debounce_time = 5000,    // 5秒
        .auto_reset_time = 30000, // 30秒
        .output_mask = ALARM_OUTPUT_LED,
        .priority = 3,
        .description = "Low Temperature"};
    strcpy(temp_low_rule.description, "Low Temperature");
    alarm_add_rule(&temp_low_rule);

    // 湿度高报警
    alarm_rule_t humidity_high_rule = {
        .id = 3,
        .type = ALARM_TYPE_HUMIDITY,
        .level = ALARM_LEVEL_WARNING,
        .condition = ALARM_CONDITION_GT,
        .enabled = true,
        .threshold_high = 900,    // 90.0%RH
        .debounce_time = 10000,   // 10秒
        .auto_reset_time = 60000, // 60秒
        .output_mask = ALARM_OUTPUT_LED,
        .priority = 2,
        .description = "High Humidity"};
    strcpy(humidity_high_rule.description, "High Humidity");
    alarm_add_rule(&humidity_high_rule);

    // 电压低报警
    alarm_rule_t voltage_low_rule = {
        .id = 4,
        .type = ALARM_TYPE_VOLTAGE,
        .level = ALARM_LEVEL_ERROR,
        .condition = ALARM_CONDITION_LT,
        .enabled = true,
        .threshold_low = 2800,    // 2.8V
        .debounce_time = 3000,    // 3秒
        .auto_reset_time = 30000, // 30秒
        .output_mask = ALARM_OUTPUT_LED | ALARM_OUTPUT_BUZZER,
        .priority = 5,
        .description = "Low Voltage"};
    strcpy(voltage_low_rule.description, "Low Voltage");
    alarm_add_rule(&voltage_low_rule);
}

/**
 * @brief 设置默认配置
 */
static void alarm_setup_default_config(void)
{
    g_alarm.config.global_enable = true;
    g_alarm.config.default_level = ALARM_LEVEL_WARNING;
    g_alarm.config.max_active_alarms = 8;
    g_alarm.config.auto_acknowledge = false;
    g_alarm.config.auto_resolve = true;
    g_alarm.config.global_debounce = 1000; // 1秒

    // LED输出配置 (假设使用PB5)
    g_alarm.config.outputs[0].type = ALARM_OUTPUT_LED;
    g_alarm.config.outputs[0].gpio_pin = 5; // PB5
    g_alarm.config.outputs[0].active_high = true;
    g_alarm.config.outputs[0].pulse_period = 1000; // 1秒
    g_alarm.config.outputs[0].pulse_duty = 50;     // 50%
    g_alarm.config.outputs[0].enabled = true;

    // 蜂鸣器输出配置 (假设使用PB6)
    g_alarm.config.outputs[1].type = ALARM_OUTPUT_BUZZER;
    g_alarm.config.outputs[1].gpio_pin = 6; // PB6
    g_alarm.config.outputs[1].active_high = true;
    g_alarm.config.outputs[1].pulse_period = 500; // 0.5秒
    g_alarm.config.outputs[1].pulse_duty = 20;    // 20%
    g_alarm.config.outputs[1].enabled = false;    // 默认禁用蜂鸣器

    // 其他输出默认禁用
    for (uint8_t i = 2; i < ALARM_MAX_OUTPUTS; i++)
    {
        g_alarm.config.outputs[i].enabled = false;
    }
}

/**
 * @brief 报警任务函数 (在主循环中调用)
 */
void alarm_task(void)
{
    alarm_process();
}