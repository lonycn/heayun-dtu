/**
 * @file mqtt.h
 * @brief MQTT通信模块接口定义
 * @version 1.0
 * @date 2025-03-28
 *
 * 憨云DTU - MQTT通信模块
 * 支持轻量级MQTT协议栈，JSON数据格式，SSL/TLS安全连接
 */

#ifndef __MQTT_H__
#define __MQTT_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ================================ 宏定义 ================================ */

// MQTT协议版本
#define MQTT_VERSION_3_1 3
#define MQTT_VERSION_3_1_1 4
#define MQTT_VERSION_5_0 5

// MQTT消息类型
#define MQTT_CONNECT 1
#define MQTT_CONNACK 2
#define MQTT_PUBLISH 3
#define MQTT_PUBACK 4
#define MQTT_PUBREC 5
#define MQTT_PUBREL 6
#define MQTT_PUBCOMP 7
#define MQTT_SUBSCRIBE 8
#define MQTT_SUBACK 9
#define MQTT_UNSUBSCRIBE 10
#define MQTT_UNSUBACK 11
#define MQTT_PINGREQ 12
#define MQTT_PINGRESP 13
#define MQTT_DISCONNECT 14

// MQTT连接标志
#define MQTT_CONNECT_FLAG_USERNAME 0x80
#define MQTT_CONNECT_FLAG_PASSWORD 0x40
#define MQTT_CONNECT_FLAG_WILL_RETAIN 0x20
#define MQTT_CONNECT_FLAG_WILL_QOS 0x18
#define MQTT_CONNECT_FLAG_WILL 0x04
#define MQTT_CONNECT_FLAG_CLEAN_SESSION 0x02

// QoS等级
#define MQTT_QOS_0 0 // 最多一次
#define MQTT_QOS_1 1 // 至少一次
#define MQTT_QOS_2 2 // 恰好一次

// 错误码
#define MQTT_SUCCESS 0
#define MQTT_ERROR_INVALID_PARAM -1
#define MQTT_ERROR_NO_MEMORY -2
#define MQTT_ERROR_NETWORK -3
#define MQTT_ERROR_TIMEOUT -4
#define MQTT_ERROR_NOT_CONNECTED -5
#define MQTT_ERROR_PROTOCOL -6
#define MQTT_ERROR_SEND -7
#define MQTT_ERROR_RECEIVE -8
#define MQTT_ERROR_REJECTED -9
#define MQTT_ERROR_SSL -10

// 缓冲区大小
#define MQTT_MAX_PACKET_SIZE 1024
#define MQTT_MAX_TOPIC_LEN 128
#define MQTT_MAX_PAYLOAD_LEN 512
#define MQTT_MAX_CLIENT_ID_LEN 32
#define MQTT_MAX_USERNAME_LEN 32
#define MQTT_MAX_PASSWORD_LEN 32
#define MQTT_MAX_HOST_LEN 64

