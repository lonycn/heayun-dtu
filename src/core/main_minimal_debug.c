/**
 * ================================================================
 * 憨云 DTU - 最简化调试测试程序
 * ================================================================
 * 文件: main_minimal_debug.c
 * 功能: 最简化的硬件测试，逐步调试问题
 * 硬件: PC8(LED), PA6(蜂鸣器), PC14(SCL), PA12(SDA)
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"

// ================================================================
// 最简化的寄存器操作
// ================================================================

// 直接使用寄存器地址，避免复杂的宏定义
#define GPIOC_PMD   0x50004080UL    // PC端口模式寄存器
#define GPIOC_DOUT  0x50004088UL    // PC端口数据输出寄存器
#define GPIOA_PMD   0x50004000UL    // PA端口模式寄存器
#define GPIOA_DOUT  0x50004008UL    // PA端口数据输出寄存器

#define CLK_PWRCTL  0x50000200UL    // 电源控制寄存器
#define CLK_CLKSTATUS 0x50000250UL  // 时钟状态寄存器
#define CLK_AHBCLK  0x50000204UL    // AHB时钟控制寄存器

#define SYS_REGLCTL 0x50000100UL    // 寄存器锁定控制

// ================================================================
// 最简化的函数实现
// ================================================================

/**
 * @brief 最简单的延时函数
 * @param count 延时计数
 */
static void simple_delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm volatile("nop");
    }
}

/**
 * @brief 写32位寄存器
 * @param addr 寄存器地址
 * @param value 写入值
 */
static void write_reg32(uint32_t addr, uint32_t value)
{
    *((volatile uint32_t*)addr) = value;
}

/**
 * @brief 读32位寄存器
 * @param addr 寄存器地址
 * @return 寄存器值
 */
static uint32_t read_reg32(uint32_t addr)
{
    return *((volatile uint32_t*)addr);
}

/**
 * @brief 设置寄存器位
 * @param addr 寄存器地址
 * @param bits 要设置的位
 */
static void set_reg_bits(uint32_t addr, uint32_t bits)
{
    uint32_t value = read_reg32(addr);
    value |= bits;
    write_reg32(addr, value);
}

/**
 * @brief 清除寄存器位
 * @param addr 寄存器地址
 * @param bits 要清除的位
 */
static void clear_reg_bits(uint32_t addr, uint32_t bits)
{
    uint32_t value = read_reg32(addr);
    value &= ~bits;
    write_reg32(addr, value);
}

/**
 * @brief 系统寄存器解锁
 */
static void unlock_regs(void)
{
    write_reg32(SYS_REGLCTL, 0x59);
    write_reg32(SYS_REGLCTL, 0x16);
    write_reg32(SYS_REGLCTL, 0x88);
}

/**
 * @brief 系统寄存器锁定
 */
static void lock_regs(void)
{
    write_reg32(SYS_REGLCTL, 0x00);
}

/**
 * @brief 最简化的时钟初始化
 */
static void minimal_clock_init(void)
{
    unlock_regs();
    
    // 使能内部高速RC振荡器 (HIRC 12MHz)
    set_reg_bits(CLK_PWRCTL, (1UL << 2));
    
    // 等待HIRC稳定
    while (!(read_reg32(CLK_CLKSTATUS) & (1UL << 2)))
    {
        simple_delay(100);
    }
    
    // 使能GPIO时钟
    set_reg_bits(CLK_AHBCLK, (1UL << 2));
    
    lock_regs();
    
    // 延时确保时钟稳定
    simple_delay(100000);
}

/**
 * @brief 最简化的LED初始化
 */
static void minimal_led_init(void)
{
    // 配置PC8为输出模式
    // PC8 = bit 8, PMD8的bit[17:16] = 01 (推挽输出)
    uint32_t pmd_value = read_reg32(GPIOC_PMD);
    pmd_value &= ~(0x3UL << 16);  // 清除bit[17:16]
    pmd_value |= (0x1UL << 16);   // 设置为01 (推挽输出)
    write_reg32(GPIOC_PMD, pmd_value);
    
    // 初始状态：LED关闭
    clear_reg_bits(GPIOC_DOUT, (1UL << 8));
}

/**
 * @brief LED开启
 */
static void led_on(void)
{
    set_reg_bits(GPIOC_DOUT, (1UL << 8));
}

