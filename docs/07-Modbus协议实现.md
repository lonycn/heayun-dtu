# Modbus 协议实现详解

## 1. 概述

本系统实现了完整的 Modbus RTU 协议栈，支持多路 UART 通信。系统既可作为 Modbus 从机响应上位机查询，也可作为主机查询下级设备。

## 2. 协议架构

### 2.1 支持的功能码

| 功能码 | 名称           | 描述                     | 实现状态 |
| ------ | -------------- | ------------------------ | -------- |
| 0x01   | 读线圈状态     | 读取离散输出状态         | ❌       |
| 0x02   | 读离散输入状态 | 读取离散输入状态         | ❌       |
| 0x03   | 读保持寄存器   | 读取参数配置寄存器       | ✅       |
| 0x04   | 读输入寄存器   | 读取实时数据寄存器       | ✅       |
| 0x05   | 写单个线圈     | 写单个离散输出           | ❌       |
| 0x06   | 写单个寄存器   | 写单个参数寄存器         | ✅       |
| 0x0F   | 写多个线圈     | 写多个离散输出           | ❌       |
| 0x10   | 写多个寄存器   | 写多个参数寄存器         | ✅       |
| 0x41   | 读历史数据     | 自定义：读取历史记录     | ✅       |
| 0x44   | 读分页数据     | 自定义：读取分页历史数据 | ✅       |

### 2.2 UART 端口分配

```c
// UART 端口功能分配
UART0 - 主通信端口 (Modbus RTU 从机/主机)
UART1 - LoRa 模块通信
UART2 - 电表/温控器通信 (Modbus RTU 主机)
UART3 - 备用通信/扩展功能
UART4 - LoRa 主机扩展
```

## 3. 协议常量定义

```c
// 协议状态定义
#define cRightStatus                 0x01    // 正确状态
#define cCrcWrong                    0x02    // CRC错误
#define cModbusOverTime              0x05    // 通信超时

// Modbus 功能码定义
#define cComandReadCoil               0x01    // 读线圈
#define cComandReadDiStatus           0x02    // 读离散输入状态
#define cComandReadHoldRegister       0x03    // 读保持寄存器
#define cComandReadDiRegister         0x04    // 读输入寄存器
#define cComandWriteSingleCoil        0x05    // 写单个线圈
#define cComandWriteMoreCoil          0x0f    // 写多个线圈
#define cComandWriteSingleUint        0x06    // 写单个寄存器
#define cComandWriteMoreUint          0x10    // 写多个寄存器
```

## 4. 主要功能实现

### 4.1 UART0 主通信端口

#### 4.1.1 协议解析主函数

```c
unsigned char CheckModbusRespond_uart0(unsigned char *pp)
{
    WatchdogReset();

    // 写单个寄存器 (0x06)
    if(pp[1] == cComandWriteSingleUint) {
        DealWithSingleRegWrite_06(pp);
        return(cRightStatus);
    }
    // 读保持寄存器 (0x03)
    else if(pp[1] == cComandReadHoldRegister) {
        DealWithMoreRegRead_03(pp);
        return(cRightStatus);
    }
    // 读输入寄存器 (0x04)
    else if(pp[1] == cComandReadDiRegister) {
        DealWithMoreDiRead_04(pp);
        return(cRightStatus);
    }
    // 写多个寄存器 (0x10)
    else if(pp[1] == 0x10) {
        DealWithMoreRegWrite_10(pp);
        return(cRightStatus);
    }
    // 读历史数据 (0x41)
    else if(pp[1] == 0x41) {
        DealWithHisRead(pp);
        return(cRightStatus);
    }
    // 读分页数据 (0x44)
    else if(pp[1] == 0x44) {
        DealWithPageRead_44(pp);
        return(cRightStatus);
    }
    // 功能码错误
    else {
        pp[0] = pp[1] + 0x80;
        pp[1] = 0x01;   // 功能码错误
        SendDataToBus1(pp, 2);
        return(cCrcWrong);
    }
}
```

### 4.2 读保持寄存器实现 (0x03)

