/**
 * ================================================================
 * 憨云 DTU - 系统初始化和控制
 * ================================================================
 * 文件: system.c
 * 功能: 系统硬件初始化、GPIO控制、延时函数
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"
#include "../../inc/system.h"

// ================================================================
// 全局变量定义
// ================================================================

static volatile uint32_t g_system_tick = 0;             // 系统滴答计数器
static volatile boolean_t g_system_initialized = FALSE; // 系统初始化标志

// ================================================================
// 延时函数实现
// ================================================================

/**
 * @brief 微秒级延时函数
 * @param us 延时微秒数
 * @note 基于CPU指令周期的软件延时，42MHz主频
 */
void delay_us(uint32_t us)
{
    // 42MHz = 42 cycles per microsecond
    // 考虑指令执行开销，使用约35个循环
    volatile uint32_t count = us * 35;
    while (count--)
    {
        __asm volatile("nop");
    }
}

/**
 * @brief 毫秒级延时函数
 * @param ms 延时毫秒数
 */
void delay_ms(uint32_t ms)
{
    while (ms--)
    {
        delay_us(1000);
    }
}

// ================================================================
// 系统时钟配置
// ================================================================

/**
 * @brief 系统时钟初始化
 * @note 配置为42MHz主频
 */
static void system_clock_init(void)
{
    // 简化的时钟配置
    // 实际项目中需要根据NANO100B数据手册配置PLL
    // 当前使用内部RC振荡器
}

// ================================================================
// GPIO 初始化和控制
// ================================================================

/**
 * @brief GPIO 初始化
 * @note 根据硬件接口说明文档配置GPIO
 */
static void gpio_init(void)
{
    // 1. 系统状态指示灯 - PA0 (推挽输出)
    GPIO_SET_MODE(SYSTEM_LED_PORT, SYSTEM_LED_PIN, GPIO_PMD_OUTPUT);
    GPIO_CLEAR_PIN(SYSTEM_LED_PORT, SYSTEM_LED_BIT); // 初始关闭

    // 2. 调试LED - PA1 (推挽输出)
    GPIO_SET_MODE(DEBUG_LED_PORT, DEBUG_LED_PIN, GPIO_PMD_OUTPUT);
    GPIO_CLEAR_PIN(DEBUG_LED_PORT, DEBUG_LED_BIT); // 初始关闭

    // 3. 用户按键 - PA2 (输入，内部上拉)
    GPIO_SET_MODE(USER_BUTTON_PORT, USER_BUTTON_PIN, GPIO_PMD_INPUT);

    // 4. OLED I2C引脚 - PC14(SCL), PA12(SDA) (开漏输出，用于软件I2C)
    GPIO_SET_MODE(OLED_SCL_PORT, OLED_SCL_PIN, GPIO_PMD_OPEN_DRAIN);
    GPIO_SET_MODE(OLED_SDA_PORT, OLED_SDA_PIN, GPIO_PMD_OPEN_DRAIN);
    GPIO_SET_PIN(OLED_SCL_PORT, OLED_SCL_BIT); // 初始高电平
    GPIO_SET_PIN(OLED_SDA_PORT, OLED_SDA_BIT); // 初始高电平

    // 5. 蜂鸣器 - PB6 (PWM输出)
    GPIO_SET_MODE(BUZZER_PORT, BUZZER_PIN, GPIO_PMD_OUTPUT);
    GPIO_CLEAR_PIN(BUZZER_PORT, BUZZER_BIT); // 初始关闭
}

// ================================================================
// PWM 蜂鸣器控制
// ================================================================

/**
 * @brief PWM初始化 (用于蜂鸣器控制)
 * @note 配置PWM0通道0，频率约2kHz
 */
static void pwm_init(void)
{
    // 配置PWM预分频器 (假设系统时钟42MHz)
    // 目标频率: 2kHz, 周期 = 42MHz / (预分频 * 分频 * 周期值)
    // 使用预分频1，分频1，周期值10500 -> 42MHz/10500 = 4kHz
    // 使用预分频1，分频2，周期值10500 -> 42MHz/(2*10500) = 2kHz

    REG32(PWM0_BASE + PWM_PPR_OFFSET) = 0;                 // 预分频器设为1
    REG32(PWM0_BASE + PWM_CSR_OFFSET) = PWM_CSR_CSR0_DIV2; // 时钟分频2
    REG32(PWM0_BASE + PWM_CNR0_OFFSET) = 10500;            // 周期值
    REG32(PWM0_BASE + PWM_CMR0_OFFSET) = 5250;             // 占空比50%

    // 禁用PWM输出 (初始状态)
    PWM_DISABLE_CH0();
}

