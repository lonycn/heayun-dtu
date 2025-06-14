# 0.91 寸 OLED 调试显示模块

## 1. 概述

本文档详细描述了为憨云 DTU 系统新增的 0.91 寸 OLED 显示模块的硬件连接、驱动移植和调试功能实现。该模块主要用于开发阶段的实时数据显示和系统状态监控，便于开发调试和问题定位。

## 2. 硬件规格

### 2.1 OLED 模块规格

| 参数         | 规格          | 备注           |
| ------------ | ------------- | -------------- |
| **尺寸**     | 0.91 寸       | 对角线尺寸     |
| **分辨率**   | 128×64 像素   | 单色显示       |
| **驱动芯片** | SSD1306       | I2C 接口       |
| **工作电压** | 3.3V / 5V     | 兼容双电压     |
| **通信接口** | I2C           | 4 线连接       |
| **显示颜色** | 白色          | 蓝色背光可选   |
| **工作温度** | -30°C ~ +70°C | 工业级温度范围 |

### 2.2 引脚定义

| OLED 引脚 | NANO100B 引脚 | 功能       | 备注      |
| --------- | ------------- | ---------- | --------- |
| **GND**   | GND           | 电源地     | 必须连接  |
| **VCC**   | 3.3V          | 电源正极   | 3.3V 供电 |
| **SCL**   | PC14          | I2C 时钟线 | 串行时钟  |
| **SDA**   | PA12          | I2C 数据线 | 串行数据  |

### 2.3 硬件连接图

```
NANO100B                    0.91" OLED
┌─────────────┐            ┌─────────────┐
│             │            │             │
│    PA12 ────┼────────────┼──── SDA     │
│             │            │             │
│    PC14 ────┼────────────┼──── SCL     │
│             │            │             │
│    3.3V ────┼────────────┼──── VCC     │
│             │            │             │
│    GND  ────┼────────────┼──── GND     │
│             │            │             │
└─────────────┘            └─────────────┘
```

## 3. 引脚配置

### 3.1 GPIO 配置

#### 3.1.1 I2C 功能配置

```c
// I2C 引脚配置
#define OLED_I2C_PORT       I2C1
#define OLED_I2C_SCL_PIN    PC14    // I2C1 SCL
#define OLED_I2C_SDA_PIN    PA12    // I2C1 SDA

// GPIO 配置函数
void OLED_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能 GPIO 时钟
    CLK_EnableAPB1Peripheral(CLK_APBPERIPH_GPIO, ENABLE);

    // 配置 SCL 引脚 (PC14)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_MODE_OPEN_DRAIN;
    GPIO_InitStructure.GPIO_Pull = GPIO_PULL_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_HIGH;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 配置 SDA 引脚 (PA12)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}
```

#### 3.1.2 I2C 外设配置

```c
// I2C 配置函数
void OLED_I2C_Config(void)
{
    I2C_InitTypeDef I2C_InitStructure;

    // 使能 I2C 时钟
    CLK_EnableAPB1Peripheral(CLK_APBPERIPH_I2C1, ENABLE);

    // I2C 配置
    I2C_InitStructure.I2C_ClockSpeed = 400000;        // 400kHz
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
}
```

## 4. 驱动移植

### 4.1 原厂驱动分析

原厂提供的驱动基于 51 单片机，主要特点：

- 基于软件 I2C 实现
- 使用 P1^0 和 P1^1 引脚
- 包含完整的 SSD1306 驱动
- 支持字符、数字、中文和图像显示

### 4.2 NANO100B 驱动移植

#### 4.2.1 驱动文件结构

```
src/drivers/oled/
├── oled_ssd1306.h          # OLED 驱动头文件
├── oled_ssd1306.c          # OLED 驱动实现
├── oled_fonts.h            # 字体数据（ASCII + 汉字字库）
└── oled_debug.c            # 调试显示功能
```

#### 4.2.2 字体数据文件

