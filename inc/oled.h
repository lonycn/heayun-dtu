/**
 * @file oled.h
 * @brief OLED显示模块接口定义
 * @version 1.0
 * @date 2025-03-28
 *
 * 憨云DTU - 0.91寸OLED显示模块
 * 支持SSD1306驱动，实时数据显示，系统状态监控
 */

#ifndef __OLED_H__
#define __OLED_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ================================ 宏定义 ================================ */

// OLED设备地址
#define OLED_I2C_ADDR 0x78
#define OLED_CMD 0x00
#define OLED_DATA 0x40

// 显示参数
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES 8

// I2C引脚定义
#define OLED_I2C_PORT I2C1
#define OLED_I2C_SCL_PIN PC14 // I2C1 SCL
#define OLED_I2C_SDA_PIN PA12 // I2C1 SDA

// 错误码
#define OLED_SUCCESS 0
#define OLED_ERROR_INIT -1
#define OLED_ERROR_I2C -2
#define OLED_ERROR_PARAM -3
#define OLED_ERROR_TIMEOUT -4

    /* ================================ 数据结构 ================================ */

    /**
     * @brief 字体大小枚举
     */
    typedef enum
    {
        FONT_SIZE_6x8 = 0, ///< 6x8像素字体
        FONT_SIZE_8x16,    ///< 8x16像素字体
        FONT_SIZE_16x32    ///< 16x32像素字体
    } oled_font_size_t;

    /**
     * @brief 显示位置结构
     */
    typedef struct
    {
        uint8_t x; ///< X坐标
        uint8_t y; ///< Y坐标
    } oled_pos_t;

    /**
     * @brief 系统状态显示结构
     */
    typedef struct
    {
        float temperature;     ///< 温度
        float humidity;        ///< 湿度
        uint16_t voltage;      ///< 电压(mV)
        uint8_t modbus_status; ///< Modbus状态
        uint8_t lora_status;   ///< LoRa状态
        uint8_t mqtt_status;   ///< MQTT状态
        uint32_t uptime;       ///< 运行时间(秒)
        uint16_t free_memory;  ///< 空闲内存(KB)
    } oled_system_status_t;

    /**
     * @brief 传感器数据显示结构
     */
    typedef struct
    {
        uint8_t sensor_id; ///< 传感器ID
        float temperature; ///< 温度
        float humidity;    ///< 湿度
        uint8_t status;    ///< 状态
    } oled_sensor_data_t;

    /**
     * @brief 错误信息枚举
     */
    typedef enum
    {
        OLED_ERROR_NONE = 0,       ///< 无错误
        OLED_ERROR_SENSOR_TIMEOUT, ///< 传感器超时
        OLED_ERROR_MODBUS_FAULT,   ///< Modbus故障
        OLED_ERROR_LORA_DISCONN,   ///< LoRa断开
        OLED_ERROR_MQTT_DISCONN,   ///< MQTT断开
        OLED_ERROR_FLASH_FAIL,     ///< Flash故障
        OLED_ERROR_OVER_TEMP,      ///< 温度过高
        OLED_ERROR_LOW_VOLTAGE     ///< 电压过低
    } oled_error_code_t;

    /**
     * @brief 显示页面枚举
     */
    typedef enum
    {
        OLED_PAGE_STATUS = 0, ///< 状态页面
        OLED_PAGE_SENSORS,    ///< 传感器页面
        OLED_PAGE_NETWORK,    ///< 网络页面
        OLED_PAGE_SYSTEM,     ///< 系统页面
        OLED_PAGE_DEBUG,      ///< 调试页面
        OLED_PAGE_MAX         ///< 页面总数
    } oled_page_t;

    /* ================================ 配置结构 ================================ */

    /**
     * @brief OLED配置结构
     */
    typedef struct
    {
        uint8_t spi_port;  ///< SPI端口
        uint8_t reset_pin; ///< 复位引脚
        uint8_t dc_pin;    ///< DC引脚
        uint8_t width;     ///< 显示宽度
        uint8_t height;    ///< 显示高度
    } oled_config_t;

    /**
     * @brief OLED状态结构
     */
    typedef struct
    {
        bool initialized;          ///< 是否已初始化
        bool display_on;           ///< 显示是否开启
        uint32_t last_update_time; ///< 最后更新时间
    } oled_status_t;

    /* ================================ 错误码定义 ================================ */

