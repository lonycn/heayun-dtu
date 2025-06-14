# 12-API 参考手册

## 1. 概述

本文档提供 NANO100B 温湿度监控系统的完整 API 参考，包括数据结构定义、函数接口和使用示例。

### 1.1 API 分类

| 模块分类      | 文件位置    | 功能描述               |
| ------------- | ----------- | ---------------------- |
| 系统初始化    | main_loop.c | 系统启动、主循环控制   |
| Modbus 通信   | Modbus.c    | Modbus RTU 协议实现    |
| UART 通信     | uart/\*.c   | 多路串口通信管理       |
| LoRa 无线通信 | lora/\*.c   | LoRa 网络通信          |
| Flash 存储    | W25Q128-1.c | 外部 Flash 存储管理    |
| LCD 显示      | lcd/\*.c    | LCD 显示控制           |
| 传感器管理    | adc/\*.c    | 温湿度传感器接口       |
| 系统配置      | sys_flash.c | 参数配置和 EEPROM 管理 |
| 错误处理      | Tempalarm.c | 温度报警和错误处理     |

### 1.2 头文件依赖

```c
#include "Nano100Series.h"         // 硬件抽象层
#include "global.h"                // 全局定义和配置
```

## 2. 核心数据结构

### 2.1 系统配置结构

#### 2.1.1 版本信息

```c
// 版本定义 (global.h)
#define cMainVersionRec      1          // 主版本号
#define cSubVersionRec       1          // 次版本号
#define cModifiedVersionRec  26         // 修订版本号

// 版本字符串格式: "v1.1.26-20250328"
```

#### 2.1.2 系统模式配置

```c
// 编译时模式选择 (global.h)
#define LoraGateway                     // LoRa 网关模式
// #define LoraNode                     // LoRa 节点模式
// #define LW_flood                     // 水浸监测模式
// #define LW_DoorMagic                 // 门磁监测模式

// 功能特性定义
#ifdef LoraGateway
    #define _BL02D                      // BL02D 硬件平台
    #define _Loramain                   // LoRa 主机功能
    #define _LoraModule                 // LoRa 模块使能
    #define __RestRfNode                // RF 节点测试
    #define __NotuseLcd                 // 禁用 LCD 显示
#endif
```

### 2.2 历史数据结构

#### 2.2.1 历史记录结构体

```c
// 历史数据结构 (global.h)
struct Hisrory_Struct {
    unsigned char AlarmLevel;           // 报警级别
    unsigned char PowerVolume;          // 电源电压

    // 定时器管理 (8个通道)
    unsigned char MiniterStartFlag[8];  // 定时器启动标志
    unsigned char MiniterDelay[8];      // 定时器延时
    unsigned char Alarm1Count[8];       // 报警计数
    uint16_t AlarmCountOverFlag[8];     // 报警溢出标志
    uint16_t MiniterDelaySet[8];        // 定时器延时设置

    // 报警状态
    unsigned char AlarmStatus1;         // 报警状态1
    unsigned char AlarmFlag;            // 报警标志
    unsigned char TempCount;            // 温度计数
    unsigned char AlarmRecordFlag;      // 报警记录标志
    unsigned char AlarmFlagBak;         // 报警标志备份
};

// 全局历史数据实例
extern struct Hisrory_Struct History;
```

### 2.3 传感器数据结构

#### 2.3.1 传感器类型定义

```c
// 传感器类型枚举 (global.h)
#define cSensorTye_OnlyRs485     0      // 仅 RS485 传感器
#define cSensorTye_OnlyNtc       1      // 仅 NTC 传感器
#define cSensorTye_Rs485_ntc     2      // RS485 + NTC
#define cSensorTye_Rs485_Qitiao  3      // RS485 + 七巧
#define cSensorTye_ntc_Qitiao    4      // NTC + 七巧
#define cSensorTye_OnlyQitiao    5      // 仅七巧传感器
```

#### 2.3.2 温湿度数据地址

```c
// 数据地址定义 (global.h)
#define cVarLenByte             84      // 变量长度
#define cPowerMetre             60      // 功率计地址
#define cDoorPos                64      // 门位置地址
#define cAlarmPos               65      // 报警位置地址
#define cTempPos                66      // 温度位置地址

#define AlarmStartAddr          104     // 报警起始地址
#define TempCompesentStatus     127     // 温度补偿状态
```

### 2.4 通信地址映射

#### 2.4.1 Modbus 地址映射

