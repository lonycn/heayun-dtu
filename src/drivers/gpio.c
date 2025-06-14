/**
 * @file gpio.c
 * @brief 憨云DTU GPIO驱动实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * NANO100B GPIO驱动实现，专为8KB RAM优化
 */

#include "system.h"
#include "gpio.h"
#include <stddef.h>

// NANO100B GPIO寄存器基址定义
#define GPIO_BASE 0x50004000
#define GPIO_PORT_OFFSET 0x40

// 每个端口的寄存器偏移
#define GPIO_PMD_OFFSET 0x00   // 引脚模式控制
#define GPIO_OFFD_OFFSET 0x04  // 数字输入路径禁用
#define GPIO_DOUT_OFFSET 0x08  // 数据输出
#define GPIO_DMASK_OFFSET 0x0C // 数据输出掩码
#define GPIO_PIN_OFFSET 0x10   // 引脚值
#define GPIO_DBEN_OFFSET 0x14  // 去抖动使能
#define GPIO_IMD_OFFSET 0x18   // 中断模式控制
#define GPIO_IEN_OFFSET 0x1C   // 中断使能
#define GPIO_ISRC_OFFSET 0x20  // 中断源标志

// GPIO寄存器访问宏
#define GPIO_PORT_BASE(port) (GPIO_BASE + (port) * GPIO_PORT_OFFSET)
#define GPIO_PMD(port) (*(volatile uint32_t *)(GPIO_PORT_BASE(port) + GPIO_PMD_OFFSET))
#define GPIO_DOUT(port) (*(volatile uint32_t *)(GPIO_PORT_BASE(port) + GPIO_DOUT_OFFSET))
#define GPIO_PIN(port) (*(volatile uint32_t *)(GPIO_PORT_BASE(port) + GPIO_PIN_OFFSET))
#define GPIO_IEN(port) (*(volatile uint32_t *)(GPIO_PORT_BASE(port) + GPIO_IEN_OFFSET))
#define GPIO_ISRC(port) (*(volatile uint32_t *)(GPIO_PORT_BASE(port) + GPIO_ISRC_OFFSET))

// GPIO引脚定义
#define LED_DEBUG_PORT GPIO_PORT_A // 调试LED端口
#define BUTTON_PORT GPIO_PORT_A    // 按键端口

// GPIO拉电阻定义
#define GPIO_PULL_NONE 0
#define GPIO_PULL_UP 1
#define GPIO_PULL_DOWN 2

// ============================================================================
// 全局变量
// ============================================================================

// GPIO中断回调函数数组 (每个端口最多16个引脚)
static gpio_int_callback_t gpio_int_callbacks[6][16] = {NULL};

// GPIO初始化状态
static bool gpio_initialized = false;

// GPIO使用统计
static struct
{
    uint32_t output_set_count;
    uint32_t input_read_count;
    uint32_t interrupt_count;
} gpio_stats = {0};

// ============================================================================
// 内部函数
// ============================================================================

/**
 * @brief 检查GPIO端口和引脚是否有效
 * @param port GPIO端口
 * @param pin 引脚号
 * @return true: 有效, false: 无效
 */
static bool gpio_is_valid_pin(gpio_port_t port, uint8_t pin)
{
    // NANO100B支持的端口和引脚检查
    if (pin >= 16)
    {
        return false; // 最多16个引脚
    }

    switch (port)
    {
    case GPIO_PORT_A:
    case GPIO_PORT_B:
    case GPIO_PORT_C:
    case GPIO_PORT_D:
    case GPIO_PORT_F:
        return true;
    default:
        return false;
    }
}

/**
 * @brief 设置引脚模式
 * @param port GPIO端口
 * @param pin 引脚号
 * @param mode 引脚模式
 * @return true: 成功, false: 失败
 */
static bool gpio_set_pin_mode(gpio_port_t port, uint8_t pin, gpio_mode_t mode)
{
    if (!gpio_is_valid_pin(port, pin))
    {
        return false;
    }

    uint32_t reg_value = GPIO_PMD(port);
    uint32_t pin_mask = 0x3 << (pin * 2); // 每个引脚占用2位

    // 清除原有配置
    reg_value &= ~pin_mask;

    // 设置新的模式
    reg_value |= ((uint32_t)mode << (pin * 2));

    GPIO_PMD(port) = reg_value;

    return true;
}

// ============================================================================
// GPIO驱动接口实现
// ============================================================================

/**
 * @brief GPIO模块初始化
 * @return true: 成功, false: 失败
 */