```c
// oled_fonts.h - 字体数据定义
#ifndef __OLED_FONTS_H
#define __OLED_FONTS_H

#include <stdint.h>

// 6x8 ASCII字体
extern const uint8_t F6x8[][6];

// 8x16 ASCII字体
extern const uint8_t F8X16[];

// 汉字字库结构
typedef struct {
    uint8_t Index[2];    // 汉字内码
    uint8_t Msk[32];     // 16x16点阵数据
} CHN_CHAR_T;

// 汉字字库数组
extern const CHN_CHAR_T Hzk[];

// 常用汉字字库（示例）
const CHN_CHAR_T Hzk[] = {
    // "华" 字
    {0xBB, 0xAA, {
        0x00,0x00,0x3F,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xFC,0x01,0x00,0x01,0x00,
        0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x20,0x08
    }},
    // "酷" 字
    {0xBF, 0xE1, {
        0x10,0x40,0x18,0x40,0x10,0x40,0x10,0x5F,0x10,0x44,0x10,0x44,0x1C,0x44,0x16,0x44,
        0x10,0x44,0x10,0x44,0x10,0x7F,0x10,0x44,0x10,0x44,0x10,0x44,0x10,0x40,0x10,0x40
    }},
    // 更多汉字...
};

// 6x8 ASCII字体数据
const uint8_t F6x8[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // sp
    {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00}, // !
    {0x00, 0x00, 0x07, 0x00, 0x07, 0x00}, // "
    // ... 更多ASCII字符
};

// 8x16 ASCII字体数据
const uint8_t F8X16[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // sp
    0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x30,0x00,0x00,0x00, // !
    // ... 更多ASCII字符
};

#endif
```

#### 4.2.2 核心驱动接口

```c
// oled_ssd1306.h
#ifndef __OLED_SSD1306_H
#define __OLED_SSD1306_H

#include "Nano100Series.h"

// OLED 设备地址
#define OLED_I2C_ADDR       0x78
#define OLED_CMD            0x00
#define OLED_DATA           0x40

// 显示参数
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_PAGES          8

// 字体大小
typedef enum {
    FONT_SIZE_6x8 = 0,
    FONT_SIZE_8x16,
    FONT_SIZE_16x32
} oled_font_size_t;

// 显示位置
typedef struct {
    uint8_t x;
    uint8_t y;
} oled_pos_t;

// 主要API接口 - 用户调用
int oled_init(void);
int sendString(const char* str);           // 显示字符串（包括汉字）
int sendPic(const uint8_t* pic);          // 显示图片

// 底层API接口
int oled_clear(void);
int oled_display_on(void);
int oled_display_off(void);
int oled_set_pos(uint8_t x, uint8_t y);
int oled_show_char(uint8_t x, uint8_t y, char ch, oled_font_size_t size);
int oled_show_string(uint8_t x, uint8_t y, const char* str, oled_font_size_t size);
int oled_show_number(uint8_t x, uint8_t y, uint32_t num, uint8_t len, oled_font_size_t size);
int oled_draw_bitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t* bmp);

#endif
```

#### 4.2.3 完整驱动程序实现

