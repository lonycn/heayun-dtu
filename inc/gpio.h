/**
 * @file gpio.h
 * @brief GPIO驱动头文件 - NANO100B GPIO控制接口
 * @version 1.0.0
 * @date 2025-03-28
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// GPIO类型定义
// ============================================================================

/**
 * @brief GPIO端口枚举
 */
typedef enum
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_D = 3,
    GPIO_PORT_F = 4
} gpio_port_t;

/**
 * @brief GPIO模式枚举
 */
typedef enum
{
    GPIO_MODE_INPUT = 0,         // 输入模式
    GPIO_MODE_OUTPUT = 1,        // 输出模式
    GPIO_MODE_OPEN_DRAIN = 2,    // 开漏输出
    GPIO_MODE_INPUT_PULLUP = 3,  // 输入上拉
    GPIO_MODE_INPUT_PULLDOWN = 4 // 输入下拉
} gpio_mode_t;

/**
 * @brief GPIO中断触发类型
 */
typedef enum
{
    GPIO_INT_RISING = 0,  // 上升沿触发
    GPIO_INT_FALLING = 1, // 下降沿触发
    GPIO_INT_BOTH = 2,    // 双边沿触发
    GPIO_INT_HIGH = 3,    // 高电平触发
    GPIO_INT_LOW = 4      // 低电平触发
} gpio_int_type_t;

/**
 * @brief GPIO中断回调函数类型
 */
typedef void (*gpio_int_callback_t)(gpio_port_t port, uint8_t pin);

/**
 * @brief GPIO配置结构体
 */
typedef struct
{
    gpio_port_t port;             // GPIO端口
    uint8_t pin;                  // GPIO引脚号
    gpio_mode_t mode;             // GPIO模式
    bool initial_state;           // 初始状态（仅输出模式有效）
    gpio_int_type_t int_type;     // 中断类型（仅需要中断时配置）
    gpio_int_callback_t callback; // 中断回调函数
} gpio_config_t;

// ============================================================================
// GPIO基础接口
// ============================================================================

/**
 * @brief GPIO驱动初始化
 * @return true: 成功, false: 失败
 */
bool gpio_init(void);

/**
 * @brief 配置GPIO引脚
 * @param config GPIO配置结构体指针
 * @return true: 成功, false: 失败
 */
bool gpio_config_pin(const gpio_config_t *config);

/**
 * @brief 设置GPIO输出电平
 * @param port GPIO端口
 * @param pin 引脚号
 * @param state 电平状态 (true: 高, false: 低)
 * @return true: 成功, false: 失败
 */
bool gpio_write_pin(gpio_port_t port, uint8_t pin, bool state);

/**
 * @brief 读取GPIO输入电平
 * @param port GPIO端口
 * @param pin 引脚号
 * @return GPIO电平状态 (true: 高, false: 低)
 */
bool gpio_read_pin(gpio_port_t port, uint8_t pin);

/**
 * @brief 翻转GPIO输出电平
 * @param port GPIO端口
 * @param pin 引脚号
 * @return true: 成功, false: 失败
 */
bool gpio_toggle_pin(gpio_port_t port, uint8_t pin);

// ============================================================================
// LED控制接口（调试用）
// ============================================================================

/**
 * @brief 设置LED状态
 * @param state LED状态 (true: 亮, false: 灭)
 */
void gpio_led_set(bool state);

/**
 * @brief LED闪烁
 * @param times 闪烁次数
 * @param interval_ms 闪烁间隔(毫秒)
 */
void gpio_led_blink(uint8_t times, uint16_t interval_ms);

/**
 * @brief 切换LED状态
 */
void gpio_led_toggle(void);

// ============================================================================
// 中断控制接口
// ============================================================================

/**
 * @brief 使能GPIO中断
 * @param port GPIO端口
 * @param pin 引脚号
 * @param type 中断触发类型
 * @param callback 中断回调函数
 * @return true: 成功, false: 失败
 */
bool gpio_enable_interrupt(gpio_port_t port, uint8_t pin,
                           gpio_int_type_t type, gpio_int_callback_t callback);

/**
 * @brief 禁用GPIO中断
 * @param port GPIO端口
 * @param pin 引脚号
 * @return true: 成功, false: 失败
 */
bool gpio_disable_interrupt(gpio_port_t port, uint8_t pin);

// ============================================================================
// 高级功能接口
// ============================================================================

/**
 * @brief 批量设置GPIO端口数据
 * @param port GPIO端口
 * @param mask 引脚掩码
 * @param data 数据值
 * @return true: 成功, false: 失败
 */
bool gpio_write_port(gpio_port_t port, uint16_t mask, uint16_t data);

/**
 * @brief 批量读取GPIO端口数据
 * @param port GPIO端口
 * @return 端口数据值
 */
uint16_t gpio_read_port(gpio_port_t port);

#endif // GPIO_H