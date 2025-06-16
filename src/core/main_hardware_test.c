/**
 * ================================================================
 * 憨云 DTU - 硬件测试程序
 * ================================================================
 * 文件: main_hardware_test.c
 * 功能: 专门用于测试硬件连接的简化程序
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"

// ================================================================
// 简化的硬件测试函数
// ================================================================

/**
 * @brief 简单延时函数
 * @param ms 延时毫秒数
 */
static void simple_delay_ms(uint32_t ms)
{
    // 简单的软件延时，适用于12MHz时钟
    volatile uint32_t count = ms * 3000;
    while (count--)
    {
        __asm volatile("nop");
    }
}

/**
 * @brief 系统时钟初始化
 */
static void hardware_test_clock_init(void)
{
    // 1. 使能内部高速RC振荡器 (HIRC 12MHz)
    REG32(CLK_BASE + CLK_PWRCTL_OFFSET) |= (1UL << 2);
    
    // 2. 等待HIRC稳定
    while(!(REG32(CLK_BASE + CLK_CLKSTATUS_OFFSET) & (1UL << 2)));
    
    // 3. 切换系统时钟源到HIRC
    REG32(CLK_BASE + CLK_CLKSEL0_OFFSET) &= ~(0x7UL << 0);
    REG32(CLK_BASE + CLK_CLKSEL0_OFFSET) |= (0x0UL << 0);
    
    // 4. 使能GPIO时钟
    REG32(CLK_BASE + CLK_AHBCLK_OFFSET) |= (1UL << 2);
    
    // 5. 使能PWM0时钟 (蜂鸣器用)
    REG32(CLK_BASE + CLK_APBCLK_OFFSET) |= (1UL << 20);
    
    // 6. 延时确保时钟稳定
    simple_delay_ms(10);
}

/**
 * @brief GPIO初始化
 */
static void hardware_test_gpio_init(void)
{
    // 1. 配置PC8为输出模式 (系统状态LED)
    // PC8 = bit 8, 需要设置PMD8的bit[17:16] = 01 (推挽输出)
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 16); // 清除bit[17:16]
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) |= (0x1UL << 16);  // 设置为01 (推挽输出)
    
    // 2. 配置PA1为输出模式 (调试LED)
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 2); // 清除bit[3:2]
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) |= (0x1UL << 2);  // 设置为01 (推挽输出)
    
    // 3. 配置PA6为输出模式 (蜂鸣器)
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 12); // 清除bit[13:12]
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) |= (0x1UL << 12);  // 设置为01 (推挽输出)
    
    // 4. 配置PA2为输入模式 (用户按键，内部上拉)
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 4); // 清除bit[5:4]
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) |= (0x0UL << 4);  // 设置为00 (输入模式)
    
    // 5. 初始状态：所有LED关闭，蜂鸣器关闭
    REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 8);  // PC8 LED关闭
    REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 1);  // PA1 LED关闭
    REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 6);  // PA6 蜂鸣器关闭
}

/**
 * @brief PWM初始化 (蜂鸣器)
 */
static void hardware_test_pwm_init(void)
{
    // 配置PWM0通道0，频率约2kHz
    REG32(PWM0_BASE + PWM_PPR_OFFSET) = 0;                 // 预分频器设为1
    REG32(PWM0_BASE + PWM_CSR_OFFSET) = 0x0UL;             // 时钟分频2
    REG32(PWM0_BASE + PWM_CNR0_OFFSET) = 3000;             // 周期值
    REG32(PWM0_BASE + PWM_CMR0_OFFSET) = 1500;             // 占空比50%
    
    // 禁用PWM输出 (初始状态)
    REG32(PWM0_BASE + PWM_PCR_OFFSET) &= ~(1UL << 0);
}

/**
 * @brief LED控制函数
 */
