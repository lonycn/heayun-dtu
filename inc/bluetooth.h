/**
 * @file bluetooth.h
 * @brief 憨云DTU - 蓝牙BLE通信模块接口定义
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 蓝牙BLE通信模块，支持BLE 4.0/5.0协议栈
 * 提供设备配对、数据传输、移动APP通信功能
 */

#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //==============================================================================
    // 宏定义
    //==============================================================================

#define BLE_MAX_DEVICE_NAME_LEN 32 // 设备名称最大长度
#define BLE_MAX_MAC_ADDR_LEN 18    // MAC地址最大长度
#define BLE_MAX_UUID_LEN 37        // UUID最大长度
#define BLE_MAX_DATA_LEN 244       // 单次传输最大数据长度
#define BLE_MAX_DEVICES 8          // 最大连接设备数
#define BLE_MAX_SERVICES 4         // 最大服务数
#define BLE_MAX_CHARACTERISTICS 8  // 最大特征数

#define BLE_SCAN_TIMEOUT_MS 10000   // 扫描超时时间
#define BLE_CONNECT_TIMEOUT_MS 5000 // 连接超时时间
#define BLE_PAIR_TIMEOUT_MS 30000   // 配对超时时间

    //==============================================================================
    // 枚举定义
    //==============================================================================

    /**
     * @brief 蓝牙状态
     */
    typedef enum
    {
        BLE_STATE_OFF = 0,      // 关闭状态
        BLE_STATE_INITIALIZING, // 初始化中
        BLE_STATE_IDLE,         // 空闲状态
        BLE_STATE_SCANNING,     // 扫描中
        BLE_STATE_ADVERTISING,  // 广播中
        BLE_STATE_CONNECTING,   // 连接中
        BLE_STATE_CONNECTED,    // 已连接
        BLE_STATE_PAIRING,      // 配对中
        BLE_STATE_PAIRED,       // 已配对
        BLE_STATE_ERROR         // 错误状态
    } ble_state_t;

    /**
     * @brief 设备角色
     */
    typedef enum
    {
        BLE_ROLE_PERIPHERAL = 0, // 外设角色
        BLE_ROLE_CENTRAL,        // 中心角色
        BLE_ROLE_BOTH            // 双角色
    } ble_role_t;

    /**
     * @brief 广播类型
     */
    typedef enum
    {
        BLE_ADV_TYPE_CONNECTABLE = 0, // 可连接广播
        BLE_ADV_TYPE_NON_CONNECTABLE, // 不可连接广播
        BLE_ADV_TYPE_SCANNABLE,       // 可扫描广播
        BLE_ADV_TYPE_DIRECTED         // 定向广播
    } ble_adv_type_t;

    /**
     * @brief 安全等级
     */
    typedef enum
    {
        BLE_SECURITY_NONE = 0,        // 无安全
        BLE_SECURITY_UNAUTHENTICATED, // 未认证加密
        BLE_SECURITY_AUTHENTICATED,   // 认证加密
        BLE_SECURITY_SECURE           // 安全连接
    } ble_security_level_t;

    /**
     * @brief 事件类型
     */
    typedef enum
    {
        BLE_EVENT_CONNECTED = 0, // 连接事件
        BLE_EVENT_DISCONNECTED,  // 断开连接事件
        BLE_EVENT_DATA_RECEIVED, // 数据接收事件
        BLE_EVENT_DATA_SENT,     // 数据发送事件
        BLE_EVENT_SCAN_RESULT,   // 扫描结果事件
        BLE_EVENT_PAIR_REQUEST,  // 配对请求事件
        BLE_EVENT_PAIR_COMPLETE, // 配对完成事件
        BLE_EVENT_ERROR          // 错误事件
    } ble_event_type_t;

    /**
     * @brief 错误码
     */
    typedef enum
    {
        BLE_SUCCESS = 0,             // 成功
        BLE_ERROR_INVALID_PARAM,     // 无效参数
        BLE_ERROR_NOT_INITIALIZED,   // 未初始化
        BLE_ERROR_TIMEOUT,           // 超时
        BLE_ERROR_NOT_CONNECTED,     // 未连接
        BLE_ERROR_CONNECTION_FAILED, // 连接失败
        BLE_ERROR_PAIR_FAILED,       // 配对失败
        BLE_ERROR_DATA_TOO_LONG,     // 数据过长
        BLE_ERROR_MEMORY,            // 内存错误
        BLE_ERROR_HARDWARE,          // 硬件错误
        BLE_ERROR_UNKNOWN            // 未知错误
    } ble_error_t;

    //==============================================================================
    // 结构体定义
    //==============================================================================

    /**
     * @brief 蓝牙配置
     */
    typedef struct
    {
        char device_name[BLE_MAX_DEVICE_NAME_LEN]; // 设备名称
        ble_role_t role;                           // 设备角色
        ble_security_level_t security_level;       // 安全等级
        uint16_t connection_interval_min;          // 最小连接间隔
        uint16_t connection_interval_max;          // 最大连接间隔
        uint16_t slave_latency;                    // 从设备延迟
        uint16_t supervision_timeout;              // 监督超时
        bool auto_advertise;                       // 自动广播
        bool auto_accept_pair;                     // 自动接受配对
    } ble_config_t;

    /**
     * @brief 设备信息
     */
    typedef struct
    {
        char mac_addr[BLE_MAX_MAC_ADDR_LEN];       // MAC地址
        char device_name[BLE_MAX_DEVICE_NAME_LEN]; // 设备名称
        int8_t rssi;                               // 信号强度
        uint8_t addr_type;                         // 地址类型
        bool is_paired;                            // 是否已配对
        bool is_connected;                         // 是否已连接
    } ble_device_info_t;

    /**
     * @brief 广播配置
     */
    typedef struct
    {
        ble_adv_type_t adv_type;   // 广播类型
        uint16_t adv_interval_min; // 最小广播间隔
        uint16_t adv_interval_max; // 最大广播间隔
        uint8_t adv_channel_map;   // 广播信道映射
        uint8_t *adv_data;         // 广播数据
        uint8_t adv_data_len;      // 广播数据长度
        uint8_t *scan_rsp_data;    // 扫描响应数据
        uint8_t scan_rsp_data_len; // 扫描响应数据长度
    } ble_adv_config_t;

    /**
     * @brief 扫描配置
     */
    typedef struct
    {
        uint16_t scan_interval;    // 扫描间隔
        uint16_t scan_window;      // 扫描窗口
        uint8_t scan_type;         // 扫描类型(主动/被动)
        uint8_t filter_policy;     // 过滤策略
        uint32_t scan_duration_ms; // 扫描持续时间
    } ble_scan_config_t;

    /**
     * @brief 服务定义
     */
    typedef struct
    {
        char uuid[BLE_MAX_UUID_LEN]; // 服务UUID
        bool is_primary;             // 是否为主服务
        uint16_t handle;             // 服务句柄
    } ble_service_t;

    /**
     * @brief 特征定义
     */
    typedef struct
    {
        char uuid[BLE_MAX_UUID_LEN]; // 特征UUID
        uint16_t properties;         // 特征属性
        uint16_t handle;             // 特征句柄
        uint16_t value_handle;       // 值句柄
        uint8_t *value;              // 特征值
        uint16_t value_len;          // 值长度
    } ble_characteristic_t;

    /**
     * @brief 事件数据
     */
    typedef struct
    {
        ble_event_type_t event_type;   // 事件类型
        uint16_t conn_handle;          // 连接句柄
        ble_device_info_t device_info; // 设备信息
        uint8_t *data;                 // 事件数据
        uint16_t data_len;             // 数据长度
        ble_error_t error_code;        // 错误码
    } ble_event_t;

    /**
     * @brief 状态信息
     */
    typedef struct
    {
        ble_state_t state;            // 当前状态
        uint8_t connected_devices;    // 已连接设备数
        uint8_t paired_devices;       // 已配对设备数
        bool is_advertising;          // 是否在广播
        bool is_scanning;             // 是否在扫描
        uint32_t uptime_seconds;      // 运行时间
        uint32_t data_sent_bytes;     // 发送数据量
        uint32_t data_received_bytes; // 接收数据量
    } ble_status_t;

    //==============================================================================
    // 回调函数类型
    //==============================================================================

    /**
     * @brief 事件回调函数类型
     * @param event 事件数据
     */
    typedef void (*ble_event_callback_t)(const ble_event_t *event);

    //==============================================================================
    // 函数声明
    //==============================================================================

    /**
     * @brief 初始化蓝牙模块
     * @param config 配置参数
     * @param callback 事件回调函数
     * @return 错误码
     */
    ble_error_t ble_init(const ble_config_t *config, ble_event_callback_t callback);

    /**
     * @brief 反初始化蓝牙模块
     * @return 错误码
     */
    ble_error_t ble_deinit(void);

    /**
     * @brief 蓝牙任务处理
     */
    void ble_task(void);

    //==============================================================================
    // 电源和状态控制
    //==============================================================================

    /**
     * @brief 开启蓝牙
     * @return 错误码
     */
    ble_error_t ble_power_on(void);

    /**
     * @brief 关闭蓝牙
     * @return 错误码
     */
    ble_error_t ble_power_off(void);

    /**
     * @brief 复位蓝牙模块
     * @return 错误码
     */
    ble_error_t ble_reset(void);

    /**
     * @brief 获取蓝牙状态
     * @return 蓝牙状态
     */
    ble_state_t ble_get_state(void);

    /**
     * @brief 获取详细状态信息
     * @param status 状态信息结构体
     * @return 错误码
     */
    ble_error_t ble_get_status(ble_status_t *status);

    //==============================================================================
    // 广播和扫描
    //==============================================================================

    /**
     * @brief 开始广播
     * @param config 广播配置
     * @return 错误码
     */
    ble_error_t ble_start_advertising(const ble_adv_config_t *config);

    /**
     * @brief 停止广播
     * @return 错误码
     */
    ble_error_t ble_stop_advertising(void);

    /**
     * @brief 开始扫描
     * @param config 扫描配置
     * @return 错误码
     */
    ble_error_t ble_start_scan(const ble_scan_config_t *config);

    /**
     * @brief 停止扫描
     * @return 错误码
     */
    ble_error_t ble_stop_scan(void);

    //==============================================================================
    // 连接管理
    //==============================================================================

    /**
     * @brief 连接设备
     * @param mac_addr 设备MAC地址
     * @param addr_type 地址类型
     * @return 错误码
     */
    ble_error_t ble_connect(const char *mac_addr, uint8_t addr_type);

    /**
     * @brief 断开连接
     * @param conn_handle 连接句柄
     * @return 错误码
     */
    ble_error_t ble_disconnect(uint16_t conn_handle);

    /**
     * @brief 断开所有连接
     * @return 错误码
     */
    ble_error_t ble_disconnect_all(void);

    /**
     * @brief 检查连接状态
     * @param conn_handle 连接句柄
     * @return true-已连接，false-未连接
     */
    bool ble_is_connected(uint16_t conn_handle);

    /**
     * @brief 获取连接设备信息
     * @param conn_handle 连接句柄
     * @param device_info 设备信息
     * @return 错误码
     */
    ble_error_t ble_get_device_info(uint16_t conn_handle, ble_device_info_t *device_info);

    //==============================================================================
    // 配对和安全
    //==============================================================================

    /**
     * @brief 开始配对
     * @param conn_handle 连接句柄
     * @return 错误码
     */
    ble_error_t ble_start_pair(uint16_t conn_handle);

    /**
     * @brief 接受配对请求
     * @param conn_handle 连接句柄
     * @param accept 是否接受
     * @return 错误码
     */
    ble_error_t ble_accept_pair(uint16_t conn_handle, bool accept);

    /**
     * @brief 删除配对信息
     * @param mac_addr 设备MAC地址
     * @return 错误码
     */
    ble_error_t ble_unpair(const char *mac_addr);

    /**
     * @brief 清除所有配对信息
     * @return 错误码
     */
    ble_error_t ble_clear_all_pairs(void);

    //==============================================================================
    // 数据传输
    //==============================================================================

    /**
     * @brief 发送数据
     * @param conn_handle 连接句柄
     * @param data 数据缓冲区
     * @param data_len 数据长度
     * @return 错误码
     */
    ble_error_t ble_send_data(uint16_t conn_handle, const uint8_t *data, uint16_t data_len);

    /**
     * @brief 广播数据
     * @param data 数据缓冲区
     * @param data_len 数据长度
     * @return 错误码
     */
    ble_error_t ble_broadcast_data(const uint8_t *data, uint16_t data_len);

    /**
     * @brief 发送通知
     * @param conn_handle 连接句柄
     * @param char_handle 特征句柄
     * @param data 数据缓冲区
     * @param data_len 数据长度
     * @return 错误码
     */
    ble_error_t ble_send_notification(uint16_t conn_handle, uint16_t char_handle,
                                      const uint8_t *data, uint16_t data_len);

    //==============================================================================
    // 服务和特征管理
    //==============================================================================

    /**
     * @brief 添加服务
     * @param service 服务定义
     * @return 错误码
     */
    ble_error_t ble_add_service(const ble_service_t *service);

    /**
     * @brief 添加特征
     * @param service_handle 服务句柄
     * @param characteristic 特征定义
     * @return 错误码
     */
    ble_error_t ble_add_characteristic(uint16_t service_handle, const ble_characteristic_t *characteristic);

    /**
     * @brief 读取特征值
     * @param conn_handle 连接句柄
     * @param char_handle 特征句柄
     * @param buffer 读取缓冲区
     * @param buffer_len 缓冲区长度
     * @param read_len 实际读取长度
     * @return 错误码
     */
    ble_error_t ble_read_characteristic(uint16_t conn_handle, uint16_t char_handle,
                                        uint8_t *buffer, uint16_t buffer_len, uint16_t *read_len);

    /**
     * @brief 写入特征值
     * @param conn_handle 连接句柄
     * @param char_handle 特征句柄
     * @param data 写入数据
     * @param data_len 数据长度
     * @return 错误码
     */
    ble_error_t ble_write_characteristic(uint16_t conn_handle, uint16_t char_handle,
                                         const uint8_t *data, uint16_t data_len);

    //==============================================================================
    // 工具函数
    //==============================================================================

    /**
     * @brief 获取状态名称
     * @param state 状态值
     * @return 状态名称字符串
     */
    const char *ble_get_state_name(ble_state_t state);

    /**
     * @brief 获取错误码描述
     * @param error 错误码
     * @return 错误描述字符串
     */
    const char *ble_get_error_string(ble_error_t error);

    /**
     * @brief MAC地址字符串转换
     * @param mac_str MAC地址字符串
     * @param mac_bytes MAC地址字节数组
     * @return 错误码
     */
    ble_error_t ble_mac_str_to_bytes(const char *mac_str, uint8_t *mac_bytes);

    /**
     * @brief MAC地址字节转字符串
     * @param mac_bytes MAC地址字节数组
     * @param mac_str MAC地址字符串缓冲区
     * @return 错误码
     */
    ble_error_t ble_mac_bytes_to_str(const uint8_t *mac_bytes, char *mac_str);

#ifdef __cplusplus
}
#endif

#endif /* __BLUETOOTH_H__ */