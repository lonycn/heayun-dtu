# tests/ - 测试模块 TODO

> **模块职责**: 单元测试、集成测试、硬件在环测试和性能测试  
> **优先级**: ⭐⭐ 高优先级  
> **覆盖率目标**: 核心功能 >80%

## 📋 测试目录规划

```
tests/
├── unit/           # 单元测试
│   ├── test_core/      # 核心模块测试
│   ├── test_drivers/   # 驱动模块测试
│   └── test_app/       # 应用模块测试
├── integration/    # 集成测试
│   ├── test_modbus/    # Modbus通信测试
│   ├── test_sensor/    # 传感器集成测试
│   └── test_storage/   # 存储集成测试
├── hardware/       # 硬件在环测试
│   ├── test_gpio/      # GPIO硬件测试
│   ├── test_uart/      # UART硬件测试
│   └── test_i2c/       # I2C硬件测试
├── performance/    # 性能测试
│   ├── memory_test.c   # 内存使用测试
│   ├── timing_test.c   # 时序性能测试
│   └── stress_test.c   # 压力测试
├── framework/      # 测试框架
│   ├── unity.c         # Unity测试框架
│   ├── test_runner.c   # 测试运行器
│   └── mock.c          # Mock框架
└── scripts/        # 测试脚本
    ├── run_all_tests.sh    # 运行所有测试
    ├── generate_report.py  # 生成测试报告
    └── flash_test.sh       # 硬件测试脚本
```

---

## 🎯 开发任务清单

### 🧪 Task 1: 测试框架建设 (Week 13)

- [ ] **Unity 测试框架集成**

  ```c
  // 功能要求:
  // 1. 轻量级单元测试框架
  // 2. 断言宏支持
  // 3. 测试报告生成
  // 4. 嵌入式环境适配
  ```

- [ ] **测试运行器实现**

  - [ ] 测试用例自动发现
  - [ ] 测试结果统计
  - [ ] 失败测试详情报告
  - [ ] 串口输出格式化

- [ ] **Mock 框架实现**

  - [ ] 硬件抽象层 Mock
  - [ ] 函数调用记录
  - [ ] 返回值控制
  - [ ] 参数验证

- [ ] **测试工具**
  - [ ] 内存泄漏检测
  - [ ] 堆栈使用监控
  - [ ] 代码覆盖率统计
  - [ ] 性能分析工具

### 📝 Task 2: 核心模块单元测试 (Week 13-14)

- [ ] **test_core/test_system.c**

  ```c
  // 测试用例:
  void test_system_init(void);
  void test_system_tick(void);
  void test_system_delay(void);
  void test_system_get_tick(void);
  void test_system_get_runtime(void);
  ```

- [ ] **test_core/test_timer.c**

  - [ ] 软件定时器创建/删除
  - [ ] 定时器启动/停止
  - [ ] 超时回调执行
  - [ ] 多定时器并发测试

- [ ] **test_core/test_interrupts.c**

  - [ ] 中断服务程序测试
  - [ ] 中断优先级测试
  - [ ] 中断嵌套测试
  - [ ] 中断延迟测试

- [ ] **test_core/test_watchdog.c**
  - [ ] 看门狗初始化测试
  - [ ] 喂狗功能测试
  - [ ] 超时复位测试
  - [ ] 状态监控测试

### 🔌 Task 3: 驱动模块单元测试 (Week 14)

- [ ] **test_drivers/test_gpio.c**

  ```c
  // 测试用例:
  void test_gpio_init(void);
  void test_gpio_set_output(void);
  void test_gpio_get_input(void);
  void test_gpio_toggle(void);
  void test_gpio_interrupt(void);
  ```

- [ ] **test_drivers/test_uart.c**

  - [ ] UART 初始化测试
  - [ ] 数据发送测试
  - [ ] 数据接收测试
  - [ ] 缓冲区管理测试
  - [ ] 错误处理测试

- [ ] **test_drivers/test_adc.c**

  - [ ] ADC 初始化测试
  - [ ] 单通道转换测试
  - [ ] 多通道扫描测试
  - [ ] 转换精度测试
  - [ ] DMA 传输测试

- [ ] **test_drivers/test_i2c.c, test_spi.c, test_flash.c**
  - [ ] 标准测试用例模板
  - [ ] 错误注入测试
  - [ ] 边界条件测试
  - [ ] 性能基准测试

### 📱 Task 4: 应用模块单元测试 (Week 14-15)

- [ ] **test_app/test_modbus.c**

  ```c
  // 测试用例:
  void test_modbus_init(void);
  void test_modbus_read_holding_registers(void);
  void test_modbus_write_single_register(void);
  void test_modbus_crc_calculation(void);
  void test_modbus_exception_handling(void);
  ```

- [ ] **test_app/test_sensor.c**

  - [ ] 传感器初始化测试
  - [ ] 数据采集测试
  - [ ] 数据滤波测试
  - [ ] 故障检测测试
  - [ ] 校准算法测试

- [ ] **test_app/test_storage.c**

  - [ ] 存储初始化测试
  - [ ] 参数读写测试
  - [ ] 历史数据存储测试
  - [ ] 磨损均衡测试
  - [ ] 数据完整性测试

- [ ] **test_app/test_alarm.c, test_config.c**
  - [ ] 功能完整性测试
  - [ ] 边界条件测试
  - [ ] 错误恢复测试
  - [ ] 状态机测试

### 🔗 Task 5: 集成测试 (Week 15)

