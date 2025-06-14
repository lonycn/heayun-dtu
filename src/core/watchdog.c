/**
 * @file watchdog.c
 * @brief 憨云DTU看门狗管理
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 看门狗初始化、喂狗和状态管理
 */

#include "system.h"

// NANO100B看门狗寄存器定义 (简化版)
#define WDT_BASE 0x40004000
#define WDT_WTCR (*(volatile uint32_t *)(WDT_BASE + 0x00))    // 控制寄存器
#define WDT_WTCRALT (*(volatile uint32_t *)(WDT_BASE + 0x04)) // 备用控制寄存器

// 看门狗控制位
#define WDT_WTCR_WTR (1 << 0)   // 看门狗定时器复位位
#define WDT_WTCR_WTRE (1 << 1)  // 看门狗定时器复位使能
#define WDT_WTCR_WTRF (1 << 2)  // 看门狗定时器复位标志
#define WDT_WTCR_WTIF (1 << 3)  // 看门狗定时器中断标志
#define WDT_WTCR_WTWKE (1 << 4) // 看门狗定时器唤醒使能
#define WDT_WTCR_WTWKF (1 << 5) // 看门狗定时器唤醒标志
#define WDT_WTCR_WTIE (1 << 6)  // 看门狗定时器中断使能
#define WDT_WTCR_WTE (1 << 7)   // 看门狗定时器使能

// 看门狗超时时间选择 (WTIS[2:0])
#define WDT_TIMEOUT_2POW4 (0 << 8)  // 2^4 * WDT_CLK
#define WDT_TIMEOUT_2POW6 (1 << 8)  // 2^6 * WDT_CLK
#define WDT_TIMEOUT_2POW8 (2 << 8)  // 2^8 * WDT_CLK
#define WDT_TIMEOUT_2POW10 (3 << 8) // 2^10 * WDT_CLK
#define WDT_TIMEOUT_2POW12 (4 << 8) // 2^12 * WDT_CLK
#define WDT_TIMEOUT_2POW14 (5 << 8) // 2^14 * WDT_CLK
#define WDT_TIMEOUT_2POW16 (6 << 8) // 2^16 * WDT_CLK
#define WDT_TIMEOUT_2POW18 (7 << 8) // 2^18 * WDT_CLK

// ============================================================================
// 全局变量
// ============================================================================

// 看门狗状态
static struct
{
    bool enabled;            // 看门狗使能状态
    uint32_t timeout_ms;     // 超时时间(毫秒)
    uint32_t last_feed_time; // 最后喂狗时间
    uint32_t feed_count;     // 喂狗计数器
    uint32_t reset_count;    // 看门狗复位计数
} watchdog_state = {0};

// ============================================================================
// 局部函数
// ============================================================================

/**
 * @brief 根据超时时间选择看门狗配置
 * @param timeout_ms 超时时间(毫秒)
 * @return 看门狗超时配置
 */
static uint32_t watchdog_get_timeout_config(uint32_t timeout_ms)
{
    // 简化计算：假设WDT_CLK = 10kHz (内部低频振荡器)
    // 实际频率需要参考NANO100B数据手册
    uint32_t wdt_clk = 10000; // 10kHz
    uint32_t timeout_ticks = (timeout_ms * wdt_clk) / 1000;

    // 选择合适的超时配置
    if (timeout_ticks <= (1 << 4))
    {
        return WDT_TIMEOUT_2POW4; // ~1.6ms @ 10kHz
    }
    else if (timeout_ticks <= (1 << 6))
    {
        return WDT_TIMEOUT_2POW6; // ~6.4ms @ 10kHz
    }
    else if (timeout_ticks <= (1 << 8))
    {
        return WDT_TIMEOUT_2POW8; // ~25.6ms @ 10kHz
    }
    else if (timeout_ticks <= (1 << 10))
    {
        return WDT_TIMEOUT_2POW10; // ~102.4ms @ 10kHz
    }
    else if (timeout_ticks <= (1 << 12))
    {
        return WDT_TIMEOUT_2POW12; // ~409.6ms @ 10kHz
    }
    else if (timeout_ticks <= (1 << 14))
    {
        return WDT_TIMEOUT_2POW14; // ~1.6s @ 10kHz
    }
    else if (timeout_ticks <= (1 << 16))
    {
        return WDT_TIMEOUT_2POW16; // ~6.5s @ 10kHz
    }
    else
    {
        return WDT_TIMEOUT_2POW18; // ~26.2s @ 10kHz
    }
}

// ============================================================================
// 看门狗接口实现
// ============================================================================

/**
 * @brief 看门狗初始化
 * @param timeout_ms 超时时间(毫秒)
 * @return true: 成功, false: 失败
 */