```c
// oled_ssd1306.c - 完整驱动实现

#include "oled_ssd1306.h"
#include "oled_fonts.h"

// 全局变量
static uint8_t oled_buffer[OLED_PAGES][OLED_WIDTH];
static uint8_t current_x = 0, current_y = 0;

// I2C 底层通信函数
static void I2C_Start(void)
{
    GPIO_SetBit(GPIOA, GPIO_Pin_12);  // SDA = 1
    GPIO_SetBit(GPIOC, GPIO_Pin_14);  // SCL = 1
    delay_us(5);
    GPIO_ClrBit(GPIOA, GPIO_Pin_12);  // SDA = 0
    delay_us(5);
    GPIO_ClrBit(GPIOC, GPIO_Pin_14);  // SCL = 0
}

static void I2C_Stop(void)
{
    GPIO_ClrBit(GPIOA, GPIO_Pin_12);  // SDA = 0
    GPIO_SetBit(GPIOC, GPIO_Pin_14);  // SCL = 1
    delay_us(5);
    GPIO_SetBit(GPIOA, GPIO_Pin_12);  // SDA = 1
    delay_us(5);
}

static void I2C_WriteByte(uint8_t data)
{
    for(int i = 7; i >= 0; i--) {
        GPIO_ClrBit(GPIOC, GPIO_Pin_14);  // SCL = 0
        if(data & (1 << i)) {
            GPIO_SetBit(GPIOA, GPIO_Pin_12);  // SDA = 1
        } else {
            GPIO_ClrBit(GPIOA, GPIO_Pin_12);  // SDA = 0
        }
        delay_us(2);
        GPIO_SetBit(GPIOC, GPIO_Pin_14);  // SCL = 1
        delay_us(2);
    }
    GPIO_ClrBit(GPIOC, GPIO_Pin_14);  // SCL = 0
    delay_us(5);
}

static void I2C_WaitAck(void)
{
    GPIO_SetBit(GPIOC, GPIO_Pin_14);  // SCL = 1
    delay_us(2);
    GPIO_ClrBit(GPIOC, GPIO_Pin_14);  // SCL = 0
}

// OLED 命令和数据写入
static int oled_write_cmd(uint8_t cmd)
{
    I2C_Start();
    I2C_WriteByte(OLED_I2C_ADDR);
    I2C_WaitAck();
    I2C_WriteByte(OLED_CMD);
    I2C_WaitAck();
    I2C_WriteByte(cmd);
    I2C_WaitAck();
    I2C_Stop();
    return 0;
}

static int oled_write_data(uint8_t data)
{
    I2C_Start();
    I2C_WriteByte(OLED_I2C_ADDR);
    I2C_WaitAck();
    I2C_WriteByte(OLED_DATA);
    I2C_WaitAck();
    I2C_WriteByte(data);
    I2C_WaitAck();
    I2C_Stop();
    return 0;
}

// 设置显示位置
int oled_set_pos(uint8_t x, uint8_t y)
{
    oled_write_cmd(0xB0 + y);                    // 设置页地址
    oled_write_cmd(((x & 0xF0) >> 4) | 0x10);   // 设置高4位列地址
    oled_write_cmd((x & 0x0F) | 0x00);          // 设置低4位列地址
    return 0;
}

// 初始化函数
int oled_init(void)
{
    // GPIO 配置
    OLED_GPIO_Config();

    // 延时等待上电稳定
    delay_ms(100);

    // SSD1306 初始化序列
    oled_write_cmd(0xAE); // 关闭显示
    oled_write_cmd(0x20); // 设置内存地址模式
    oled_write_cmd(0x10); // 00,水平地址模式
    oled_write_cmd(0xB0); // 设置页地址模式(0~7)
    oled_write_cmd(0xC8); // 设置 COM 扫描方向
    oled_write_cmd(0x00); // 设置低列地址
    oled_write_cmd(0x10); // 设置高列地址
    oled_write_cmd(0x40); // 设置起始行地址
    oled_write_cmd(0x81); // 设置对比度控制
    oled_write_cmd(0xFF); // 最大对比度
    oled_write_cmd(0xA1); // 设置段重映射
    oled_write_cmd(0xA6); // 设置正常显示
    oled_write_cmd(0xA8); // 设置复用率
    oled_write_cmd(0x3F); // 1/64 duty
    oled_write_cmd(0xA4); // 全局显示开启
    oled_write_cmd(0xD3); // 设置显示偏移
    oled_write_cmd(0x00); // 无偏移
    oled_write_cmd(0xD5); // 设置显示时钟分频因子
    oled_write_cmd(0xF0); // 分频因子
    oled_write_cmd(0xD9); // 设置预充电
    oled_write_cmd(0x22); // 预充电值
    oled_write_cmd(0xDA); // 设置 COM 脚硬件配置
    oled_write_cmd(0x12); // 配置值
    oled_write_cmd(0xDB); // 设置 VCOMH 电压倍率
    oled_write_cmd(0x20); // 0.77xVcc
    oled_write_cmd(0x8D); // 设置 DC-DC 使能
    oled_write_cmd(0x14); // 开启 DC-DC
    oled_write_cmd(0xAF); // 开启显示

    // 清屏
    oled_clear();

    return 0;
}

// 清屏函数
int oled_clear(void)
{
    for(uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_set_pos(0, page);
        for(uint8_t col = 0; col < OLED_WIDTH; col++) {
            oled_write_data(0x00);
            oled_buffer[page][col] = 0x00;
        }
    }
    current_x = 0;
    current_y = 0;
    return 0;
}

// 显示单个字符
static void oled_show_char_internal(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    uint8_t c = ch - ' ';  // 获取字符偏移

    if(size == 16) {  // 8x16字体
        oled_set_pos(x, y);
        for(uint8_t i = 0; i < 8; i++) {
            oled_write_data(F8X16[c * 16 + i]);
        }
        oled_set_pos(x, y + 1);
        for(uint8_t i = 0; i < 8; i++) {
            oled_write_data(F8X16[c * 16 + i + 8]);
        }
    } else {  // 6x8字体
        oled_set_pos(x, y);
        for(uint8_t i = 0; i < 6; i++) {
            oled_write_data(F6x8[c][i]);
        }
    }
}

// 显示汉字
static void oled_show_chinese(uint8_t x, uint8_t y, uint8_t* hz)
{
    uint8_t wm = 0;
    uint16_t adder = 1;

    // 查找汉字在字库中的位置
    while(adder <= sizeof(Hzk) / sizeof(Hzk[0])) {
        wm = 0;
        if((Hzk[adder].Index[0] == hz[0]) && (Hzk[adder].Index[1] == hz[1])) {
            wm = 1;
            adder = adder;
            break;
        }
        adder++;
    }

    if(wm == 0) return;  // 未找到汉字

    // 显示汉字 (16x16)
    oled_set_pos(x, y);
    for(uint8_t i = 0; i < 16; i++) {
        oled_write_data(Hzk[adder].Msk[i]);
    }
    oled_set_pos(x, y + 1);
    for(uint8_t i = 0; i < 16; i++) {
        oled_write_data(Hzk[adder].Msk[i + 16]);
    }
}

// 主要API函数：显示字符串（包括汉字）
int sendString(const char* str)
{
    uint8_t x = current_x;
    uint8_t y = current_y;

    while(*str != '\0') {
        if(*str > 127) {  // 汉字（UTF-8编码）
            oled_show_chinese(x, y, (uint8_t*)str);
            str += 2;  // 汉字占2个字节
            x += 16;   // 汉字宽度16像素
        } else {  // ASCII字符
            oled_show_char_internal(x, y, *str, 16);
            str++;
            x += 8;   // ASCII字符宽度8像素
        }

        // 换行处理
        if(x >= OLED_WIDTH - 16) {
            x = 0;
            y += 2;  // 16像素高度占2页
            if(y >= OLED_PAGES) {
                y = 0;  // 回到顶部
            }
        }
    }

    current_x = x;
    current_y = y;
    return 0;
}

// 主要API函数：显示图片
int sendPic(const uint8_t* pic)
{
    if(pic == NULL) return -1;

    // 显示128x64全屏图片
    for(uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_set_pos(0, page);
        for(uint8_t col = 0; col < OLED_WIDTH; col++) {
            uint8_t data = pic[page * OLED_WIDTH + col];
            oled_write_data(data);
            oled_buffer[page][col] = data;
        }
    }
    return 0;
}

// 延时函数
static void delay_us(uint32_t us)
{
    volatile uint32_t i;
    for(i = 0; i < us * 8; i++);  // 根据系统时钟调整
}

static void delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++) {
        delay_us(1000);
    }
}
```

