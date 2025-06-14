#include "lora.h"
#include "system.h"
#include <stdio.h>
#include <string.h>

//==============================================================================
// 全局变量
//==============================================================================

static lora_config_t g_lora_config;
static lora_status_t g_lora_status = LORA_STATUS_IDLE;
static bool g_lora_initialized = false;
static uint32_t g_last_heartbeat_time = 0;

//==============================================================================
// 核心API实现
//==============================================================================

int lora_init(const lora_config_t *config)
{
    if (g_lora_initialized)
    {
        return LORA_OK; // 已经初始化
    }

    // 复制配置
    if (config)
    {
        g_lora_config = *config;
    }
    else
    {
        // 使用默认配置
        g_lora_config.frequency = 433000000; // 433MHz
        g_lora_config.bandwidth = 125;       // 125kHz
        g_lora_config.spreading_factor = 7;  // SF7
        g_lora_config.coding_rate = 5;       // 4/5
        g_lora_config.tx_power = 14;         // 14dBm
        g_lora_config.sync_word = 0x12;      // 同步字
    }

    // 初始化硬件
    // TODO: 实际的LoRa硬件初始化代码

    g_lora_status = LORA_STATUS_IDLE;
    g_lora_initialized = true;

    printf("LoRa: 模块初始化完成，频率 %u Hz\n", g_lora_config.frequency);
    return LORA_OK;
}

lora_status_t lora_get_status(void)
{
    return g_lora_status;
}

int lora_send_heartbeat(void)
{
    if (!g_lora_initialized)
    {
        return LORA_ERROR_NOT_INITIALIZED;
    }

    uint32_t current_time = system_get_tick();

    // 检查心跳间隔（每30秒发送一次）
    if (current_time - g_last_heartbeat_time < 30000)
    {
        return LORA_OK; // 跳过本次心跳
    }

    // 构建心跳数据包
    uint8_t heartbeat_data[16];
    heartbeat_data[0] = 0xAA; // 帧头
    heartbeat_data[1] = 0x55; // 帧头
    heartbeat_data[2] = 0x01; // 心跳包类型
    heartbeat_data[3] = 0x0C; // 数据长度

    // 设备ID (4字节)
    heartbeat_data[4] = 0x12;
    heartbeat_data[5] = 0x34;
    heartbeat_data[6] = 0x56;
    heartbeat_data[7] = 0x78;

    // 时间戳 (4字节)
    heartbeat_data[8] = (current_time >> 24) & 0xFF;
    heartbeat_data[9] = (current_time >> 16) & 0xFF;
    heartbeat_data[10] = (current_time >> 8) & 0xFF;
    heartbeat_data[11] = current_time & 0xFF;

    // 状态信息 (4字节)
    heartbeat_data[12] = (uint8_t)g_lora_status;
    heartbeat_data[13] = g_lora_config.tx_power;
    heartbeat_data[14] = 0x00; // 保留
    heartbeat_data[15] = 0x00; // 保留

    // 发送心跳包
    printf("LoRa: 发送心跳包\n");
    g_last_heartbeat_time = current_time;

    return LORA_OK;
}

int lora_receive_packet(lora_rx_info_t *rx_info, uint32_t timeout_ms)
{
    if (!g_lora_initialized || !rx_info)
    {
        return LORA_ERROR_INVALID_PARAM;
    }

    // 简化的接收实现
    g_lora_status = LORA_STATUS_RX;

    // 模拟接收数据
    static uint8_t dummy_data[32] = {0xAA, 0x55, 0x02, 0x10,
                                     0x12, 0x34, 0x56, 0x78,
                                     0x01, 0x02, 0x03, 0x04};

    // 模拟接收到数据
    if (system_get_tick() % 10 == 0) // 10%概率接收到数据
    {
        rx_info->data = dummy_data;
        rx_info->length = 12;
        rx_info->rssi = -80;
        rx_info->snr = 8;

        printf("LoRa: 接收到数据包，长度 %d，RSSI %d dBm\n",
               rx_info->length, rx_info->rssi);

        g_lora_status = LORA_STATUS_IDLE;
        return LORA_OK;
    }

    g_lora_status = LORA_STATUS_IDLE;
    return LORA_ERROR_NO_DATA;
}