bool watchdog_init(uint32_t timeout_ms)
{
    // 参数检查
    if (timeout_ms < 10 || timeout_ms > 30000)
    {
        // 超时时间应在10ms到30秒之间
        return false;
    }

    // 保存配置
    watchdog_state.timeout_ms = timeout_ms;
    watchdog_state.enabled = false;
    watchdog_state.last_feed_time = system_get_tick();
    watchdog_state.feed_count = 0;

    // 检查是否是看门狗复位
    if (WDT_WTCR & WDT_WTCR_WTRF)
    {
        watchdog_state.reset_count++;

        // 清除复位标志
        WDT_WTCR |= WDT_WTCR_WTRF;

        debug_printf("[WDT] Watchdog reset detected (count: %lu)\n",
                     watchdog_state.reset_count);
    }

    // 获取超时配置
    uint32_t timeout_config = watchdog_get_timeout_config(timeout_ms);

    // 配置看门狗寄存器
    uint32_t wtcr_value = 0;
    wtcr_value |= timeout_config; // 超时时间
    wtcr_value |= WDT_WTCR_WTRE;  // 使能复位
    // 注意：暂时不启用看门狗，等待明确调用watchdog_enable()

    WDT_WTCR = wtcr_value;

    debug_printf("[WDT] Watchdog initialized, timeout: %lu ms\n", timeout_ms);

    return true;
}

/**
 * @brief 刷新看门狗 (喂狗)
 */
void watchdog_refresh(void)
{
    if (!watchdog_state.enabled)
    {
        return;
    }

    // 写入看门狗刷新位
    WDT_WTCR |= WDT_WTCR_WTR;

    // 更新状态
    watchdog_state.last_feed_time = system_get_tick();
    watchdog_state.feed_count++;

    // 每1000次喂狗打印一次统计
    if (watchdog_state.feed_count % 1000 == 0)
    {
        debug_printf("[WDT] Feed count: %lu\n", watchdog_state.feed_count);
    }
}

/**
 * @brief 使能/禁用看门狗
 * @param enable true: 使能, false: 禁用
 */
void watchdog_enable(bool enable)
{
    if (enable)
    {
        // 使能看门狗
        WDT_WTCR |= WDT_WTCR_WTE;
        watchdog_state.enabled = true;
        watchdog_state.last_feed_time = system_get_tick();

        debug_printf("[WDT] Watchdog enabled\n");

        // 立即喂一次狗
        watchdog_refresh();
    }
    else
    {
        // 禁用看门狗
        WDT_WTCR &= ~WDT_WTCR_WTE;
        watchdog_state.enabled = false;

        debug_printf("[WDT] Watchdog disabled\n");
    }
}

/**
 * @brief 检查看门狗是否使能
 * @return true: 使能, false: 禁用
 */
bool watchdog_is_enabled(void)
{
    return watchdog_state.enabled;
}

/**
 * @brief 获取看门狗状态信息
 * @param timeout_ms 超时时间(毫秒)
 * @param last_feed_time 最后喂狗时间
 * @param feed_count 喂狗计数
 * @param reset_count 复位计数
 */
void watchdog_get_status(uint32_t *timeout_ms, uint32_t *last_feed_time,
                         uint32_t *feed_count, uint32_t *reset_count)
{
    if (timeout_ms)
    {
        *timeout_ms = watchdog_state.timeout_ms;
    }

    if (last_feed_time)
    {
        *last_feed_time = watchdog_state.last_feed_time;
    }

    if (feed_count)
    {
        *feed_count = watchdog_state.feed_count;
    }

    if (reset_count)
    {
        *reset_count = watchdog_state.reset_count;
    }
}

/**
 * @brief 检查看门狗是否即将超时
 * @param warning_threshold_ms 预警阈值(毫秒)
 * @return true: 即将超时, false: 正常
 */
bool watchdog_is_near_timeout(uint32_t warning_threshold_ms)
{
    if (!watchdog_state.enabled)
    {
        return false;
    }

    uint32_t current_time = system_get_tick();
    uint32_t elapsed = current_time - watchdog_state.last_feed_time;
    uint32_t remaining = watchdog_state.timeout_ms - elapsed;

    return (remaining <= warning_threshold_ms);
}

/**
 * @brief 强制触发看门狗复位 (测试用)
 */
void watchdog_force_reset(void)
{
    debug_printf("[WDT] Forcing watchdog reset...\n");

    // 启用看门狗但不喂狗，等待超时复位
    watchdog_enable(true);

    while (1)
    {
        // 等待看门狗复位
        system_delay_ms(100);
    }
}

/**
 * @brief 看门狗状态信息打印 (调试用)
 */
void watchdog_print_status(void)
{
    uint32_t current_time = system_get_tick();
    uint32_t elapsed = current_time - watchdog_state.last_feed_time;

    debug_printf("\n[WDT] Watchdog Status:\n");
    debug_printf("Enabled: %s\n", watchdog_state.enabled ? "yes" : "no");
    debug_printf("Timeout: %lu ms\n", watchdog_state.timeout_ms);
    debug_printf("Last feed: %lu ms ago\n", elapsed);
    debug_printf("Feed count: %lu\n", watchdog_state.feed_count);
    debug_printf("Reset count: %lu\n", watchdog_state.reset_count);

    if (watchdog_state.enabled)
    {
        uint32_t remaining = watchdog_state.timeout_ms - elapsed;
        debug_printf("Remaining: %lu ms\n", remaining);

        if (watchdog_is_near_timeout(100))
        {
            debug_printf("WARNING: Near timeout!\n");
        }
    }

    debug_printf("WTCR register: 0x%08lX\n", WDT_WTCR);
    debug_printf("\n");
}