bool gpio_init(void)
{
    if (gpio_initialized)
    {
        return true; // 已经初始化
    }

    // 清除中断回调函数数组
    for (int port = 0; port < 6; port++)
    {
        for (int pin = 0; pin < 16; pin++)
        {
            gpio_int_callbacks[port][pin] = NULL;
        }
    }

    // 初始化常用GPIO
    gpio_config_t led_config = {
        .port = LED_DEBUG_PORT,
        .pin = LED_DEBUG_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .initial_state = false,
        .int_type = GPIO_INT_RISING,
        .callback = NULL};
    gpio_config_pin(&led_config);

    gpio_config_t button_config = {
        .port = BUTTON_PORT,
        .pin = BUTTON_PIN,
        .mode = GPIO_MODE_INPUT_PULLUP,
        .initial_state = false,
        .int_type = GPIO_INT_RISING,
        .callback = NULL};
    gpio_config_pin(&button_config);

    // 清零统计
    gpio_stats.output_set_count = 0;
    gpio_stats.input_read_count = 0;
    gpio_stats.interrupt_count = 0;

    gpio_initialized = true;

    debug_printf("[GPIO] GPIO module initialized\n");
    return true;
}

/**
 * @brief 配置GPIO引脚
 * @param config GPIO配置结构体
 * @return true: 成功, false: 失败
 */
bool gpio_config_pin(const gpio_config_t *config)
{
    if (!config || !gpio_is_valid_pin(config->port, config->pin))
    {
        return false;
    }

    // 设置引脚模式
    if (!gpio_set_pin_mode(config->port, config->pin, config->mode))
    {
        return false;
    }

    // 如果是输出模式，设置初始状态
    if (config->mode == GPIO_MODE_OUTPUT || config->mode == GPIO_MODE_OPEN_DRAIN)
    {
        gpio_write_pin(config->port, config->pin, config->initial_state);
    }

    // TODO: 配置上拉下拉 (需要查阅NANO100B数据手册)
    // 当前简化实现，不配置上拉下拉

    return true;
}

/**
 * @brief 设置GPIO输出状态
 * @param port GPIO端口
 * @param pin 引脚号
 * @param state 输出状态 (true: 高电平, false: 低电平)
 * @return true: 成功, false: 失败
 */
bool gpio_write_pin(gpio_port_t port, uint8_t pin, bool state)
{
    if (!gpio_is_valid_pin(port, pin))
    {
        return false;
    }

    uint32_t pin_bit = 1 << pin;

    if (state)
    {
        GPIO_DOUT(port) |= pin_bit; // 设置为高电平
    }
    else
    {
        GPIO_DOUT(port) &= ~pin_bit; // 设置为低电平
    }

    gpio_stats.output_set_count++;
    return true;
}

/**
 * @brief 读取GPIO输入状态
 * @param port GPIO端口
 * @param pin 引脚号
 * @return true: 高电平, false: 低电平
 */
bool gpio_read_pin(gpio_port_t port, uint8_t pin)
{
    if (!gpio_is_valid_pin(port, pin))
    {
        return false;
    }

    uint32_t pin_bit = 1 << pin;
    bool state = (GPIO_PIN(port) & pin_bit) != 0;

    gpio_stats.input_read_count++;
    return state;
}

/**
 * @brief 翻转GPIO输出状态
 * @param port GPIO端口
 * @param pin 引脚号
 * @return true: 成功, false: 失败
 */
bool gpio_toggle_pin(gpio_port_t port, uint8_t pin)
{
    if (!gpio_is_valid_pin(port, pin))
    {
        return false;
    }

    uint32_t pin_bit = 1 << pin;
    GPIO_DOUT(port) ^= pin_bit; // 异或翻转

    gpio_stats.output_set_count++;
    return true;
}

/**
 * @brief 设置GPIO端口多个引脚状态
 * @param port GPIO端口
 * @param pin_mask 引脚掩码
 * @param state_mask 状态掩码
 * @return true: 成功, false: 失败
 */
bool gpio_write_port(gpio_port_t port, uint16_t pin_mask, uint16_t state_mask)
{
    if (port >= 6)
    {
        return false;
    }

    uint32_t current_value = GPIO_DOUT(port);

    // 清除要设置的位
    current_value &= ~((uint32_t)pin_mask);

    // 设置新的状态
    current_value |= ((uint32_t)state_mask & (uint32_t)pin_mask);

    GPIO_DOUT(port) = current_value;

    gpio_stats.output_set_count++;
    return true;
}

/**
 * @brief 读取GPIO端口所有引脚状态
 * @param port GPIO端口
 * @return 端口状态值 (每个位代表一个引脚)
 */
uint16_t gpio_read_port(gpio_port_t port)
{
    if (port >= 6)
    {
        return 0;
    }

    gpio_stats.input_read_count++;
    return (uint16_t)(GPIO_PIN(port) & 0xFFFF);
}

// ============================================================================
// 便捷接口实现
// ============================================================================

/**
 * @brief 设置调试LED状态
 * @param state LED状态 (true: 亮, false: 灭)
 */
void gpio_led_set(bool state)
{
    gpio_write_pin(LED_DEBUG_PORT, LED_DEBUG_PIN, state);
}

/**
 * @brief 翻转调试LED状态
 */
