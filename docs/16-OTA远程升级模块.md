# OTA 远程升级模块设计文档

## 1. 模块概述

### 1.1 模块功能

OTA（Over-The-Air）远程升级模块提供固件的远程更新能力，支持通过多种通信方式（4G/LoRa/WiFi）进行固件下载、验证、安装和回滚，确保系统的可维护性和可扩展性。

### 1.2 在系统中的作用

- **远程维护**: 支持远程固件升级，降低维护成本
- **安全升级**: 提供固件验证、数字签名、回滚机制
- **多通道支持**: 兼容现有的多种通信接口
- **断点续传**: 支持网络中断后的续传功能
- **状态监控**: 提供详细的升级状态反馈

### 1.3 与其他模块的关系

```
OTA 远程升级模块
    ├─← 通信管理模块 (网络传输服务)
    ├─← 配置管理模块 (升级配置参数)
    ├─→ 数据管理模块 (升级日志存储)
    ├─→ 用户界面模块 (升级状态显示)
    ├─→ 报警管理模块 (升级异常报警)
    └─→ 系统服务模块 (系统重启控制)
```

## 2. 功能需求

### 2.1 核心功能模块

| 功能模块 | 描述                     | 优先级 |
| -------- | ------------------------ | ------ |
| 升级检查 | 检查是否有新固件版本     | P1     |
| 固件下载 | 从服务器下载固件包       | P1     |
| 安全验证 | 校验和验证、数字签名验证 | P1     |
| 固件安装 | 写入新固件到备用分区     | P1     |
| 启动切换 | 切换到新固件启动         | P1     |
| 回滚机制 | 升级失败时回滚到旧版本   | P1     |
| 断点续传 | 网络中断后继续下载       | P2     |
| 状态监控 | 升级进度和状态反馈       | P2     |

### 2.2 支持的传输协议

```c
// 支持的传输协议
typedef enum {
    OTA_TRANSPORT_HTTP = 0,     // HTTP/HTTPS协议
    OTA_TRANSPORT_MQTT,         // MQTT协议
    OTA_TRANSPORT_LORA,         // LoRa协议
    OTA_TRANSPORT_FTP,          // FTP协议
    OTA_TRANSPORT_COUNT
} ota_transport_type_t;
```

### 2.3 固件分区设计

```c
// Flash分区布局 (基于NANO100B 128KB)
#define BOOTLOADER_START        0x08000000  // Bootloader: 8KB
#define BOOTLOADER_SIZE         0x2000

#define APP1_START              0x08002000  // 主应用: 56KB
#define APP1_SIZE               0xE000

#define APP2_START              0x08010000  // 备份应用: 56KB
#define APP2_SIZE               0xE000

#define CONFIG_START            0x0801E000  // 配置区: 4KB
#define CONFIG_SIZE             0x1000

#define OTA_INFO_START          0x0801F000  // OTA信息: 4KB
#define OTA_INFO_SIZE           0x1000
```

### 2.4 性能要求

| 指标     | 要求     | 备注           |
| -------- | -------- | -------------- |
| 下载速度 | 1-10KB/s | 取决于网络条件 |
| 校验时间 | <30s     | 56KB 固件      |
| 安装时间 | <60s     | Flash 写入     |
| 重启时间 | <10s     | 系统重启       |
| 成功率   | >95%     | 正常网络条件   |

## 3. 接口设计

### 3.1 主要 API 接口

```c
// OTA模块初始化
int ota_init(const ota_config_t* config);
int ota_deinit(void);

// 升级管理接口
int ota_check_update(ota_info_t* info);
int ota_start_download(const ota_info_t* info, ota_progress_cb_t progress_cb, ota_state_cb_t state_cb);
int ota_pause_download(void);
int ota_resume_download(void);
int ota_cancel_update(void);
int ota_install_firmware(void);
int ota_rollback(void);

// 状态查询接口
ota_state_t ota_get_state(void);
int ota_get_progress(uint32_t* downloaded, uint32_t* total);
int ota_get_version_info(ota_version_info_t* info);

// 回调注册接口
int ota_register_progress_callback(ota_progress_cb_t callback);
int ota_register_state_callback(ota_state_cb_t callback);
int ota_register_error_callback(ota_error_cb_t callback);
```

