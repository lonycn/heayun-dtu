/**
 * ================================================================
 * 憨云 DTU - OLED SSD1306 驱动程序
 * ================================================================
 * 文件: oled_ssd1306.c
 * 功能: 0.91寸OLED显示屏驱动 (基于厂家驱动移植)
 * 硬件: SSD1306控制器，128x64分辨率，I2C接口
 * 引脚: PC14(SCL), PA12(SDA)
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#include "../../inc/nano100b_types.h"
#include "../../inc/nano100b_reg.h"
#include "oled_ssd1306.h"

// ================================================================
// 私有函数声明
// ================================================================

static void oled_delay_ms(uint32_t ms);
static void i2c_start(void);
static void i2c_stop(void);
static void i2c_write_byte(uint8_t data);
static void i2c_wait_ack(void);
static void oled_write_command(uint8_t cmd);
void oled_write_data(uint8_t data);

// ================================================================
// GPIO控制宏定义
// ================================================================

#define OLED_SCL_HIGH() SET_BIT(REG32(OLED_SCL_PORT + GPIO_DOUT_OFFSET), OLED_SCL_BIT)
#define OLED_SCL_LOW()  CLEAR_BIT(REG32(OLED_SCL_PORT + GPIO_DOUT_OFFSET), OLED_SCL_BIT)

#define OLED_SDA_HIGH() SET_BIT(REG32(OLED_SDA_PORT + GPIO_DOUT_OFFSET), OLED_SDA_BIT)
#define OLED_SDA_LOW()  CLEAR_BIT(REG32(OLED_SDA_PORT + GPIO_DOUT_OFFSET), OLED_SDA_BIT)

// ================================================================
// 私有函数实现
// ================================================================

/**
 * @brief 延时函数
 * @param ms 延时毫秒数
 */
static void oled_delay_ms(uint32_t ms)
{
    // 简单的软件延时，适用于12MHz时钟
    volatile uint32_t count = ms * 3000;
    while (count--)
    {
        __asm volatile("nop");
    }
}

/**
 * @brief I2C起始信号
 */
static void i2c_start(void)
{
    OLED_SCL_HIGH();
    OLED_SDA_HIGH();
    OLED_SDA_LOW();
    OLED_SCL_LOW();
}

/**
 * @brief I2C停止信号
 */
static void i2c_stop(void)
{
    OLED_SCL_HIGH();
    OLED_SDA_LOW();
    OLED_SDA_HIGH();
}

/**
 * @brief I2C等待应答
 */
static void i2c_wait_ack(void)
{
    OLED_SCL_HIGH();
    OLED_SCL_LOW();
}

/**
 * @brief I2C写字节
 * @param data 要写入的数据
 */
static void i2c_write_byte(uint8_t data)
{
    uint8_t i;
    OLED_SCL_LOW();
    
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            OLED_SDA_HIGH();
        else
            OLED_SDA_LOW();
            
        data <<= 1;
        OLED_SCL_HIGH();
        OLED_SCL_LOW();
    }
}

/**
 * @brief 写OLED命令
 * @param cmd 命令字节
 */
static void oled_write_command(uint8_t cmd)
{
    i2c_start();
    i2c_write_byte(0x78);    // OLED设备地址
    i2c_wait_ack();
    i2c_write_byte(0x00);    // 命令模式
    i2c_wait_ack();
    i2c_write_byte(cmd);     // 命令数据
    i2c_wait_ack();
    i2c_stop();
}

/**
 * @brief 写OLED数据
 * @param data 数据字节
 */
void oled_write_data(uint8_t data)
{
    i2c_start();
    i2c_write_byte(0x78);    // OLED设备地址
    i2c_wait_ack();
    i2c_write_byte(0x40);    // 数据模式
    i2c_wait_ack();
    i2c_write_byte(data);    // 数据
    i2c_wait_ack();
    i2c_stop();
}

// ================================================================
// 公共函数实现
// ================================================================

/**
 * @brief OLED GPIO初始化
 */
