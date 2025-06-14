/**
 * ================================================================
 * 憨云 DTU - 系统控制头文件
 * ================================================================
 * 文件: system.h
 * 功能: 系统初始化、GPIO控制、延时函数声明
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "nano100b_types.h"

// ================================================================
// 函数声明
// ================================================================

// 延时函数
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

// 系统初始化
void system_init(void);
void system_reset(void);

// 系统滴答控制
void system_tick_increment(void);
uint32_t system_get_tick(void);

// LED控制
void led_set_status(boolean_t state);
void led_set_debug(boolean_t state);
void led_blink(uint8_t count, uint16_t duration_ms);

// 蜂鸣器控制
void buzzer_control(boolean_t enable);
void buzzer_beep(uint8_t count, uint16_t duration_ms, uint16_t interval_ms);

// OLED显示控制
void oled_write_cmd(uint8_t cmd);
void oled_write_data(uint8_t data);
void oled_init(void);
void oled_clear(void);
void oled_show_string(uint8_t x, uint8_t y, const char *str);

// 外设控制
void sensor_power_control(boolean_t enable);
void lora_reset_control(boolean_t reset);
void lora_enable_control(boolean_t enable);
boolean_t button_read_user(void);

// 看门狗控制
void watchdog_feed(void);

#endif /* __SYSTEM_H__ */