### 3.2 数据结构定义

```c
// OTA状态枚举
typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CHECKING,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_VERIFYING,
    OTA_STATE_INSTALLING,
    OTA_STATE_REBOOTING,
    OTA_STATE_COMPLETED,
    OTA_STATE_FAILED,
    OTA_STATE_ROLLBACK
} ota_state_t;

// OTA配置结构
typedef struct {
    ota_transport_type_t transport_type;
    char server_url[256];
    uint16_t server_port;
    char device_id[64];
    char api_key[128];
    uint32_t timeout_ms;
    uint32_t retry_count;
    uint32_t chunk_size;
    bool enable_signature_verify;
    bool enable_encryption;
    ota_checksum_type_t checksum_type;
} ota_config_t;

// 固件信息结构
typedef struct {
    char version[32];
    uint32_t size;
    char url[256];
    char checksum[64];
    char signature[512];
    uint32_t timestamp;
    char release_notes[256];
} ota_info_t;
```

## 4. 架构设计

### 4.1 内部架构图

```
┌─────────────────────────────────────────────────────────┐
│                  OTA 远程升级模块架构                    │
├─────────────────────────────────────────────────────────┤
│  应用接口层 (Public API)                                │
├─────────────────────────────────────────────────────────┤
│  升级管理层                                             │
│  ├─ 升级控制器   ├─ 状态管理器   ├─ 进度监控器       │
│  └─ 错误处理器   └─ 回滚管理器   └─ 版本管理器       │
├─────────────────────────────────────────────────────────┤
│  协议处理层                                             │
│  ├─ HTTP客户端   ├─ MQTT客户端   ├─ LoRa传输         │
│  └─ 断点续传     └─ 重试机制     └─ 传输加密         │
├─────────────────────────────────────────────────────────┤
│  安全验证层                                             │
│  ├─ 校验和验证   ├─ 数字签名     ├─ 固件解密         │
│  └─ 完整性检查   └─ 版本验证     └─ 权限验证         │
├─────────────────────────────────────────────────────────┤
│  存储管理层                                             │
│  ├─ Flash分区    ├─ 固件写入     ├─ 备份管理         │
│  └─ 磨损均衡     └─ 坏块管理     └─ 启动切换         │
├─────────────────────────────────────────────────────────┤
│  硬件抽象层                                             │
│  └─ Flash驱动    └─ 网络接口     └─ 看门狗控制       │
└─────────────────────────────────────────────────────────┘
```

### 4.2 关键实现算法

#### 4.2.1 固件下载算法

```c
int ota_download_firmware(const ota_info_t* info) {
    uint32_t downloaded = 0;
    uint32_t total_size = info->size;
    uint8_t buffer[OTA_CHUNK_SIZE];

    // 检查续传点
    downloaded = ota_get_download_progress();

    while (downloaded < total_size) {
        uint32_t chunk_size = MIN(OTA_CHUNK_SIZE, total_size - downloaded);

        // 设置HTTP Range头进行断点续传
        int ret = http_download_range(info->url, downloaded,
                                    downloaded + chunk_size - 1,
                                    buffer, chunk_size);
        if (ret < 0) {
            if (ret == HTTP_ERROR_TIMEOUT) {
                continue; // 超时重试
            } else {
                return OTA_ERROR_DOWNLOAD;
            }
        }

        // 写入Flash
        ret = ota_write_firmware_chunk(downloaded, buffer, chunk_size);
        if (ret < 0) {
            return OTA_ERROR_INSTALL;
        }

        downloaded += chunk_size;
        ota_update_progress(downloaded, total_size);

        // 检查是否被取消
        if (ota_is_cancelled()) {
            return OTA_ERROR_CANCELLED;
        }

        watchdog_feed(); // 喂狗
    }

    return OTA_SUCCESS;
}
```

#### 4.2.2 固件验证算法