/**
 * @brief LED关闭
 */
static void led_off(void)
{
    clear_reg_bits(GPIOC_DOUT, (1UL << 8));
}

/**
 * @brief LED闪烁测试
 * @param times 闪烁次数
 * @param delay_count 延时计数
 */
static void led_blink_test(uint8_t times, uint32_t delay_count)
{
    for (uint8_t i = 0; i < times; i++)
    {
        led_on();
        simple_delay(delay_count);
        led_off();
        simple_delay(delay_count);
    }
}

/**
 * @brief 最简化的PA6初始化（用于蜂鸣器测试）
 */
static void minimal_buzzer_gpio_init(void)
{
    // 配置PA6为输出模式
    // PA6 = bit 6, PMD6的bit[13:12] = 01 (推挽输出)
    uint32_t pmd_value = read_reg32(GPIOA_PMD);
    pmd_value &= ~(0x3UL << 12);  // 清除bit[13:12]
    pmd_value |= (0x1UL << 12);   // 设置为01 (推挽输出)
    write_reg32(GPIOA_PMD, pmd_value);
    
    // 初始状态：输出低电平
    clear_reg_bits(GPIOA_DOUT, (1UL << 6));
}

/**
 * @brief 简单的蜂鸣器测试（GPIO方波）
 * @param times 响声次数
 * @param freq_delay 频率延时
 * @param duration 持续时间
 */
static void simple_buzzer_test(uint8_t times, uint32_t freq_delay, uint32_t duration)
{
    for (uint8_t i = 0; i < times; i++)
    {
        // 产生方波
        for (uint32_t j = 0; j < duration; j++)
        {
            set_reg_bits(GPIOA_DOUT, (1UL << 6));    // PA6高电平
            simple_delay(freq_delay);
            clear_reg_bits(GPIOA_DOUT, (1UL << 6));  // PA6低电平
            simple_delay(freq_delay);
        }
        
        // 间隔
        simple_delay(500000);
    }
}

/**
 * @brief 系统状态检查
 */
static void system_status_check(void)
{
    // 检查时钟状态
    uint32_t clk_status = read_reg32(CLK_CLKSTATUS);
    uint32_t ahb_clk = read_reg32(CLK_AHBCLK);
    
    // 通过LED闪烁模式指示状态
    if (clk_status & (1UL << 2))  // HIRC稳定
    {
        led_blink_test(2, 100000);  // 快闪2次表示时钟正常
    }
    else
    {
        led_blink_test(5, 50000);   // 快闪5次表示时钟异常
    }
    
    simple_delay(1000000);
    
    if (ahb_clk & (1UL << 2))     // GPIO时钟使能
    {
        led_blink_test(3, 200000);  // 慢闪3次表示GPIO时钟正常
    }
    else
    {
        led_blink_test(6, 50000);   // 快闪6次表示GPIO时钟异常
    }
}

/**
 * @brief 主程序入口
 */
int main(void)
{
    // 1. 最简化的系统初始化
    minimal_clock_init();
    
    // 2. LED初始化
    minimal_led_init();
    
    // 3. 蜂鸣器GPIO初始化
    minimal_buzzer_gpio_init();
    
    // 4. 启动指示：LED快闪10次
    led_blink_test(10, 200000);
    
    // 5. 延时
    simple_delay(2000000);
    
    // 6. 系统状态检查
    system_status_check();
    
    // 7. 主循环
    uint32_t loop_count = 0;
    
    while (1)
    {
        // 使用位运算代替除法，避免链接错误
        // 每1024次循环执行一次测试序列 (2^10 = 1024)
        if ((loop_count & 0x3FF) == 0)
        {
            // LED测试
            led_blink_test(3, 300000);

            simple_delay(1000000);

            // 简单蜂鸣器测试
            simple_buzzer_test(2, 100, 1000);
        }

        // 心跳指示：LED慢闪 (每128次循环，2^7 = 128)
        if ((loop_count & 0x7F) == 0)
        {
            led_on();
            simple_delay(50000);
            led_off();
        }

        loop_count++;
        simple_delay(10000);

        // 防止溢出
        if (loop_count >= 0xFFFFF000)
        {
            loop_count = 0;
        }
    }
    
    return 0;
}
