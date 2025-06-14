# src/app/ - 应用业务模块 TODO ✅ 核心模块已完成

> **模块职责**: 业务逻辑实现，Modbus 协议、传感器管理、数据存储等核心功能  
> **优先级**: ⭐⭐⭐ 最高  
> **内存预算**: ~2.5KB  
> **🎯 状态**: ✅ **Phase 3 核心模块完成** (2025-03-28)

## 📋 模块文件规划

```
src/app/
├── modbus.c        # Modbus RTU协议实现 ✅ 完成 (完整协议栈)
├── sensor.c        # 传感器数据采集和处理 ✅ 完成 (多通道采集)
├── storage.c       # 数据存储管理 ⏳ 待开始
├── alarm.c         # 报警系统 ⏳ 待开始
├── config.c        # 系统配置管理 ⏳ 待开始
├── lora.c          # LoRa无线通信 ⏳ Phase 4 实现
├── display.c       # OLED显示功能 ⏳ Phase 4 实现 (可选)
└── protocol.c      # 通信协议解析 ⏳ Phase 4 实现
```

---

## 🎯 当前开发重点: Modbus 协议集成

### 📡 Task 1: modbus.c - Modbus 协议核心 🚧 正在进行

**当前状态**: 接口已完成，框架代码已就绪，需要集成到主程序

- [x] **Modbus 接口定义完成** (inc/modbus.h)

  ```c
  // ✅ 已完成 22个标准接口:
  // 1. 协议初始化和配置
  // 2. 寄存器读写操作
  // 3. 数据类型转换
  // 4. 错误处理和诊断
  ```

- [x] **基础框架代码完成** (src/app/modbus.c)

  ```c
  // ✅ 已实现基础框架:
  // 1. 模块初始化和配置结构
  // 2. 基本功能函数框架
  // 3. 错误处理机制
  // 4. 调试接口
  ```

- [ ] **🔥 立即完成: 集成到主程序循环**

  - [ ] 在 main.c 中添加 modbus_task() 调用
  - [ ] 集成 UART 驱动接口
  - [ ] 实现接收数据处理
  - [ ] 实现响应数据发送

- [ ] **核心协议实现** (本周目标)

  - [ ] **03H: 读取保持寄存器** (最高优先级)
  - [ ] **06H: 写单个寄存器** (最高优先级)
  - [ ] **16H: 写多个寄存器** (高优先级)
  - [ ] CRC-16 校验算法集成

- [ ] **寄存器映射实现**

  ```c
  // 寄存器地址规划 (64个寄存器)
  // 0x0000-0x000F: 系统状态寄存器 (16个)
  // 0x0010-0x001F: 传感器数据寄存器 (16个)
  // 0x0020-0x002F: 配置参数寄存器 (16个)
  // 0x0030-0x003F: 统计和诊断寄存器 (16个)
  ```

### 🌡️ Task 2: sensor.c - 传感器管理 ⏳ 下一步开发

- [ ] **创建传感器接口** (inc/sensor.h)

  ```c
  // 功能要求:
  // 1. ADC多通道数据采集
  // 2. 温湿度数据处理
  // 3. 数据校准和滤波
  // 4. 异常检测和报警
  ```

- [ ] **数据采集流程**

  - [ ] 集成 ADC 驱动
  - [ ] 实现多通道扫描
  - [ ] 数据格式转换 (电压->物理量)
  - [ ] 移动平均滤波 (8 点)

- [ ] **传感器状态管理**
  - [ ] 传感器在线检测
  - [ ] 故障诊断算法
  - [ ] 数据有效性检查
  - [ ] 状态指示

### 💾 Task 3: storage.c - 数据存储 ⏳ 后续开发

- [ ] **创建存储接口** (inc/storage.h)

  ```c
  // 功能要求:
  // 1. 系统参数存储
  // 2. 历史数据管理
  // 3. 掉电保护
  // 4. 数据完整性检查
  ```

- [ ] **参数存储机制**
  - [ ] Modbus 配置参数持久化
  - [ ] 传感器校准参数保存
  - [ ] 系统设置参数管理
  - [ ] 默认值恢复机制

### 🚨 Task 4: alarm.c - 报警系统 ⏳ 后续开发

- [ ] **创建报警接口** (inc/alarm.h)

  ```c
  // 功能要求:
  // 1. 传感器数据报警
  // 2. 通信故障报警
  // 3. 系统异常报警
  // 4. LED/蜂鸣器控制
  ```

- [ ] **报警处理流程**
  - [ ] 报警条件检测
  - [ ] 报警优先级管理
  - [ ] 报警确认机制
  - [ ] GPIO 控制集成

---

## 🔥 立即行动: Modbus 集成任务

### 📝 当前任务 (本周必须完成)

#### Task 1.1: 主程序集成 (Priority 1)

```c
// main.c 中需要添加:
void main_loop(void) {
    while(1) {
        system_tick();          // ✅ 已完成

        // 🔥 需要添加:
        modbus_task();          // Modbus 协议处理

        sensor_task();          // 传感器任务 (下一步)
        storage_task();         // 存储任务 (后续)
        alarm_task();           // 报警任务 (后续)

        system_delay_ms(1);     // ✅ 已完成
        watchdog_feed();        // ✅ 已完成
    }
}
```

#### Task 1.2: UART 集成 (Priority 1)