```c
int ota_verify_firmware(const ota_info_t* info) {
    uint32_t calculated_crc = 0;
    uint8_t calculated_hash[32];
    uint8_t buffer[1024];

    // 1. CRC32校验
    uint32_t addr = APP2_START;
    uint32_t remaining = info->size;

    while (remaining > 0) {
        uint32_t read_size = MIN(sizeof(buffer), remaining);
        flash_read(addr, buffer, read_size);

        calculated_crc = crc32_update(calculated_crc, buffer, read_size);
        addr += read_size;
        remaining -= read_size;
    }

    // 比较CRC32
    uint32_t expected_crc = strtoul(info->checksum, NULL, 16);
    if (calculated_crc != expected_crc) {
        return OTA_ERROR_VERIFY;
    }

    // 2. SHA256校验（如果提供）
    if (strlen(info->signature) > 0) {
        sha256_context_t sha_ctx;
        sha256_init(&sha_ctx);

        addr = APP2_START;
        remaining = info->size;

        while (remaining > 0) {
            uint32_t read_size = MIN(sizeof(buffer), remaining);
            flash_read(addr, buffer, read_size);

            sha256_update(&sha_ctx, buffer, read_size);
            addr += read_size;
            remaining -= read_size;
        }

        sha256_final(&sha_ctx, calculated_hash);

        // 验证数字签名
        if (rsa_verify_signature(calculated_hash, sizeof(calculated_hash),
                                info->signature, strlen(info->signature)) != 0) {
            return OTA_ERROR_VERIFY;
        }
    }

    return OTA_SUCCESS;
}
```

## 5. 开发任务分解

### 5.1 阶段一：基础框架 (P1.1)

| 任务 | 描述                       | 估时 | 状态      | 依赖       |
| ---- | -------------------------- | ---- | --------- | ---------- |
| T1.1 | ⚫ 创建 OTA 模块基础框架   | 1d   | ⚫ 未开始 | 无         |
| T1.2 | ⚫ 设计 API 接口和数据结构 | 1.5d | ⚫ 未开始 | T1.1       |
| T1.3 | ⚫ 实现状态机管理          | 2d   | ⚫ 未开始 | T1.2       |
| T1.4 | ⚫ 实现 Flash 分区管理     | 2d   | ⚫ 未开始 | T1.2       |
| T1.5 | ⚫ 基础框架测试            | 1d   | ⚫ 未开始 | T1.3, T1.4 |

**里程碑**: OTA 基础框架可用

### 5.2 阶段二：网络下载 (P1.2)

| 任务 | 描述                    | 估时 | 状态      | 依赖       |
| ---- | ----------------------- | ---- | --------- | ---------- |
| T2.1 | ⚫ 实现 HTTP 下载客户端 | 3d   | ⚫ 未开始 | T1.5       |
| T2.2 | ⚫ 实现 MQTT 下载协议   | 2.5d | ⚫ 未开始 | T1.5       |
| T2.3 | ⚫ 实现断点续传功能     | 2d   | ⚫ 未开始 | T2.1       |
| T2.4 | ⚫ 实现重试和超时机制   | 1.5d | ⚫ 未开始 | T2.1, T2.2 |
| T2.5 | ⚫ 网络下载集成测试     | 1d   | ⚫ 未开始 | T2.1-T2.4  |

**里程碑**: 网络下载功能可用

### 5.3 阶段三：安全验证 (P1.3)

| 任务 | 描述                     | 估时 | 状态      | 依赖      |
| ---- | ------------------------ | ---- | --------- | --------- |
| T3.1 | ⚫ 实现 CRC32 校验算法   | 1.5d | ⚫ 未开始 | T2.5      |
| T3.2 | ⚫ 实现 SHA256 校验算法  | 2d   | ⚫ 未开始 | T2.5      |
| T3.3 | ⚫ 实现 RSA 数字签名验证 | 2.5d | ⚫ 未开始 | T3.2      |
| T3.4 | ⚫ 实现固件加密解密      | 2d   | ⚫ 未开始 | T3.3      |
| T3.5 | ⚫ 安全验证集成测试      | 1d   | ⚫ 未开始 | T3.1-T3.4 |

**里程碑**: 安全验证功能可用

### 5.4 阶段四：升级安装 (P1.4)

| 任务 | 描述                    | 估时 | 状态      | 依赖      |
| ---- | ----------------------- | ---- | --------- | --------- |
| T4.1 | ⚫ 实现 Bootloader 设计 | 3d   | ⚫ 未开始 | T3.5      |
| T4.2 | ⚫ 实现固件安装机制     | 2.5d | ⚫ 未开始 | T4.1      |
| T4.3 | ⚫ 实现回滚机制         | 2d   | ⚫ 未开始 | T4.2      |
| T4.4 | ⚫ 实现升级状态监控     | 1.5d | ⚫ 未开始 | T4.2      |
| T4.5 | ⚫ 完整升级流程测试     | 2d   | ⚫ 未开始 | T4.1-T4.4 |