- [ ] **test_integration/test_modbus_communication.c**

  ```c
  // 集成测试场景:
  void test_pc_to_device_communication(void);
  void test_real_sensor_data_reading(void);
  void test_parameter_configuration(void);
  void test_alarm_status_reporting(void);
  ```

- [ ] **test_integration/test_sensor_to_storage.c**

  - [ ] 传感器数据实时存储
  - [ ] 历史数据查询
  - [ ] 数据完整性验证
  - [ ] 存储容量管理

- [ ] **test_integration/test_end_to_end.c**
  - [ ] 完整业务流程测试
  - [ ] 长期稳定性测试
  - [ ] 异常恢复测试
  - [ ] 并发操作测试

### 🔧 Task 6: 硬件在环测试 (Week 15-16)

- [ ] **test_hardware/test_real_uart.c**

  ```c
  // 硬件测试要求:
  // 1. 真实硬件环境
  // 2. 外部设备连接
  // 3. 信号完整性验证
  // 4. 电气特性测试
  ```

- [ ] **test_hardware/test_real_i2c.c**

  - [ ] SHT2x 传感器实际通信
  - [ ] 总线时序验证
  - [ ] 多设备总线测试
  - [ ] 错误条件测试

- [ ] **test_hardware/test_real_spi.c**
  - [ ] SPI Flash 实际读写
  - [ ] 高速传输测试
  - [ ] 信号质量验证
  - [ ] 温度特性测试

### ⚡ Task 7: 性能测试 (Week 16)

- [ ] **performance/memory_test.c**

  ```c
  // 内存使用分析:
  void test_static_memory_usage(void);
  void test_stack_usage_analysis(void);
  void test_heap_usage_monitoring(void);
  void test_memory_fragmentation(void);
  ```

- [ ] **performance/timing_test.c**

  - [ ] 主循环执行时间测试
  - [ ] 中断响应时间测试
  - [ ] 任务切换时间测试
  - [ ] 关键路径时序测试

- [ ] **performance/stress_test.c**
  - [ ] 高负载压力测试
  - [ ] 长时间运行测试
  - [ ] 极端条件测试
  - [ ] 资源耗尽测试

---

## 📊 测试覆盖率规划

### 代码覆盖率目标

```
核心模块 (src/core/)     - 目标: 90%+
├── main.c              - 85% (主循环难以完全测试)
├── system.c            - 95%
├── interrupts.c        - 90%
├── timer.c             - 95%
└── watchdog.c          - 85%

驱动模块 (src/drivers/)  - 目标: 80%+
├── gpio.c              - 85%
├── uart.c              - 90%
├── adc.c               - 85%
├── i2c.c               - 80%
├── spi.c               - 80%
└── flash.c             - 75%

应用模块 (src/app/)      - 目标: 85%+
├── modbus.c            - 90%
├── sensor.c            - 85%
├── storage.c           - 80%
├── alarm.c             - 85%
└── config.c            - 90%
```

### 功能覆盖率目标

- [ ] **核心功能**: 100% 覆盖

  - [ ] Modbus 通信基本功能码
  - [ ] 传感器数据采集
  - [ ] 数据存储读写
  - [ ] 报警检测

- [ ] **扩展功能**: 70% 覆盖

  - [ ] LoRa 通信 (如果启用)
  - [ ] OLED 显示 (如果启用)
  - [ ] 高级配置功能

- [ ] **错误处理**: 80% 覆盖
  - [ ] 通信错误恢复
  - [ ] 硬件故障处理
  - [ ] 内存不足处理

---

## 🧪 测试环境和工具

### 开发环境测试

- [ ] **编译环境测试**

  ```bash
  # 测试不同编译器版本
  gcc-arm-none-eabi-9
  gcc-arm-none-eabi-10
  gcc-arm-none-eabi-11

  # 测试不同优化级别
  -O0, -O1, -O2, -Os
  ```

- [ ] **静态分析工具**
  - [ ] Cppcheck 静态代码分析
  - [ ] PC-lint 代码规范检查
  - [ ] Clang 静态分析器
  - [ ] MISRA C 规范检查

### 硬件测试环境

- [ ] **测试硬件清单**

  - [ ] NANO100B 开发板
  - [ ] USB 转串口模块
  - [ ] 逻辑分析仪
  - [ ] 示波器
  - [ ] SHT2x 传感器模块
  - [ ] SPI Flash 模块

- [ ] **测试工具软件**
  - [ ] Modbus Poll (PC 端测试工具)
  - [ ] 串口调试助手
  - [ ] 逻辑分析仪软件
  - [ ] Python 自动化测试脚本

---

## 🚀 开发优先级

1. **Week 13**: 测试框架建设 (最高优先级)
2. **Week 13-14**: 核心模块和驱动模块单元测试 (高优先级)
3. **Week 14-15**: 应用模块和集成测试 (中优先级)
4. **Week 15-16**: 硬件在环测试 (中优先级)
5. **Week 16**: 性能测试和测试报告 (低优先级)

## ✅ 完成标准

- [ ] 测试框架正常工作，可运行所有测试
- [ ] 核心功能单元测试覆盖率 >85%
- [ ] 集成测试通过率 >95%
- [ ] 硬件在环测试通过，功能验证正确
- [ ] 性能测试满足设计指标
- [ ] 生成完整的测试报告
- [ ] 发现的问题都有对应的修复
- [ ] 回归测试通过

---

**📅 最后更新**: 2025-03-28  
**🎯 模块负责人**: 测试工程师  
**⏱️ 预估工期**: 4 周 (Week 13-16)