#define OLED_OK 0
#define OLED_ERROR_INVALID_PARAM -1
#define OLED_ERROR_NOT_INITIALIZED -2
#define OLED_ERROR_HARDWARE -3

    /* ================================ API接口 ================================ */

    /**
     * @brief 初始化OLED模块
     * @param config 配置参数
     * @return 0:成功, <0:失败
     */
    int oled_init(const oled_config_t *config);

    /**
     * @brief 反初始化OLED模块
     * @return 0:成功, <0:失败
     */
    int oled_deinit(void);

    /**
     * @brief 清屏
     * @return 0:成功, <0:失败
     */
    int oled_clear(void);

    /**
     * @brief 开启显示
     * @return 0:成功, <0:失败
     */
    int oled_display_on(void);

    /**
     * @brief 关闭显示
     * @return 0:成功, <0:失败
     */
    int oled_display_off(void);

    /**
     * @brief 设置显示位置
     * @param x X坐标
     * @param y Y坐标(页地址)
     * @return 0:成功, <0:失败
     */
    int oled_set_pos(uint8_t x, uint8_t y);

    /**
     * @brief 显示单个字符
     * @param x X坐标
     * @param y Y坐标
     * @param ch 字符
     * @param size 字体大小
     * @return 0:成功, <0:失败
     */
    int oled_show_char(uint8_t x, uint8_t y, char ch, oled_font_size_t size);

    /**
     * @brief 显示字符串
     * @param x X坐标
     * @param y Y坐标
     * @param str 字符串
     * @param size 字体大小
     * @return 0:成功, <0:失败
     */
    int oled_show_string(uint8_t x, uint8_t y, const char *str, oled_font_size_t size);

    /**
     * @brief 显示数字
     * @param x X坐标
     * @param y Y坐标
     * @param num 数字
     * @param len 显示长度
     * @param size 字体大小
     * @return 0:成功, <0:失败
     */
    int oled_show_number(uint8_t x, uint8_t y, uint32_t num, uint8_t len, oled_font_size_t size);

    /**
     * @brief 显示浮点数
     * @param x X坐标
     * @param y Y坐标
     * @param num 浮点数
     * @param decimal 小数位数
     * @param size 字体大小
     * @return 0:成功, <0:失败
     */
    int oled_show_float(uint8_t x, uint8_t y, float num, uint8_t decimal, oled_font_size_t size);

    /**
     * @brief 显示图片
     * @param x0 起始X坐标
     * @param y0 起始Y坐标
     * @param x1 结束X坐标
     * @param y1 结束Y坐标
     * @param bmp 图片数据
     * @return 0:成功, <0:失败
     */
    int oled_draw_bitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t *bmp);

    /**
     * @brief 画点
     * @param x X坐标
     * @param y Y坐标
     * @param dot 点状态(0:灭, 1:亮)
     * @return 0:成功, <0:失败
     */
    int oled_draw_point(uint8_t x, uint8_t y, uint8_t dot);

    /**
     * @brief 画线
     * @param x1 起始X坐标
     * @param y1 起始Y坐标
     * @param x2 结束X坐标
     * @param y2 结束Y坐标
     * @return 0:成功, <0:失败
     */
    int oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    /**
     * @brief 画矩形
     * @param x1 起始X坐标
     * @param y1 起始Y坐标
     * @param x2 结束X坐标
     * @param y2 结束Y坐标
     * @param fill 是否填充
     * @return 0:成功, <0:失败
     */
    int oled_draw_rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool fill);

    /* ================================ 高级显示功能 ================================ */

    /**
     * @brief 显示系统状态
     * @param status 系统状态数据
     * @return 0:成功, <0:失败
     */
    int oled_show_system_status(const oled_system_status_t *status);

    /**
     * @brief 显示传感器数据
     * @param sensor_data 传感器数据数组
     * @param count 传感器数量
     * @return 0:成功, <0:失败
     */
    int oled_show_sensor_data(const oled_sensor_data_t *sensor_data, uint8_t count);

    /**
     * @brief 显示网络状态
     * @param modbus_online Modbus在线状态
     * @param lora_connected LoRa连接状态
     * @param mqtt_connected MQTT连接状态
     * @param wifi_rssi WiFi信号强度
     * @return 0:成功, <0:失败
     */
    int oled_show_network_status(bool modbus_online, bool lora_connected,
                                 bool mqtt_connected, int8_t wifi_rssi);

    /**
     * @brief 显示错误信息
     * @param error_code 错误码
     * @param message 错误消息
     * @return 0:成功, <0:失败
     */
    int oled_show_error(oled_error_code_t error_code, const char *message);

    /**
     * @brief 显示启动画面
     * @return 0:成功, <0:失败
     */
    int oled_show_boot_screen(void);

    /**
     * @brief 显示版本信息
     * @param version 版本字符串
     * @param build_date 编译日期
     * @return 0:成功, <0:失败
     */
    int oled_show_version_info(const char *version, const char *build_date);

    /**
     * @brief 显示进度条
     * @param x X坐标
     * @param y Y坐标
     * @param width 宽度
     * @param height 高度
     * @param progress 进度(0-100)
     * @return 0:成功, <0:失败
     */
    int oled_show_progress_bar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress);

    /* ================================ 页面管理 ================================ */

    /**
     * @brief 设置当前显示页面
     * @param page 页面类型
     * @return 0:成功, <0:失败
     */
    int oled_set_page(oled_page_t page);

    /**
     * @brief 获取当前显示页面
     * @return 当前页面类型
     */
    oled_page_t oled_get_page(void);

    /**
     * @brief 切换到下一页面
     * @return 0:成功, <0:失败
     */
    int oled_next_page(void);

    /**
     * @brief 切换到上一页面
     * @return 0:成功, <0:失败
     */
    int oled_prev_page(void);

    /**
     * @brief 刷新当前页面
     * @return 0:成功, <0:失败
     */
    int oled_refresh_page(void);

    /* ================================ 调试功能 ================================ */

    /**
     * @brief 显示调试信息
     * @param line 行号(0-7)
     * @param format 格式化字符串
     * @param ... 参数
     * @return 0:成功, <0:失败
     */
    int oled_debug_printf(uint8_t line, const char *format, ...);

    /**
     * @brief 清除调试信息
     * @return 0:成功, <0:失败
     */
    int oled_debug_clear(void);

    /**
     * @brief 显示内存使用情况
     * @param total_memory 总内存(KB)
     * @param free_memory 空闲内存(KB)
     * @return 0:成功, <0:失败
     */
    int oled_show_memory_usage(uint16_t total_memory, uint16_t free_memory);

    /**
     * @brief 显示任务状态
     * @param task_count 任务数量
     * @param cpu_usage CPU使用率(%)
     * @return 0:成功, <0:失败
     */
    int oled_show_task_status(uint8_t task_count, uint8_t cpu_usage);

    /* ================================ 简化API ================================ */

    /**
     * @brief 显示字符串(简化版)
     * @param str 字符串(支持中文)
     * @return 0:成功, <0:失败
     * @note 兼容原有的sendString接口
     */
    int sendString(const char *str);

    /**
     * @brief 显示图片(简化版)
     * @param pic 图片数据
     * @return 0:成功, <0:失败
     * @note 兼容原有的sendPic接口
     */
    int sendPic(const uint8_t *pic);

    /* ================================ 硬件测试 ================================ */

    /**
     * @brief OLED硬件连接测试
     * @return 0:成功, <0:失败
     */
    int oled_hardware_test(void);

    /**
     * @brief OLED显示功能测试
     * @return 0:成功, <0:失败
     */
    int oled_display_test(void);

    /**
     * @brief 获取OLED状态
     * @return OLED状态结构
     */
    oled_status_t oled_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */