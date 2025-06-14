# inc/ - 头文件目录 TODO

> **模块职责**: 统一的头文件管理，接口定义和类型声明  
> **优先级**: ⭐⭐⭐ 最高  
> **维护要求**: 接口稳定，向后兼容

## 📋 头文件规划

```
inc/
├── system.h        # 系统核心接口
├── drivers/        # 驱动层头文件
│   ├── gpio.h      # GPIO接口
│   ├── uart.h      # UART接口
│   ├── adc.h       # ADC接口
│   ├── i2c.h       # I2C接口
│   ├── spi.h       # SPI接口
│   └── flash.h     # Flash接口
├── app/            # 应用层头文件
│   ├── modbus.h    # Modbus协议接口
│   ├── sensor.h    # 传感器接口
│   ├── storage.h   # 存储接口
│   ├── alarm.h     # 报警接口
│   └── config.h    # 配置接口
├── common/         # 通用头文件
│   ├── types.h     # 数据类型定义
│   ├── errors.h    # 错误码定义
│   ├── config.h    # 编译配置
│   └── version.h   # 版本信息
└── nano100b.h      # 硬件平台定义
```

---

## 🎯 开发任务清单

### 📝 Task 1: 核心系统头文件 (Day 3)

- [ ] **system.h - 系统核心接口**

  ```c
  // 必须包含的接口:
  void system_init(void);
  void system_tick(void);
  uint32_t system_get_tick(void);
  void system_delay_ms(uint32_t ms);
  uint32_t system_get_runtime(void);

  // 系统状态枚举
  typedef enum {
      SYSTEM_STATE_INIT,
      SYSTEM_STATE_RUNNING,
      SYSTEM_STATE_ERROR,
      SYSTEM_STATE_SLEEP
  } system_state_t;
  ```

- [ ] **nano100b.h - 硬件平台定义**

  ```c
  // 硬件资源定义:
  #define SYSTEM_CLOCK_HZ     32000000
  #define RAM_SIZE_KB         8
  #define FLASH_SIZE_KB       64
  #define UART_COUNT          5
  #define ADC_CHANNELS        8

  // GPIO引脚定义
  #define LED_PIN             0
  #define BUTTON_PIN          1
  // ...更多引脚定义
  ```

### 🔌 Task 2: 驱动层头文件 (Day 4-5)

- [ ] **drivers/gpio.h**

  ```c
  void gpio_init(void);
  void gpio_set_output(uint8_t pin, bool state);
  bool gpio_get_input(uint8_t pin);
  void gpio_toggle(uint8_t pin);
  void gpio_set_interrupt(uint8_t pin, void (*callback)(void));
  ```

- [ ] **drivers/uart.h**

  ```c
  void uart_init(uint8_t port, uint32_t baudrate);
  int uart_send(uint8_t port, const uint8_t *data, uint16_t len);
  int uart_receive(uint8_t port, uint8_t *data, uint16_t max_len);
  bool uart_is_tx_ready(uint8_t port);
  void uart_set_callback(uint8_t port, void (*rx_callback)(void));
  ```

- [ ] **drivers/adc.h, i2c.h, spi.h, flash.h**
  - [ ] 标准化接口设计
  - [ ] 错误码统一
  - [ ] 回调函数定义
  - [ ] 配置参数结构

### 📱 Task 3: 应用层头文件 (Day 6-7)

- [ ] **app/modbus.h**

  ```c
  void modbus_init(uint8_t device_addr, uint32_t baudrate);
  void modbus_task(void);
  void modbus_set_register(uint16_t addr, uint16_t value);
  uint16_t modbus_get_register(uint16_t addr);

  // Modbus寄存器地址定义
  #define MODBUS_REG_SYSTEM_STATUS    0x0000
  #define MODBUS_REG_TEMPERATURE      0x0010
  #define MODBUS_REG_HUMIDITY         0x0011
  // ...更多寄存器定义
  ```

- [ ] **app/sensor.h, storage.h, alarm.h, config.h**
  - [ ] 接口函数声明
  - [ ] 数据结构定义
  - [ ] 常量和枚举定义
  - [ ] 回调函数类型定义

### 🔧 Task 4: 通用头文件 (Day 3)

- [ ] **common/types.h**

  ```c
  // 基础数据类型
  typedef unsigned char       uint8_t;
  typedef unsigned short      uint16_t;
  typedef unsigned int        uint32_t;
  typedef signed char         int8_t;
  typedef signed short        int16_t;
  typedef signed int          int32_t;
  typedef float               float32_t;
  typedef double              float64_t;

  #ifndef bool
  #define bool    uint8_t
  #define true    1
  #define false   0
  #endif
  ```

- [ ] **common/errors.h**

  ```c
  typedef enum {
      ERROR_OK = 0,
      ERROR_INVALID_PARAM,
      ERROR_TIMEOUT,
      ERROR_BUSY,
      ERROR_NOT_READY,
      ERROR_CRC_FAIL,
      ERROR_MEMORY_FULL,
      ERROR_DEVICE_NOT_FOUND,
      ERROR_HARDWARE_FAULT
  } error_code_t;

  const char* error_to_string(error_code_t error);
  ```

- [ ] **common/config.h**

  ```c
  // 编译时配置
  #define ENABLE_DEBUG_OUTPUT     1
  #define ENABLE_LORA_MODULE      0
  #define ENABLE_OLED_DISPLAY     0
  #define ENABLE_WATCHDOG         1

  // 内存使用配置
  #define MAX_UART_BUFFER_SIZE    256
  #define MAX_MODBUS_REGISTERS    64
  #define MAX_SENSOR_CHANNELS     8
  ```

- [ ] **common/version.h**

  ```c
  #define VERSION_MAJOR           1
  #define VERSION_MINOR           0
  #define VERSION_PATCH           0
  #define VERSION_BUILD           1

  #define VERSION_STRING          "1.0.0-build1"
  #define BUILD_DATE              __DATE__
  #define BUILD_TIME              __TIME__
  ```

---

## 📋 接口设计原则

### 🎯 设计规范

- [ ] **命名规范**

  - [ ] 函数名: `模块_动作_对象()` 格式
  - [ ] 变量名: 小写+下划线分隔
  - [ ] 宏定义: 全大写+下划线分隔
  - [ ] 类型名: 小写+下划线+\_t 后缀

- [ ] **返回值规范**

  - [ ] 成功返回 0 或正值
  - [ ] 失败返回负值错误码
  - [ ] 布尔函数返回 true/false
  - [ ] 指针函数返回 NULL 表示失败

- [ ] **参数规范**
  - [ ] 输入参数在前，输出参数在后
  - [ ] 使用 const 修饰只读参数
  - [ ] 指针参数进行空指针检查
  - [ ] 数组参数同时传递长度

### 📦 模块化设计

- [ ] **头文件保护**

  ```c
  #ifndef MODULE_NAME_H
  #define MODULE_NAME_H

  // 头文件内容

  #endif /* MODULE_NAME_H */
  ```

- [ ] **C++兼容**

  ```c
  #ifdef __cplusplus
  extern "C" {
  #endif

  // C接口声明

  #ifdef __cplusplus
  }
  #endif
  ```

- [ ] **依赖管理**
  - [ ] 最小化头文件依赖
  - [ ] 使用前向声明减少包含
  - [ ] 避免循环依赖
  - [ ] 明确依赖关系

---

## 📊 内存和性能考虑

### 内存优化

- [ ] **结构体对齐**

  ```c
  // 合理安排成员顺序，减少内存浪费
  typedef struct {
      uint32_t large_member;      // 4字节对齐
      uint16_t medium_member;     // 2字节对齐
      uint8_t  small_member;      // 1字节
      uint8_t  padding;           // 手动填充
  } optimized_struct_t;
  ```

- [ ] **位域使用**
  ```c
  // 用于标志位和状态组合
  typedef struct {
      uint8_t flag1    : 1;
      uint8_t flag2    : 1;
      uint8_t state    : 3;
      uint8_t reserved : 3;
  } status_bits_t;
  ```

### 性能优化

- [ ] **内联函数**

  ```c
  // 简单函数使用inline关键字
  static inline uint16_t swap_bytes(uint16_t value) {
      return ((value << 8) | (value >> 8));
  }
  ```

- [ ] **宏函数**
  ```c
  // 频繁调用的简单操作
  #define MAX(a, b)           ((a) > (b) ? (a) : (b))
  #define MIN(a, b)           ((a) < (b) ? (a) : (b))
  #define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
  ```

---

## 🧪 接口验证

### 编译验证

- [ ] **语法检查**

  - [ ] 所有头文件可独立编译
  - [ ] 没有语法错误和警告
  - [ ] 符合 C99 标准
  - [ ] 通过静态分析工具检查

- [ ] **依赖检查**
  - [ ] 验证包含关系正确
  - [ ] 检查循环依赖
  - [ ] 确认接口完整性
  - [ ] 验证向后兼容性

### 功能验证

- [ ] **接口测试**
  - [ ] 创建测试存根(stub)
  - [ ] 验证参数传递正确
  - [ ] 检查返回值处理
  - [ ] 测试错误处理路径

---

## 🚀 开发优先级

1. **Day 3**: common/ 头文件 (最高优先级)
2. **Day 3**: system.h + nano100b.h (最高优先级)
3. **Day 4-5**: drivers/ 头文件 (高优先级)
4. **Day 6-7**: app/ 头文件 (中优先级)

## ✅ 完成标准

- [ ] 所有头文件可独立编译通过
- [ ] 接口定义完整，符合设计规范
- [ ] 没有循环依赖，依赖关系清晰
- [ ] 通过静态分析，无警告
- [ ] 接口文档化，注释完整
- [ ] 版本信息正确，可追溯

---

**📅 最后更新**: 2025-03-28  
**🎯 模块负责人**: 系统架构师  
**⏱️ 预估工期**: 5 天 (Day 3-7)