## 5. 调试功能设计

### 5.1 调试信息显示

#### 5.1.1 系统状态显示

```c
// oled_debug.c
typedef struct {
    float temperature;
    float humidity;
    uint16_t voltage;
    uint8_t modbus_status;
    uint8_t lora_status;
    uint32_t uptime;
} system_status_t;

void oled_show_system_status(const system_status_t* status)
{
    char buffer[32];

    // 标题
    oled_show_string(0, 0, "HuaCool DTU v2.0", FONT_SIZE_6x8);

    // 温湿度
    snprintf(buffer, sizeof(buffer), "T:%.1fC H:%.1f%%",
             status->temperature, status->humidity);
    oled_show_string(0, 16, buffer, FONT_SIZE_6x8);

    // 电压
    snprintf(buffer, sizeof(buffer), "Vol:%dmV", status->voltage);
    oled_show_string(0, 24, buffer, FONT_SIZE_6x8);

    // 通信状态
    oled_show_string(0, 32, "MB:", FONT_SIZE_6x8);
    oled_show_string(24, 32, status->modbus_status ? "OK" : "ERR", FONT_SIZE_6x8);

    oled_show_string(60, 32, "LoRa:", FONT_SIZE_6x8);
    oled_show_string(90, 32, status->lora_status ? "OK" : "ERR", FONT_SIZE_6x8);

    // 运行时间
    uint32_t hours = status->uptime / 3600;
    uint32_t minutes = (status->uptime % 3600) / 60;
    snprintf(buffer, sizeof(buffer), "UP:%02ld:%02ld", hours, minutes);
    oled_show_string(0, 48, buffer, FONT_SIZE_6x8);
}
```