/**
 * @brief 蜂鸣器控制
 * @param enable TRUE=开启, FALSE=关闭
 */
void buzzer_control(boolean_t enable)
{
    if (enable)
    {
        PWM_ENABLE_CH0();
    }
    else
    {
        PWM_DISABLE_CH0();
        GPIO_CLEAR_PIN(BUZZER_PORT, BUZZER_BIT); // 确保输出低电平
    }
}

/**
 * @brief 蜂鸣器响指定次数
 * @param count 响声次数
 * @param duration_ms 每次响声持续时间(毫秒)
 * @param interval_ms 响声间隔时间(毫秒)
 */
void buzzer_beep(uint8_t count, uint16_t duration_ms, uint16_t interval_ms)
{
    for (uint8_t i = 0; i < count; i++)
    {
        buzzer_control(TRUE);
        delay_ms(duration_ms);
        buzzer_control(FALSE);

        if (i < count - 1)
        { // 最后一次不需要间隔
            delay_ms(interval_ms);
        }
    }
}

// ================================================================
// LED 控制函数
// ================================================================

/**
 * @brief 系统状态LED控制
 * @param state TRUE=点亮, FALSE=熄灭
 */
void led_set_status(boolean_t state)
{
    if (state)
    {
        GPIO_SET_PIN(SYSTEM_LED_PORT, SYSTEM_LED_BIT);
    }
    else
    {
        GPIO_CLEAR_PIN(SYSTEM_LED_PORT, SYSTEM_LED_BIT);
    }
}

/**
 * @brief 调试LED控制
 * @param state TRUE=点亮, FALSE=熄灭
 */
void led_set_debug(boolean_t state)
{
    if (state)
    {
        GPIO_SET_PIN(DEBUG_LED_PORT, DEBUG_LED_BIT);
    }
    else
    {
        GPIO_CLEAR_PIN(DEBUG_LED_PORT, DEBUG_LED_BIT);
    }
}

/**
 * @brief LED闪烁指定次数
 * @param count 闪烁次数
 * @param duration_ms 每次闪烁持续时间(毫秒)
 */
void led_blink(uint8_t count, uint16_t duration_ms)
{
    for (uint8_t i = 0; i < count; i++)
    {
        led_set_status(TRUE);
        delay_ms(duration_ms);
        led_set_status(FALSE);

        if (i < count - 1)
        { // 最后一次不需要间隔
            delay_ms(duration_ms);
        }
    }
}

// ================================================================
// OLED 软件I2C 基础函数
// ================================================================

/**
 * @brief I2C开始信号
 */
static void i2c_start(void)
{
    GPIO_SET_PIN(OLED_SDA_PORT, OLED_SDA_BIT); // SDA高
    GPIO_SET_PIN(OLED_SCL_PORT, OLED_SCL_BIT); // SCL高
    delay_us(5);
    GPIO_CLEAR_PIN(OLED_SDA_PORT, OLED_SDA_BIT); // SDA低 (开始信号)
    delay_us(5);
    GPIO_CLEAR_PIN(OLED_SCL_PORT, OLED_SCL_BIT); // SCL低
    delay_us(5);
}

/**
 * @brief I2C停止信号
 */
static void i2c_stop(void)
{
    GPIO_CLEAR_PIN(OLED_SDA_PORT, OLED_SDA_BIT); // SDA低
    GPIO_SET_PIN(OLED_SCL_PORT, OLED_SCL_BIT);   // SCL高
    delay_us(5);
    GPIO_SET_PIN(OLED_SDA_PORT, OLED_SDA_BIT); // SDA高 (停止信号)
    delay_us(5);
}

/**
 * @brief I2C发送一个字节
 * @param data 要发送的数据
 * @return TRUE=ACK, FALSE=NACK
 */
