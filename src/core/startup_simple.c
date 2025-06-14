/**
 * ================================================================
 * 憨云 DTU - NANO100B 启动文件
 * ================================================================
 * 文件: startup.c
 * 功能: 系统启动和中断向量表
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

// 基本类型定义 (避免使用 stdint.h)
typedef unsigned int uint32_t;

// 外部符号声明
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

// 主程序入口
extern int main(void);

// 默认中断处理函数
void Default_Handler(void) __attribute__((weak));
void Reset_Handler(void);

// 中断处理函数声明
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

// NANO100B 外设中断
void BOD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void WDT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EINT0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EINT1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GPIOP0P1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GPIOP2P3P4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWMA_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWMB_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TMR3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2C1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

// 中断向量表
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_estack,          // 0: 初始栈指针
    (uint32_t)Reset_Handler,     // 1: 复位处理函数
    (uint32_t)NMI_Handler,       // 2: NMI
    (uint32_t)HardFault_Handler, // 3: 硬件错误
    0,                           // 4: 保留
    0,                           // 5: 保留
    0,                           // 6: 保留
    0,                           // 7: 保留
    0,                           // 8: 保留
    0,                           // 9: 保留
    0,                           // 10: 保留
    (uint32_t)SVC_Handler,       // 11: SVCall
    0,                           // 12: 保留
    0,                           // 13: 保留
    (uint32_t)PendSV_Handler,    // 14: PendSV
    (uint32_t)SysTick_Handler,   // 15: SysTick

    // 外设中断
    (uint32_t)BOD_IRQHandler,        // 16: BOD
    (uint32_t)WDT_IRQHandler,        // 17: WDT
    (uint32_t)EINT0_IRQHandler,      // 18: EINT0
    (uint32_t)EINT1_IRQHandler,      // 19: EINT1
    (uint32_t)GPIOP0P1_IRQHandler,   // 20: GPIO P0/P1
    (uint32_t)GPIOP2P3P4_IRQHandler, // 21: GPIO P2/P3/P4
    (uint32_t)PWMA_IRQHandler,       // 22: PWMA
    (uint32_t)PWMB_IRQHandler,       // 23: PWMB
    (uint32_t)TMR0_IRQHandler,       // 24: Timer 0
    (uint32_t)TMR1_IRQHandler,       // 25: Timer 1
    (uint32_t)TMR2_IRQHandler,       // 26: Timer 2
    (uint32_t)TMR3_IRQHandler,       // 27: Timer 3
    (uint32_t)UART0_IRQHandler,      // 28: UART0
    (uint32_t)UART1_IRQHandler,      // 29: UART1
    (uint32_t)SPI0_IRQHandler,       // 30: SPI0
    (uint32_t)SPI1_IRQHandler,       // 31: SPI1
    0,                               // 32: 保留
    0,                               // 33: 保留
    (uint32_t)I2C0_IRQHandler,       // 34: I2C0
    (uint32_t)I2C1_IRQHandler,       // 35: I2C1
    0,                               // 36: 保留
    0,                               // 37: 保留
    0,                               // 38: 保留
    (uint32_t)ADC_IRQHandler,        // 39: ADC
};

/**
 * 复位处理函数
 */
void Reset_Handler(void)
{
    uint32_t *src, *dst;

    // 复制初始化数据从 Flash 到 RAM
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    // 清零 BSS 段
    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0;
    }

    // 调用主程序
    main();

    // 如果 main 返回，进入无限循环
    while (1)
    {
        __asm("nop");
    }
}

/**
 * 默认中断处理函数
 */
void Default_Handler(void)
{
    // 默认中断处理 - 无限循环
    while (1)
    {
        __asm("nop");
    }
}