#### 5.1.2 实时数据监控

```c
void oled_show_sensor_data(uint8_t sensor_id, float temp, float humi)
{
    char buffer[32];
    uint8_t y_pos = (sensor_id % 4) * 16;

    snprintf(buffer, sizeof(buffer), "S%d:%.1fC %.1f%%",
             sensor_id, temp, humi);
    oled_show_string(0, y_pos, buffer, FONT_SIZE_6x8);
}

void oled_show_modbus_activity(void)
{
    static uint8_t activity_counter = 0;
    char indicator[] = {'-', '\\', '|', '/'};

    oled_show_char(120, 0, indicator[activity_counter++ % 4], FONT_SIZE_6x8);
}
```

### 5.2 错误诊断显示

```c
typedef enum {
    ERROR_NONE = 0,
    ERROR_SENSOR_TIMEOUT,
    ERROR_MODBUS_FAULT,
    ERROR_LORA_DISCONN,
    ERROR_FLASH_FAIL,
    ERROR_OVER_TEMP,
    ERROR_LOW_VOLTAGE
} error_code_t;

void oled_show_error(error_code_t error)
{
    const char* error_messages[] = {
        "No Error",
        "Sensor Timeout",
        "Modbus Fault",
        "LoRa Disconn",
        "Flash Fail",
        "Over Temp",
        "Low Voltage"
    };

    oled_clear();
    oled_show_string(0, 0, "ERROR:", FONT_SIZE_8x16);
    oled_show_string(0, 20, error_messages[error], FONT_SIZE_6x8);

    // 闪烁显示
    for(int i = 0; i < 5; i++) {
        oled_display_off();
        HAL_Delay(200);
        oled_display_on();
        HAL_Delay(200);
    }
}
```

## 6. 开发调试接口

### 6.1 命令行调试接口

