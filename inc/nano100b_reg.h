/**
 * ================================================================
 * 憨云 DTU - NANO100B 寄存器定义
 * ================================================================
 * 文件: nano100b_reg.h
 * 功能: NANO100B 微控制器寄存器地址和位定义
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#ifndef __NANO100B_REG_H__
#define __NANO100B_REG_H__

#include "nano100b_types.h"

// ================================================================
// 基础地址定义
// ================================================================

// 外设基地址
#define PERIPH_BASE 0x40000000UL
#define AHB_BASE (PERIPH_BASE + 0x00020000UL)
#define APB1_BASE (PERIPH_BASE + 0x00000000UL)
#define APB2_BASE (PERIPH_BASE + 0x00010000UL)

// GPIO 基地址
#define GPIOA_BASE (AHB_BASE + 0x4000UL)
#define GPIOB_BASE (AHB_BASE + 0x4040UL)
#define GPIOC_BASE (AHB_BASE + 0x4080UL)
#define GPIOD_BASE (AHB_BASE + 0x40C0UL)
#define GPIOE_BASE (AHB_BASE + 0x4100UL)
#define GPIOF_BASE (AHB_BASE + 0x4140UL)

// 系统控制基地址
#define SYS_BASE (AHB_BASE + 0x0000UL)
#define CLK_BASE (AHB_BASE + 0x0200UL)

// PWM 基地址
#define PWM0_BASE (APB1_BASE + 0x40000UL)
#define PWM1_BASE (APB1_BASE + 0x41000UL)

// ================================================================
// GPIO 寄存器偏移定义
// ================================================================

#define GPIO_PMD_OFFSET 0x00   // 引脚模式控制寄存器
#define GPIO_OFFD_OFFSET 0x04  // 数字输入路径禁用寄存器
#define GPIO_DOUT_OFFSET 0x08  // 数据输出寄存器
#define GPIO_DMASK_OFFSET 0x0C // 数据输出写入掩码寄存器
#define GPIO_PIN_OFFSET 0x10   // 引脚值寄存器
#define GPIO_DBEN_OFFSET 0x14  // 去抖动使能寄存器
#define GPIO_IMD_OFFSET 0x18   // 中断模式控制寄存器
#define GPIO_IEN_OFFSET 0x1C   // 中断使能寄存器
#define GPIO_ISRC_OFFSET 0x20  // 中断源标志寄存器

// ================================================================
// PWM 寄存器偏移定义
// ================================================================

#define PWM_PPR_OFFSET 0x00    // 预分频寄存器
#define PWM_CSR_OFFSET 0x04    // 时钟选择寄存器
#define PWM_PCR_OFFSET 0x08    // 控制寄存器
#define PWM_CNR0_OFFSET 0x0C   // 计数器寄存器0
#define PWM_CMR0_OFFSET 0x10   // 比较寄存器0
#define PWM_PDR0_OFFSET 0x14   // 数据寄存器0
#define PWM_CNR1_OFFSET 0x18   // 计数器寄存器1
#define PWM_CMR1_OFFSET 0x1C   // 比较寄存器1
#define PWM_PDR1_OFFSET 0x20   // 数据寄存器1
#define PWM_CNR2_OFFSET 0x24   // 计数器寄存器2
#define PWM_CMR2_OFFSET 0x28   // 比较寄存器2
#define PWM_PDR2_OFFSET 0x2C   // 数据寄存器2
#define PWM_CNR3_OFFSET 0x30   // 计数器寄存器3
#define PWM_CMR3_OFFSET 0x34   // 比较寄存器3
#define PWM_PDR3_OFFSET 0x38   // 数据寄存器3
#define PWM_PIER_OFFSET 0x40   // 中断使能寄存器
#define PWM_PIIR_OFFSET 0x44   // 中断指示寄存器
#define PWM_CCR0_OFFSET 0x50   // 捕获控制寄存器0
#define PWM_CCR2_OFFSET 0x54   // 捕获控制寄存器2
#define PWM_CRLR0_OFFSET 0x58  // 捕获上升沿锁存寄存器0
#define PWM_CFLR0_OFFSET 0x5C  // 捕获下降沿锁存寄存器0
#define PWM_CRLR1_OFFSET 0x60  // 捕获上升沿锁存寄存器1
#define PWM_CFLR1_OFFSET 0x64  // 捕获下降沿锁存寄存器1
#define PWM_CRLR2_OFFSET 0x68  // 捕获上升沿锁存寄存器2
#define PWM_CFLR2_OFFSET 0x6C  // 捕获下降沿锁存寄存器2
#define PWM_CRLR3_OFFSET 0x70  // 捕获上升沿锁存寄存器3
#define PWM_CFLR3_OFFSET 0x74  // 捕获下降沿锁存寄存器3
#define PWM_CAPENR_OFFSET 0x78 // 捕获输入使能寄存器
#define PWM_POE_OFFSET 0x7C    // 输出使能寄存器

// ================================================================
// 项目特定的GPIO引脚定义 (根据硬件接口说明文档)
// ================================================================

// 系统状态指示灯 - PA0
#define SYSTEM_LED_PORT GPIOA_BASE
#define SYSTEM_LED_PIN 0
#define SYSTEM_LED_BIT BIT(0)

// OLED显示 - 软件I2C
#define OLED_SCL_PORT GPIOC_BASE
#define OLED_SCL_PIN 14
#define OLED_SCL_BIT BIT(14)

#define OLED_SDA_PORT GPIOA_BASE
#define OLED_SDA_PIN 12
#define OLED_SDA_BIT BIT(12)

// 蜂鸣器 - PB6 (PWM0)
#define BUZZER_PORT GPIOB_BASE
#define BUZZER_PIN 6
#define BUZZER_BIT BIT(6)

// 调试LED - PA1 (可选)
#define DEBUG_LED_PORT GPIOA_BASE
#define DEBUG_LED_PIN 1
#define DEBUG_LED_BIT BIT(1)

// 用户按键 - PA2 (可选)
#define USER_BUTTON_PORT GPIOA_BASE
#define USER_BUTTON_PIN 2
#define USER_BUTTON_BIT BIT(2)

// ================================================================
// GPIO 模式定义
// ================================================================

// GPIO 模式控制位
#define GPIO_PMD_INPUT 0x0UL      // 输入模式
#define GPIO_PMD_OUTPUT 0x1UL     // 推挽输出模式
#define GPIO_PMD_OPEN_DRAIN 0x2UL // 开漏输出模式
#define GPIO_PMD_QUASI 0x3UL      // 准双向模式

// ================================================================
// PWM 控制位定义
// ================================================================

// PWM 控制寄存器位
#define PWM_PCR_CH0EN BIT(0)  // PWM通道0使能
#define PWM_PCR_CH0INV BIT(2) // PWM通道0反相
#define PWM_PCR_CH0MOD BIT(3) // PWM通道0模式
#define PWM_PCR_DZEN01 BIT(4) // 死区使能01
#define PWM_PCR_DZEN23 BIT(5) // 死区使能23
#define PWM_PCR_DZEN45 BIT(6) // 死区使能45

// PWM 时钟选择
#define PWM_CSR_CSR0_DIV2 0x0UL  // 时钟/2
#define PWM_CSR_CSR0_DIV4 0x1UL  // 时钟/4
#define PWM_CSR_CSR0_DIV8 0x2UL  // 时钟/8
#define PWM_CSR_CSR0_DIV16 0x3UL // 时钟/16

// ================================================================
// 常用操作宏
// ================================================================

// GPIO 操作宏
#define GPIO_SET_MODE(port, pin, mode)                   \
    do                                                   \
    {                                                    \
        uint32_t temp = REG32((port) + GPIO_PMD_OFFSET); \
        temp &= ~(0x3UL << ((pin) * 2));                 \
        temp |= ((mode) << ((pin) * 2));                 \
        REG32((port) + GPIO_PMD_OFFSET) = temp;          \
    } while (0)

#define GPIO_SET_PIN(port, bit) SET_BIT(REG32((port) + GPIO_DOUT_OFFSET), (bit))
#define GPIO_CLEAR_PIN(port, bit) CLEAR_BIT(REG32((port) + GPIO_DOUT_OFFSET), (bit))
#define GPIO_TOGGLE_PIN(port, bit) TOGGLE_BIT(REG32((port) + GPIO_DOUT_OFFSET), (bit))
#define GPIO_READ_PIN(port, bit) READ_BIT(REG32((port) + GPIO_PIN_OFFSET), (bit))

// PWM 操作宏
#define PWM_ENABLE_CH0() SET_BIT(REG32(PWM0_BASE + PWM_PCR_OFFSET), PWM_PCR_CH0EN)
#define PWM_DISABLE_CH0() CLEAR_BIT(REG32(PWM0_BASE + PWM_PCR_OFFSET), PWM_PCR_CH0EN)

#endif /* __NANO100B_REG_H__ */