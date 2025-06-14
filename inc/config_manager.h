/**
 * @file config_manager.h
 * @brief 配置管理模块接口定义
 * @version 1.0
 * @date 2025-03-28
 *
 * 憨云DTU - 配置管理模块
 * 支持Web配置界面、HTTP服务器、参数管理、远程配置更新
 */

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ================================ 宏定义 ================================ */

// HTTP服务器配置
#define CONFIG_HTTP_PORT 80
#define CONFIG_HTTPS_PORT 443
#define CONFIG_MAX_CONNECTIONS 5
#define CONFIG_MAX_REQUEST_SIZE 2048
#define CONFIG_MAX_RESPONSE_SIZE 4096

// 配置参数限制
#define CONFIG_MAX_STRING_LEN 64
#define CONFIG_MAX_PARAM_COUNT 100
#define CONFIG_MAX_GROUP_COUNT 20

// 错误码
#define CONFIG_SUCCESS 0
#define CONFIG_ERROR_INVALID_PARAM -1
#define CONFIG_ERROR_NO_MEMORY -2
#define CONFIG_ERROR_NOT_FOUND -3
#define CONFIG_ERROR_READ_ONLY -4
#define CONFIG_ERROR_VALIDATION -5
#define CONFIG_ERROR_STORAGE -6
#define CONFIG_ERROR_NETWORK -7
#define CONFIG_ERROR_AUTH -8

    /* ================================ 数据结构 ================================ */

    /**
     * @brief 配置参数类型枚举
     */
    typedef enum
    {
        CONFIG_TYPE_BOOL = 0, ///< 布尔类型
        CONFIG_TYPE_INT8,     ///< 8位整数
        CONFIG_TYPE_INT16,    ///< 16位整数
        CONFIG_TYPE_INT32,    ///< 32位整数
        CONFIG_TYPE_UINT8,    ///< 8位无符号整数
        CONFIG_TYPE_UINT16,   ///< 16位无符号整数
        CONFIG_TYPE_UINT32,   ///< 32位无符号整数
        CONFIG_TYPE_FLOAT,    ///< 浮点数
        CONFIG_TYPE_STRING,   ///< 字符串
        CONFIG_TYPE_IP_ADDR,  ///< IP地址
        CONFIG_TYPE_MAC_ADDR  ///< MAC地址
    } config_param_type_t;

    /**
     * @brief 配置参数访问权限枚举
     */
    typedef enum
    {
        CONFIG_ACCESS_READ_ONLY = 0, ///< 只读
        CONFIG_ACCESS_READ_WRITE,    ///< 读写
        CONFIG_ACCESS_ADMIN_ONLY     ///< 管理员专用
    } config_access_t;

    /**
     * @brief 配置参数结构
     */
    typedef struct
    {
        const char *name;                                               ///< 参数名称
        const char *description;                                        ///< 参数描述
        config_param_type_t type;                                       ///< 参数类型
        config_access_t access;                                         ///< 访问权限
        void *value;                                                    ///< 参数值指针
        void *default_value;                                            ///< 默认值指针
        void *min_value;                                                ///< 最小值指针(可选)
        void *max_value;                                                ///< 最大值指针(可选)
        uint16_t size;                                                  ///< 参数大小
        const char *group;                                              ///< 参数组名
        bool (*validator)(const void *value);                           ///< 验证函数(可选)
        void (*callback)(const void *old_value, const void *new_value); ///< 变更回调(可选)
    } config_param_t;

    /**
     * @brief 配置组结构
     */
    typedef struct
    {
        const char *name;             ///< 组名
        const char *description;      ///< 组描述
        const config_param_t *params; ///< 参数数组
        uint16_t param_count;         ///< 参数数量
    } config_group_t;

    /**
     * @brief HTTP请求类型枚举
     */
    typedef enum
    {
        HTTP_METHOD_GET = 0, ///< GET请求
        HTTP_METHOD_POST,    ///< POST请求
        HTTP_METHOD_PUT,     ///< PUT请求
        HTTP_METHOD_DELETE,  ///< DELETE请求
        HTTP_METHOD_OPTIONS  ///< OPTIONS请求
    } http_method_t;

    /**
     * @brief HTTP请求结构
     */
    typedef struct
    {
        http_method_t method; ///< 请求方法
        char uri[256];        ///< 请求URI
        char query[512];      ///< 查询字符串
        char *body;           ///< 请求体
        uint16_t body_len;    ///< 请求体长度
        char headers[1024];   ///< 请求头
        char client_ip[16];   ///< 客户端IP
    } http_request_t;

    /**
     * @brief HTTP响应结构
     */
    typedef struct
    {
        uint16_t status_code;  ///< 状态码
        char *body;            ///< 响应体
        uint16_t body_len;     ///< 响应体长度
        char content_type[64]; ///< 内容类型
        char headers[512];     ///< 响应头
    } http_response_t;

    /**
     * @brief HTTP处理器函数类型
     */
    typedef int (*http_handler_t)(const http_request_t *request, http_response_t *response);

    /**
     * @brief 配置变更事件结构
     */
    typedef struct
    {
        const char *param_name;   ///< 参数名称
        const void *old_value;    ///< 旧值
        const void *new_value;    ///< 新值
        config_param_type_t type; ///< 参数类型
        uint32_t timestamp;       ///< 时间戳
        char client_ip[16];       ///< 客户端IP
    } config_change_event_t;

    /**
     * @brief 配置变更回调函数类型
     */
    typedef void (*config_change_callback_t)(const config_change_event_t *event);

    /**
     * @brief 用户认证信息结构
     */
    typedef struct
    {
        char username[32];            ///< 用户名
        char password_hash[64];       ///< 密码哈希
        config_access_t access_level; ///< 访问级别
        bool enabled;                 ///< 是否启用
    } config_user_t;

    /* ================================ API接口 ================================ */

    /**
     * @brief 初始化配置管理模块
     * @param groups 配置组数组
     * @param group_count 配置组数量
     * @return 0:成功, <0:失败
     */
    int config_manager_init(const config_group_t *groups, uint16_t group_count);

    /**
     * @brief 反初始化配置管理模块
     * @return 0:成功, <0:失败
     */
    int config_manager_deinit(void);

    /**
     * @brief 启动HTTP配置服务器
     * @param port HTTP端口号
     * @param enable_https 是否启用HTTPS
     * @return 0:成功, <0:失败
     */
    int config_start_http_server(uint16_t port, bool enable_https);

    /**
     * @brief 停止HTTP配置服务器
     * @return 0:成功, <0:失败
     */
    int config_stop_http_server(void);

    /**
     * @brief 设置配置变更回调函数
     * @param callback 回调函数
     * @return 0:成功, <0:失败
     */
    int config_set_change_callback(config_change_callback_t callback);

    /* ================================ 参数操作 ================================ */

    /**
     * @brief 获取参数值
     * @param name 参数名称
     * @param value 输出缓冲区
     * @param size 缓冲区大小
     * @return 0:成功, <0:失败
     */
    int config_get_param(const char *name, void *value, uint16_t size);

    /**
     * @brief 设置参数值
     * @param name 参数名称
     * @param value 参数值
     * @param size 值大小
     * @return 0:成功, <0:失败
     */
    int config_set_param(const char *name, const void *value, uint16_t size);

    /**
     * @brief 获取字符串参数
     * @param name 参数名称
     * @param value 输出缓冲区
     * @param max_len 缓冲区最大长度
     * @return 0:成功, <0:失败
     */
    int config_get_string(const char *name, char *value, uint16_t max_len);

    /**
     * @brief 设置字符串参数
     * @param name 参数名称
     * @param value 字符串值
     * @return 0:成功, <0:失败
     */
    int config_set_string(const char *name, const char *value);

    /**
     * @brief 获取整数参数
     * @param name 参数名称
     * @param value 输出值指针
     * @return 0:成功, <0:失败
     */
    int config_get_int(const char *name, int32_t *value);

    /**
     * @brief 设置整数参数
     * @param name 参数名称
     * @param value 整数值
     * @return 0:成功, <0:失败
     */
    int config_set_int(const char *name, int32_t value);

    /**
     * @brief 获取浮点数参数
     * @param name 参数名称
     * @param value 输出值指针
     * @return 0:成功, <0:失败
     */
    int config_get_float(const char *name, float *value);

    /**
     * @brief 设置浮点数参数
     * @param name 参数名称
     * @param value 浮点数值
     * @return 0:成功, <0:失败
     */
    int config_set_float(const char *name, float value);

    /**
     * @brief 获取布尔参数
     * @param name 参数名称
     * @param value 输出值指针
     * @return 0:成功, <0:失败
     */
    int config_get_bool(const char *name, bool *value);

    /**
     * @brief 设置布尔参数
     * @param name 参数名称
     * @param value 布尔值
     * @return 0:成功, <0:失败
     */
    int config_set_bool(const char *name, bool value);

    /* ================================ 配置文件操作 ================================ */

    /**
     * @brief 保存配置到文件
     * @param filename 文件名
     * @return 0:成功, <0:失败
     */
    int config_save_to_file(const char *filename);

    /**
     * @brief 从文件加载配置
     * @param filename 文件名
     * @return 0:成功, <0:失败
     */
    int config_load_from_file(const char *filename);

    /**
     * @brief 导出配置为JSON格式
     * @param buffer 输出缓冲区
     * @param size 缓冲区大小
     * @return JSON字符串长度, <0:失败
     */
    int config_export_json(char *buffer, uint16_t size);

    /**
     * @brief 从JSON导入配置
     * @param json JSON字符串
     * @return 0:成功, <0:失败
     */
    int config_import_json(const char *json);

    /**
     * @brief 重置所有参数为默认值
     * @return 0:成功, <0:失败
     */
    int config_reset_to_defaults(void);

    /**
     * @brief 重置指定组的参数为默认值
     * @param group_name 组名
     * @return 0:成功, <0:失败
     */
    int config_reset_group_to_defaults(const char *group_name);

    /* ================================ 用户认证 ================================ */

    /**
     * @brief 添加用户
     * @param username 用户名
     * @param password 密码
     * @param access_level 访问级别
     * @return 0:成功, <0:失败
     */
    int config_add_user(const char *username, const char *password, config_access_t access_level);

    /**
     * @brief 删除用户
     * @param username 用户名
     * @return 0:成功, <0:失败
     */
    int config_remove_user(const char *username);

    /**
     * @brief 验证用户
     * @param username 用户名
     * @param password 密码
     * @param access_level 输出访问级别
     * @return 0:成功, <0:失败
     */
    int config_authenticate_user(const char *username, const char *password, config_access_t *access_level);

    /**
     * @brief 修改用户密码
     * @param username 用户名
     * @param old_password 旧密码
     * @param new_password 新密码
     * @return 0:成功, <0:失败
     */
    int config_change_password(const char *username, const char *old_password, const char *new_password);

    /* ================================ HTTP处理器 ================================ */

    /**
     * @brief 注册HTTP处理器
     * @param uri URI路径
     * @param method HTTP方法
     * @param handler 处理器函数
     * @return 0:成功, <0:失败
     */
    int config_register_http_handler(const char *uri, http_method_t method, http_handler_t handler);

    /**
     * @brief 处理HTTP请求
     * @param request 请求结构
     * @param response 响应结构
     * @return 0:成功, <0:失败
     */
    int config_handle_http_request(const http_request_t *request, http_response_t *response);

    /* ================================ 内置HTTP处理器 ================================ */

    /**
     * @brief 主页处理器
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 0:成功, <0:失败
     */
    int config_handler_index(const http_request_t *request, http_response_t *response);

    /**
     * @brief 获取配置API处理器
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 0:成功, <0:失败
     */
    int config_handler_get_config(const http_request_t *request, http_response_t *response);

    /**
     * @brief 设置配置API处理器
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 0:成功, <0:失败
     */
    int config_handler_set_config(const http_request_t *request, http_response_t *response);

    /**
     * @brief 重置配置API处理器
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 0:成功, <0:失败
     */
    int config_handler_reset_config(const http_request_t *request, http_response_t *response);

    /**
     * @brief 系统状态API处理器
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 0:成功, <0:失败
     */
    int config_handler_system_status(const http_request_t *request, http_response_t *response);

    /**
     * @brief 重启系统API处理器
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 0:成功, <0:失败
     */
    int config_handler_system_reboot(const http_request_t *request, http_response_t *response);

    /* ================================ 工具函数 ================================ */

    /**
     * @brief 获取参数信息
     * @param name 参数名称
     * @param param 输出参数信息
     * @return 0:成功, <0:失败
     */
    int config_get_param_info(const char *name, const config_param_t **param);

    /**
     * @brief 获取所有参数列表
     * @param params 输出参数数组
     * @param max_count 最大参数数量
     * @return 实际参数数量, <0:失败
     */
    int config_get_all_params(const config_param_t **params, uint16_t max_count);

    /**
     * @brief 获取组内参数列表
     * @param group_name 组名
     * @param params 输出参数数组
     * @param max_count 最大参数数量
     * @return 实际参数数量, <0:失败
     */
    int config_get_group_params(const char *group_name, const config_param_t **params, uint16_t max_count);

    /**
     * @brief 验证参数值
     * @param name 参数名称
     * @param value 参数值
     * @param size 值大小
     * @return true:有效, false:无效
     */
    bool config_validate_param(const char *name, const void *value, uint16_t size);

    /**
     * @brief 获取参数默认值
     * @param name 参数名称
     * @param value 输出缓冲区
     * @param size 缓冲区大小
     * @return 0:成功, <0:失败
     */
    int config_get_default_value(const char *name, void *value, uint16_t size);

    /**
     * @brief 检查参数是否为默认值
     * @param name 参数名称
     * @return true:是默认值, false:不是默认值
     */
    bool config_is_default_value(const char *name);

    /* ================================ 配置模板 ================================ */

    /**
     * @brief 系统配置组
     */
    extern const config_group_t config_group_system;

    /**
     * @brief 网络配置组
     */
    extern const config_group_t config_group_network;

    /**
     * @brief Modbus配置组
     */
    extern const config_group_t config_group_modbus;

    /**
     * @brief LoRa配置组
     */
    extern const config_group_t config_group_lora;

    /**
     * @brief MQTT配置组
     */
    extern const config_group_t config_group_mqtt;

    /**
     * @brief 传感器配置组
     */
    extern const config_group_t config_group_sensors;

    /**
     * @brief 报警配置组
     */
    extern const config_group_t config_group_alarms;

    /**
     * @brief 所有配置组数组
     */
    extern const config_group_t *config_all_groups[];

    /**
     * @brief 配置组总数
     */
    extern const uint16_t config_group_count;

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_MANAGER_H__ */