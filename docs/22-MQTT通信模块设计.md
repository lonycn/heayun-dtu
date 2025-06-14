# MQTT 通信模块设计文档

## 1. 模块概述

### 1.1 功能描述

MQTT 通信模块为憨云 DTU 提供轻量级的物联网通信能力，支持设备与云端平台的双向数据传输。

### 1.2 设计目标

- 实现完整的 MQTT 客户端协议栈
- 支持 JSON 格式的数据编解码
- 提供可靠的连接管理和重连机制
- 优化内存使用，适配嵌入式环境
- 支持多种 QoS 等级和消息类型

### 1.3 技术规格

- **协议版本**: MQTT 3.1.1
- **传输层**: TCP/IP
- **数据格式**: JSON
- **内存占用**: < 2KB
- **连接超时**: 可配置 (默认 30 秒)
- **心跳间隔**: 可配置 (默认 60 秒)

## 2. 架构设计

### 2.1 模块架构

```
MQTT通信模块
├── 连接管理层
│   ├── 连接建立/断开
│   ├── 心跳保活
│   └── 自动重连
├── 消息处理层
│   ├── 消息发布
│   ├── 消息订阅
│   └── 消息接收
├── 数据编码层
│   ├── JSON编码器
│   ├── JSON解码器
│   └── 数据验证
└── 事件管理层
    ├── 连接事件
    ├── 消息事件
    └── 错误事件
```

### 2.2 接口设计

#### 2.2.1 核心 API

```c
// 初始化和配置
int mqtt_init(const mqtt_config_t *config);
int mqtt_deinit(void);
int mqtt_set_event_callback(mqtt_event_callback_t callback);

// 连接管理
int mqtt_connect(void);
int mqtt_disconnect(void);
mqtt_state_t mqtt_get_state(void);
bool mqtt_is_connected(void);

// 消息发布
int mqtt_publish(const char *topic, const void *payload, uint16_t len, uint8_t qos, bool retain);
int mqtt_publish_json(const char *topic, const char *json_str, uint8_t qos);

// 消息订阅
int mqtt_subscribe(const char *topic, uint8_t qos);
int mqtt_unsubscribe(const char *topic);

// 任务处理
void mqtt_task(void);
int mqtt_ping(void);
```

#### 2.2.2 数据结构

```c
// MQTT配置结构
typedef struct {
    char broker_host[64];      // 服务器地址
    uint16_t broker_port;      // 服务器端口
    char client_id[32];        // 客户端ID
    char username[32];         // 用户名
    char password[32];         // 密码
    uint16_t keep_alive;       // 心跳间隔
    uint8_t clean_session;     // 清除会话
    uint16_t connect_timeout;  // 连接超时
} mqtt_config_t;

// MQTT状态枚举
typedef enum {
    MQTT_STATE_DISCONNECTED,  // 未连接
    MQTT_STATE_CONNECTING,    // 连接中
    MQTT_STATE_CONNECTED,     // 已连接
    MQTT_STATE_DISCONNECTING, // 断开中
    MQTT_STATE_ERROR          // 错误状态
} mqtt_state_t;

// MQTT事件结构
typedef struct {
    mqtt_event_type_t event;   // 事件类型
    void *data;                // 事件数据
    uint16_t data_len;         // 数据长度
    int error_code;            // 错误码
} mqtt_event_t;
```

### 2.3 JSON 数据格式

#### 2.3.1 传感器数据上报

```json
{
  "device_id": "HCK_DTU_001",
  "timestamp": 1640995200,
  "data": {
    "temperature": 25.6,
    "humidity": 65.2,
    "voltage": 3.3,
    "current": 0.15,
    "power": 0.495
  }
}
```

#### 2.3.2 设备状态上报

```json
{
  "device_id": "HCK_DTU_001",
  "timestamp": 1640995200,
  "status": {
    "modbus_online": true,
    "lora_connected": true,
    "storage_normal": true,
    "alarm_active": false,
    "uptime": 86400,
    "free_memory": 2048
  }
}
```

#### 2.3.3 报警信息上报

```json
{
  "device_id": "HCK_DTU_001",
  "timestamp": 1640995200,
  "alarm": {
    "type": "TEMPERATURE_HIGH",
    "level": "WARNING",
    "value": 85.5,
    "threshold": 80.0,
    "duration": 300
  }
}
```

### 2.4 主题设计

#### 2.4.1 上行主题 (设备 → 云端)