```c
// 调试命令处理
typedef struct {
    const char* cmd;
    void (*handler)(const char* args);
} debug_cmd_t;

void debug_cmd_oled(const char* args)
{
    if(strncmp(args, "clear", 5) == 0) {
        oled_clear();
    }
    else if(strncmp(args, "test", 4) == 0) {
        oled_show_string(0, 0, "OLED Test OK", FONT_SIZE_8x16);
    }
    else if(strncmp(args, "show", 4) == 0) {
        oled_show_string(0, 0, args + 5, FONT_SIZE_6x8);
    }
}

static const debug_cmd_t debug_commands[] = {
    {"oled", debug_cmd_oled},
    {"status", debug_cmd_status},
    {"sensor", debug_cmd_sensor},
    {NULL, NULL}
};
```

### 6.2 系统启动画面

```c
void oled_show_boot_screen(void)
{
    // 清屏
    oled_clear();

    // 显示 Logo
    oled_show_string(30, 10, "HuaCool", FONT_SIZE_8x16);
    oled_show_string(50, 30, "DTU", FONT_SIZE_8x16);
    oled_show_string(20, 50, "System Starting...", FONT_SIZE_6x8);

    HAL_Delay(2000);

    // 显示版本信息
    oled_clear();
    oled_show_string(0, 0, "Firmware v2.0.0", FONT_SIZE_6x8);
    oled_show_string(0, 10, "Build: " __DATE__, FONT_SIZE_6x8);
    oled_show_string(0, 20, "NANO100B Platform", FONT_SIZE_6x8);

    HAL_Delay(1500);
}
```

## 7. 性能优化

### 7.1 显示刷新优化

```c
// 显示缓存
static uint8_t oled_buffer[OLED_PAGES][OLED_WIDTH];
static uint8_t dirty_pages = 0;  // 脏页标记

// 只刷新变化的页
void oled_refresh_display(void)
{
    for(uint8_t page = 0; page < OLED_PAGES; page++) {
        if(dirty_pages & (1 << page)) {
            oled_set_pos(0, page);
            for(uint8_t col = 0; col < OLED_WIDTH; col++) {
                oled_write_data(oled_buffer[page][col]);
            }
            dirty_pages &= ~(1 << page);
        }
    }
}
```

### 7.2 I2C 性能优化

```c
// DMA 批量传输
int oled_write_block(const uint8_t* data, uint16_t len)
{
    return i2c_dma_transmit(OLED_I2C_PORT, OLED_I2C_ADDR, data, len);
}

// 快速清屏
void oled_fast_clear(void)
{
    uint8_t clear_data[OLED_WIDTH + 1];
    clear_data[0] = OLED_DATA;
    memset(&clear_data[1], 0x00, OLED_WIDTH);

    for(uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_set_pos(0, page);
        oled_write_block(clear_data, sizeof(clear_data));
    }
}
```

## 8. 集成测试

### 8.1 硬件连接测试

```c
// I2C 连接测试
int oled_hardware_test(void)
{
    uint8_t test_data = 0x55;

    // 尝试与 OLED 通信
    if(i2c_master_transmit(OLED_I2C_PORT, OLED_I2C_ADDR, &test_data, 1, 1000) != 0) {
        return -1;  // 连接失败
    }

    return 0;  // 连接成功
}
```

### 8.2 显示功能测试

```c
void oled_display_test(void)
{
    // 测试 1: 像素测试
    oled_clear();
    for(int i = 0; i < OLED_WIDTH; i += 8) {
        for(int j = 0; j < OLED_HEIGHT; j += 8) {
            oled_draw_point(i, j, 1);
        }
    }
    HAL_Delay(1000);

    // 测试 2: 文字显示
    oled_clear();
    oled_show_string(0, 0, "Display Test", FONT_SIZE_8x16);
    oled_show_string(0, 20, "0123456789", FONT_SIZE_6x8);
    oled_show_string(0, 30, "ABCDEFGHIJK", FONT_SIZE_6x8);
    HAL_Delay(2000);

    // 测试 3: 动态显示
    for(int i = 0; i < 100; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Count: %d", i);
        oled_show_string(0, 50, buffer, FONT_SIZE_6x8);
        HAL_Delay(50);
    }
}
```