static boolean_t i2c_write_byte(uint8_t data)
{
    // 发送8位数据
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
        {
            GPIO_SET_PIN(OLED_SDA_PORT, OLED_SDA_BIT);
        }
        else
        {
            GPIO_CLEAR_PIN(OLED_SDA_PORT, OLED_SDA_BIT);
        }
        delay_us(2);
        GPIO_SET_PIN(OLED_SCL_PORT, OLED_SCL_BIT); // SCL高
        delay_us(5);
        GPIO_CLEAR_PIN(OLED_SCL_PORT, OLED_SCL_BIT); // SCL低
        delay_us(2);
        data <<= 1;
    }

    // 读取ACK
    GPIO_SET_PIN(OLED_SDA_PORT, OLED_SDA_BIT); // 释放SDA
    delay_us(2);
    GPIO_SET_PIN(OLED_SCL_PORT, OLED_SCL_BIT); // SCL高
    delay_us(5);
    boolean_t ack = !GPIO_READ_PIN(OLED_SDA_PORT, OLED_SDA_BIT); // 读取ACK (低电平为ACK)
    GPIO_CLEAR_PIN(OLED_SCL_PORT, OLED_SCL_BIT);                 // SCL低
    delay_us(5);

    return ack;
}

/**
 * @brief OLED写命令
 * @param cmd 命令字节
 */
void oled_write_cmd(uint8_t cmd)
{
    i2c_start();
    i2c_write_byte(0x78); // OLED设备地址 (写)
    i2c_write_byte(0x00); // 命令模式
    i2c_write_byte(cmd);  // 命令数据
    i2c_stop();
}

/**
 * @brief OLED写数据
 * @param data 数据字节
 */
void oled_write_data(uint8_t data)
{
    i2c_start();
    i2c_write_byte(0x78); // OLED设备地址 (写)
    i2c_write_byte(0x40); // 数据模式
    i2c_write_byte(data); // 显示数据
    i2c_stop();
}

/**
 * @brief OLED基础初始化
 * @note 简化的SSD1306初始化序列
 */
void oled_init(void)
{
    delay_ms(100); // 等待OLED稳定

    // SSD1306初始化序列
    oled_write_cmd(0xAE); // 关闭显示
    oled_write_cmd(0x20); // 设置内存地址模式
    oled_write_cmd(0x10); // 00,水平地址模式;01,垂直地址模式;10,页地址模式(RESET);11,无效
    oled_write_cmd(0xB0); // 设置页地址为0
    oled_write_cmd(0xC8); // 设置COM扫描方向
    oled_write_cmd(0x00); // 设置低列地址
    oled_write_cmd(0x10); // 设置高列地址
    oled_write_cmd(0x40); // 设置起始行地址
    oled_write_cmd(0x81); // 设置对比度控制寄存器
    oled_write_cmd(0xFF); // 对比度值
    oled_write_cmd(0xA1); // 设置段重定义设置,bit0:0,0->0;1,0->127;
    oled_write_cmd(0xA6); // 设置显示方式;bit0:1,反相显示;0,正常显示
    oled_write_cmd(0xA8); // 设置驱动路数
    oled_write_cmd(0x3F); // 默认0X3F(1/64)
    oled_write_cmd(0xA4); // 全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
    oled_write_cmd(0xD3); // 设置显示偏移
    oled_write_cmd(0x00); // 默认为0
    oled_write_cmd(0xD5); // 设置时钟分频因子,震荡频率
    oled_write_cmd(0x80); // [3:0],分频因子;[7:4],震荡频率
    oled_write_cmd(0xD9); // 设置预充电周期
    oled_write_cmd(0xF1); // [3:0],PHASE 1;[7:4],PHASE 2;
    oled_write_cmd(0xDA); // 设置COM硬件引脚配置
    oled_write_cmd(0x12);
    oled_write_cmd(0xDB); // 设置VCOMH 电压倍率
    oled_write_cmd(0x40); // [6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;
    oled_write_cmd(0x8D); // 设置DC-DC开启
    oled_write_cmd(0x14); //
    oled_write_cmd(0xAF); // 开启显示

    delay_ms(100);
}

