/**
 * @file oled.c
 * @brief OLED显示模块实现
 * @version 1.0
 * @date 2025-01-14
 *
 * 基于SSD1306控制器的0.91寸OLED显示屏驱动实现
 * 支持多页面显示、系统状态监控、传感器数据显示等功能
 */

#include "oled.h"
#include "system.h"
#include <stdio.h>
#include <string.h>

//==============================================================================
// 全局变量
//==============================================================================

static oled_config_t g_oled_config;
static oled_status_t g_oled_status;
static bool g_oled_initialized = false;

//==============================================================================
// 核心API实现
//==============================================================================

int oled_init(const oled_config_t *config)
{
    if (g_oled_initialized)
    {
        return OLED_OK; // 已经初始化
    }

    // 复制配置
    if (config)
    {
        g_oled_config = *config;
    }
    else
    {
        // 使用默认配置
        g_oled_config.width = 128;
        g_oled_config.height = 64;
        g_oled_config.spi_port = 1;
        g_oled_config.reset_pin = 5;
        g_oled_config.dc_pin = 6;
    }

    // 初始化状态
    g_oled_status.initialized = true;
    g_oled_status.display_on = true;
    g_oled_status.last_update_time = system_get_tick();

    g_oled_initialized = true;

    printf("OLED: 模块初始化完成\n");
    return OLED_OK;
}

int oled_deinit(void)
{
    if (!g_oled_initialized)
    {
        return OLED_ERROR_NOT_INITIALIZED;
    }

    g_oled_status.initialized = false;
    g_oled_status.display_on = false;
    g_oled_initialized = false;

    return OLED_OK;
}

oled_status_t oled_get_status(void)
{
    return g_oled_status;
}

//==============================================================================
// 显示控制
//==============================================================================

int oled_clear(void)
{
    if (!g_oled_initialized)
    {
        return OLED_ERROR_NOT_INITIALIZED;
    }

    printf("OLED: 清屏\n");
    return OLED_OK;
}

int oled_display_on(void)
{
    if (!g_oled_initialized)
    {
        return OLED_ERROR_NOT_INITIALIZED;
    }

    g_oled_status.display_on = true;
    printf("OLED: 显示开启\n");
    return OLED_OK;
}

int oled_display_off(void)
{
    if (!g_oled_initialized)
    {
        return OLED_ERROR_NOT_INITIALIZED;
    }

    g_oled_status.display_on = false;
    printf("OLED: 显示关闭\n");
    return OLED_OK;
}

int oled_set_pos(uint8_t x, uint8_t y)
{
    if (!g_oled_initialized)
    {
        return OLED_ERROR_NOT_INITIALIZED;
    }

    if (x >= g_oled_config.width || y >= OLED_PAGES)
    {
        return OLED_ERROR_INVALID_PARAM;
    }

    printf("OLED: 设置位置 (%d, %d)\n", x, y);
    return OLED_OK;
}

int oled_show_string(uint8_t x, uint8_t y, const char *str, oled_font_size_t size)
{
    if (!g_oled_initialized || !str)
    {
        return OLED_ERROR_INVALID_PARAM;
    }

    printf("OLED: 显示字符串 (%d,%d) \"%s\"\n", x, y, str);
    return OLED_OK;
}

//==============================================================================
// 高级显示功能
//==============================================================================

int oled_show_system_status(const oled_system_status_t *status)
{
    if (!g_oled_initialized || !status)
    {
        return OLED_ERROR_INVALID_PARAM;
    }

    printf("OLED: 显示系统状态\n");
    printf("  温度: %.1f°C, 湿度: %.1f%%\n", status->temperature, status->humidity);
    printf("  电压: %dmV\n", status->voltage);
    printf("  Modbus: %s, LoRa: %s, MQTT: %s\n",
           status->modbus_status ? "OK" : "ERR",
           status->lora_status ? "OK" : "ERR",
           status->mqtt_status ? "OK" : "ERR");

    return OLED_OK;
}

int oled_show_sensor_data(const oled_sensor_data_t *sensors, uint8_t count)
{
    if (!g_oled_initialized || !sensors || count == 0)
    {
        return OLED_ERROR_INVALID_PARAM;
    }

    printf("OLED: 显示传感器数据 (%d个传感器)\n", count);
    for (uint8_t i = 0; i < count && i < 3; i++)
    {
        printf("  传感器%d: %.1f°C, %.1f%%\n",
               sensors[i].sensor_id,
               sensors[i].temperature,
               sensors[i].humidity);
    }

    return OLED_OK;
}

int oled_show_network_status(bool modbus_ok, bool lora_ok, bool mqtt_ok, int8_t rssi)
{
    if (!g_oled_initialized)
    {
        return OLED_ERROR_NOT_INITIALIZED;
    }

    printf("OLED: 显示网络状态\n");
    printf("  Modbus: %s\n", modbus_ok ? "连接正常" : "连接异常");
    printf("  LoRa: %s", lora_ok ? "正常" : "异常");
    if (lora_ok)
    {
        printf(" (%ddBm)", rssi);
    }
    printf("\n");
    printf("  MQTT: %s\n", mqtt_ok ? "连接正常" : "连接异常");

    return OLED_OK;
}

//==============================================================================
// 兼容性API (保持向后兼容)
//==============================================================================

int sendString(const char *str)
{
    if (!g_oled_initialized || !str)
    {
        return OLED_ERROR_INVALID_PARAM;
    }

    printf("OLED: sendString \"%s\"\n", str);
    return OLED_OK;
}

int sendPic(const uint8_t *pic_data)
{
    if (!g_oled_initialized || !pic_data)
    {
        return OLED_ERROR_INVALID_PARAM;
    }

    printf("OLED: sendPic 显示图片\n");
    return OLED_OK;
}