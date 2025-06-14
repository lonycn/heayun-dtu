/**
 * @file timer.c
 * @brief 憨云DTU软件定时器管理
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 实现8个软件定时器，专为8KB RAM优化
 */

#include "system.h"
#include <stddef.h> // for NULL

// ============================================================================
// 全局变量
// ============================================================================

// 软件定时器数组 (在timer.c中定义)
static timer_t g_timers[MAX_TIMERS] = {0};

// 定时器管理状态
static struct
{
    uint8_t active_count;     // 活跃定时器数量
    uint32_t process_time_us; // 定时器处理时间统计
} timer_stats = {0};

/**
 * @brief 定时器系统初始化
 * @return true: 成功, false: 失败
 */
bool timer_init(void)
{
    // 初始化所有定时器状态
    for (int i = 0; i < MAX_TIMERS; i++)
    {
        g_timers[i].enabled = false;
        g_timers[i].auto_reload = false;
        g_timers[i].interval = 0;
        g_timers[i].last_tick = 0;
        g_timers[i].callback = NULL;
    }

    timer_stats.active_count = 0;
    timer_stats.process_time_us = 0;

    return true;
}

// ============================================================================
// 软件定时器接口实现
// ============================================================================

/**
 * @brief 创建软件定时器
 * @param timer_id 定时器ID (0-7)
 * @param interval_ms 定时间隔(毫秒)
 * @param auto_reload 是否自动重载
 * @param callback 回调函数
 * @return true: 成功, false: 失败
 */
bool timer_create(uint8_t timer_id, uint32_t interval_ms, bool auto_reload, void (*callback)(void))
{
    // 参数检查
    if (timer_id >= MAX_TIMERS)
    {
        return false;
    }

    if (interval_ms == 0 || callback == NULL)
    {
        return false;
    }

    // 配置定时器
    timer_t *timer = &g_timers[timer_id];

    timer->interval = interval_ms;
    timer->auto_reload = auto_reload;
    timer->callback = callback;
    timer->enabled = false;
    timer->last_tick = 0;

    return true;
}

/**
 * @brief 启动定时器
 * @param timer_id 定时器ID
 * @return true: 成功, false: 失败
 */
bool timer_start(uint8_t timer_id)
{
    if (timer_id >= MAX_TIMERS)
    {
        return false;
    }

    timer_t *timer = &g_timers[timer_id];

    // 检查定时器是否已配置
    if (timer->callback == NULL)
    {
        return false;
    }

    // 启动定时器
    timer->enabled = true;
    timer->last_tick = system_get_tick();

    // 更新活跃定时器计数
    timer_stats.active_count++;

    return true;
}

/**
 * @brief 停止定时器
 * @param timer_id 定时器ID
 * @return true: 成功, false: 失败
 */
bool timer_stop(uint8_t timer_id)
{
    if (timer_id >= MAX_TIMERS)
    {
        return false;
    }

    timer_t *timer = &g_timers[timer_id];

    if (timer->enabled)
    {
        timer->enabled = false;

        // 更新活跃定时器计数
        if (timer_stats.active_count > 0)
        {
            timer_stats.active_count--;
        }
    }

    return true;
}

/**
 * @brief 定时器处理函数 (在系统tick中调用)
 */
void timer_process(void)
{
    uint32_t current_tick = system_get_tick();
    uint32_t process_start = current_tick;

    // 遍历所有定时器
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        timer_t *timer = &g_timers[i];

        // 跳过未启用的定时器
        if (!timer->enabled || timer->callback == NULL)
        {
            continue;
        }

        // 检查是否到期
        uint32_t elapsed = current_tick - timer->last_tick;
        if (elapsed >= timer->interval)
        {

            // 执行回调函数
            timer->callback();

            // 更新定时器状态
            if (timer->auto_reload)
            {
                // 自动重载：更新最后触发时间
                timer->last_tick = current_tick;
            }
            else
            {
                // 单次触发：停止定时器
                timer->enabled = false;
                if (timer_stats.active_count > 0)
                {
                    timer_stats.active_count--;
                }
            }
        }
    }

    // 统计处理时间
    uint32_t process_end = system_get_tick();
    timer_stats.process_time_us = (process_end - process_start) * 1000;

    // 性能警告：定时器处理时间过长
    if (timer_stats.process_time_us > 100)
    { // 超过100us
        debug_printf("[WARN] Timer process time: %lu us\n", timer_stats.process_time_us);
    }
}

