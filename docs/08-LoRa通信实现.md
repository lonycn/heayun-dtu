# 06-LoRa 通信实现

## 目录

1. [LoRa 系统概述](#1-lora-系统概述)
2. [LoRa 硬件配置](#2-lora-硬件配置)
3. [网关模式实现](#3-网关模式实现)
4. [节点模式实现](#4-节点模式实现)
5. [通信协议设计](#5-通信协议设计)
6. [数据传输机制](#6-数据传输机制)
7. [网络管理](#7-网络管理)
8. [故障处理机制](#8-故障处理机制)

## 1. LoRa 系统概述

### 1.1 系统架构

```
┌─────────────────┐    LoRa 无线    ┌─────────────────┐
│   LoRa 节点     │ <=============> │  LoRa 网关      │
│  (终端设备)     │     433MHz      │  (集中器)       │
│                 │                 │                 │
│ - 温湿度采集    │                 │ - 数据汇总      │
│ - 本地存储      │                 │ - 协议转换      │
│ - 低功耗运行    │                 │ - 远程通信      │
└─────────────────┘                 └─────────────────┘
                                            │
                                            │ Modbus/Ethernet
                                            ▼
                                    ┌─────────────────┐
                                    │   上位机系统    │
                                    │ (监控中心)      │
                                    └─────────────────┘
```

### 1.2 工作模式配置

| 模式         | 编译宏         | 功能描述       | 应用场景     |
| ------------ | -------------- | -------------- | ------------ |
| **网关模式** | `LoraGateway`  | 数据汇聚和转发 | 监控中心     |
| **节点模式** | `LoraNode`     | 数据采集和上报 | 分布式传感器 |
| **洪水监控** | `LW_flood`     | 专用洪水监测   | 环境监测     |
| **门磁监控** | `LW_DoorMagic` | 门状态监测     | 安防系统     |

### 1.3 LoRa 参数配置

```c
// LoRa 模块配置参数
typedef struct {
    uint32_t frequency;        // 工作频率 (433MHz)
    uint8_t  power;           // 发射功率 (0-7)
    uint8_t  bandwidth;       // 带宽设置
    uint8_t  spreading_factor; // 扩频因子 (7-12)
    uint8_t  coding_rate;     // 编码率
    uint16_t preamble_length; // 前导码长度
    uint8_t  sync_word;       // 同步字
} LoRa_Config_t;

// 默认配置
static const LoRa_Config_t default_lora_config = {
    .frequency = 433000000,    // 433MHz
    .power = 7,               // 最大功率
    .bandwidth = 125,         // 125kHz
    .spreading_factor = 12,   // SF12 (最大距离)
    .coding_rate = 5,         // 4/5 编码率
    .preamble_length = 8,     // 8 字节前导码
    .sync_word = 0x34         // 同步字
};
```

## 2. LoRa 硬件配置

### 2.1 硬件接口连接

#### 2.1.1 LoRa 模块接口

| 信号        | MCU 引脚 | LoRa 模块 | 功能       |
| ----------- | -------- | --------- | ---------- |
| **UART_TX** | PC11     | RXD       | 数据发送   |
| **UART_RX** | PC10     | TXD       | 数据接收   |
| **RESET**   | PA8      | RESET     | 模块复位   |
| **M0**      | PA9      | M0        | 模式控制 0 |
| **M1**      | PA10     | M1        | 模式控制 1 |
| **AUX**     | PA11     | AUX       | 状态指示   |

#### 2.1.2 双 LoRa 模块支持

```c
// 双 LoRa 模块配置
typedef struct {
    UART_T* uart;             // UART 接口
    GPIO_PIN reset_pin;       // 复位引脚
    GPIO_PIN m0_pin;          // M0 控制引脚
    GPIO_PIN m1_pin;          // M1 控制引脚
    GPIO_PIN aux_pin;         // AUX 状态引脚
    uint8_t  device_addr;     // 设备地址
    uint8_t  channel;         // 信道号
} LoRa_Module_t;

// LoRa 模块实例
static LoRa_Module_t lora_modules[2] = {
    // LoRa 模块 1 (UART1)
    {
        .uart = UART1,
        .reset_pin = PA8,
        .m0_pin = PA9,
        .m1_pin = PA10,
        .aux_pin = PA11,
        .device_addr = 0x0001,
        .channel = 23
    },
    // LoRa 模块 2 (UART3)
    {
        .uart = UART3,
        .reset_pin = PE8,
        .m0_pin = PE9,
        .m1_pin = PE10,
        .aux_pin = PE11,
        .device_addr = 0x0002,
        .channel = 24
    }
};
```

### 2.2 LoRa 模块初始化

#### 2.2.1 模块配置函数

```c
void LoRa_ModuleInit(uint8_t module_id)
{
    LoRa_Module_t* module = &lora_modules[module_id];

    // 硬件复位
    GPIO_SetBitValue(module->reset_pin, 0);
    delay1ms(10);
    GPIO_SetBitValue(module->reset_pin, 1);
    delay1ms(100);

    // 进入配置模式 (M0=1, M1=1)
    LoRa_SetMode(module, LORA_MODE_CONFIG);
    delay1ms(50);

    // 配置参数
    LoRa_SetAddress(module, module->device_addr);
    LoRa_SetChannel(module, module->channel);
    LoRa_SetAirRate(module, LORA_AIR_RATE_2400);

    // 退出配置模式
    LoRa_SetMode(module, LORA_MODE_NORMAL);
    delay1ms(50);
}

// 模式设置
void LoRa_SetMode(LoRa_Module_t* module, LoRa_Mode_t mode)
{
    switch(mode) {
        case LORA_MODE_NORMAL:      // 正常模式 (M0=0, M1=0)
            GPIO_SetBitValue(module->m0_pin, 0);
            GPIO_SetBitValue(module->m1_pin, 0);
            break;

        case LORA_MODE_WAKEUP:      // 唤醒模式 (M0=1, M1=0)
            GPIO_SetBitValue(module->m0_pin, 1);
            GPIO_SetBitValue(module->m1_pin, 0);
            break;

        case LORA_MODE_POWERDOWN:   // 休眠模式 (M0=0, M1=1)
            GPIO_SetBitValue(module->m0_pin, 0);
            GPIO_SetBitValue(module->m1_pin, 1);
            break;

        case LORA_MODE_CONFIG:      // 配置模式 (M0=1, M1=1)
            GPIO_SetBitValue(module->m0_pin, 1);
            GPIO_SetBitValue(module->m1_pin, 1);
            break;
    }
}
```

## 3. 网关模式实现

### 3.1 网关功能架构

#### 3.1.1 网关数据流

```
LoRa 节点数据 -> LoRa 接收 -> 数据解析 -> 协议转换 -> Modbus 输出
    ↑                                                        │
    └─── LoRa 发送 <- 命令转换 <- Modbus 接收 <---------------┘
```

#### 3.1.2 网关主要功能

```c
// 网关状态机
typedef enum {
    GATEWAY_STATE_IDLE = 0,        // 空闲状态
    GATEWAY_STATE_RECEIVING,       // 接收数据
    GATEWAY_STATE_PROCESSING,      // 处理数据
    GATEWAY_STATE_TRANSMITTING,    // 发送数据
    GATEWAY_STATE_ERROR           // 错误状态
} Gateway_State_t;

void LoRa_GatewayTask(void)
{
    static Gateway_State_t state = GATEWAY_STATE_IDLE;

    switch(state) {
        case GATEWAY_STATE_IDLE:
            // 检查是否有数据需要处理
            if(LoRa_HasIncomingData()) {
                state = GATEWAY_STATE_RECEIVING;
            } else if(Modbus_HasCommand()) {
                state = GATEWAY_STATE_TRANSMITTING;
            }
            break;

        case GATEWAY_STATE_RECEIVING:
            // 接收 LoRa 数据
            if(LoRa_ReceiveComplete()) {
                ProcessLoRaData();
                state = GATEWAY_STATE_PROCESSING;
            }
            break;

        case GATEWAY_STATE_PROCESSING:
            // 处理数据并更新 Modbus 寄存器
            UpdateModbusRegisters();
            state = GATEWAY_STATE_IDLE;
            break;

        case GATEWAY_STATE_TRANSMITTING:
            // 转发 Modbus 命令到 LoRa 节点
            ForwardCommandToNode();
            state = GATEWAY_STATE_IDLE;
            break;
    }
}
```

### 3.2 节点数据管理

#### 3.2.1 节点信息表

```c
// 节点信息结构
typedef struct {
    uint16_t node_id;              // 节点 ID
    uint8_t  node_status;          // 节点状态
    uint32_t last_update_time;     // 最后更新时间
    int16_t  temperature[4];       // 温度数据
    int16_t  humidity[4];          // 湿度数据
    uint16_t battery_level;        // 电池电量
    int8_t   signal_strength;      // 信号强度
    uint16_t error_count;          // 错误计数
} LoRa_Node_t;

// 节点管理表
#define MAX_LORA_NODES  32
static LoRa_Node_t lora_nodes[MAX_LORA_NODES];

// 节点数据更新
void UpdateNodeData(uint16_t node_id, LoRa_Packet_t* packet)
{
    LoRa_Node_t* node = FindNode(node_id);
    if(node == NULL) {
        node = AllocateNewNode(node_id);
    }

    if(node != NULL) {
        // 更新节点数据
        memcpy(node->temperature, packet->temperature, sizeof(node->temperature));
        memcpy(node->humidity, packet->humidity, sizeof(node->humidity));
        node->battery_level = packet->battery_level;
        node->signal_strength = packet->rssi;
        node->last_update_time = GetCurrentTime();
        node->node_status = NODE_STATUS_ONLINE;

        // 更新到 Modbus 寄存器
        UpdateModbusFromNode(node);
    }
}
```

### 3.3 协议转换

#### 3.3.1 LoRa 到 Modbus 转换

```c
void UpdateModbusFromNode(LoRa_Node_t* node)
{
    uint16_t base_addr = (node->node_id - 1) * 16;  // 每个节点 16 个寄存器

    // 温度数据 (寄存器 0-3)
    for(int i = 0; i < 4; i++) {
        uart0_Var_List[base_addr + i] = node->temperature[i];
    }

    // 湿度数据 (寄存器 4-7)
    for(int i = 0; i < 4; i++) {
        uart0_Var_List[base_addr + 4 + i] = node->humidity[i];
    }

    // 节点状态 (寄存器 8-11)
    uart0_Var_List[base_addr + 8] = node->node_status;
    uart0_Var_List[base_addr + 9] = node->battery_level;
    uart0_Var_List[base_addr + 10] = (int16_t)node->signal_strength;
    uart0_Var_List[base_addr + 11] = node->error_count;
}
```

## 4. 节点模式实现

### 4.1 节点功能架构

#### 4.1.1 节点工作流程

```c
void LoRa_NodeTask(void)
{
    static uint32_t last_report_time = 0;
    static uint8_t sleep_counter = 0;

    // 检查是否需要上报数据
    if(GetCurrentTime() - last_report_time >= REPORT_INTERVAL) {

        // 采集传感器数据
        CollectSensorData();

        // 组织数据包
        LoRa_Packet_t packet;
        PrepareDataPacket(&packet);

        // 发送数据
        LoRa_SendPacket(&packet);

        last_report_time = GetCurrentTime();
        sleep_counter = 0;
    }

    // 检查网关命令
    if(LoRa_HasIncomingCommand()) {
        ProcessGatewayCommand();
    }

    // 低功耗管理
    sleep_counter++;
    if(sleep_counter >= SLEEP_THRESHOLD) {
        Enter_LoRa_SleepMode();
    }
}
```

### 4.2 数据采集和上报

#### 4.2.1 数据包格式

```c
// LoRa 数据包结构
typedef struct {
    uint8_t  packet_type;          // 包类型
    uint16_t node_id;              // 节点 ID
    uint8_t  sequence;             // 序列号
    uint32_t timestamp;            // 时间戳
    int16_t  temperature[4];       // 温度数据
    int16_t  humidity[4];          // 湿度数据
    uint16_t battery_level;        // 电池电量
    uint8_t  alarm_status;         // 报警状态
    uint16_t crc;                  // CRC 校验
} __attribute__((packed)) LoRa_Packet_t;

// 数据包类型定义
#define PACKET_TYPE_DATA       0x01    // 数据上报
#define PACKET_TYPE_HEARTBEAT  0x02    // 心跳包
#define PACKET_TYPE_ALARM      0x03    // 报警包
#define PACKET_TYPE_CONFIG     0x04    // 配置包
#define PACKET_TYPE_ACK        0x05    // 确认包
```

#### 4.2.2 数据包发送

```c
void PrepareDataPacket(LoRa_Packet_t* packet)
{
    static uint8_t sequence = 0;

    // 包头信息
    packet->packet_type = PACKET_TYPE_DATA;
    packet->node_id = GetNodeID();
    packet->sequence = sequence++;
    packet->timestamp = GetCurrentTime();

    // 传感器数据
    for(int i = 0; i < 4; i++) {
        packet->temperature[i] = GetTemperature(i);
        packet->humidity[i] = GetHumidity(i);
    }

    // 系统状态
    packet->battery_level = GetBatteryLevel();
    packet->alarm_status = GetAlarmStatus();

    // CRC 校验
    packet->crc = CalculateCRC((uint8_t*)packet,
                               sizeof(LoRa_Packet_t) - sizeof(uint16_t));
}

bool LoRa_SendPacket(LoRa_Packet_t* packet)
{
    uint8_t retry_count = 0;
    bool send_success = false;

    while(retry_count < MAX_RETRY_COUNT && !send_success) {

        // 等待模块就绪
        if(LoRa_WaitReady(1000)) {

            // 发送数据
            UART_Write(UART1, (uint8_t*)packet, sizeof(LoRa_Packet_t));

            // 等待发送完成确认
            if(LoRa_WaitSendComplete(2000)) {
                send_success = true;
            } else {
                retry_count++;
                delay1ms(100 * retry_count);  // 指数退避
            }
        }
    }

    return send_success;
}
```

## 5. 通信协议设计

### 5.1 协议栈架构

```
┌─────────────────────────────────────┐
│          应用层协议                 │  <- Modbus 映射, 业务逻辑
├─────────────────────────────────────┤
│          传输层协议                 │  <- 包序列号, 重传机制
├─────────────────────────────────────┤
│          网络层协议                 │  <- 节点路由, 网络管理
├─────────────────────────────────────┤
│          数据链路层                 │  <- 帧同步, 错误检测
├─────────────────────────────────────┤
│          物理层                     │  <- LoRa 无线传输
└─────────────────────────────────────┘
```

### 5.2 命令和响应机制

#### 5.2.1 命令类型定义

```c
// 命令类型
typedef enum {
    CMD_READ_DATA     = 0x01,      // 读取数据
    CMD_WRITE_CONFIG  = 0x02,      // 写入配置
    CMD_SET_TIME      = 0x03,      // 设置时间
    CMD_RESET_NODE    = 0x04,      // 复位节点
    CMD_READ_STATUS   = 0x05,      // 读取状态
    CMD_START_CALIBRATE = 0x06,    // 开始校准
    CMD_ENTER_SLEEP   = 0x07       // 进入睡眠
} LoRa_Command_t;

// 命令包结构
typedef struct {
    uint8_t  packet_type;          // 包类型 (CMD)
    uint16_t target_node;          // 目标节点
    uint8_t  command;              // 命令码
    uint8_t  data_length;          // 数据长度
    uint8_t  data[32];             // 命令数据
    uint16_t crc;                  // CRC 校验
} __attribute__((packed)) LoRa_Command_Packet_t;
```

#### 5.2.2 命令处理

```c
void ProcessGatewayCommand(void)
{
    LoRa_Command_Packet_t cmd_packet;

    if(LoRa_ReceiveCommand(&cmd_packet)) {

        // 验证目标节点
        if(cmd_packet.target_node == GetNodeID() ||
           cmd_packet.target_node == BROADCAST_ADDRESS) {

            LoRa_Response_Packet_t response;

            // 处理命令
            switch(cmd_packet.command) {
                case CMD_READ_DATA:
                    HandleReadData(&cmd_packet, &response);
                    break;

                case CMD_WRITE_CONFIG:
                    HandleWriteConfig(&cmd_packet, &response);
                    break;

                case CMD_SET_TIME:
                    HandleSetTime(&cmd_packet, &response);
                    break;

                case CMD_RESET_NODE:
                    HandleResetNode(&cmd_packet, &response);
                    break;

                default:
                    response.status = STATUS_UNKNOWN_COMMAND;
                    break;
            }

            // 发送响应
            LoRa_SendResponse(&response);
        }
    }
}
```

## 6. 数据传输机制

### 6.1 可靠传输机制

#### 6.1.1 重传机制

```c
// 重传配置
#define MAX_RETRY_COUNT    3
#define ACK_TIMEOUT_MS     2000
#define RETRY_INTERVAL_MS  500

typedef struct {
    uint8_t  sequence;             // 序列号
    uint8_t  retry_count;          // 重传次数
    uint32_t send_time;            // 发送时间
    uint8_t  ack_received;         // ACK 接收标志
    LoRa_Packet_t packet;          // 原始数据包
} TransmitBuffer_t;

bool LoRa_SendWithRetry(LoRa_Packet_t* packet)
{
    TransmitBuffer_t* tx_buf = AllocateTransmitBuffer();
    if(tx_buf == NULL) return false;

    // 初始化重传缓冲区
    tx_buf->sequence = packet->sequence;
    tx_buf->retry_count = 0;
    tx_buf->send_time = GetCurrentTime();
    tx_buf->ack_received = 0;
    memcpy(&tx_buf->packet, packet, sizeof(LoRa_Packet_t));

    // 发送数据包
    LoRa_PhysicalSend(packet);

    // 等待 ACK 或重传
    while(tx_buf->retry_count < MAX_RETRY_COUNT && !tx_buf->ack_received) {

        if(GetCurrentTime() - tx_buf->send_time > ACK_TIMEOUT_MS) {
            tx_buf->retry_count++;
            if(tx_buf->retry_count < MAX_RETRY_COUNT) {
                // 重传
                LoRa_PhysicalSend(&tx_buf->packet);
                tx_buf->send_time = GetCurrentTime();
            }
        }

        // 检查 ACK
        CheckAckReceived(tx_buf);
    }

    bool success = tx_buf->ack_received;
    FreeTransmitBuffer(tx_buf);

    return success;
}
```

### 6.2 流量控制

#### 6.2.1 自适应传输间隔

```c
typedef struct {
    uint32_t base_interval;        // 基础传输间隔
    uint32_t current_interval;     // 当前传输间隔
    uint8_t  success_count;        // 连续成功次数
    uint8_t  failure_count;        // 连续失败次数
    float    channel_quality;      // 信道质量评估
} FlowControl_t;

void AdaptTransmissionInterval(FlowControl_t* fc, bool transmission_success)
{
    if(transmission_success) {
        fc->success_count++;
        fc->failure_count = 0;

        // 连续成功，可以提高传输频率
        if(fc->success_count >= 5) {
            fc->current_interval = fc->current_interval * 0.9;
            if(fc->current_interval < fc->base_interval) {
                fc->current_interval = fc->base_interval;
            }
            fc->success_count = 0;
        }

    } else {
        fc->failure_count++;
        fc->success_count = 0;

        // 传输失败，降低传输频率
        fc->current_interval = fc->current_interval * (1.0 + 0.2 * fc->failure_count);
        if(fc->current_interval > fc->base_interval * 4) {
            fc->current_interval = fc->base_interval * 4;
        }
    }

    // 更新信道质量评估
    fc->channel_quality = (float)fc->success_count /
                         (fc->success_count + fc->failure_count + 1);
}
```

## 7. 网络管理

### 7.1 节点管理

#### 7.1.1 节点状态监控

```c
void LoRa_NetworkManager(void)
{
    static uint32_t last_check_time = 0;

    if(GetCurrentTime() - last_check_time >= NODE_CHECK_INTERVAL) {

        // 检查所有节点状态
        for(int i = 0; i < MAX_LORA_NODES; i++) {
            LoRa_Node_t* node = &lora_nodes[i];

            if(node->node_id != 0) {  // 有效节点

                uint32_t offline_time = GetCurrentTime() - node->last_update_time;

                if(offline_time > NODE_OFFLINE_TIMEOUT) {
                    // 节点离线
                    if(node->node_status != NODE_STATUS_OFFLINE) {
                        node->node_status = NODE_STATUS_OFFLINE;
                        LogNodeOffline(node->node_id);

                        // 清除节点数据
                        ClearNodeData(node);
                    }

                } else if(offline_time > NODE_WARNING_TIMEOUT) {
                    // 节点通信异常
                    node->node_status = NODE_STATUS_WARNING;
                }
            }
        }

        last_check_time = GetCurrentTime();
    }
}
```

### 7.2 网络拓扑管理

#### 7.2.1 信号强度监控

```c
void UpdateSignalQuality(uint16_t node_id, int8_t rssi, uint8_t snr)
{
    LoRa_Node_t* node = FindNode(node_id);
    if(node != NULL) {

        // 更新信号强度
        node->signal_strength = rssi;

        // 计算链路质量
        float link_quality = CalculateLinkQuality(rssi, snr);

        // 信号质量评级
        if(link_quality > 0.8) {
            node->link_status = LINK_EXCELLENT;
        } else if(link_quality > 0.6) {
            node->link_status = LINK_GOOD;
        } else if(link_quality > 0.4) {
            node->link_status = LINK_FAIR;
        } else {
            node->link_status = LINK_POOR;
        }

        // 根据链路质量调整参数
        if(node->link_status == LINK_POOR) {
            // 建议节点降低传输频率
            SendLinkQualityCommand(node_id, REDUCE_TX_RATE);
        }
    }
}
```

## 8. 故障处理机制

### 8.1 通信故障检测

#### 8.1.1 故障类型定义

```c
typedef enum {
    LORA_ERROR_NONE = 0,           // 无错误
    LORA_ERROR_TIMEOUT,            // 超时错误
    LORA_ERROR_CRC,                // CRC 校验错误
    LORA_ERROR_NO_ACK,             // 无应答
    LORA_ERROR_BUFFER_FULL,        // 缓冲区满
    LORA_ERROR_MODULE_FAULT,       // 模块故障
    LORA_ERROR_SIGNAL_WEAK         // 信号弱
} LoRa_Error_t;

// 错误统计
typedef struct {
    uint32_t total_packets;        // 总包数
    uint32_t success_packets;      // 成功包数
    uint32_t timeout_errors;       // 超时错误
    uint32_t crc_errors;           // CRC 错误
    uint32_t no_ack_errors;        // 无应答错误
    float    packet_loss_rate;     // 丢包率
} LoRa_ErrorStats_t;
```

#### 8.1.2 故障恢复机制

```c
void LoRa_FaultRecovery(LoRa_Error_t error_type)
{
    static uint8_t consecutive_errors = 0;

    consecutive_errors++;

    switch(error_type) {
        case LORA_ERROR_TIMEOUT:
        case LORA_ERROR_NO_ACK:
            if(consecutive_errors >= 3) {
                // 检查模块状态
                if(!LoRa_ModuleHealthCheck()) {
                    LoRa_ModuleReset();
                }
                consecutive_errors = 0;
            }
            break;

        case LORA_ERROR_MODULE_FAULT:
            // 立即重置模块
            LoRa_ModuleReset();
            consecutive_errors = 0;
            break;

        case LORA_ERROR_SIGNAL_WEAK:
            // 提高发射功率
            LoRa_IncreaseTxPower();
            break;

        default:
            break;
    }

    // 错误统计
    UpdateErrorStatistics(error_type);
}

bool LoRa_ModuleHealthCheck(void)
{
    // 发送测试命令
    uint8_t test_cmd[] = {0xC0, 0x00, 0x00, 0x01, 0x61};
    UART_Write(UART1, test_cmd, sizeof(test_cmd));

    // 等待响应
    uint8_t response[16];
    if(UART_Read(UART1, response, sizeof(response), 1000)) {
        // 检查响应内容
        if(response[0] == 0xC0 && response[3] == 0x01) {
            return true;  // 模块正常
        }
    }

    return false;  // 模块异常
}
```

### 8.2 系统级容错

#### 8.2.1 备用通信机制

```c
void LoRa_FallbackCommunication(void)
{
    static bool using_backup = false;

    // 检查主 LoRa 模块状态
    if(!LoRa_ModuleHealthCheck(0)) {  // 模块 0 故障

        if(!using_backup) {
            // 切换到备用模块
            LoRa_SwitchToBackup();
            using_backup = true;
            LogEvent("切换到备用 LoRa 模块");
        }

    } else if(using_backup) {
        // 主模块恢复，切换回主模块
        LoRa_SwitchToPrimary();
        using_backup = false;
        LogEvent("切换回主 LoRa 模块");
    }
}

void LoRa_SwitchToBackup(void)
{
    // 停用主模块
    LoRa_SetMode(&lora_modules[0], LORA_MODE_POWERDOWN);

    // 启用备用模块
    LoRa_ModuleInit(1);

    // 更新通信句柄
    current_lora_module = &lora_modules[1];
}
```

## 总结

本文档详细介绍了 LoRa 通信系统的完整实现：

### 关键特性

- **双模式支持**: 网关模式和节点模式灵活切换
- **可靠传输**: 重传机制、ACK 确认、CRC 校验
- **网络管理**: 节点状态监控、信号质量评估
- **故障恢复**: 自动重传、模块重置、备用切换

### 技术亮点

- **协议栈设计**: 分层架构，支持不同应用需求
- **自适应机制**: 根据信道质量调整传输参数
- **容错能力**: 多级故障检测和恢复机制
- **低功耗优化**: 节点休眠管理，延长电池寿命

### 扩展性

- **多节点支持**: 最多支持 32 个节点
- **协议扩展**: 预留自定义命令接口
- **模块兼容**: 支持不同厂商的 LoRa 模块
- **应用适配**: 可适配洪水监控、门磁监控等应用

该 LoRa 通信系统为分布式监控提供了可靠的无线通信解决方案。
