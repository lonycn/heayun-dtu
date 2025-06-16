/**
 * ================================================================
 * 憨云 DTU - 修复版硬件测试程序
 * ================================================================
 * 文件: main_fixed_hardware_test.c
 * 功能: 基于旧项目配置的正确硬件测试程序
 * 硬件: PC8(LED), PA6(蜂鸣器PWM), PA13(SCL), PA12(SDA)
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"

// ================================================================
// 基于旧项目的正确配置
// ================================================================

/**
 * @brief 简单延时函数
 * @param ms 延时毫秒数
 */
static void simple_delay_ms(uint32_t ms)
{
    // 基于12MHz时钟的软件延时
    volatile uint32_t count = ms * 3000;
    while (count--)
    {
        __asm volatile("nop");
    }
}

/**
 * @brief 系统时钟初始化 (基于旧项目)
 */
static void fixed_clock_init(void)
{
    // 解锁系统寄存器
    REG32(0x50000100) = 0x59;
    REG32(0x50000100) = 0x16;
    REG32(0x50000100) = 0x88;
    
    // 1. 使能内部高速RC振荡器 (HIRC 12MHz)
    REG32(CLK_BASE + CLK_PWRCTL_OFFSET) |= (1UL << 2);
    
    // 2. 等待HIRC稳定
    while(!(REG32(CLK_BASE + CLK_CLKSTATUS_OFFSET) & (1UL << 2)));
    
    // 3. 切换系统时钟源到HIRC
    REG32(CLK_BASE + CLK_CLKSEL0_OFFSET) &= ~(0x7UL << 0);
    REG32(CLK_BASE + CLK_CLKSEL0_OFFSET) |= (0x0UL << 0);
    
    // 4. 使能必要的外设时钟
    REG32(CLK_BASE + CLK_AHBCLK_OFFSET) |= (1UL << 2);  // GPIO时钟
    
    // 5. 使能PWM0时钟 (蜂鸣器用)
    REG32(CLK_BASE + CLK_APBCLK_OFFSET) |= (1UL << 20); // PWM0时钟
    
    // 锁定系统寄存器
    REG32(0x50000100) = 0x00;
    
    // 延时确保时钟稳定
    simple_delay_ms(10);
}

/**
 * @brief LED初始化 (PC8)
 */
static void led_init(void)
{
    // 配置PC8为输出模式 (基于旧项目: GPIO_SetMode(PC, BIT8, GPIO_PMD_OUTPUT))
    // PC8 = bit 8, 需要设置PMD8的bit[17:16] = 01 (推挽输出)
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 16); // 清除bit[17:16]
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) |= (0x1UL << 16);  // 设置为01 (推挽输出)
    
    // 初始状态：LED关闭 (PC8=0)
    REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 8);
}

/**
 * @brief PWM初始化 (PA6蜂鸣器)
 */
static void pwm_init(void)
{
    // 解锁系统寄存器
    REG32(0x50000100) = 0x59;
    REG32(0x50000100) = 0x16;
    REG32(0x50000100) = 0x88;
    
    // 1. 使能PWM0_CH23模块时钟 (基于旧项目)
    REG32(CLK_BASE + CLK_APBCLK_OFFSET) |= (1UL << 20); // PWM0时钟
    
    // 2. 设置PWM0时钟源为HCLK
    REG32(CLK_BASE + CLK_CLKSEL1_OFFSET) &= ~(0x3UL << 28);
    REG32(CLK_BASE + CLK_CLKSEL1_OFFSET) |= (0x0UL << 28); // HCLK
    
    // 3. 配置PA6为PWM0_CH3功能 (基于旧项目)
    // SYS->PA_L_MFP = (SYS->PA_L_MFP &(~SYS_PA_L_MFP_PA6_MFP_Msk ))| SYS_PA_L_MFP_PA6_MFP_PWM0_CH3;
    REG32(0x50000030) &= ~(0xFUL << 24); // 清除PA6的MFP设置
    REG32(0x50000030) |= (0x3UL << 24);  // 设置PA6为PWM0_CH3
    
    // 4. 配置PWM0通道3 (基于旧项目: 2700Hz, 30%占空比)
    // PWM_ConfigOutputChannel(PWM0, 3, 2700, 30);
    uint32_t pwm_base = PWM0_BASE;
    
    // 设置预分频器 (假设12MHz时钟)
    REG32(pwm_base + 0x04) = 0; // PPR寄存器，预分频为1
    
    // 设置时钟分频器
    REG32(pwm_base + 0x08) = 0; // CSR寄存器，分频为1
    
    // 设置周期值 (12MHz / 2700Hz ≈ 4444)
    REG32(pwm_base + 0x18) = 4444; // CNR3寄存器
    
    // 设置占空比 (30% = 4444 * 0.3 ≈ 1333)
    REG32(pwm_base + 0x1C) = 1333; // CMR3寄存器
    
    // 5. 使能PWM输出
    REG32(pwm_base + 0x00) |= (1UL << 11); // PCR寄存器，使能CH3输出
    
    // 锁定系统寄存器
    REG32(0x50000100) = 0x00;
}