## 9. 故障处理

### 9.1 常见问题

| 问题     | 可能原因     | 解决方案                   |
| -------- | ------------ | -------------------------- |
| 无显示   | 供电问题     | 检查 3.3V 供电和 GND 连接  |
| 花屏     | I2C 通信异常 | 检查 SCL/SDA 引脚连接      |
| 显示模糊 | 对比度设置   | 调整对比度控制寄存器       |
| 刷新慢   | I2C 频率低   | 提高 I2C 时钟频率到 400kHz |

### 9.2 调试方法

```c
// 调试信息输出
void oled_debug_info(void)
{
    printf("OLED Status:\n");
    printf("I2C Port: %p\n", OLED_I2C_PORT);
    printf("Device Address: 0x%02X\n", OLED_I2C_ADDR);
    printf("SCL Pin: PC14\n");
    printf("SDA Pin: PA12\n");

    // 测试 I2C 连接
    if(oled_hardware_test() == 0) {
        printf("I2C Connection: OK\n");
    } else {
        printf("I2C Connection: FAILED\n");
    }
}
```

## 10. 使用示例

### 10.1 主程序集成

```c
int main(void)
{
    // 系统初始化
    system_init();

    // OLED 初始化
    if(oled_init() != 0) {
        printf("OLED init failed!\n");
        return -1;
    }

    // 显示启动信息
    sendString("憨云DTU启动中...");
    delay_ms(2000);

    // 显示版本信息
    oled_clear();
    sendString("固件版本: v2.0.0");
    delay_ms(1500);

    // 主循环
    while(1) {
        // 显示系统状态
        oled_clear();
        sendString("温度: 25.6°C");
        sendString("湿度: 60.2%");
        sendString("电压: 3.3V");
        sendString("状态: 正常运行");

        // 延时
        delay_ms(5000);

        // 显示图片示例
        sendPic(logo_bitmap);  // 显示Logo图片
        delay_ms(3000);
    }
}
```

### 10.2 调试应用示例

```c
void debug_task(void)
{
    static uint32_t last_update = 0;
    uint32_t current_time = HAL_GetTick();

    if(current_time - last_update >= 500) {  // 500ms 更新一次
        // 显示实时数据
        char buffer[32];

        // 显示系统时间
        snprintf(buffer, sizeof(buffer), "Time: %lds", current_time / 1000);
        oled_show_string(0, 0, buffer, FONT_SIZE_6x8);

        // 显示内存使用
        uint32_t free_heap = get_free_heap_size();
        snprintf(buffer, sizeof(buffer), "Heap: %ld", free_heap);
        oled_show_string(0, 10, buffer, FONT_SIZE_6x8);

        // 显示任务状态
        oled_show_string(0, 20, "Tasks: Running", FONT_SIZE_6x8);

        last_update = current_time;
    }
}
```

## 11. 总结

0.91 寸 OLED 显示模块为憨云 DTU 系统提供了强大的可视化调试能力：

### 11.1 主要优势

- **实时监控**：系统状态和传感器数据实时显示
- **故障诊断**：错误信息直观显示，便于问题定位
- **开发友好**：简化调试过程，提高开发效率
- **低功耗**：仅在需要时刷新显示，节省功耗
- **易集成**：标准 I2C 接口，与系统无缝集成

### 11.2 应用场景

- **开发阶段**：实时查看系统运行状态
- **现场调试**：无需连接电脑即可获取诊断信息
- **产品演示**：直观展示设备工作状态
- **维护检修**：快速定位系统故障

该 OLED 模块将成为憨云 DTU 开发和维护过程中的重要工具。

---

_文档版本：v1.0_  
_创建日期：2025 年 3 月 28 日_  
_适用硬件：NANO100B + SSD1306 OLED_