**里程碑**: OTA 模块完整功能

### 5.5 总体进度跟踪

| 阶段          | 总任务数 | 已完成 | 进行中 | 未开始 | 完成率 |
| ------------- | -------- | ------ | ------ | ------ | ------ |
| P1.1 基础框架 | 5        | 0      | 0      | 5      | 0%     |
| P1.2 网络下载 | 5        | 0      | 0      | 5      | 0%     |
| P1.3 安全验证 | 5        | 0      | 0      | 5      | 0%     |
| P1.4 升级安装 | 5        | 0      | 0      | 5      | 0%     |
| **总计**      | **20**   | **0**  | **0**  | **20** | **0%** |

## 6. 测试计划

### 6.1 单元测试

| 测试项 | 描述                | 覆盖率要求 | 状态      |
| ------ | ------------------- | ---------- | --------- |
| UT1.1  | ⚫ 固件下载功能测试 | >90%       | ⚫ 未开始 |
| UT1.2  | ⚫ 校验算法测试     | >95%       | ⚫ 未开始 |
| UT1.3  | ⚫ 分区管理测试     | >90%       | ⚫ 未开始 |
| UT1.4  | ⚫ 状态机测试       | >85%       | ⚫ 未开始 |
| UT1.5  | ⚫ 错误处理测试     | >85%       | ⚫ 未开始 |

### 6.2 集成测试

| 测试项 | 描述                | 测试条件        | 状态      |
| ------ | ------------------- | --------------- | --------- |
| IT1.1  | ⚫ 完整升级流程测试 | 正常网络环境    | ⚫ 未开始 |
| IT1.2  | ⚫ 网络中断恢复测试 | 模拟网络中断    | ⚫ 未开始 |
| IT1.3  | ⚫ 掉电重启测试     | 升级过程中掉电  | ⚫ 未开始 |
| IT1.4  | ⚫ 固件损坏回滚测试 | 损坏固件安装    | ⚫ 未开始 |
| IT1.5  | ⚫ 大文件升级测试   | 接近 Flash 限制 | ⚫ 未开始 |

## 7. 风险控制

### 7.1 技术风险

| 风险           | 概率 | 影响 | 应对措施            | 状态      |
| -------------- | ---- | ---- | ------------------- | --------- |
| 升级过程中断电 | 中   | 高   | 实现原子性升级+回滚 | ⚫ 监控中 |
| 网络传输不稳定 | 高   | 中   | 断点续传+重试机制   | ⚫ 监控中 |
| 固件验证失败   | 低   | 高   | 多重校验+数字签名   | ⚫ 监控中 |
| Flash 写入失败 | 低   | 高   | 坏块管理+备份分区   | ⚫ 监控中 |

### 7.2 安全风险

| 风险         | 概率 | 影响 | 应对措施       | 状态      |
| ------------ | ---- | ---- | -------------- | --------- |
| 恶意固件注入 | 低   | 高   | RSA 签名验证   | ⚫ 监控中 |
| 中间人攻击   | 低   | 中   | HTTPS 加密传输 | ⚫ 监控中 |

## 8. 交付标准

### 8.1 功能验收标准

- ✅ 支持远程固件版本检查
- ✅ 支持多协议固件下载
- ✅ 支持断点续传功能
- ✅ 支持固件完整性验证
- ✅ 支持安全升级和回滚

### 8.2 性能验收标准

- ✅ 升级成功率>95%
- ✅ 下载速度 1-10KB/s
- ✅ 升级时间<10 分钟
- ✅ 回滚时间<30 秒

### 8.3 安全验收标准

- ✅ 通过校验和验证测试
- ✅ 通过数字签名验证测试
- ✅ 通过回滚机制测试
- ✅ 通过掉电恢复测试

---

**模块负责人**: [待分配]  
**预计开发时间**: 22 个工作日  
**创建时间**: 2024 年 12 月  
**最后更新**: 2024 年 12 月