// 消息池大小
#define MQTT_MESSAGE_POOL_SIZE 5
#define MQTT_SUBSCRIPTION_MAX 10

    /* ================================ 数据结构 ================================ */

    /**
     * @brief MQTT连接状态枚举
     */
    typedef enum
    {
        MQTT_STATE_DISCONNECTED = 0, ///< 未连接
        MQTT_STATE_CONNECTING,       ///< 连接中
        MQTT_STATE_CONNECTED,        ///< 已连接
        MQTT_STATE_DISCONNECTING,    ///< 断开连接中
        MQTT_STATE_ERROR             ///< 错误状态
    } mqtt_state_t;

    /**
     * @brief MQTT事件类型枚举
     */
    typedef enum
    {
        MQTT_EVENT_CONNECTED = 0,       ///< 连接成功
        MQTT_EVENT_DISCONNECTED,        ///< 连接断开
        MQTT_EVENT_MESSAGE_RECEIVED,    ///< 收到消息
        MQTT_EVENT_MESSAGE_SENT,        ///< 消息发送成功
        MQTT_EVENT_SUBSCRIBE_SUCCESS,   ///< 订阅成功
        MQTT_EVENT_UNSUBSCRIBE_SUCCESS, ///< 取消订阅成功
        MQTT_EVENT_ERROR                ///< 错误事件
    } mqtt_event_type_t;

    /**
     * @brief MQTT连接配置结构
     */
    typedef struct
    {
        char broker_host[MQTT_MAX_HOST_LEN];    ///< 服务器地址
        uint16_t broker_port;                   ///< 服务器端口
        char client_id[MQTT_MAX_CLIENT_ID_LEN]; ///< 客户端ID
        char username[MQTT_MAX_USERNAME_LEN];   ///< 用户名
        char password[MQTT_MAX_PASSWORD_LEN];   ///< 密码
        uint16_t keep_alive;                    ///< 心跳间隔(秒)
        bool clean_session;                     ///< 清除会话
        bool use_ssl;                           ///< 启用SSL/TLS
        uint8_t qos_level;                      ///< 默认QoS等级
        uint8_t protocol_version;               ///< 协议版本
        uint32_t connect_timeout;               ///< 连接超时(ms)
        uint32_t message_timeout;               ///< 消息超时(ms)
    } mqtt_config_t;

    /**
     * @brief MQTT消息结构
     */
    typedef struct
    {
        char topic[MQTT_MAX_TOPIC_LEN]; ///< 主题
        uint8_t *payload;               ///< 消息内容
        uint16_t payload_len;           ///< 消息长度
        uint8_t qos;                    ///< 服务质量等级
        bool retain;                    ///< 保留消息标志
        uint16_t message_id;            ///< 消息ID
        uint32_t timestamp;             ///< 时间戳
        bool dup;                       ///< 重复标志
    } mqtt_message_t;

    /**
     * @brief MQTT事件结构
     */
    typedef struct
    {
        mqtt_event_type_t event; ///< 事件类型
        void *data;              ///< 事件数据
        uint16_t data_len;       ///< 数据长度
        int error_code;          ///< 错误码
    } mqtt_event_t;

    /**
     * @brief MQTT数据包结构
     */
    typedef struct
    {
        uint8_t type;         ///< 消息类型
        uint8_t flags;        ///< 标志位
        uint16_t message_id;  ///< 消息ID
        uint8_t qos;          ///< QoS等级
        bool retain;          ///< 保留标志
        bool dup;             ///< 重复标志
        uint8_t *payload;     ///< 载荷数据
        uint16_t payload_len; ///< 载荷长度
    } mqtt_packet_t;

    /**
     * @brief MQTT订阅信息结构
     */
    typedef struct
    {
        char topic[MQTT_MAX_TOPIC_LEN]; ///< 订阅主题
        uint8_t qos;                    ///< QoS等级
        bool active;                    ///< 是否激活
    } mqtt_subscription_t;

    /**
     * @brief MQTT统计信息结构
     */
    typedef struct
    {
        uint32_t tx_count;          ///< 发送消息计数
        uint32_t rx_count;          ///< 接收消息计数
        uint32_t error_count;       ///< 错误计数
        uint32_t reconnect_count;   ///< 重连计数
        uint32_t bytes_sent;        ///< 发送字节数
        uint32_t bytes_received;    ///< 接收字节数
        uint32_t last_ping_time;    ///< 最后心跳时间
        uint32_t last_message_time; ///< 最后消息时间
    } mqtt_statistics_t;

    /**
     * @brief MQTT SSL/TLS配置结构
     */
    typedef struct
    {
        bool enable_ssl;        ///< 启用SSL/TLS
        char ca_cert[1024];     ///< CA证书
        char client_cert[1024]; ///< 客户端证书
        char client_key[1024];  ///< 客户端私钥
        bool verify_server;     ///< 验证服务器证书
        bool verify_hostname;   ///< 验证主机名
    } mqtt_ssl_config_t;

    /**
     * @brief MQTT事件回调函数类型
     */
    typedef void (*mqtt_event_callback_t)(const mqtt_event_t *event);

    /**
     * @brief MQTT日志回调函数类型
     */
    typedef void (*mqtt_log_callback_t)(int level, const char *message);

    /* ================================ API接口 ================================ */

    /**
     * @brief 初始化MQTT模块
     * @param config MQTT配置参数
     * @return 0:成功, <0:失败
     */
    int mqtt_init(const mqtt_config_t *config);

    /**
     * @brief 反初始化MQTT模块
     * @return 0:成功, <0:失败
     */
    int mqtt_deinit(void);

    /**
     * @brief 设置事件回调函数
     * @param callback 事件回调函数
     * @return 0:成功, <0:失败
     */
    int mqtt_set_event_callback(mqtt_event_callback_t callback);

    /**
     * @brief 设置日志回调函数
     * @param callback 日志回调函数
     * @return 0:成功, <0:失败
     */
    int mqtt_set_log_callback(mqtt_log_callback_t callback);

    /**
     * @brief 连接到MQTT服务器
     * @return 0:成功, <0:失败
     */
    int mqtt_connect(void);

    /**
     * @brief 断开MQTT连接
     * @return 0:成功, <0:失败
     */
    int mqtt_disconnect(void);

    /**
     * @brief 获取连接状态
     * @return MQTT连接状态
     */
    mqtt_state_t mqtt_get_state(void);

    /**
     * @brief 检查是否已连接
     * @return true:已连接, false:未连接
     */
    bool mqtt_is_connected(void);

    /**
     * @brief 发布消息
     * @param topic 主题
     * @param payload 消息内容
     * @param len 消息长度
     * @param qos QoS等级
     * @param retain 保留标志
     * @return 0:成功, <0:失败
     */
    int mqtt_publish(const char *topic, const void *payload, uint16_t len,
                     uint8_t qos, bool retain);

    /**
     * @brief 发布JSON格式消息
     * @param topic 主题
     * @param json_str JSON字符串
     * @param qos QoS等级
     * @return 0:成功, <0:失败
     */
    int mqtt_publish_json(const char *topic, const char *json_str, uint8_t qos);

    /**
     * @brief 订阅主题
     * @param topic 主题
     * @param qos QoS等级
     * @return 0:成功, <0:失败
     */
    int mqtt_subscribe(const char *topic, uint8_t qos);

    /**
     * @brief 取消订阅主题
     * @param topic 主题
     * @return 0:成功, <0:失败
     */
    int mqtt_unsubscribe(const char *topic);

    /**
     * @brief MQTT任务处理函数
     * @note 需要在主循环中定期调用
     */
    void mqtt_task(void);

    /**
     * @brief 获取状态名称字符串
     * @param state 状态值
     * @return 状态名称字符串
     */
    const char *mqtt_get_state_name(mqtt_state_t state);

    /**
     * @brief 获取统计信息
     * @param stats 统计信息结构指针
     * @return 0:成功, <0:失败
     */
    int mqtt_get_statistics(mqtt_statistics_t *stats);

    /**
     * @brief 重置统计信息
     * @return 0:成功, <0:失败
     */
    int mqtt_reset_statistics(void);

    /**
     * @brief 设置SSL/TLS配置
     * @param ssl_config SSL配置
     * @return 0:成功, <0:失败
     */
    int mqtt_set_ssl_config(const mqtt_ssl_config_t *ssl_config);

    /**
     * @brief 强制重连
     * @return 0:成功, <0:失败
     */
    int mqtt_force_reconnect(void);

    /**
     * @brief 发送心跳包
     * @return 0:成功, <0:失败
     */
    int mqtt_ping(void);

    /**
     * @brief 获取下一个消息ID
     * @return 消息ID
     */
    uint16_t mqtt_get_next_message_id(void);

    /**
     * @brief 检查主题是否匹配
     * @param topic_filter 主题过滤器
     * @param topic_name 主题名称
     * @return true:匹配, false:不匹配
     */
    bool mqtt_topic_matches(const char *topic_filter, const char *topic_name);

    /* ================================ JSON工具函数 ================================ */

    /**
     * @brief JSON编码器结构
     */
    typedef struct
    {
        char *buffer;  ///< 缓冲区
        uint16_t size; ///< 缓冲区大小
        uint16_t pos;  ///< 当前位置
        bool error;    ///< 错误标志
    } json_encoder_t;

    /**
     * @brief JSON解析器结构
     */
    typedef struct
    {
        const char *json; ///< JSON字符串
        uint16_t len;     ///< 字符串长度
        uint16_t pos;     ///< 当前位置
        bool error;       ///< 错误标志
    } json_parser_t;

    /**
     * @brief 初始化JSON编码器
     * @param encoder 编码器指针
     * @param buffer 缓冲区
     * @param size 缓冲区大小
     */
    void json_encoder_init(json_encoder_t *encoder, char *buffer, uint16_t size);

    /**
     * @brief 开始JSON对象
     * @param encoder 编码器指针
     */
    void json_start_object(json_encoder_t *encoder);

    /**
     * @brief 结束JSON对象
     * @param encoder 编码器指针
     */
    void json_end_object(json_encoder_t *encoder);

    /**
     * @brief 开始JSON数组
     * @param encoder 编码器指针
     */
    void json_start_array(json_encoder_t *encoder);

    /**
     * @brief 结束JSON数组
     * @param encoder 编码器指针
     */
    void json_end_array(json_encoder_t *encoder);

    /**
     * @brief 添加字符串字段
     * @param encoder 编码器指针
     * @param key 键名
     * @param value 字符串值
     */
    void json_add_string(json_encoder_t *encoder, const char *key, const char *value);

    /**
     * @brief 添加整数字段
     * @param encoder 编码器指针
     * @param key 键名
     * @param value 整数值
     */
    void json_add_int(json_encoder_t *encoder, const char *key, int32_t value);

    /**
     * @brief 添加浮点数字段
     * @param encoder 编码器指针
     * @param key 键名
     * @param value 浮点数值
     */
    void json_add_float(json_encoder_t *encoder, const char *key, float value);

    /**
     * @brief 添加布尔字段
     * @param encoder 编码器指针
     * @param key 键名
     * @param value 布尔值
     */
    void json_add_bool(json_encoder_t *encoder, const char *key, bool value);

    /**
     * @brief 初始化JSON解析器
     * @param parser 解析器指针
     * @param json JSON字符串
     * @param len 字符串长度
     */
    void json_parser_init(json_parser_t *parser, const char *json, uint16_t len);

    /**
     * @brief 查找JSON对象
     * @param parser 解析器指针
     * @param key 键名
     * @return true:找到, false:未找到
     */
    bool json_find_object(json_parser_t *parser, const char *key);

    /**
     * @brief 获取字符串值
     * @param parser 解析器指针
     * @param key 键名
     * @param value 值缓冲区
     * @param max_len 缓冲区最大长度
     * @return true:成功, false:失败
     */
    bool json_get_string(json_parser_t *parser, const char *key, char *value, uint16_t max_len);

    /**
     * @brief 获取整数值
     * @param parser 解析器指针
     * @param key 键名
     * @param value 值指针
     * @return true:成功, false:失败
     */
    bool json_get_int(json_parser_t *parser, const char *key, int32_t *value);

    /**
     * @brief 获取浮点数值
     * @param parser 解析器指针
     * @param key 键名
     * @param value 值指针
     * @return true:成功, false:失败
     */
    bool json_get_float(json_parser_t *parser, const char *key, float *value);

    /**
     * @brief 获取布尔值
     * @param parser 解析器指针
     * @param key 键名
     * @param value 值指针
     * @return true:成功, false:失败
     */
    bool json_get_bool(json_parser_t *parser, const char *key, bool *value);

    /* ================================ 设备数据结构 ================================ */

    /**
     * @brief MQTT传感器数据结构
     */
    typedef struct
    {
        float temperature; ///< 温度
        float humidity;    ///< 湿度
        float voltage;     ///< 电压
        float current;     ///< 电流
        float power;       ///< 功率
    } mqtt_sensor_data_t;

    /**
     * @brief 设备状态结构
     */
    typedef struct
    {
        bool modbus_online;   ///< Modbus在线状态
        bool lora_connected;  ///< LoRa连接状态
        bool storage_normal;  ///< 存储正常状态
        bool alarm_active;    ///< 报警激活状态
        uint32_t uptime;      ///< 运行时间
        uint16_t free_memory; ///< 空闲内存
    } device_status_t;

    /**
     * @brief MQTT报警信息结构
     */
    typedef struct
    {
        uint16_t alarm_id;   ///< 报警ID
        uint8_t alarm_type;  ///< 报警类型
        uint8_t alarm_level; ///< 报警级别
        char message[64];    ///< 报警消息
        float value;         ///< 报警值
        float threshold;     ///< 阈值
        uint8_t sensor_id;   ///< 传感器ID
        uint32_t timestamp;  ///< 时间戳
    } mqtt_alarm_info_t;

    /**
     * @brief 设备配置结构
     */
    typedef struct
    {
        uint16_t sampling_interval;    ///< 采样间隔(秒)
        uint16_t upload_interval;      ///< 上传间隔(秒)
        float temp_high_threshold;     ///< 温度高阈值
        float temp_low_threshold;      ///< 温度低阈值
        float humidity_high_threshold; ///< 湿度高阈值
        float voltage_low_threshold;   ///< 电压低阈值
        uint32_t modbus_baudrate;      ///< Modbus波特率
        uint8_t modbus_slave_id;       ///< Modbus从机ID
    } device_config_t;

    /**
     * @brief 编码传感器数据为JSON
     * @param buffer 输出缓冲区
     * @param size 缓冲区大小
     * @param data 传感器数据
     * @return JSON字符串长度, <0:失败
     */
    int mqtt_encode_sensor_data(char *buffer, uint16_t size, const mqtt_sensor_data_t *data);

    /**
     * @brief 编码设备状态为JSON
     * @param buffer 输出缓冲区
     * @param size 缓冲区大小
     * @param status 设备状态
     * @return JSON字符串长度, <0:失败
     */
    int mqtt_encode_device_status(char *buffer, uint16_t size, const device_status_t *status);

    /**
     * @brief 编码报警信息为JSON
     * @param buffer 输出缓冲区
     * @param size 缓冲区大小
     * @param alarm 报警信息
     * @return JSON字符串长度, <0:失败
     */
    int mqtt_encode_alarm_info(char *buffer, uint16_t size, const mqtt_alarm_info_t *alarm);

    /**
     * @brief 解析配置JSON
     * @param json JSON字符串
     * @param config 配置结构
     * @return 0:成功, <0:失败
     */
    int mqtt_parse_device_config(const char *json, device_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_H__ */