/**
 * @file 4g.h
 * @brief 憨云DTU - 4G无线通信模块接口定义
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 4G无线通信模块，支持EC20/SIM7600等主流4G模组
 * 提供TCP/UDP网络通信、HTTP/HTTPS客户端功能
 */

#ifndef __4G_H__
#define __4G_H__

#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

    //==============================================================================
    // 宏定义
    //==============================================================================

#define G4_MAX_APN_LEN 32        // APN最大长度
#define G4_MAX_OPERATOR_LEN 32   // 运营商名称最大长度
#define G4_MAX_IMEI_LEN 16       // IMEI最大长度
#define G4_MAX_ICCID_LEN 24      // ICCID最大长度
#define G4_MAX_IP_LEN 16         // IP地址最大长度
#define G4_MAX_URL_LEN 128       // URL最大长度
#define G4_MAX_RESPONSE_LEN 1024 // HTTP响应最大长度

#define G4_AT_TIMEOUT_MS 5000       // AT命令超时时间
#define G4_CONNECT_TIMEOUT_MS 30000 // 网络连接超时时间
#define G4_HTTP_TIMEOUT_MS 10000    // HTTP请求超时时间

    //==============================================================================
    // 枚举定义
    //==============================================================================

    /**
     * @brief 4G模块状态
     */
    typedef enum
    {
        G4_STATE_POWER_OFF = 0,      // 关机状态
        G4_STATE_POWER_ON,           // 开机状态
        G4_STATE_SIM_READY,          // SIM卡就绪
        G4_STATE_NETWORK_SEARCHING,  // 搜索网络
        G4_STATE_NETWORK_REGISTERED, // 网络注册成功
        G4_STATE_PDP_ACTIVATED,      // PDP上下文激活
        G4_STATE_CONNECTED,          // 网络连接成功
        G4_STATE_ERROR               // 错误状态
    } g4_state_t;

    /**
     * @brief 网络类型
     */
    typedef enum
    {
        G4_NET_TYPE_UNKNOWN = 0, // 未知网络
        G4_NET_TYPE_2G,          // 2G网络
        G4_NET_TYPE_3G,          // 3G网络
        G4_NET_TYPE_4G,          // 4G网络
        G4_NET_TYPE_5G           // 5G网络
    } g4_net_type_t;

    /**
     * @brief 信号质量等级
     */
    typedef enum
    {
        G4_SIGNAL_NONE = 0, // 无信号
        G4_SIGNAL_POOR,     // 信号差
        G4_SIGNAL_FAIR,     // 信号一般
        G4_SIGNAL_GOOD,     // 信号良好
        G4_SIGNAL_EXCELLENT // 信号优秀
    } g4_signal_level_t;

    /**
     * @brief HTTP方法
     */
    typedef enum
    {
        G4_HTTP_GET = 0, // GET请求
        G4_HTTP_POST,    // POST请求
        G4_HTTP_PUT,     // PUT请求
        G4_HTTP_DELETE   // DELETE请求
    } g4_http_method_t;

    /**
     * @brief 错误码
     */
    typedef enum
    {
        G4_SUCCESS = 0,           // 成功
        G4_ERROR_INVALID_PARAM,   // 无效参数
        G4_ERROR_NOT_INITIALIZED, // 未初始化
        G4_ERROR_TIMEOUT,         // 超时
        G4_ERROR_NETWORK,         // 网络错误
        G4_ERROR_SIM_NOT_READY,   // SIM卡未就绪
        G4_ERROR_NO_SIGNAL,       // 无信号
        G4_ERROR_AT_COMMAND,      // AT命令错误
        G4_ERROR_HTTP,            // HTTP错误
        G4_ERROR_MEMORY,          // 内存错误
        G4_ERROR_UNKNOWN          // 未知错误
    } g4_error_t;

    //==============================================================================
    // 结构体定义
    //==============================================================================

    /**
     * @brief 4G模块配置
     */
    typedef struct
    {
        char apn[G4_MAX_APN_LEN];   // APN接入点
        char username[32];          // 用户名
        char password[32];          // 密码
        uint8_t uart_port;          // UART端口号
        uint32_t baudrate;          // 波特率
        uint8_t power_pin;          // 电源控制引脚
        uint8_t reset_pin;          // 复位引脚
        uint16_t power_on_delay_ms; // 开机延时
        uint16_t reset_delay_ms;    // 复位延时
        bool auto_connect;          // 自动连接
    } g4_config_t;

    /**
     * @brief 4G模块状态信息
     */
    typedef struct
    {
        g4_state_t state;                        // 模块状态
        g4_net_type_t net_type;                  // 网络类型
        g4_signal_level_t signal_level;          // 信号等级
        int8_t rssi;                             // 信号强度(dBm)
        uint8_t signal_quality;                  // 信号质量(0-31)
        char operator_name[G4_MAX_OPERATOR_LEN]; // 运营商名称
        char imei[G4_MAX_IMEI_LEN];              // IMEI号
        char iccid[G4_MAX_ICCID_LEN];            // ICCID号
        char local_ip[G4_MAX_IP_LEN];            // 本地IP地址
        uint32_t uptime_seconds;                 // 运行时间
        uint32_t data_sent_bytes;                // 发送数据量
        uint32_t data_received_bytes;            // 接收数据量
    } g4_status_t;

    /**
     * @brief HTTP请求配置
     */
    typedef struct
    {
        g4_http_method_t method;  // HTTP方法
        char url[G4_MAX_URL_LEN]; // 请求URL
        char *headers;            // 请求头
        char *body;               // 请求体
        uint16_t timeout_ms;      // 超时时间
        bool verify_ssl;          // 验证SSL证书
    } g4_http_request_t;

    /**
     * @brief HTTP响应
     */
    typedef struct
    {
        uint16_t status_code; // HTTP状态码
        char *headers;        // 响应头
        char *body;           // 响应体
        uint16_t body_len;    // 响应体长度
    } g4_http_response_t;

    /**
     * @brief TCP/UDP连接配置
     */
    typedef struct
    {
        char remote_host[64]; // 远程主机
        uint16_t remote_port; // 远程端口
        uint16_t local_port;  // 本地端口
        bool is_tcp;          // TCP/UDP选择
        uint16_t timeout_ms;  // 超时时间
        bool keep_alive;      // 保持连接
    } g4_socket_config_t;

    //==============================================================================
    // 函数声明
    //==============================================================================

    /**
     * @brief 初始化4G模块
     * @param config 配置参数
     * @return 错误码
     */
    g4_error_t g4_init(const g4_config_t *config);

    /**
     * @brief 反初始化4G模块
     * @return 错误码
     */
    g4_error_t g4_deinit(void);

    /**
     * @brief 4G模块任务处理
     */
    void g4_task(void);

    //==============================================================================
    // 电源和复位控制
    //==============================================================================

    /**
     * @brief 开机
     * @return 错误码
     */
    g4_error_t g4_power_on(void);

    /**
     * @brief 关机
     * @return 错误码
     */
    g4_error_t g4_power_off(void);

    /**
     * @brief 复位模块
     * @return 错误码
     */
    g4_error_t g4_reset(void);

    /**
     * @brief 检查模块是否就绪
     * @return true-就绪，false-未就绪
     */
    bool g4_is_ready(void);

    //==============================================================================
    // 状态查询
    //==============================================================================

    /**
     * @brief 获取模块状态
     * @return 模块状态
     */
    g4_state_t g4_get_state(void);

    /**
     * @brief 获取详细状态信息
     * @param status 状态信息结构体
     * @return 错误码
     */
    g4_error_t g4_get_status(g4_status_t *status);

    /**
     * @brief 获取信号强度
     * @param rssi 信号强度(dBm)
     * @param quality 信号质量(0-31)
     * @return 错误码
     */
    g4_error_t g4_get_signal_strength(int8_t *rssi, uint8_t *quality);

    /**
     * @brief 获取网络信息
     * @param net_type 网络类型
     * @param operator_name 运营商名称
     * @return 错误码
     */
    g4_error_t g4_get_network_info(g4_net_type_t *net_type, char *operator_name);

    //==============================================================================
    // 网络连接管理
    //==============================================================================

    /**
     * @brief 连接网络
     * @return 错误码
     */
    g4_error_t g4_connect_network(void);

    /**
     * @brief 断开网络连接
     * @return 错误码
     */
    g4_error_t g4_disconnect_network(void);

    /**
     * @brief 检查网络连接状态
     * @return true-已连接，false-未连接
     */
    bool g4_is_network_connected(void);

    /**
     * @brief 获取本地IP地址
     * @param ip_addr IP地址字符串
     * @return 错误码
     */
    g4_error_t g4_get_local_ip(char *ip_addr);

    //==============================================================================
    // HTTP客户端
    //==============================================================================

    /**
     * @brief 发送HTTP请求
     * @param request 请求配置
     * @param response 响应结构体
     * @return 错误码
     */
    g4_error_t g4_http_request(const g4_http_request_t *request, g4_http_response_t *response);

    /**
     * @brief HTTP GET请求
     * @param url 请求URL
     * @param response 响应缓冲区
     * @param response_len 响应缓冲区长度
     * @return 错误码
     */
    g4_error_t g4_http_get(const char *url, char *response, uint16_t response_len);

    /**
     * @brief HTTP POST请求
     * @param url 请求URL
     * @param data 请求数据
     * @param data_len 数据长度
     * @param response 响应缓冲区
     * @param response_len 响应缓冲区长度
     * @return 错误码
     */
    g4_error_t g4_http_post(const char *url, const char *data, uint16_t data_len,
                            char *response, uint16_t response_len);

    //==============================================================================
    // TCP/UDP通信
    //==============================================================================

    /**
     * @brief 创建TCP/UDP连接
     * @param config 连接配置
     * @param socket_id 连接ID
     * @return 错误码
     */
    g4_error_t g4_socket_create(const g4_socket_config_t *config, uint8_t *socket_id);

    /**
     * @brief 关闭连接
     * @param socket_id 连接ID
     * @return 错误码
     */
    g4_error_t g4_socket_close(uint8_t socket_id);

    /**
     * @brief 发送数据
     * @param socket_id 连接ID
     * @param data 数据缓冲区
     * @param data_len 数据长度
     * @return 错误码
     */
    g4_error_t g4_socket_send(uint8_t socket_id, const uint8_t *data, uint16_t data_len);

    /**
     * @brief 接收数据
     * @param socket_id 连接ID
     * @param buffer 接收缓冲区
     * @param buffer_len 缓冲区长度
     * @param received_len 实际接收长度
     * @return 错误码
     */
    g4_error_t g4_socket_receive(uint8_t socket_id, uint8_t *buffer, uint16_t buffer_len,
                                 uint16_t *received_len);

    //==============================================================================
    // AT命令接口
    //==============================================================================

    /**
     * @brief 发送AT命令
     * @param command AT命令
     * @param response 响应缓冲区
     * @param response_len 响应缓冲区长度
     * @param timeout_ms 超时时间
     * @return 错误码
     */
    g4_error_t g4_send_at_command(const char *command, char *response, uint16_t response_len,
                                  uint32_t timeout_ms);

    /**
     * @brief 检查AT命令响应
     * @param response 响应字符串
     * @return true-成功，false-失败
     */
    bool g4_check_at_response(const char *response);

    //==============================================================================
    // 工具函数
    //==============================================================================

    /**
     * @brief 获取状态名称
     * @param state 状态值
     * @return 状态名称字符串
     */
    const char *g4_get_state_name(g4_state_t state);

    /**
     * @brief 获取网络类型名称
     * @param net_type 网络类型
     * @return 网络类型名称
     */
    const char *g4_get_net_type_name(g4_net_type_t net_type);

    /**
     * @brief 获取错误码描述
     * @param error 错误码
     * @return 错误描述字符串
     */
    const char *g4_get_error_string(g4_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* __4G_H__ */