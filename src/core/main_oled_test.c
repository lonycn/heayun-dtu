/**
 * ================================================================
 * 憨云 DTU - OLED测试程序
 * ================================================================
 * 文件: main_oled_test.c
 * 功能: 专门用于测试OLED显示的程序
 * 硬件: PC14(SCL), PA12(SDA)
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"
#include "../drivers/oled_ssd1306.h"

// ================================================================
// 简化的系统函数
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
static void oled_test_clock_init(void)
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
    
    // 5. 延时确保时钟稳定
    simple_delay_ms(10);
}

/**
 * @brief LED初始化 (用于状态指示)
 */
static void led_init(void)
{
    // 配置PC8为输出模式 (系统状态LED)
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 16); // 清除bit[17:16]
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) |= (0x1UL << 16);  // 设置为01 (推挽输出)
    
    // 初始状态：LED关闭
    REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 8);
}

/**
 * @brief LED控制
 */
static void led_toggle(void)
{
    static boolean_t led_state = FALSE;
    
    if (led_state)
    {
        REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) &= ~(1UL << 8);  // LED关
        led_state = FALSE;
    }
    else
    {
        REG32(GPIOC_BASE + GPIO_DOUT_OFFSET) |= (1UL << 8);   // LED开
        led_state = TRUE;
    }
}

/**
 * @brief OLED测试序列
 */
static void oled_test_sequence(void)
{
    // 测试1: 清屏测试
    oled_clear();
    simple_delay_ms(1000);
    
    // 测试2: 全屏点亮测试
    oled_fill();
    simple_delay_ms(1000);
    
    // 测试3: 棋盘格测试
    oled_test_pattern();
    simple_delay_ms(2000);
    
    // 测试4: 清屏
    oled_clear();
    simple_delay_ms(500);
    
    // 测试5: 简单字符串显示测试
    oled_show_string(0, 0, "HANYUN DTU");
    oled_show_string(0, 2, "OLED TEST");
    oled_show_string(0, 4, "PC14-SCL");
    oled_show_string(0, 6, "PA12-SDA");
    simple_delay_ms(3000);
}

/**
 * @brief 连续显示测试
 */
static void oled_continuous_test(void)
{
    static uint32_t test_counter = 0;

    oled_clear();

    // 显示测试信息
    oled_show_string(0, 0, "OLED WORKING");
    oled_show_string(0, 2, "COUNT:");

    // 简单的计数显示 (用点阵模拟数字，避免除法运算)
    uint8_t x_pos = 48;

    oled_set_pos(x_pos, 2);
    for (uint8_t i = 0; i < 8; i++)
    {
        // 根据计数器的低位显示不同的模式
        if (test_counter & 0x01)
            oled_write_data(0x7E); // 竖线模式
        else
            oled_write_data(0x18); // 点模式
    }

    oled_show_string(0, 4, "PC14-SCL");
    oled_show_string(0, 6, "PA12-SDA");

    test_counter++;
    if (test_counter >= 1000)
    {
        test_counter = 0;
    }
}

/**
 * @brief 主程序入口
 */
int main(void)
{
    // 1. 系统初始化
    oled_test_clock_init();
    led_init();
    
    // 2. 启动指示：LED快速闪烁3次
    for (uint8_t i = 0; i < 6; i++)
    {
        led_toggle();
        simple_delay_ms(200);
    }
    
    // 3. OLED初始化
    oled_init();
    
    // 4. 初始化成功指示
    led_toggle(); // LED常亮表示初始化完成
    
    // 5. 执行完整的OLED测试序列
    oled_test_sequence();
    
    // 6. 主循环
    uint32_t loop_counter = 0;
    
    while (1)
    {
        // 使用位运算代替除法，避免链接错误
        // 每512次循环执行一次完整测试序列 (2^9 = 512)
        if ((loop_counter & 0x1FF) == 0)
        {
            oled_test_sequence();
        }
        // 其他时间执行连续测试 (每64次循环，2^6 = 64)
        else if ((loop_counter & 0x3F) == 0)
        {
            oled_continuous_test();
        }

        // 心跳指示：LED慢闪 (每128次循环，2^7 = 128)
        if ((loop_counter & 0x7F) == 0)
        {
            led_toggle();
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