```c
// Modbus 寄存器地址 (基于源码分析)
typedef enum {
    // 系统状态寄存器 (0x0000-0x001F)
    MODBUS_REG_DEVICE_ID        = 0x0000,   // 设备 ID
    MODBUS_REG_VERSION_MAJOR    = 0x0001,   // 主版本号
    MODBUS_REG_VERSION_MINOR    = 0x0002,   // 次版本号
    MODBUS_REG_VERSION_PATCH    = 0x0003,   // 修订版本号
    MODBUS_REG_UPTIME           = 0x0004,   // 运行时间
    MODBUS_REG_RESET_COUNT      = 0x0005,   // 复位次数

    // 温湿度数据 (0x0020-0x005F)
    MODBUS_REG_TEMP_CH1         = 0x0020,   // 通道1温度
    MODBUS_REG_HUMI_CH1         = 0x0021,   // 通道1湿度
    MODBUS_REG_TEMP_CH2         = 0x0022,   // 通道2温度
    MODBUS_REG_HUMI_CH2         = 0x0023,   // 通道2湿度
    // ... 更多通道

    // 报警状态 (0x0060-0x007F)
    MODBUS_REG_ALARM_STATUS     = 0x0060,   // 报警状态
    MODBUS_REG_ALARM_MASK       = 0x0061,   // 报警屏蔽
    MODBUS_REG_ALARM_COUNT      = 0x0062,   // 报警计数

    // 系统配置 (0x0100-0x01FF)
    MODBUS_REG_CONFIG_BASE      = 0x0100,   // 配置基地址
    MODBUS_REG_TEMP_OFFSET      = 0x0108,   // 温度偏移
    MODBUS_REG_HUMI_OFFSET      = 0x0109,   // 湿度偏移

    // LoRa 网络 (0x0200-0x02FF)
    MODBUS_REG_LORA_NODE_START  = 0x0200,   // LoRa 节点起始地址
    MODBUS_REG_LORA_NODE_COUNT  = 0x0220,   // LoRa 节点数量

} modbus_register_address_t;
```

### 2.5 错误代码定义

#### 2.5.1 系统错误码

```c
// 系统错误类型 (global.h)
#define cConSensorError     0           // 传感器连接错误
#define cFrozeSensorError   1           // 传感器冻结错误
#define cHighTempError      2           // 高温错误
#define cLowTempError       3           // 低温错误
#define cExtInputError      4           // 外部输入错误
#define cMotorStatus        5           // 电机状态
#define cFanStatus          6           // 风扇状态
#define cFrozenStatus       7           // 冷冻状态
#define cRemoteFrozen       8           // 远程冷冻
#define cRemoteOpen         9           // 远程开启
#define cSysDisFrozen       10          // 系统禁止冷冻
#define cComError           11          // 通信错误
```

#### 2.5.2 无效数据定义

```c
// 无效温度值 (global.h)
#ifdef _BL02D
    #ifdef _Loramain
        #define cInValidTemp         3000    // LoRa 主机无效温度
        #define cInValidTempNouse    3000    // 不使用的无效温度
    #else
        #define cInValidTemp         3000    // 普通无效温度
    #endif
#endif

#ifdef _BL03D
    #ifdef __Thermoberg03d
        #define cInValidTemp         3000    // Thermoberg 无效温度
    #else
        #define cInValidTemp         30000   // BL03D 无效温度
    #endif
#endif
```

## 3. 系统初始化 API

### 3.1 主循环控制

#### 3.1.1 主循环函数

```c
// main_loop.c
int main(void);
```

**功能描述**: 系统主入口函数，负责系统初始化和主循环执行

**参数**: 无

**返回值**:

- `int`: 程序退出码（正常情况下不会返回）

**使用示例**:

```c
// 系统自动调用，无需手动调用
int main(void) {
    // 硬件初始化
    SystemInit();

    // 外设初始化
    PeripheralInit();

    // 主循环
    while(1) {
        MainLoopTask();
        WatchdogReset();
    }
}
```

#### 3.1.2 参数初始化

```c
// 函数原型 (global.h)
extern void InitialPara(void);
extern unsigned char GetInitialFlag(void);
```

**InitialPara**

**功能描述**: 初始化系统参数，从 EEPROM 读取配置

**参数**: 无

**返回值**: 无

**GetInitialFlag**

**功能描述**: 获取初始化完成标志

**参数**: 无

**返回值**:

- `unsigned char`: 初始化状态 (0=未完成, 1=已完成)

### 3.2 BL03D 特殊初始化

#### 3.2.1 BL03D 初始化函数

```c
// 函数原型 (global.h)
extern void BL03D_InitialFlag(void);
extern void BL03D_BH_ParaInitial(void);
extern void BL03D_Base_ParaInitial(void);
extern void BL03D_Base_Update(void);
```

**BL03D_InitialFlag**

**功能描述**: 设置 BL03D 初始化标志

**参数**: 无

**返回值**: 无

**BL03D_BH_ParaInitial**