static void led_control(boolean_t system_led, boolean_t debug_led)
{
    if (system_led)
        REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) |= (1UL << 8);   // PC8 LED开
    else
        REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 8);  // PC8 LED关
        
    if (debug_led)
        REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) |= (1UL << 1);   // PA1 LED开
    else
        REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 1);  // PA1 LED关
}

/**
 * @brief 蜂鸣器控制函数
 */
static void buzzer_control(boolean_t enable)
{
    if (enable)
    {
        // 使能PWM输出
        REG32(PWM0_BASE + PWM_PCR_OFFSET) |= (1UL << 0);
    }
    else
    {
        // 禁用PWM输出
        REG32(PWM0_BASE + PWM_PCR_OFFSET) &= ~(1UL << 0);
        // 确保输出低电平
        REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 6);
    }
}

/**
 * @brief 读取按键状态
 */
static boolean_t read_button(void)
{
    // 读取PA2按键状态 (假设按下为低电平)
    return !(REG32(GPIOA_BASE + GPIO_PIN_OFFSET) & (1UL << 2));
}

/**
 * @brief 硬件测试序列
 */
static void hardware_test_sequence(void)
{
    // 测试序列1: LED闪烁测试
    for (uint8_t i = 0; i < 5; i++)
    {
        led_control(TRUE, FALSE);   // 系统LED开
        simple_delay_ms(200);
        led_control(FALSE, TRUE);   // 调试LED开
        simple_delay_ms(200);
        led_control(FALSE, FALSE);  // 全部LED关
        simple_delay_ms(200);
    }
    
    // 测试序列2: 蜂鸣器测试
    for (uint8_t i = 0; i < 3; i++)
    {
        buzzer_control(TRUE);       // 蜂鸣器开
        simple_delay_ms(100);
        buzzer_control(FALSE);      // 蜂鸣器关
        simple_delay_ms(100);
    }
    
    // 测试序列3: 同时测试LED和蜂鸣器
    for (uint8_t i = 0; i < 2; i++)
    {
        led_control(TRUE, TRUE);    // 所有LED开
        buzzer_control(TRUE);       // 蜂鸣器开
        simple_delay_ms(300);
        led_control(FALSE, FALSE);  // 所有LED关
        buzzer_control(FALSE);      // 蜂鸣器关
        simple_delay_ms(300);
    }
}

/**
 * @brief 主程序入口
 */
int main(void)
{
    // 1. 系统初始化
    hardware_test_clock_init();
    hardware_test_gpio_init();
    hardware_test_pwm_init();
    
    // 2. 启动指示：快速闪烁3次
    for (uint8_t i = 0; i < 3; i++)
    {
        led_control(TRUE, TRUE);
        buzzer_control(TRUE);
        simple_delay_ms(100);
        led_control(FALSE, FALSE);
        buzzer_control(FALSE);
        simple_delay_ms(100);
    }
    
    // 3. 主循环
    uint32_t loop_counter = 0;
    
    while (1)
    {
        // 每1000次循环执行一次硬件测试序列
        if ((loop_counter % 1000) == 0)
        {
            hardware_test_sequence();
        }
        
        // 检查按键
        if (read_button())
        {
            // 按键按下：蜂鸣器响一声，LED亮
            buzzer_control(TRUE);
            led_control(TRUE, TRUE);
            simple_delay_ms(200);
            buzzer_control(FALSE);
            led_control(FALSE, FALSE);
            
            // 等待按键释放
            while (read_button())
            {
                simple_delay_ms(10);
            }
        }
        
        // 心跳指示：系统LED慢闪
        if ((loop_counter % 500) == 0)
        {
            led_control(TRUE, FALSE);
            simple_delay_ms(50);
            led_control(FALSE, FALSE);
        }
        
        loop_counter++;
        simple_delay_ms(1);
        
        // 防止计数器溢出
        if (loop_counter >= 0xFFFFF000)
        {
            loop_counter = 0;
        }
    }
    
    return 0;
}