/**
 * @brief 重置定时器 (重新开始计时)
 * @param timer_id 定时器ID
 * @return true: 成功, false: 失败
 */
bool timer_reset(uint8_t timer_id)
{
    if (timer_id >= MAX_TIMERS)
    {
        return false;
    }

    timer_t *timer = &g_timers[timer_id];

    if (timer->enabled)
    {
        timer->last_tick = system_get_tick();
        return true;
    }

    return false;
}

/**
 * @brief 修改定时器间隔
 * @param timer_id 定时器ID
 * @param new_interval_ms 新的定时间隔(毫秒)
 * @return true: 成功, false: 失败
 */
bool timer_set_interval(uint8_t timer_id, uint32_t new_interval_ms)
{
    if (timer_id >= MAX_TIMERS || new_interval_ms == 0)
    {
        return false;
    }

    timer_t *timer = &g_timers[timer_id];

    timer->interval = new_interval_ms;

    // 如果定时器正在运行，重置计时
    if (timer->enabled)
    {
        timer->last_tick = system_get_tick();
    }

    return true;
}

/**
 * @brief 检查定时器是否正在运行
 * @param timer_id 定时器ID
 * @return true: 运行中, false: 未运行
 */
bool timer_is_running(uint8_t timer_id)
{
    if (timer_id >= MAX_TIMERS)
    {
        return false;
    }

    return g_timers[timer_id].enabled;
}

/**
 * @brief 获取定时器剩余时间
 * @param timer_id 定时器ID
 * @return 剩余时间(毫秒)，-1表示定时器未运行
 */
int32_t timer_get_remaining_time(uint8_t timer_id)
{
    if (timer_id >= MAX_TIMERS)
    {
        return -1;
    }

    timer_t *timer = &g_timers[timer_id];

    if (!timer->enabled)
    {
        return -1;
    }

    uint32_t current_tick = system_get_tick();
    uint32_t elapsed = current_tick - timer->last_tick;

    if (elapsed >= timer->interval)
    {
        return 0; // 已到期
    }

    return (int32_t)(timer->interval - elapsed);
}

/**
 * @brief 获取活跃定时器数量
 * @return 活跃定时器数量
 */
uint8_t timer_get_active_count(void)
{
    return timer_stats.active_count;
}

/**
 * @brief 获取定时器处理统计信息
 * @param process_time_us 处理时间(微秒)
 * @param active_count 活跃定时器数量
 */
void timer_get_stats(uint32_t *process_time_us, uint8_t *active_count)
{
    if (process_time_us)
    {
        *process_time_us = timer_stats.process_time_us;
    }

    if (active_count)
    {
        *active_count = timer_stats.active_count;
    }
}

/**
 * @brief 停止所有定时器
 */
void timer_stop_all(void)
{
    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        timer_stop(i);
    }

    timer_stats.active_count = 0;
}

/**
 * @brief 定时器模块信息打印 (调试用)
 */
void timer_print_info(void)
{
    debug_printf("\n[TIMER] Software Timer Status:\n");
    debug_printf("Active timers: %d/%d\n", timer_stats.active_count, MAX_TIMERS);
    debug_printf("Process time: %lu us\n", timer_stats.process_time_us);

    for (uint8_t i = 0; i < MAX_TIMERS; i++)
    {
        timer_t *timer = &g_timers[i];

        if (timer->callback != NULL)
        {
            debug_printf("Timer %d: ", i);
            debug_printf("interval=%lu ms, ", timer->interval);
            debug_printf("enabled=%s, ", timer->enabled ? "yes" : "no");
            debug_printf("auto_reload=%s\n", timer->auto_reload ? "yes" : "no");

            if (timer->enabled)
            {
                int32_t remaining = timer_get_remaining_time(i);
                debug_printf("         remaining=%ld ms\n", remaining);
            }
        }
    }
    debug_printf("\n");
}