**功能描述**: BL03D 博华版本参数初始化

**参数**: 无

**返回值**: 无

**BL03D_Base_ParaInitial**

**功能描述**: BL03D 基础参数初始化

**参数**: 无

**返回值**: 无

**BL03D_Base_Update**

**功能描述**: BL03D 基础参数更新

**参数**: 无

**返回值**: 无

## 4. Modbus 通信 API

### 4.1 核心通信函数

#### 4.1.1 数据发送

```c
// 函数原型 (global.h)
extern void SendByteAscii(unsigned char i);
extern void SendDataToBus(unsigned char *pp);
```

**SendByteAscii**

**功能描述**: 发送单个字节的 ASCII 格式数据

**参数**:

- `unsigned char i`: 要发送的字节数据

**返回值**: 无

**使用示例**:

```c
// 发送字符 'A'
SendByteAscii(0x41);

// 发送数字 5
SendByteAscii(0x35);
```

**SendDataToBus**

**功能描述**: 向总线发送数据包

**参数**:

- `unsigned char *pp`: 指向要发送数据的指针

**返回值**: 无

**使用示例**:

```c
unsigned char modbus_frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
SendDataToBus(modbus_frame);
```

### 4.2 Modbus 功能码支持

#### 4.2.1 标准功能码

| 功能码 | 名称                     | 描述           |
| ------ | ------------------------ | -------------- |
| 0x03   | Read Holding Registers   | 读取保持寄存器 |
| 0x04   | Read Input Registers     | 读取输入寄存器 |
| 0x06   | Write Single Register    | 写单个寄存器   |
| 0x10   | Write Multiple Registers | 写多个寄存器   |

#### 4.2.2 自定义功能码

| 功能码 | 名称           | 描述         |
| ------ | -------------- | ------------ |
| 0x41   | Extended Read  | 扩展读取功能 |
| 0x44   | Extended Write | 扩展写入功能 |

## 5. Flash 存储 API

### 5.1 W25Q128 Flash 操作

#### 5.1.1 基础操作函数

```c
// 函数原型 (global.h)
extern uint16_t W25QXX_Init(void);
extern uint16_t W25QXX_ReadID(void);
extern uint8_t W25QXX_ReadSR(void);
extern void W25QXX_Write_SR(uint8_t sr);
```

**W25QXX_Init**

**功能描述**: 初始化 W25Q128 Flash 芯片

**参数**: 无

**返回值**:

- `uint16_t`: Flash 设备 ID (0xEF17 表示正常)

**使用示例**:

```c
uint16_t flash_id = W25QXX_Init();
if (flash_id == 0xEF17) {
    // Flash 初始化成功
    printf("W25Q128 Flash initialized successfully\n");
} else {
    // Flash 初始化失败
    printf("Flash initialization failed, ID: 0x%04X\n", flash_id);
}
```

**W25QXX_ReadID**

**功能描述**: 读取 Flash 设备 ID

**参数**: 无

**返回值**:

- `uint16_t`: 设备 ID

**W25QXX_ReadSR**

**功能描述**: 读取状态寄存器

**参数**: 无

**返回值**:

- `uint8_t`: 状态寄存器值

#### 5.1.2 读写操作函数

```c
// 函数原型 (global.h)
extern void W25QXX_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
extern uint8_t W25QXX_Write(const uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
extern uint8_t W25QXX_Write_Check(const uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite, uint8_t NEED_CHECK);
```

**W25QXX_Read**

**功能描述**: 从 Flash 读取数据

**参数**:

- `uint8_t* pBuffer`: 读取数据的缓冲区指针
- `uint32_t ReadAddr`: 读取起始地址
- `uint16_t NumByteToRead`: 要读取的字节数

**返回值**: 无

**使用示例**:

```c
uint8_t read_buffer[256];
W25QXX_Read(read_buffer, 0x1000, 256);
```

**W25QXX_Write**

**功能描述**: 向 Flash 写入数据

**参数**:

- `const uint8_t* pBuffer`: 要写入的数据缓冲区指针
- `uint32_t WriteAddr`: 写入起始地址
- `uint16_t NumByteToWrite`: 要写入的字节数

**返回值**:

- `uint8_t`: 写入结果 (0=成功, 其他=失败)

**使用示例**:

```c
uint8_t write_data[] = {0x01, 0x02, 0x03, 0x04};
uint8_t result = W25QXX_Write(write_data, 0x2000, 4);
if (result == 0) {
    printf("Write successful\n");
}
```

#### 5.1.3 擦除操作函数

```c
// 函数原型 (global.h)
extern void W25QXX_Erase_Sector(uint32_t Dst_Addr);
extern void W25QXX_Erase_Chip(void);
```

**W25QXX_Erase_Sector**