```c
void DealWithMoreRegRead_03(unsigned char *SendTempBuffer)
{
    uint16_t TempCrc = 0;
    uint16_t i, k, j = 3;
    unsigned char SendTempBuffer1[300];

    // 复制请求数据
    for(i = 0; i < 8; i++)
        SendTempBuffer1[i] = SendTempBuffer[i];

    if(SendTempBuffer1[0] == DeviceNum) {
        // 解析起始地址
        TempCrc = SendTempBuffer[2] * 256 + SendTempBuffer[3];

        // 构造响应帧
        SendTempBuffer1[0] = DeviceNum;
        SendTempBuffer1[1] = 0x03;
        SendTempBuffer1[2] = SendTempBuffer1[5] * 2;   // 字节数

        if(SendTempBuffer1[5] <= ParaNum/2) {
            // 处理不同地址范围的参数
            if(TempCrc < cSaveDataFlag03D) {
                // 基础参数区域
                i = (unsigned char)TempCrc;
                for(k = i; k < i + SendTempBuffer1[2]/2; k++) {
                    if(k == 0) {
                        // 设备号特殊处理
                        SendTempBuffer1[j++] = ParaList[cDeviceNum*2];
                        SendTempBuffer1[j++] = ParaList[cDeviceNum*2+1];
                    } else {
                        SendTempBuffer1[j++] = ParaList[k*2];
                        SendTempBuffer1[j++] = ParaList[k*2+1];
                    }
                }
            }
            else if(TempCrc >= cTempOff) {
                // 温度偏移参数区域
                TempCrc -= c02D_ParaActual;
                i = (unsigned char)TempCrc;
                for(k = i; k < i + SendTempBuffer1[2]/2; k++) {
                    SendTempBuffer1[j++] = ParaList[k*2];
                    SendTempBuffer1[j++] = ParaList[k*2+1];
                }
            }
            else if(TempCrc >= cSaveDataFlag03D) {
                // 保存数据标志区域
                TempCrc -= cParaActual;
                i = (unsigned char)TempCrc;
                for(k = i; k < i + SendTempBuffer1[2]/2; k++) {
                    SendTempBuffer1[j++] = ParaList[k*2];
                    SendTempBuffer1[j++] = ParaList[k*2+1];
                }
            }

            // 计算并添加 CRC
            TempCrc = CRC(SendTempBuffer1, j);
            SendTempBuffer1[j++] = TempCrc / 256;  // CRC 高字节
            SendTempBuffer1[j++] = TempCrc % 256;  // CRC 低字节
            SendDataToBus1(SendTempBuffer1, j);
        }
    }
    // LoRa 节点参数读取处理
    else {
        if((SendTempBuffer[0] >= AddrStart) &&
           (SendTempBuffer[0] < AddrStart + AddrLen) &&
           (ParaList[(cLoraNodeAliveSet + SendTempBuffer[0] - AddrStart)*2+1] == 1)) {

            TempCrc = SendTempBuffer[2] * 256 + SendTempBuffer[3];
            SendTempBuffer1[0] = SendTempBuffer[0];
            SendTempBuffer1[1] = 0x03;
            SendTempBuffer1[2] = SendTempBuffer1[5] * 2;

            i = (unsigned char)TempCrc;
            for(k = i; k < i + SendTempBuffer1[2]/2; k++) {
                // LoRa 节点参数偏移计算
                SendTempBuffer1[j++] = ParaList[(k-cTempOffset)*2 +
                    (SendTempBuffer[0]-AddrStart)*4 + cLoraNodeOffset*2];
                SendTempBuffer1[j++] = ParaList[(k-cTempOffset)*2 +
                    (SendTempBuffer[0]-AddrStart)*4 + cLoraNodeOffset*2+1];
            }

            TempCrc = CRC(SendTempBuffer1, j);
            SendTempBuffer1[j++] = TempCrc / 256;
            SendTempBuffer1[j++] = TempCrc % 256;
            SendDataToBus1(SendTempBuffer1, j);
        }
    }
}
```

### 4.3 读输入寄存器实现 (0x04)