- `hck/dtu/{device_id}/data/sensor` - 传感器数据
- `hck/dtu/{device_id}/data/status` - 设备状态
- `hck/dtu/{device_id}/alarm` - 报警信息
- `hck/dtu/{device_id}/heartbeat` - 心跳消息

#### 2.4.2 下行主题 (云端 → 设备)

- `hck/dtu/{device_id}/cmd/config` - 配置命令
- `hck/dtu/{device_id}/cmd/control` - 控制命令
- `hck/dtu/{device_id}/cmd/ota` - OTA 升级
- `hck/dtu/{device_id}/response` - 响应消息

## 3. 实现细节

### 3.1 连接管理

#### 3.1.1 连接流程

1. 初始化 TCP 连接
2. 发送 CONNECT 报文
3. 等待 CONNACK 响应
4. 启动心跳定时器
5. 进入已连接状态

#### 3.1.2 重连机制

- 指数退避算法
- 最大重连次数限制
- 连接状态监控
- 网络异常检测

### 3.2 消息处理

#### 3.2.1 发布流程

1. 构造 PUBLISH 报文
2. 分配消息 ID (QoS > 0)
3. 发送消息
4. 等待确认 (QoS > 0)
5. 更新统计信息

#### 3.2.2 订阅管理

- 主题过滤器匹配
- QoS 等级协商
- 订阅状态维护
- 消息分发机制

### 3.3 JSON 编解码

#### 3.3.1 编码器特性

- 轻量级实现
- 内存优化
- 类型安全
- 错误处理

#### 3.3.2 解码器特性

- 流式解析
- 容错处理
- 数据验证
- 内存保护

## 4. 性能优化

### 4.1 内存优化

- 静态内存分配
- 缓冲区复用
- 数据结构紧凑
- 内存池管理

### 4.2 网络优化

- 消息合并发送
- 压缩算法支持
- 流量控制
- 超时管理

### 4.3 功耗优化

- 智能心跳调节
- 连接保持策略
- 数据缓存机制
- 睡眠模式支持

## 5. 错误处理

### 5.1 错误类型

- 网络连接错误
- 协议解析错误
- 内存分配错误
- 超时错误

### 5.2 错误恢复

- 自动重连
- 消息重发
- 状态重置
- 降级处理

## 6. 测试验证

### 6.1 单元测试

- API 接口测试
- 数据编解码测试
- 错误处理测试
- 边界条件测试

### 6.2 集成测试

- 端到端通信测试
- 网络异常测试
- 性能压力测试
- 兼容性测试

## 7. 使用示例

### 7.1 基本使用

```c
// 初始化配置
mqtt_config_t config = {
    .broker_host = "iot.example.com",
    .broker_port = 1883,
    .client_id = "HCK_DTU_001",
    .keep_alive = 60,
    .clean_session = 1
};

// 初始化MQTT模块
mqtt_init(&config);

// 设置事件回调
mqtt_set_event_callback(mqtt_event_handler);

// 连接服务器
mqtt_connect();

// 发布传感器数据
char json_data[256];
mqtt_encode_sensor_data(json_data, sizeof(json_data), &sensor_data);
mqtt_publish_json("hck/dtu/001/data/sensor", json_data, 1);

// 订阅控制命令
mqtt_subscribe("hck/dtu/001/cmd/+", 1);
```

### 7.2 事件处理

```c
void mqtt_event_handler(const mqtt_event_t *event)
{
    switch (event->event)
    {
    case MQTT_EVENT_CONNECTED:
        printf("MQTT连接成功\n");
        break;

    case MQTT_EVENT_DISCONNECTED:
        printf("MQTT连接断开\n");
        break;

    case MQTT_EVENT_MESSAGE_RECEIVED:
        printf("收到消息: %.*s\n", event->data_len, (char*)event->data);
        break;

    case MQTT_EVENT_ERROR:
        printf("MQTT错误: %d\n", event->error_code);
        break;
    }
}
```

## 8. 总结

MQTT 通信模块为憨云 DTU 提供了可靠的物联网通信能力，具有以下特点：

1. **轻量级设计**: 针对嵌入式环境优化，内存占用小
2. **功能完整**: 支持完整的 MQTT 协议和 JSON 数据格式
3. **可靠性高**: 具备完善的错误处理和重连机制
4. **易于使用**: 提供简洁的 API 接口和丰富的示例
5. **扩展性强**: 支持多种 QoS 等级和自定义主题

该模块已成功集成到憨云 DTU 系统中，为设备的远程监控和管理提供了强有力的支持。

---

**文档版本**: v1.0  
**创建日期**: 2025 年 3 月 28 日  
**适用版本**: 憨云 DTU v2.0+
