/**
 * ================================================================
 * 憨云 DTU - 调试版主程序 (极简版)
 * ================================================================
 * 文件: main_debug.c
 * 功能: 极简的硬件测试程序，用于诊断硬件问题
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"

// ================================================================
// 极简延时函数
// ================================================================

/**
 * @brief 极简延时函数
 * @param count 延时循环次数
 */
static void simple_delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm volatile("nop");
    }
}

// ================================================================
// 极简GPIO控制
// ================================================================

/**
 * @brief 极简GPIO初始化
 */
static void simple_gpio_init(void)
{
    // 直接操作寄存器，确保GPIO配置正确

    // 1. 使能GPIO时钟 (如果需要)
    // NANO100B可能需要使能GPIO时钟，但先尝试不使能

    // 2. 配置PA0为输出模式 (系统状态LED)
    // PA0 = bit 0, 需要设置PMD0的bit[1:0] = 01 (推挽输出)
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) &= ~(0x3 << 0); // 清除bit[1:0]
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) |= (0x1 << 0);  // 设置为01 (推挽输出)

    // 3. 配置PA1为输出模式 (调试LED)
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) &= ~(0x3 << 2); // 清除bit[3:2]
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) |= (0x1 << 2);  // 设置为01 (推挽输出)

    // 4. 配置PB6为输出模式 (蜂鸣器)
    REG32(GPIOB_BASE + GPIO_PMD_OFFSET) &= ~(0x3 << 12); // 清除bit[13:12]
    REG32(GPIOB_BASE + GPIO_PMD_OFFSET) |= (0x1 << 12);  // 设置为01 (推挽输出)

    // 5. 初始状态：所有输出为低电平
    REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1 << 0); // PA0 = 0
    REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1 << 1); // PA1 = 0
    REG32(GPIOB_BASE + GPIO_DOUT_OFFSET) &= ~(1 << 6); // PB6 = 0
}

/**
 * @brief 设置LED状态
 * @param led_num LED编号 (0=PA0, 1=PA1)
 * @param state TRUE=点亮, FALSE=熄灭
 */
static void set_led(uint8_t led_num, boolean_t state)
{
    if (led_num == 0) // PA0
    {
        if (state)
        {
            REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) |= (1 << 0); // PA0 = 1
        }
        else
        {
            REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1 << 0); // PA0 = 0
        }
    }
    else if (led_num == 1) // PA1
    {
        if (state)
        {
            REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) |= (1 << 1); // PA1 = 1
        }
        else
        {
            REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1 << 1); // PA1 = 0
        }
    }
}

/**
 * @brief 设置蜂鸣器状态
 * @param state TRUE=开启, FALSE=关闭
 */
static void set_buzzer(boolean_t state)
{
    if (state)
    {
        REG32(GPIOB_BASE + GPIO_DOUT_OFFSET) |= (1 << 6); // PB6 = 1
    }
    else
    {
        REG32(GPIOB_BASE + GPIO_DOUT_OFFSET) &= ~(1 << 6); // PB6 = 0
    }
}

// ================================================================
// 主程序入口
// ================================================================

/**
 * @brief 调试版主程序入口
 * @return 程序退出码 (实际上不会返回)
 */
int main(void)
{
    // 1. 极简GPIO初始化
    simple_gpio_init();

    // 2. 启动指示：快速闪烁所有LED和蜂鸣器
    for (uint8_t i = 0; i < 5; i++)
    {
        // 全部开启
        set_led(0, TRUE);     // PA0 LED
        set_led(1, TRUE);     // PA1 LED
        set_buzzer(TRUE);     // 蜂鸣器
        simple_delay(500000); // 约100ms延时

        // 全部关闭
        set_led(0, FALSE);
        set_led(1, FALSE);
        set_buzzer(FALSE);
        simple_delay(500000); // 约100ms延时
    }

    // 3. 主循环：交替闪烁LED (避免除法运算)
    uint32_t counter = 0;
    uint8_t state = 0; // 状态机：0,1,2,3循环

    while (1)
    {
        counter++;

        // 每1M次循环切换一次状态 (使用位运算检查)
        if ((counter & 0xFFFFF) == 0) // 每1048576次循环
        {
            // 状态机：4个状态循环
            switch (state)
            {
            case 0:
                set_led(0, TRUE);
                set_led(1, FALSE);
                set_buzzer(FALSE);
                break;
            case 1:
                set_led(0, FALSE);
                set_led(1, TRUE);
                set_buzzer(FALSE);
                break;
            case 2:
                set_led(0, FALSE);
                set_led(1, FALSE);
                set_buzzer(TRUE);
                break;
            case 3:
            default:
                set_led(0, FALSE);
                set_led(1, FALSE);
                set_buzzer(FALSE);
                break;
            }

            // 状态递增，循环0-3
            state++;
            if (state >= 4)
            {
                state = 0;
            }
        }

        // 防止计数器溢出
        if (counter >= 0xFFFFF000)
        {
            counter = 0;
        }
    }

    return 0;
}