**功能描述**: 擦除指定扇区

**参数**:

- `uint32_t Dst_Addr`: 要擦除的扇区地址

**返回值**: 无

**W25QXX_Erase_Chip**

**功能描述**: 擦除整个芯片

**参数**: 无

**返回值**: 无

**使用示例**:

```c
// 擦除地址 0x1000 所在的扇区
W25QXX_Erase_Sector(0x1000);

// 擦除整个芯片 (谨慎使用)
// W25QXX_Erase_Chip();
```

## 6. LCD 显示 API

### 6.1 LCD 控制函数

#### 6.1.1 基础控制

```c
// 函数原型 (global.h)
extern void LCDInital(void);
extern void LCD_init(void);
extern void LCD4_Clear(void);
extern void LCD4_On(void);
extern void LCD4_Off(void);
extern void LCD4_Home(void);
```

**LCDInital / LCD_init**

**功能描述**: 初始化 LCD 显示器

**参数**: 无

**返回值**: 无

**LCD4_Clear**

**功能描述**: 清除 LCD 显示内容

**参数**: 无

**返回值**: 无

**LCD4_On / LCD4_Off**

**功能描述**: 开启/关闭 LCD 显示

**参数**: 无

**返回值**: 无

#### 6.1.2 数据显示

```c
// 函数原型 (global.h)
extern unsigned char LCD_char(char str);
extern unsigned char setxy(char x, char y);
extern void LCD_string(unsigned char x, unsigned char y, unsigned char *s);
extern void LCD4_PutC(unsigned char dat);
extern void LCD4_PutS(unsigned char *dat);
```

**LCD_char**

**功能描述**: 在当前位置显示单个字符

**参数**:

- `char str`: 要显示的字符

**返回值**:

- `unsigned char`: 操作结果

**setxy**

**功能描述**: 设置 LCD 光标位置

**参数**:

- `char x`: X 坐标
- `char y`: Y 坐标

**返回值**:

- `unsigned char`: 操作结果

**LCD_string**

**功能描述**: 在指定位置显示字符串

**参数**:

- `unsigned char x`: X 坐标
- `unsigned char y`: Y 坐标
- `unsigned char *s`: 要显示的字符串指针

**返回值**: 无

**使用示例**:

```c
// 初始化 LCD
LCD_init();

// 在第1行第2列显示 "Temperature"
LCD_string(1, 2, "Temperature");

// 在第2行第1列显示温度值
LCD_string(2, 1, "25.6C");
```

### 6.2 专用显示函数

#### 6.2.1 系统信息显示

```c
// 函数原型 (global.h)
extern void DisplayTitle(void);
extern void DisplayTitle_02D(void);
extern void DisplayTitle_03D(void);
extern void DisplayTemp(void);
extern void DisplayTemp_02D(void);
extern void DisplayTemp_Lora(void);
extern void DisplayTemp_03D(void);
```

**DisplayTitle**

**功能描述**: 显示标题信息

**参数**: 无

**返回值**: 无

**DisplayTemp**

**功能描述**: 显示温度信息

**参数**: 无

**返回值**: 无

#### 6.2.2 格式化显示

```c
// 函数原型 (global.h)
extern void FloatConvert2ASCII(int16_t floatVal, unsigned char *p, unsigned char type);
extern void DisplayDat(unsigned char Pos, unsigned char dat);
```

**FloatConvert2ASCII**

**功能描述**: 将浮点数转换为 ASCII 字符串

**参数**:

- `int16_t floatVal`: 要转换的浮点数值
- `unsigned char *p`: 输出字符串缓冲区指针
- `unsigned char type`: 转换类型

**返回值**: 无

**使用示例**:

```c
int16_t temperature = 2560;  // 25.60°C (放大100倍)
unsigned char temp_str[10];
FloatConvert2ASCII(temperature, temp_str, 0);
LCD_string(1, 1, temp_str);
```

## 7. 注意事项

### 7.1 API 使用约定

1. **数据类型**: 优先使用标准整数类型 (`uint8_t`, `uint16_t`, `uint32_t`)
2. **返回值**: 错误码使用 0 表示成功，非 0 表示失败
3. **指针参数**: 调用前确保指针非空且指向有效内存
4. **并发安全**: 大部分 API 非线程安全，需要外部同步

### 7.2 平台依赖

1. **编译条件**: 部分 API 依赖编译时宏定义
2. **硬件平台**: BL02D 和 BL03D 平台 API 可能有差异
3. **功能模式**: LoRa 网关/节点模式下 API 行为不同

### 7.3 错误处理

1. **参数检查**: 调用前验证参数有效性
2. **状态检查**: 关键操作后检查返回状态
3. **异常恢复**: 实现适当的错误恢复机制