void gpio_led_toggle(void)
{
    gpio_toggle_pin(LED_DEBUG_PORT, LED_DEBUG_PIN);
}

/**
 * @brief 读取按键状态
 * @return true: 按下, false: 未按下
 */
bool gpio_button_read(void)
{
    // 按键配置为上拉输入，按下时为低电平
    return !gpio_read_pin(BUTTON_PORT, BUTTON_PIN);
}

/**
 * @brief LED闪烁
 * @param times 闪烁次数
 * @param interval_ms 闪烁间隔(毫秒)
 */
void gpio_led_blink(uint8_t times, uint16_t interval_ms)
{
    for (uint8_t i = 0; i < times; i++)
    {
        gpio_led_set(true);
        system_delay_ms(interval_ms);
        gpio_led_set(false);
        if (i < times - 1)
        { // 最后一次不延时
            system_delay_ms(interval_ms);
        }
    }
}

// ============================================================================
// 中断相关接口实现 (简化版)
// ============================================================================

/**
 * @brief 配置GPIO中断
 * @param port GPIO端口
 * @param pin 引脚号
 * @param type 中断触发类型
 * @param callback 中断回调函数
 * @return true: 成功, false: 失败
 */
bool gpio_config_interrupt(gpio_port_t port, uint8_t pin,
                           gpio_int_type_t type, gpio_int_callback_t callback)
{
    if (!gpio_is_valid_pin(port, pin) || !callback)
    {
        return false;
    }

    // 保存回调函数
    gpio_int_callbacks[port][pin] = callback;

    // TODO: 配置中断触发类型寄存器
    // 当前简化实现，需要参考NANO100B数据手册

    return true;
}

/**
 * @brief 使能GPIO中断
 * @param port GPIO端口
 * @param pin 引脚号
 * @param enable true: 使能, false: 禁用
 * @return true: 成功, false: 失败
 */
bool gpio_disable_interrupt(gpio_port_t port, uint8_t pin)
{
    if (!gpio_is_valid_pin(port, pin))
    {
        return false;
    }

    uint32_t pin_bit = 1 << pin;

    // 禁用中断
    GPIO_IEN(port) &= ~pin_bit;

    return true;
}

/**
 * @brief GPIO中断处理函数 (在中断服务程序中调用)
 * @param port GPIO端口
 */
void gpio_interrupt_handler(gpio_port_t port)
{
    if (port >= 6)
    {
        return;
    }

    // 读取中断状态
    uint32_t int_status = GPIO_ISRC(port);

    // 处理每个引脚的中断
    for (uint8_t pin = 0; pin < 16; pin++)
    {
        uint32_t pin_bit = 1 << pin;

        if ((int_status & pin_bit) && gpio_int_callbacks[port][pin])
        {
            // 调用回调函数
            gpio_int_callbacks[port][pin](port, pin);
            gpio_stats.interrupt_count++;
        }
    }

    // 清除中断标志
    GPIO_ISRC(port) = int_status;
}

// ============================================================================
// 调试和状态接口实现
// ============================================================================

/**
 * @brief 获取GPIO模块状态信息
 * @param port_count 端口数量
 * @param pin_count 每端口引脚数量
 */
void gpio_get_info(uint8_t *port_count, uint8_t *pin_count)
{
    if (port_count)
    {
        *port_count = 5; // NANO100B有5个GPIO端口 (A, B, C, D, F)
    }

    if (pin_count)
    {
        *pin_count = 16; // 每个端口最多16个引脚
    }
}

/**
 * @brief 打印GPIO状态信息 (调试用)
 * @param port GPIO端口
 */
void gpio_print_port_status(gpio_port_t port)
{
    if (port >= 6)
    {
        return;
    }

    uint32_t mode_reg = GPIO_PMD(port);
    uint32_t out_reg = GPIO_DOUT(port);
    uint32_t in_reg = GPIO_PIN(port);

    debug_printf("\n[GPIO] Port %c Status:\n", 'A' + port);
    debug_printf("Mode: 0x%08lX\n", mode_reg);
    debug_printf("Output: 0x%04lX\n", out_reg & 0xFFFF);
    debug_printf("Input: 0x%04lX\n", in_reg & 0xFFFF);
}

/**
 * @brief 打印所有GPIO状态 (调试用)
 */
void gpio_print_all_status(void)
{
    debug_printf("\n[GPIO] All GPIO Status:\n");
    debug_printf("Initialized: %s\n", gpio_initialized ? "Yes" : "No");
    debug_printf("Output operations: %lu\n", gpio_stats.output_set_count);
    debug_printf("Input operations: %lu\n", gpio_stats.input_read_count);
    debug_printf("Interrupts: %lu\n", gpio_stats.interrupt_count);

    for (int port = 0; port < 5; port++)
    {
        gpio_print_port_status((gpio_port_t)port);
    }
    debug_printf("\n");
}