```c
void DealWithMoreDiRead_04(unsigned char *SendTempBuffer)
{
    uint16_t TempCrc = 0;
    unsigned char i, k, j = 3;

    WatchdogReset();

    // 解析起始地址
    TempCrc = SendTempBuffer[2] * 256 + SendTempBuffer[3];

    // 构造响应帧
    SendTempBuffer[0] = SendTempBuffer[0];
    SendTempBuffer[1] = 0x04;
    SendTempBuffer[2] = SendTempBuffer[5] * 2;   // 字节数

    // 地址映射处理
    if(TempCrc < cModifiedTime) {
        if((SendTempBuffer[0] != DeviceNum) &&
           ((SendTempBuffer[5] == 3) || (SendTempBuffer[5] == 2)) &&
           (ParaList[(cLoraNodeAliveSet + SendTempBuffer[0] - AddrStart)*2+1] == 1)) {
            // LoRa 网关模式
            TempCrc += cTempStartAddr + (SendTempBuffer[0] - AddrStart) * 2;
        } else {
            TempCrc += cTemp;
        }
    }
    else if(TempCrc >= cRealData) {
        // 03D 实时数据
        TempCrc = TempCrc - cRealData + cRealDataAct;
    }
    else if((TempCrc >= cModifiedTime) && (TempCrc < cTempStartAddr)) {
        // 03D 时间数据
        TempCrc -= cModifiedTime;
        TempCrc += 2;
    }

    i = (unsigned char)TempCrc;

    // 填充数据
    for(k = i; k < i + SendTempBuffer[2]/2; k++) {
        WatchdogReset();
        SendTempBuffer[j++] = VarList[k*2];
        SendTempBuffer[j++] = VarList[k*2+1];
    }

    // 计算并添加 CRC
    TempCrc = CRC(SendTempBuffer, j);
    SendTempBuffer[j++] = TempCrc / 256;
    SendTempBuffer[j++] = TempCrc % 256;
    SendDataToBus1(SendTempBuffer, j);
}
```

### 4.4 写单个寄存器实现 (0x06)

```c
void DealWithSingleRegWrite_06(unsigned char *SendTempBuffer)
{
    uint16_t i, j;
    uint16_t TempCrc = 0;
    int16_t TEmpInter1, TEmpInter2;
    int8_t SetT[7];

    // 回显原始帧
    for(i = 0; i < 8; i++)
        SendTempBuffer[i] = SendTempBuffer[i];

    // 解析寄存器地址
    TempCrc = SendTempBuffer[2] * 256 + SendTempBuffer[3];
    i = TempCrc;

    if(i < 2010) {
        // 温度偏移值设定
        if((i == cTempOffset) || (i == cTempOffset+1)) {
            #ifdef _Loramain
            if((SendTempBuffer[0] >= AddrStart) &&
               (SendTempBuffer[0] < AddrStart + AddrLen) &&
               (ParaList[(cLoraNodeAliveSet + SendTempBuffer[0] - AddrStart)*2+1] == 1)) {
                // LoRa 节点参数设置
                ParaList[(i-cTempOffset)*2 + (SendTempBuffer[0]-AddrStart)*4 +
                    cLoraNodeOffset*2] = SendTempBuffer[4];
                ParaList[(i-cTempOffset)*2 + (SendTempBuffer[0]-AddrStart)*4 +
                    cLoraNodeOffset*2+1] = SendTempBuffer[5];
                Program();  // 保存到 Flash
            }
            #else
            ParaList[i*2] = SendTempBuffer[4];
            ParaList[i*2+1] = SendTempBuffer[5];
            Program();
            #endif
        }
        // 内部温湿度偏移
        else if((i == cInterTOffet) || (i == cInterHROffet)) {
            if(i == cInterTOffet) {
                if(ParaList[cDoorVar*2+1] == 78) {
                    j = SendTempBuffer[4] * 256 + SendTempBuffer[5];
                    if((j < 300) || (j > 65236)) {
                        ParaList[i*2] = SendTempBuffer[4];
                        ParaList[i*2+1] = SendTempBuffer[5];
                        Program();
                    }
                }
            }
            // 湿度偏移处理类似...
        }

        // 复位功能 (watchdog 248 写入 78)
        if(i == 248) {
            if(SendTempBuffer[5] == 78) {
                // 执行软件复位
                NVIC_SystemReset();
            }
        }

        // 高低压检测功能开关 (变量 249)
        if(i == cUseDi) {
            ParaList[i*2] = SendTempBuffer[4];
            ParaList[i*2+1] = SendTempBuffer[5];
            Program();

            // 更新高低压检测状态
            if(ParaList[cUseDi*2+1] == 0x01) {
                uart0_Var_List[(AlarmStartAddr+9)*2+1] = 0;
                uart0_Var_List[(AlarmStartAddr+9)*2] = 0;
            } else {
                uart0_Var_List[(AlarmStartAddr+9)*2+1] = 2;  // 不使用
                uart0_Var_List[(AlarmStartAddr+9)*2] = 0;
            }
        }
    }

    // 计算并发送 CRC
    TempCrc = CRC(SendTempBuffer, 6);
    SendTempBuffer[6] = TempCrc / 256;
    SendTempBuffer[7] = TempCrc % 256;
    SendDataToBus1(SendTempBuffer, 8);
}
```