/**
 * @brief OLED清屏
 */
void oled_clear(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        oled_write_cmd(0xB0 + page); // 设置页地址
        oled_write_cmd(0x00);        // 设置低列地址
        oled_write_cmd(0x10);        // 设置高列地址

        for (uint8_t col = 0; col < 128; col++)
        {
            oled_write_data(0x00); // 清除数据
        }
    }
}

/**
 * @brief OLED显示字符串 (简化版)
 * @param x 列位置 (0-127)
 * @param y 页位置 (0-7)
 * @param str 字符串
 */
void oled_show_string(uint8_t x, uint8_t y, const char *str)
{
    // 简化实现：只显示基本字符
    // 实际项目中需要字库支持
    oled_write_cmd(0xB0 + y);          // 设置页地址
    oled_write_cmd(0x00 + (x & 0x0F)); // 设置低列地址
    oled_write_cmd(0x10 + (x >> 4));   // 设置高列地址

    while (*str)
    {
        // 简单的字符显示 (需要字库数据)
        for (uint8_t i = 0; i < 6; i++)
        {
            oled_write_data(0xFF); // 临时显示全亮点
        }
        str++;
    }
}

// ================================================================
// 传感器和外设控制
// ================================================================

/**
 * @brief 传感器电源控制
 * @param enable TRUE=开启, FALSE=关闭
 */
void sensor_power_control(boolean_t enable)
{
    // 根据实际硬件连接实现
    // 当前为空实现
    (void)enable;
}

/**
 * @brief LoRa模块复位控制
 * @param reset TRUE=复位, FALSE=正常
 */
void lora_reset_control(boolean_t reset)
{
    // 根据实际硬件连接实现
    // 当前为空实现
    (void)reset;
}

/**
 * @brief LoRa模块使能控制
 * @param enable TRUE=使能, FALSE=禁用
 */
void lora_enable_control(boolean_t enable)
{
    // 根据实际硬件连接实现
    // 当前为空实现
    (void)enable;
}

/**
 * @brief 读取用户按键状态
 * @return TRUE=按下, FALSE=未按下
 */
boolean_t button_read_user(void)
{
    // 读取按键状态 (假设按下为低电平)
    return !GPIO_READ_PIN(USER_BUTTON_PORT, USER_BUTTON_BIT);
}

// ================================================================
// 看门狗控制
// ================================================================

/**
 * @brief 看门狗喂狗
 */
void watchdog_feed(void)
{
    // 根据NANO100B看门狗寄存器实现
    // 当前为空实现
}

// ================================================================
// 系统控制函数
// ================================================================

/**
 * @brief 系统滴答递增
 */
void system_tick_increment(void)
{
    g_system_tick++;
}

/**
 * @brief 获取系统滴答计数
 * @return 系统滴答计数值
 */
uint32_t system_get_tick(void)
{
    return g_system_tick;
}

/**
 * @brief 系统复位
 */
void system_reset(void)
{
    // 软件复位
    // 根据NANO100B复位寄存器实现
    while (1)
    {
        // 等待看门狗复位
    }
}

/**
 * @brief 系统初始化
 * @note 初始化所有硬件外设
 */
void system_init(void)
{
    // 1. 系统时钟初始化
    system_clock_init();

    // 2. GPIO初始化
    gpio_init();

    // 3. PWM初始化 (蜂鸣器)
    pwm_init();

    // 4. OLED初始化
    oled_init();

    // 5. 清空OLED显示
    oled_clear();

    // 6. 显示启动信息
    oled_show_string(0, 0, "HUA-COOL DTU");
    oled_show_string(0, 2, "Starting...");

    // 7. 设置初始化完成标志
    g_system_initialized = TRUE;

    // 8. 启动指示：LED闪烁2次，蜂鸣器响2次
    for (uint8_t i = 0; i < 2; i++)
    {
        // 同时控制LED和蜂鸣器
        led_set_status(TRUE);
        buzzer_control(TRUE);
        delay_ms(200);

        led_set_status(FALSE);
        buzzer_control(FALSE);
        delay_ms(200);
    }

    // 9. 更新OLED显示
    oled_show_string(0, 2, "Ready!    ");
}