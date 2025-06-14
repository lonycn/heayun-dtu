/**
 * @file lora.h
 * @brief LoRa通信模块头文件
 * @version 1.0
 * @date 2025-01-14
 */

#ifndef __LORA_H__
#define __LORA_H__

#include <stdint.h>
#include <stdbool.h>

/* ================================ 错误码定义 ================================ */

#define LORA_OK 0
#define LORA_ERROR_INVALID_PARAM -1
#define LORA_ERROR_NOT_INITIALIZED -2
#define LORA_ERROR_HARDWARE -3
#define LORA_ERROR_TIMEOUT -4
#define LORA_ERROR_NO_DATA -5

/* ================================ 类型定义 ================================ */

/**
 * @brief LoRa状态枚举
 */
typedef enum
{
    LORA_STATUS_IDLE = 0,
    LORA_STATUS_TX,
    LORA_STATUS_RX,
    LORA_STATUS_ERROR
} lora_status_t;

/**
 * @brief LoRa配置结构
 */
typedef struct
{
    uint32_t frequency;       ///< 频率
    uint8_t spreading_factor; ///< 扩频因子
    uint8_t bandwidth;        ///< 带宽
    uint8_t coding_rate;      ///< 编码率
    uint8_t tx_power;         ///< 发射功率
    uint8_t sync_word;        ///< 同步字
} lora_config_t;

/**
 * @brief LoRa接收信息结构
 */
typedef struct
{
    uint8_t length; ///< 数据长度
    int16_t rssi;   ///< 信号强度
    int8_t snr;     ///< 信噪比
    uint8_t *data;  ///< 数据指针
} lora_rx_info_t;

/* ================================ 函数声明 ================================ */

/**
 * @brief 初始化LoRa模块
 * @param config 配置参数
 * @return 0:成功, <0:失败
 */
int lora_init(const lora_config_t *config);

/**
 * @brief 发送心跳包
 * @return 0:成功, <0:失败
 */
int lora_send_heartbeat(void);

/**
 * @brief 接收数据包
 * @param rx_info 接收信息结构
 * @param timeout_ms 超时时间(毫秒)
 * @return 0:成功, <0:失败
 */
int lora_receive_packet(lora_rx_info_t *rx_info, uint32_t timeout_ms);

/**
 * @brief 获取LoRa状态
 * @return LoRa状态
 */
lora_status_t lora_get_status(void);

#endif /* __LORA_H__ */