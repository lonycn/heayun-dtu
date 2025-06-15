/**
 * ================================================================
 * 憨云 DTU - 超简化主程序 (测试版)
 * ================================================================
 * 文件: main_simple.c
 * 功能: 最简单的 Hello World，用于测试编译和烧录
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

// 基本类型定义 (避免使用 stdint.h)
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// NANO100B 基础寄存器定义
#define GPIOA_BASE 0x50004000UL
#define GPIOA_DOUT (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_MODE (*(volatile uint32_t *)(GPIOA_BASE + 0x00))

// 系统控制寄存器
#define CLK_BASE 0x50000200UL
#define CLK_AHBCLK (*(volatile uint32_t *)(CLK_BASE + 0x04))

// LED 控制 (PC8 连接 LED，根据硬件工程师确认)
#define LED_PIN 8
#define LED_ON() (GPIOC_DOUT |= (1U << LED_PIN))
#define LED_OFF() (GPIOC_DOUT &= ~(1U << LED_PIN))

/**
 * 简单延时函数
 */
static void delay_ms(uint32_t ms)
{
    // 简单的延时循环 (32MHz 系统时钟)
    volatile uint32_t count = ms * 8000;
    while (count--)
    {
        // 空循环
    }
}

/**
 * GPIO 初始化
 */
static void gpio_init(void)
{
    // 使能 GPIOC 时钟
    CLK_AHBCLK |= (1U << 4); // GPIOC 时钟使能

    // 配置 PC8 为输出模式 (系统状态LED)
    GPIOC_MODE &= ~(3U << (LED_PIN * 2));
    GPIOC_MODE |= (1U << (LED_PIN * 2));

    // 初始状态 LED 关闭
    LED_OFF();
}

/**
 * 主程序
 */
int main(void)
{
    // GPIO 初始化
    gpio_init();

    // 主循环 - LED 闪烁
    while (1)
    {
        LED_ON();      // 点亮 LED
        delay_ms(500); // 延时 500ms

        LED_OFF();     // 熄灭 LED
        delay_ms(500); // 延时 500ms
    }

    return 0;
}