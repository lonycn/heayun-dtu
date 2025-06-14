/**
 * ================================================================
 * 憨云 DTU - 启动文件
 * ================================================================
 * 文件: startup.c
 * 功能: 系统启动代码，中断向量表，复位处理
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"

// ================================================================
// 简单的库函数实现 (避免依赖标准库)
// ================================================================

/**
 * @brief 简单的内存复制函数
 * @param dest 目标地址
 * @param src 源地址
 * @param n 字节数
 * @return 目标地址
 */
static void *simple_memcpy(void *dest, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    while (n--)
    {
        *d++ = *s++;
    }

    return dest;
}

/**
 * @brief 简单的内存设置函数
 * @param s 目标地址
 * @param c 设置的值
 * @param n 字节数
 * @return 目标地址
 */
static void *simple_memset(void *s, int c, uint32_t n)
{
    uint8_t *p = (uint8_t *)s;

    while (n--)
    {
        *p++ = (uint8_t)c;
    }

    return s;
}

// ================================================================
// 外部符号声明 (来自链接脚本)
// ================================================================

extern uint32_t __StackTop;     // 栈顶地址
extern uint32_t __data_start__; // 数据段起始地址
extern uint32_t __data_end__;   // 数据段结束地址
extern uint32_t __etext;        // 数据段加载地址
extern uint32_t __bss_start__;  // BSS段起始地址
extern uint32_t __bss_end__;    // BSS段结束地址

// ================================================================
// 外部函数声明
// ================================================================

extern int main(void); // 主程序入口

// ================================================================
// 中断服务程序声明
// ================================================================

void Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

// NANO100B 特定中断
void BOD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void WDT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EINT0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EINT1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GPABC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GPDEF_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void HIRC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SC2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SC0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SC1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USBD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void LCD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PDMA_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2S_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PDWU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DAC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

// ================================================================
// 中断向量表
// ================================================================

__attribute__((section(".vectors")))
const uint32_t vector_table[] = {
    // ARM Cortex-M0 核心中断
    (uint32_t)&__StackTop,       // 0: 初始栈指针
    (uint32_t)Reset_Handler,     // 1: 复位处理程序
    (uint32_t)NMI_Handler,       // 2: NMI 处理程序
    (uint32_t)HardFault_Handler, // 3: 硬件错误处理程序
    0,                           // 4: 保留
    0,                           // 5: 保留
    0,                           // 6: 保留
    0,                           // 7: 保留
    0,                           // 8: 保留
    0,                           // 9: 保留
    0,                           // 10: 保留
    (uint32_t)SVC_Handler,       // 11: SVC 处理程序
    0,                           // 12: 保留
    0,                           // 13: 保留
    (uint32_t)PendSV_Handler,    // 14: PendSV 处理程序
    (uint32_t)SysTick_Handler,   // 15: SysTick 处理程序

    // NANO100B 外设中断 (与厂家SDK一致)
    (uint32_t)BOD_IRQHandler,   // 16: BOD
    (uint32_t)WDT_IRQHandler,   // 17: WDT
    (uint32_t)EINT0_IRQHandler, // 18: EINT0
    (uint32_t)EINT1_IRQHandler, // 19: EINT1
    (uint32_t)GPABC_IRQHandler, // 20: GPABC
    (uint32_t)GPDEF_IRQHandler, // 21: GPDEF
    (uint32_t)PWM0_IRQHandler,  // 22: PWM0
    (uint32_t)PWM1_IRQHandler,  // 23: PWM1
    (uint32_t)TMR0_IRQHandler,  // 24: TMR0
    (uint32_t)TMR1_IRQHandler,  // 25: TMR1
    (uint32_t)TMR2_IRQHandler,  // 26: TMR2
    (uint32_t)TMR3_IRQHandler,  // 27: TMR3
    (uint32_t)UART0_IRQHandler, // 28: UART0
    (uint32_t)UART1_IRQHandler, // 29: UART1
    (uint32_t)SPI0_IRQHandler,  // 30: SPI0
    (uint32_t)SPI1_IRQHandler,  // 31: SPI1
    (uint32_t)SPI2_IRQHandler,  // 32: SPI2
    (uint32_t)HIRC_IRQHandler,  // 33: HIRC
    (uint32_t)I2C0_IRQHandler,  // 34: I2C0
    (uint32_t)I2C1_IRQHandler,  // 35: I2C1
    (uint32_t)SC2_IRQHandler,   // 36: SC2
    (uint32_t)SC0_IRQHandler,   // 37: SC0
    (uint32_t)SC1_IRQHandler,   // 38: SC1
    (uint32_t)USBD_IRQHandler,  // 39: USBD
    0,                          // 40: 保留
    (uint32_t)LCD_IRQHandler,   // 41: LCD
    (uint32_t)PDMA_IRQHandler,  // 42: PDMA
    (uint32_t)I2S_IRQHandler,   // 43: I2S
    (uint32_t)PDWU_IRQHandler,  // 44: PDWU
    (uint32_t)ADC_IRQHandler,   // 45: ADC
    (uint32_t)DAC_IRQHandler,   // 46: DAC
    (uint32_t)RTC_IRQHandler,   // 47: RTC
};