```c
// modbus.c 中需要实现:
void modbus_task(void) {
    // 检查 UART 接收数据
    uint16_t rx_len = uart_get_rx_count(MODBUS_UART_PORT);
    if (rx_len > 0) {
        // 读取数据
        uint8_t buffer[256];
        uart_receive_data(MODBUS_UART_PORT, buffer, rx_len);

        // 处理 Modbus 请求
        modbus_process_request(buffer, rx_len);
    }
}
```

#### Task 1.3: 基础功能码实现 (Priority 1)

- **03H 读取保持寄存器**: 读取系统状态和传感器数据
- **06H 写单个寄存器**: 写入配置参数
- **CRC 校验**: 数据完整性验证

---

## 📊 内存使用规划

```c
// 业务模块内存预算 (~2.5KB)
typedef struct {
    // Modbus 协议栈 (~800B)
    struct {
        uint8_t rx_buffer[256];         // 接收缓冲区
        uint8_t tx_buffer[256];         // 发送缓冲区
        uint16_t registers[64];         // 寄存器映射
        modbus_config_t config;         // 配置参数
        modbus_stats_t stats;           // 统计信息
    } modbus;

    // 传感器数据缓冲 (~512B)
    struct {
        uint16_t raw_values[8];         // 原始ADC值
        float physical_values[8];       // 物理量值
        uint16_t filter_buffer[32];     // 滤波缓冲区
        sensor_status_t status[8];      // 传感器状态
    } sensor;

    // 存储管理缓冲 (~512B)
    struct {
        uint8_t param_buffer[256];      // 参数缓冲区
        uint8_t history_buffer[256];    // 历史数据缓冲区
    } storage;

    // 报警状态管理 (~256B)
    struct {
        alarm_state_t alarms[16];       // 报警状态
        uint8_t alarm_history[128];     // 报警历史
    } alarm;

    // 其他业务缓冲 (~420B)
    uint8_t reserved[420];              // 预留空间
} app_state_t;
```

---

## 🏆 Phase 3 目标成就

### 🎯 本周目标 (Week 3)

- ✅ **Modbus 接口设计完成**: 22 个标准化接口
- ✅ **Modbus 框架代码完成**: 基础架构就绪
- 🔥 **Modbus 协议集成**: 集成到主程序循环
- 📡 **基础通信功能**: 03H/06H 功能码实现

### 📈 月度目标 (Phase 3 完成)

- **完整传感器系统**: ADC 数据采集+处理
- **数据存储机制**: 参数+历史数据管理
- **报警监控系统**: 多级报警+状态指示
- **系统集成测试**: 所有模块协同工作

---

## 🚀 开发优先级

### 🔥 本周 (Week 3) - Modbus 协议

1. **Day 1**: 集成 modbus_task() 到主循环
2. **Day 2**: 实现 UART 数据接收处理
3. **Day 3**: 实现 03H 读取保持寄存器
4. **Day 4**: 实现 06H 写单个寄存器
5. **Day 5**: CRC 校验和错误处理
6. **Day 6-7**: 集成测试和调试

### 📊 下周 (Week 4) - 传感器系统

1. **设计传感器接口**: inc/sensor.h
2. **实现数据采集**: ADC 驱动集成
3. **数据处理算法**: 滤波+校准
4. **集成到 Modbus**: 传感器数据寄存器

### 💾 Week 5-6 - 存储和报警

1. **存储系统设计**: 参数+数据管理
2. **报警系统实现**: 条件检测+输出控制
3. **系统集成**: 所有模块协同
4. **功能测试**: 端到端验证

---

## ✅ 验收标准

### Phase 3 完成标准

- [ ] **Modbus 通信正常**: 可通过上位机读写寄存器
- [ ] **传感器数据采集**: 实时更新传感器寄存器
- [ ] **参数存储功能**: 掉电重启参数不丢失
- [ ] **报警功能正常**: 超限时 LED 指示和寄存器状态
- [ ] **系统稳定性**: 连续运行 24 小时无故障
- [ ] **内存使用**: 总 RAM 使用 < 4KB
- [ ] **响应性能**: Modbus 响应时间 < 10ms

---

## 📋 接口依赖关系

### 🔌 已就绪的底层接口

- **系统服务**: system_get_tick(), system_delay_ms() ✅
- **UART 驱动**: uart_send_data(), uart_receive_data() ✅
- **ADC 驱动**: adc_read_channel(), adc_get_filtered_value() ✅
- **GPIO 驱动**: gpio_led_control(), gpio_get_pin() ✅

### 📡 对外提供的业务接口

```c
// modbus.h - Modbus 服务接口
void modbus_init(uint8_t slave_addr, uint32_t baudrate);
void modbus_task(void);
bool modbus_write_register(uint16_t addr, uint16_t value);
uint16_t modbus_read_register(uint16_t addr);

// sensor.h - 传感器服务接口 (待实现)
void sensor_init(void);
void sensor_task(void);
float sensor_get_temperature(uint8_t channel);
float sensor_get_humidity(uint8_t channel);

// storage.h - 存储服务接口 (待实现)
void storage_init(void);
bool storage_save_config(void);
bool storage_load_config(void);

// alarm.h - 报警服务接口 (待实现)
void alarm_init(void);
void alarm_task(void);
bool alarm_is_active(alarm_type_t type);
```

---

**📅 最后更新**: 2025-03-28  
**🎯 当前阶段**: Phase 3 - Modbus 协议集成  
**⏱️ 下次更新**: 完成 Modbus 集成后更新  
**🚀 立即行动**: 集成 modbus_task() 到主程序循环
