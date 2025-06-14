/**
 * @file cloud.h
 * @brief 憨云DTU - 云端集成模块接口定义
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 云端集成模块，支持阿里云IoT、腾讯云IoT、华为云IoT等主流平台
 * 提供设备影子、远程控制、OTA升级、数据上云等功能
 */

#ifndef __CLOUD_H__
#define __CLOUD_H__

#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //==============================================================================
    // 宏定义
    //==============================================================================

#define CLOUD_MAX_DEVICE_ID_LEN 64     // 设备ID最大长度
#define CLOUD_MAX_PRODUCT_KEY_LEN 32   // 产品密钥最大长度
#define CLOUD_MAX_DEVICE_SECRET_LEN 64 // 设备密钥最大长度
#define CLOUD_MAX_ENDPOINT_LEN 128     // 接入点最大长度
#define CLOUD_MAX_TOPIC_LEN 256        // 主题最大长度
#define CLOUD_MAX_PAYLOAD_LEN 1024     // 负载最大长度
#define CLOUD_MAX_PROPERTIES 32        // 最大属性数
#define CLOUD_MAX_SERVICES 16          // 最大服务数
#define CLOUD_MAX_EVENTS 16            // 最大事件数

#define CLOUD_HEARTBEAT_INTERVAL 30000       // 心跳间隔(ms)
#define CLOUD_RECONNECT_INTERVAL 5000        // 重连间隔(ms)
#define CLOUD_PROPERTY_REPORT_INTERVAL 60000 // 属性上报间隔(ms)

    //==============================================================================
    // 枚举定义
    //==============================================================================

    /**
     * @brief 云平台类型
     */
    typedef enum
    {
        CLOUD_PLATFORM_ALIYUN = 0, // 阿里云IoT
        CLOUD_PLATFORM_TENCENT,    // 腾讯云IoT
        CLOUD_PLATFORM_HUAWEI,     // 华为云IoT
        CLOUD_PLATFORM_BAIDU,      // 百度云IoT
        CLOUD_PLATFORM_AWS,        // AWS IoT
        CLOUD_PLATFORM_AZURE,      // Azure IoT
        CLOUD_PLATFORM_CUSTOM      // 自定义平台
    } cloud_platform_t;

    /**
     * @brief 云端连接状态
     */
    typedef enum
    {
        CLOUD_STATE_DISCONNECTED = 0, // 断开连接
        CLOUD_STATE_CONNECTING,       // 连接中
        CLOUD_STATE_CONNECTED,        // 已连接
        CLOUD_STATE_AUTHENTICATING,   // 认证中
        CLOUD_STATE_AUTHENTICATED,    // 已认证
        CLOUD_STATE_ONLINE,           // 在线
        CLOUD_STATE_ERROR             // 错误状态
    } cloud_state_t;

    /**
     * @brief 数据类型
     */
    typedef enum
    {
        CLOUD_DATA_TYPE_INT = 0, // 整数
        CLOUD_DATA_TYPE_FLOAT,   // 浮点数
        CLOUD_DATA_TYPE_DOUBLE,  // 双精度浮点数
        CLOUD_DATA_TYPE_BOOL,    // 布尔值
        CLOUD_DATA_TYPE_STRING,  // 字符串
        CLOUD_DATA_TYPE_ENUM,    // 枚举
        CLOUD_DATA_TYPE_DATE,    // 日期
        CLOUD_DATA_TYPE_STRUCT,  // 结构体
        CLOUD_DATA_TYPE_ARRAY    // 数组
    } cloud_data_type_t;

    /**
     * @brief 消息类型
     */
    typedef enum
    {
        CLOUD_MSG_TYPE_PROPERTY_POST = 0,  // 属性上报
        CLOUD_MSG_TYPE_PROPERTY_SET,       // 属性设置
        CLOUD_MSG_TYPE_SERVICE_INVOKE,     // 服务调用
        CLOUD_MSG_TYPE_EVENT_POST,         // 事件上报
        CLOUD_MSG_TYPE_DEVICE_INFO_UPDATE, // 设备信息更新
        CLOUD_MSG_TYPE_DEVICE_INFO_DELETE, // 设备信息删除
        CLOUD_MSG_TYPE_OTA_UPGRADE,        // OTA升级
        CLOUD_MSG_TYPE_CONFIG_PUSH,        // 配置推送
        CLOUD_MSG_TYPE_SHADOW_GET,         // 获取设备影子
        CLOUD_MSG_TYPE_SHADOW_UPDATE       // 更新设备影子
    } cloud_msg_type_t;

    /**
     * @brief 错误码
     */
    typedef enum
    {
        CLOUD_SUCCESS = 0,             // 成功
        CLOUD_ERROR_INVALID_PARAM,     // 无效参数
        CLOUD_ERROR_NOT_INITIALIZED,   // 未初始化
        CLOUD_ERROR_NOT_CONNECTED,     // 未连接
        CLOUD_ERROR_AUTHENTICATION,    // 认证失败
        CLOUD_ERROR_NETWORK,           // 网络错误
        CLOUD_ERROR_TIMEOUT,           // 超时
        CLOUD_ERROR_MEMORY,            // 内存错误
        CLOUD_ERROR_JSON_PARSE,        // JSON解析错误
        CLOUD_ERROR_TOPIC_INVALID,     // 主题无效
        CLOUD_ERROR_PAYLOAD_TOO_LARGE, // 负载过大
        CLOUD_ERROR_UNKNOWN            // 未知错误
    } cloud_error_t;

    //==============================================================================
    // 结构体定义
    //==============================================================================

    /**
     * @brief 云端配置
     */
    typedef struct
    {
        cloud_platform_t platform;                       // 云平台类型
        char device_id[CLOUD_MAX_DEVICE_ID_LEN];         // 设备ID
        char product_key[CLOUD_MAX_PRODUCT_KEY_LEN];     // 产品密钥
        char device_secret[CLOUD_MAX_DEVICE_SECRET_LEN]; // 设备密钥
        char endpoint[CLOUD_MAX_ENDPOINT_LEN];           // 接入点
        uint16_t port;                                   // 端口号
        bool use_tls;                                    // 是否使用TLS
        uint16_t keepalive_interval;                     // 保活间隔
        bool auto_reconnect;                             // 自动重连
        uint8_t max_reconnect_attempts;                  // 最大重连次数
    } cloud_config_t;

    /**
     * @brief 属性定义
     */
    typedef struct
    {
        char identifier[32];         // 属性标识符
        char name[64];               // 属性名称
        cloud_data_type_t data_type; // 数据类型
        bool read_only;              // 是否只读
        union
        {
            int32_t int_value;      // 整数值
            float float_value;      // 浮点值
            double double_value;    // 双精度值
            bool bool_value;        // 布尔值
            char string_value[128]; // 字符串值
        } value;                    // 属性值
        uint32_t last_update_time;  // 最后更新时间
    } cloud_property_t;

    /**
     * @brief 服务定义
     */
    typedef struct
    {
        char identifier[32];               // 服务标识符
        char name[64];                     // 服务名称
        cloud_property_t input_params[8];  // 输入参数
        cloud_property_t output_params[8]; // 输出参数
        uint8_t input_param_count;         // 输入参数数量
        uint8_t output_param_count;        // 输出参数数量
    } cloud_service_t;

    /**
     * @brief 事件定义
     */
    typedef struct
    {
        char identifier[32];               // 事件标识符
        char name[64];                     // 事件名称
        uint8_t event_type;                // 事件类型(0-信息,1-告警,2-故障)
        cloud_property_t output_params[8]; // 输出参数
        uint8_t output_param_count;        // 输出参数数量
    } cloud_event_t;

    /**
     * @brief 消息结构
     */
    typedef struct
    {
        cloud_msg_type_t msg_type;           // 消息类型
        char topic[CLOUD_MAX_TOPIC_LEN];     // 主题
        char payload[CLOUD_MAX_PAYLOAD_LEN]; // 负载
        uint16_t payload_len;                // 负载长度
        uint32_t timestamp;                  // 时间戳
        uint16_t msg_id;                     // 消息ID
    } cloud_message_t;

    /**
     * @brief 设备影子
     */
    typedef struct
    {
        cloud_property_t desired[CLOUD_MAX_PROPERTIES];  // 期望属性
        cloud_property_t reported[CLOUD_MAX_PROPERTIES]; // 上报属性
        uint8_t desired_count;                           // 期望属性数量
        uint8_t reported_count;                          // 上报属性数量
        uint32_t version;                                // 版本号
        uint32_t last_update_time;                       // 最后更新时间
    } cloud_shadow_t;

    /**
     * @brief OTA升级信息
     */
    typedef struct
    {
        char version[32];       // 版本号
        char download_url[256]; // 下载URL
        char md5[33];           // MD5校验值
        uint32_t file_size;     // 文件大小
        char description[128];  // 升级描述
        bool force_upgrade;     // 是否强制升级
    } cloud_ota_info_t;

    /**
     * @brief 云端状态
     */
    typedef struct
    {
        cloud_state_t state;                // 连接状态
        cloud_platform_t platform;          // 云平台类型
        bool is_connected;                  // 是否已连接
        bool is_authenticated;              // 是否已认证
        uint32_t uptime_seconds;            // 在线时间
        uint32_t reconnect_count;           // 重连次数
        uint32_t messages_sent;             // 发送消息数
        uint32_t messages_received;         // 接收消息数
        uint32_t last_heartbeat_time;       // 最后心跳时间
        uint32_t last_property_report_time; // 最后属性上报时间
        int8_t signal_strength;             // 信号强度
    } cloud_status_t;

    //==============================================================================
    // 回调函数类型
    //==============================================================================

    /**
     * @brief 消息接收回调函数类型
     * @param message 接收到的消息
     */
    typedef void (*cloud_message_callback_t)(const cloud_message_t *message);

    /**
     * @brief 属性设置回调函数类型
     * @param properties 属性列表
     * @param property_count 属性数量
     */
    typedef void (*cloud_property_set_callback_t)(const cloud_property_t *properties, uint8_t property_count);

    /**
     * @brief 服务调用回调函数类型
     * @param service 服务定义
     * @param input_params 输入参数
     * @param output_params 输出参数
     * @return 错误码
     */
    typedef cloud_error_t (*cloud_service_invoke_callback_t)(const cloud_service_t *service,
                                                             const cloud_property_t *input_params,
                                                             cloud_property_t *output_params);

    /**
     * @brief OTA升级回调函数类型
     * @param ota_info OTA升级信息
     */
    typedef void (*cloud_ota_callback_t)(const cloud_ota_info_t *ota_info);

    //==============================================================================
    // 函数声明
    //==============================================================================

    /**
     * @brief 初始化云端模块
     * @param config 配置参数
     * @param msg_callback 消息回调函数
     * @param property_callback 属性设置回调函数
     * @param service_callback 服务调用回调函数
     * @param ota_callback OTA升级回调函数
     * @return 错误码
     */
    cloud_error_t cloud_init(const cloud_config_t *config,
                             cloud_message_callback_t msg_callback,
                             cloud_property_set_callback_t property_callback,
                             cloud_service_invoke_callback_t service_callback,
                             cloud_ota_callback_t ota_callback);

    /**
     * @brief 反初始化云端模块
     * @return 错误码
     */
    cloud_error_t cloud_deinit(void);

    /**
     * @brief 云端任务处理
     */
    void cloud_task(void);

    //==============================================================================
    // 连接管理
    //==============================================================================

    /**
     * @brief 连接云端
     * @return 错误码
     */
    cloud_error_t cloud_connect(void);

    /**
     * @brief 断开云端连接
     * @return 错误码
     */
    cloud_error_t cloud_disconnect(void);

    /**
     * @brief 检查连接状态
     * @return true-已连接，false-未连接
     */
    bool cloud_is_connected(void);

    /**
     * @brief 获取云端状态
     * @param status 状态信息
     * @return 错误码
     */
    cloud_error_t cloud_get_status(cloud_status_t *status);

    //==============================================================================
    // 属性管理
    //==============================================================================

    /**
     * @brief 注册属性
     * @param property 属性定义
     * @return 错误码
     */
    cloud_error_t cloud_register_property(const cloud_property_t *property);

    /**
     * @brief 上报属性
     * @param properties 属性列表
     * @param property_count 属性数量
     * @return 错误码
     */
    cloud_error_t cloud_post_properties(const cloud_property_t *properties, uint8_t property_count);

    /**
     * @brief 上报单个属性
     * @param identifier 属性标识符
     * @param value 属性值
     * @param data_type 数据类型
     * @return 错误码
     */
    cloud_error_t cloud_post_property(const char *identifier, const void *value, cloud_data_type_t data_type);

    //==============================================================================
    // 服务管理
    //==============================================================================

    /**
     * @brief 注册服务
     * @param service 服务定义
     * @return 错误码
     */
    cloud_error_t cloud_register_service(const cloud_service_t *service);

    //==============================================================================
    // 事件管理
    //==============================================================================

    /**
     * @brief 注册事件
     * @param event 事件定义
     * @return 错误码
     */
    cloud_error_t cloud_register_event(const cloud_event_t *event);

    /**
     * @brief 上报事件
     * @param identifier 事件标识符
     * @param output_params 输出参数
     * @param param_count 参数数量
     * @return 错误码
     */
    cloud_error_t cloud_post_event(const char *identifier, const cloud_property_t *output_params, uint8_t param_count);

    //==============================================================================
    // 设备影子
    //==============================================================================

    /**
     * @brief 获取设备影子
     * @param shadow 设备影子
     * @return 错误码
     */
    cloud_error_t cloud_get_shadow(cloud_shadow_t *shadow);

    /**
     * @brief 更新设备影子
     * @param shadow 设备影子
     * @return 错误码
     */
    cloud_error_t cloud_update_shadow(const cloud_shadow_t *shadow);

    //==============================================================================
    // OTA升级
    //==============================================================================

    /**
     * @brief 检查OTA升级
     * @param ota_info OTA升级信息
     * @return 错误码
     */
    cloud_error_t cloud_check_ota(cloud_ota_info_t *ota_info);

    /**
     * @brief 上报OTA升级进度
     * @param progress 升级进度(0-100)
     * @param description 描述信息
     * @return 错误码
     */
    cloud_error_t cloud_report_ota_progress(uint8_t progress, const char *description);

    /**
     * @brief 上报OTA升级结果
     * @param success 是否成功
     * @param description 描述信息
     * @return 错误码
     */
    cloud_error_t cloud_report_ota_result(bool success, const char *description);

    //==============================================================================
    // 消息发送
    //==============================================================================

    /**
     * @brief 发送自定义消息
     * @param topic 主题
     * @param payload 负载
     * @param payload_len 负载长度
     * @return 错误码
     */
    cloud_error_t cloud_send_message(const char *topic, const char *payload, uint16_t payload_len);

    /**
     * @brief 订阅主题
     * @param topic 主题
     * @return 错误码
     */
    cloud_error_t cloud_subscribe_topic(const char *topic);

    /**
     * @brief 取消订阅主题
     * @param topic 主题
     * @return 错误码
     */
    cloud_error_t cloud_unsubscribe_topic(const char *topic);

    //==============================================================================
    // 工具函数
    //==============================================================================

    /**
     * @brief 获取平台名称
     * @param platform 平台类型
     * @return 平台名称字符串
     */
    const char *cloud_get_platform_name(cloud_platform_t platform);

    /**
     * @brief 获取状态名称
     * @param state 状态值
     * @return 状态名称字符串
     */
    const char *cloud_get_state_name(cloud_state_t state);

    /**
     * @brief 获取错误码描述
     * @param error 错误码
     * @return 错误描述字符串
     */
    const char *cloud_get_error_string(cloud_error_t error);

    /**
     * @brief 生成设备签名
     * @param device_id 设备ID
     * @param product_key 产品密钥
     * @param device_secret 设备密钥
     * @param timestamp 时间戳
     * @param signature 签名缓冲区
     * @param signature_len 签名缓冲区长度
     * @return 错误码
     */
    cloud_error_t cloud_generate_signature(const char *device_id, const char *product_key,
                                           const char *device_secret, uint32_t timestamp,
                                           char *signature, uint16_t signature_len);

#ifdef __cplusplus
}
#endif

#endif /* __CLOUD_H__ */