/**
 * @brief LED控制
 */
static void led_control(boolean_t enable)
{
    if (enable)
        REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) |= (1UL << 8);   // PC8=1
    else
        REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 8);  // PC8=0
}

/**
 * @brief 蜂鸣器控制
 */
static void buzzer_control(boolean_t enable)
{
    uint32_t pwm_base = PWM0_BASE;
    
    if (enable)
    {
        // 启动PWM0通道3 (基于旧项目: PWM_Start(PWM0, PWM_CH_3_MASK))
        REG32(pwm_base + 0x00) |= (1UL << 3); // PCR寄存器，启动CH3
    }
    else
    {
        // 停止PWM0通道3 (基于旧项目: PWM_Stop(PWM0, PWM_CH_3_MASK))
        REG32(pwm_base + 0x00) &= ~(1UL << 3); // PCR寄存器，停止CH3
    }
}

/**
 * @brief 硬件测试序列 (基于旧项目的BellBell函数)
 */
static void hardware_test_sequence(void)
{
    // 测试序列1: LED和蜂鸣器同时工作 (模拟BellBell(10,10,2))
    for (uint8_t i = 0; i < 2; i++)
    {
        led_control(TRUE);
        buzzer_control(TRUE);
        simple_delay_ms(100);  // 10 * 10ms = 100ms
        
        led_control(FALSE);
        buzzer_control(FALSE);
        simple_delay_ms(100);  // 10 * 10ms = 100ms
    }
    
    simple_delay_ms(500);
    
    // 测试序列2: 更长的测试 (模拟BellBell(20,20,2))
    for (uint8_t i = 0; i < 2; i++)
    {
        led_control(TRUE);
        buzzer_control(TRUE);
        simple_delay_ms(200);  // 20 * 10ms = 200ms
        
        led_control(FALSE);
        buzzer_control(FALSE);
        simple_delay_ms(200);  // 20 * 10ms = 200ms
    }
}

/**
 * @brief 主程序入口
 */
int main(void)
{
    // 1. 系统初始化
    fixed_clock_init();
    led_init();
    pwm_init();
    
    // 2. 启动指示：快速闪烁3次
    for (uint8_t i = 0; i < 3; i++)
    {
        led_control(TRUE);
        buzzer_control(TRUE);
        simple_delay_ms(100);
        led_control(FALSE);
        buzzer_control(FALSE);
        simple_delay_ms(100);
    }
    
    simple_delay_ms(1000);
    
    // 3. 主循环
    uint32_t loop_counter = 0;
    
    while (1)
    {
        // 每1024次循环执行一次完整测试序列
        if ((loop_counter & 0x3FF) == 0)
        {
            hardware_test_sequence();
        }
        
        // 心跳指示：LED慢闪 (每256次循环)
        if ((loop_counter & 0xFF) == 0)
        {
            led_control(TRUE);
            simple_delay_ms(50);
            led_control(FALSE);
        }
        
        loop_counter++;
        simple_delay_ms(10);
        
        // 防止计数器溢出
        if (loop_counter >= 0xFFFFF000)
        {
            loop_counter = 0;
        }
    }
    
    return 0;
}