// ================================================================
// 复位处理程序
// ================================================================

void Reset_Handler(void) __attribute__((optimize("O0")));

void Reset_Handler(void)
{
    // 1. 初始化数据段 (.data) - 手动循环复制，使用volatile防止优化
    volatile uint32_t *src = (volatile uint32_t *)&__etext;
    volatile uint32_t *dst = (volatile uint32_t *)&__data_start__;
    volatile uint32_t *end = (volatile uint32_t *)&__data_end__;

    while (dst < end)
    {
        *dst = *src;
        dst++;
        src++;
    }

    // 2. 清零 BSS 段 (.bss) - 手动循环清零，使用volatile防止优化
    dst = (volatile uint32_t *)&__bss_start__;
    end = (volatile uint32_t *)&__bss_end__;

    while (dst < end)
    {
        *dst = 0;
        dst++;
    }

    // 3. 调用主程序
    main();

    // 4. 如果 main 函数返回，进入无限循环
    while (1)
    {
        // 空循环
    }
}

// ================================================================
// 默认中断处理程序
// ================================================================

void Default_Handler(void)
{
    // 默认中断处理：进入无限循环
    // 在实际应用中，可以在这里添加错误处理代码
    while (1)
    {
        // 可以在这里添加 LED 闪烁或其他错误指示
        // 当前保持简单的无限循环
    }
}

// ================================================================
// 系统异常处理程序
// ================================================================

/**
 * @brief 硬件错误处理程序
 * @note 当发生严重的系统错误时调用
 */
void HardFault_Handler(void)
{
    // 硬件错误处理
    // 在实际应用中，可以在这里保存错误信息到 Flash

    // 简单的错误指示：如果可能的话，闪烁 LED
    // 注意：在硬件错误状态下，GPIO 可能无法正常工作

    // 进入无限循环，等待看门狗复位或外部复位
    while (1)
    {
        // 可以尝试简单的延时和 LED 控制
        volatile int delay = 100000;
        while (delay--)
            ;

        // 尝试控制 LED (如果 GPIO 还能工作的话)
        // 这里使用直接寄存器操作，避免函数调用
        REG32(GPIOA_BASE + GPIO_DOUT_OFFSET) ^= BIT(0);
    }
}

/**
 * @brief NMI 处理程序
 * @note 不可屏蔽中断处理
 */
void NMI_Handler(void)
{
    // NMI 处理
    // 通常用于处理关键的系统事件，如电源故障

    // 简单实现：调用默认处理程序
    Default_Handler();
}

// ================================================================
// 系统服务调用
// ================================================================

/**
 * @brief SVC 处理程序
 * @note 系统服务调用处理
 */
void SVC_Handler(void)
{
    // SVC 处理
    // 在简单的嵌入式系统中通常不使用
    Default_Handler();
}

/**
 * @brief PendSV 处理程序
 * @note 可挂起的系统服务调用
 */
void PendSV_Handler(void)
{
    // PendSV 处理
    // 通常用于任务切换，在简单系统中不使用
    Default_Handler();
}

// ================================================================
// 系统滴答定时器 (可选实现)
// ================================================================

/**
 * @brief SysTick 处理程序
 * @note 系统滴答定时器中断
 */
void SysTick_Handler(void)
{
    // SysTick 处理
    // 可以在这里更新系统时间
    // 当前使用软件延时，所以这个中断可能不会被使用

    // 如果需要精确的时间基准，可以在这里调用系统时间更新函数
    // system_tick_increment();
}

// ================================================================
// 启动代码辅助函数
// ================================================================

/**
 * @brief 获取复位原因
 * @return 复位原因代码
 */
uint32_t get_reset_reason(void)
{
    // 简化实现：返回固定值
    // 在实际应用中，应该读取系统控制寄存器获取真实的复位原因
    return 0x01; // 上电复位
}

/**
 * @brief 系统早期初始化
 * @note 在 main 函数之前调用的初始化代码
 */
void early_init(void) __attribute__((constructor));

void early_init(void)
{
    // 早期初始化代码
    // 这个函数会在 main 函数之前自动调用

    // 当前保持空实现
    // 如果需要在 main 之前进行一些关键初始化，可以在这里添加
}