## 5. 历史数据协议

### 5.1 读历史数据 (0x41)

```c
void DealWithHisRead(unsigned char *SendTempBuffer)
{
    // 历史数据读取实现
    // 支持按时间范围查询历史记录
    // 返回格式：时间戳 + 温湿度数据 + 报警状态
}
```

### 5.2 读分页数据 (0x44)

```c
void DealWithPageRead_44(unsigned char *SendTempBuffer)
{
    unsigned char TemBuffer[10];
    uint16_t page;

    // 解析页号
    page = SendTempBuffer[4] * 256 + SendTempBuffer[5];

    if(SendTempBuffer[0] == DeviceNum) {
        if(SendTempBuffer[3] == 0x65) {
            // 生成指定页数据
            DealwithPage(page);
            DealWithPageRead_44_1(SendTempBuffer, 1);
        } else {
            DealWithPageRead_44_1(SendTempBuffer, 0);
        }
    }
}
```

## 6. CRC 校验实现

```c
uint16_t CRC(unsigned char *data, unsigned char length)
{
    uint16_t crc = 0xFFFF;
    unsigned char i, j;

    for(i = 0; i < length; i++) {
        crc ^= data[i];
        for(j = 0; j < 8; j++) {
            if(crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
```

## 7. 地址映射

### 7.1 参数寄存器映射 (0x03 功能码)

| 地址范围  | 描述         | 备注           |
| --------- | ------------ | -------------- |
| 0-999     | 基础参数     | 设备配置参数   |
| 1000-1999 | 温度偏移参数 | 传感器校准参数 |
| 2000-2999 | 扩展参数     | 高级功能配置   |
| 3000+     | 保存标志     | 参数持久化控制 |

### 7.2 数据寄存器映射 (0x04 功能码)

| 地址范围 | 描述       | 数据类型              |
| -------- | ---------- | --------------------- |
| 0-99     | 实时温湿度 | int16_t, 0.1°C/0.1%RH |
| 100-199  | 系统状态   | 报警、通信状态等      |
| 200-299  | 电压电流   | int16_t, 0.1V/0.01A   |
| 300-399  | 时间数据   | BCD 格式时间          |
| 400+     | 扩展数据   | LoRa 节点数据等       |

## 8. 错误处理

### 8.1 异常响应

```c
// 功能码不支持
if(function_code_not_supported) {
    response[0] = device_address;
    response[1] = request[1] | 0x80;  // 异常响应标志
    response[2] = 0x01;               // 非法功能码
    send_response(response, 3);
}

// 地址超出范围
if(address_out_of_range) {
    response[0] = device_address;
    response[1] = request[1] | 0x80;
    response[2] = 0x02;               // 非法数据地址
    send_response(response, 3);
}

// 数据值超出范围
if(data_value_out_of_range) {
    response[0] = device_address;
    response[1] = request[1] | 0x80;
    response[2] = 0x03;               // 非法数据值
    send_response(response, 3);
}
```