void oled_gpio_init(void)
{
    // 配置PC14为输出模式 (SCL)
    // PC14 = bit 14, 需要设置PMD14的bit[29:28] = 01 (推挽输出)
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 28); // 清除bit[29:28]
    REG32(GPIOC_BASE + GPIO_PMD_OFFSET) |= (0x1UL << 28);  // 设置为01 (推挽输出)
    
    // 配置PA12为输出模式 (SDA)
    // PA12 = bit 12, 需要设置PMD12的bit[25:24] = 01 (推挽输出)
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) &= ~(0x3UL << 24); // 清除bit[25:24]
    REG32(GPIOA_BASE + GPIO_PMD_OFFSET) |= (0x1UL << 24);  // 设置为01 (推挽输出)
    
    // 初始状态：SCL和SDA都为高电平
    OLED_SCL_HIGH();
    OLED_SDA_HIGH();
}

/**
 * @brief OLED初始化
 */
void oled_init(void)
{
    // 1. GPIO初始化
    oled_gpio_init();
    
    // 2. 延时等待OLED上电稳定
    oled_delay_ms(100);
    
    // 3. SSD1306初始化序列 (基于厂家驱动)
    oled_write_command(0xAE); // 关闭显示
    
    oled_write_command(0x40); // 设置显示起始行
    oled_write_command(0xB0); // 设置页地址
    
    oled_write_command(0xC8); // 设置COM扫描方向
    
    oled_write_command(0x81); // 设置对比度
    oled_write_command(0xFF); // 对比度值
    
    oled_write_command(0xA1); // 设置段重映射
    oled_write_command(0xA6); // 正常显示
    
    oled_write_command(0xA8); // 设置复用比
    oled_write_command(0x1F); // 32行
    
    oled_write_command(0xD3); // 设置显示偏移
    oled_write_command(0x00); // 无偏移
    
    oled_write_command(0xD5); // 设置时钟分频
    oled_write_command(0xF0); // 分频比
    
    oled_write_command(0xD9); // 设置预充电周期
    oled_write_command(0x22); // 预充电周期
    
    oled_write_command(0xDA); // 设置COM引脚配置
    oled_write_command(0x02); // COM引脚配置
    
    oled_write_command(0xDB); // 设置VCOMH电压
    oled_write_command(0x49); // VCOMH电压
    
    oled_write_command(0x8D); // 电荷泵设置
    oled_write_command(0x14); // 使能电荷泵
    
    oled_write_command(0xAF); // 开启显示
    
    // 4. 清屏
    oled_clear();
}

/**
 * @brief 设置光标位置
 * @param x 列位置 (0-127)
 * @param y 页位置 (0-7)
 */
void oled_set_pos(uint8_t x, uint8_t y)
{
    oled_write_command(0xB0 + y);                    // 设置页地址
    oled_write_command(((x & 0xF0) >> 4) | 0x10);   // 设置列地址高4位
    oled_write_command(x & 0x0F);                    // 设置列地址低4位
}

/**
 * @brief 清屏
 */
void oled_clear(void)
{
    uint8_t i, n;
    
    for (i = 0; i < 8; i++)
    {
        oled_set_pos(0, i);
        for (n = 0; n < 128; n++)
        {
            oled_write_data(0x00);
        }
    }
}

/**
 * @brief 全屏点亮
 */
void oled_fill(void)
{
    uint8_t i, n;
    
    for (i = 0; i < 8; i++)
    {
        oled_set_pos(0, i);
        for (n = 0; n < 128; n++)
        {
            oled_write_data(0xFF);
        }
    }
}

/**
 * @brief 显示字符串 (简化版本)
 * @param x 起始列位置
 * @param y 起始页位置
 * @param str 字符串
 */
void oled_show_string(uint8_t x, uint8_t y, const char* str)
{
    // 简化实现：显示基本的ASCII字符
    // 这里只是一个框架，实际需要字体数据
    uint8_t i = 0;
    
    while (str[i] != '\0' && x < 128)
    {
        oled_set_pos(x, y);
        
        // 简单的字符显示 (这里需要字体数据)
        // 暂时用固定模式显示
        for (uint8_t j = 0; j < 6; j++)
        {
            oled_write_data(0x7E); // 简单的竖线模式
        }
        
        x += 6;
        i++;
    }
}

/**
 * @brief 显示测试模式
 */
void oled_test_pattern(void)
{
    uint8_t i, j;
    
    // 测试模式1：棋盘格
    for (i = 0; i < 8; i++)
    {
        oled_set_pos(0, i);
        for (j = 0; j < 128; j++)
        {
            if ((i + j) % 2 == 0)
                oled_write_data(0xFF);
            else
                oled_write_data(0x00);
        }
    }
}
