/**
 * ================================================================
 * 憨云 DTU - OLED SSD1306 驱动程序头文件
 * ================================================================
 * 文件: oled_ssd1306.h
 * 功能: 0.91寸OLED显示屏驱动头文件
 * 硬件: SSD1306控制器，128x64分辨率，I2C接口
 * 引脚: PC14(SCL), PA12(SDA)
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#ifndef __OLED_SSD1306_H__
#define __OLED_SSD1306_H__

#include "../../inc/nano100b_types.h"

// ================================================================
// 常量定义
// ================================================================

#define OLED_WIDTH  128     // OLED宽度
#define OLED_HEIGHT 64      // OLED高度
#define OLED_PAGES  8       // OLED页数 (64/8=8)

// SSD1306 I2C地址
#define SSD1306_I2C_ADDR 0x78

// 命令/数据标识
#define OLED_CMD  0         // 命令模式
#define OLED_DATA 1         // 数据模式

// ================================================================
// 公共函数声明
// ================================================================

/**
 * @brief OLED初始化
 * @note 必须在使用其他OLED函数前调用
 */
void oled_init(void);

/**
 * @brief OLED GPIO初始化
 * @note 配置PC14(SCL)和PA12(SDA)为输出模式
 */
void oled_gpio_init(void);

/**
 * @brief 设置光标位置
 * @param x 列位置 (0-127)
 * @param y 页位置 (0-7)
 */
void oled_set_pos(uint8_t x, uint8_t y);

/**
 * @brief 清屏
 * @note 将整个屏幕清空为黑色
 */
void oled_clear(void);

/**
 * @brief 全屏点亮
 * @note 将整个屏幕点亮为白色
 */
void oled_fill(void);

/**
 * @brief 显示字符串 (简化版本)
 * @param x 起始列位置 (0-127)
 * @param y 起始页位置 (0-7)
 * @param str 要显示的字符串
 * @note 当前为简化实现，需要字体数据支持完整功能
 */
void oled_show_string(uint8_t x, uint8_t y, const char* str);

/**
 * @brief 显示测试模式
 * @note 显示棋盘格测试图案，用于验证OLED工作状态
 */
void oled_test_pattern(void);

/**
 * @brief 写数据到OLED (内部函数，供测试使用)
 * @param data 要写入的数据
 */
void oled_write_data(uint8_t data);

// ================================================================
// 用户接口函数 (符合需求文档)
// ================================================================

/**
 * @brief 发送字符串到OLED显示
 * @param str 要显示的字符串指针
 * @note 这是需求文档中要求的sendString函数
 */
static inline void sendString(const char* str)
{
    oled_show_string(0, 0, str);
}

/**
 * @brief 发送图片到OLED显示
 * @param pic 图片数据指针
 * @note 这是需求文档中要求的sendPic函数，当前为简化实现
 */
static inline void sendPic(const uint8_t* pic)
{
    // 简化实现：显示测试图案
    // 实际应用中需要根据图片格式进行解析和显示
    oled_test_pattern();
}

#endif /* __OLED_SSD1306_H__ */