### 8.2 通信超时处理

```c
void ModbusTimeoutHandler(void)
{
    if(modbus_timeout_counter > MODBUS_TIMEOUT_LIMIT) {
        // 清空接收缓冲区
        uart_rx_buffer_clear();

        // 重置状态机
        modbus_state = MODBUS_IDLE;

        // 设置通信故障标志
        communication_error_flag = 1;

        modbus_timeout_counter = 0;
    }
}
```

## 9. 多 UART 支持

### 9.1 UART 配置

```c
// UART0 配置 - 主通信端口
void UART0_Modbus_Init(void)
{
    UART_Open(UART0, 9600);
    UART_ENABLE_INT(UART0, UART_IER_RDA_IEN);
    NVIC_EnableIRQ(UART0_IRQn);
}

// UART2 配置 - 电表通信
void UART2_Modbus_Init(void)
{
    UART_Open(UART2, 9600);
    UART_ENABLE_INT(UART2, UART_IER_RDA_IEN);
    NVIC_EnableIRQ(UART2_IRQn);
}
```

### 9.2 中断处理

```c
void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_ISR_RDA_INT)) {
        uart0_rx_buffer[uart0_rx_index++] = UART_READ(UART0);
        uart0_rx_timeout = 0;

        if(uart0_rx_index >= UART_BUFFER_SIZE) {
            uart0_rx_index = 0;  // 防止溢出
        }
    }
}
```

## 10. 性能优化

### 10.1 缓冲区管理

```c
// 循环缓冲区实现
typedef struct {
    unsigned char buffer[UART_BUFFER_SIZE];
    unsigned char head;
    unsigned char tail;
    unsigned char count;
} uart_buffer_t;

// 缓冲区操作
unsigned char uart_buffer_put(uart_buffer_t *buf, unsigned char data)
{
    if(buf->count >= UART_BUFFER_SIZE) {
        return 0;  // 缓冲区满
    }

    buf->buffer[buf->head] = data;
    buf->head = (buf->head + 1) % UART_BUFFER_SIZE;
    buf->count++;

    return 1;
}
```

### 10.2 看门狗集成

```c
void WatchdogReset(void)
{
    // 在长时间操作中定期调用
    WDT_RESET_COUNTER();
}
```

## 11. 调试和测试

### 11.1 协议调试

```c
#ifdef DEBUG_MODBUS
void debug_print_frame(unsigned char *frame, unsigned char length)
{
    printf("Modbus Frame: ");
    for(int i = 0; i < length; i++) {
        printf("%02X ", frame[i]);
    }
    printf("\r\n");
}
#endif
```

### 11.2 测试用例

```bash
# 读取设备号 (地址 0)
01 03 00 00 00 01 84 0A

# 写入设备号为 5
01 06 00 00 00 05 89 C9

# 读取温度数据 (地址 11-12)
01 04 00 0B 00 02 F1 C8

# 读取历史数据
01 41 00 00 00 10 XX XX
```

## 12. 协议扩展

### 12.1 自定义功能码

系统实现了两个自定义功能码：

- **0x41**: 历史数据读取
- **0x44**: 分页数据读取

这些扩展保持了 Modbus 协议的基本格式，同时增加了特定应用功能。

### 12.2 LoRa 网关支持

通过地址映射机制，系统支持 LoRa 网关模式，可以透传访问远程节点的参数和数据。

## 13. 注意事项

1. **地址冲突**: 不同功能的地址映射需要避免重叠
2. **数据一致性**: 多 UART 访问共享数据时需要注意同步
3. **超时处理**: 合理设置通信超时避免死锁
4. **缓冲区溢出**: 实现循环缓冲区防止数据丢失
5. **看门狗**: 在长时间操作中及时喂狗

这个 Modbus 协议实现为系统提供了标准化的通信接口，支持多种设备集成和远程监控功能。
