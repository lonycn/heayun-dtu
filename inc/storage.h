/**
 * @file storage.h
 * @brief 憨云DTU数据存储模块接口
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 数据存储管理接口，支持Flash存储、历史数据记录、配置参数持久化
 * 设计目标：高可靠性、磨损均衡、数据完整性保证
 */

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // ============================================================================
    // 宏定义
    // ============================================================================

#define STORAGE_MAX_RECORDS 1000      // 最大历史记录数
#define STORAGE_CONFIG_SIZE 256       // 配置参数区大小
#define STORAGE_HISTORY_SIZE 4096     // 历史数据区大小
#define STORAGE_BACKUP_SIZE 512       // 备份区大小
#define STORAGE_MAGIC_NUMBER 0x48434B // "HCK" 憨云标识

// Flash分区定义 (内部Flash 64KB)
#define STORAGE_CONFIG_ADDR 0x0000F000  // 配置区 (4KB)
#define STORAGE_HISTORY_ADDR 0x0000E000 // 历史区 (4KB)
#define STORAGE_BACKUP_ADDR 0x0000D000  // 备份区 (4KB)
#define STORAGE_LOG_ADDR 0x0000C000     // 日志区 (4KB)

// 数据类型定义
#define STORAGE_TYPE_CONFIG 0x01 // 配置数据
#define STORAGE_TYPE_SENSOR 0x02 // 传感器数据
#define STORAGE_TYPE_ALARM 0x03  // 报警数据
#define STORAGE_TYPE_STATUS 0x04 // 状态数据
#define STORAGE_TYPE_LOG 0x05    // 日志数据

    // 存储状态
    typedef enum
    {
        STORAGE_STATUS_OK = 0,        // 正常
        STORAGE_STATUS_INIT_FAILED,   // 初始化失败
        STORAGE_STATUS_READ_ERROR,    // 读取错误
        STORAGE_STATUS_WRITE_ERROR,   // 写入错误
        STORAGE_STATUS_ERASE_ERROR,   // 擦除错误
        STORAGE_STATUS_FULL,          // 存储空间已满
        STORAGE_STATUS_CORRUPTED,     // 数据损坏
        STORAGE_STATUS_NOT_FOUND,     // 数据未找到
        STORAGE_STATUS_INVALID_PARAM, // 参数无效
        STORAGE_STATUS_COUNT
    } storage_status_t;

    // ============================================================================
    // 数据结构定义
    // ============================================================================

    /**
     * @brief 存储记录头部
     */
    typedef struct
    {
        uint16_t magic;     // 魔数标识
        uint8_t type;       // 数据类型
        uint8_t length;     // 数据长度
        uint32_t timestamp; // 时间戳
        uint16_t crc16;     // CRC16校验
        uint16_t reserved;  // 保留字段
    } __attribute__((packed)) storage_header_t;

    /**
     * @brief 传感器历史数据记录
     */
    typedef struct
    {
        storage_header_t header; // 记录头部
        int16_t temperature;     // 温度 (0.1°C)
        uint16_t humidity;       // 湿度 (0.1%RH)
        uint16_t voltage;        // 电压 (mV)
        uint8_t sensor_status;   // 传感器状态
        uint8_t reserved[3];     // 保留字段
    } __attribute__((packed)) storage_sensor_record_t;

    /**
     * @brief 报警历史数据记录
     */
    typedef struct
    {
        storage_header_t header; // 记录头部
        uint8_t alarm_type;      // 报警类型
        uint8_t alarm_level;     // 报警级别
        uint16_t alarm_value;    // 报警值
        uint32_t alarm_duration; // 报警持续时间
    } __attribute__((packed)) storage_alarm_record_t;

    /**
     * @brief 系统状态记录
     */
    typedef struct
    {
        storage_header_t header; // 记录头部
        uint32_t uptime;         // 运行时间
        uint16_t reboot_count;   // 重启次数
        uint8_t error_code;      // 错误代码
        uint8_t reserved[5];     // 保留字段
    } __attribute__((packed)) storage_status_record_t;

    /**
     * @brief 配置参数结构
     */
    typedef struct
    {
        uint32_t magic;   // 魔数标识
        uint16_t version; // 配置版本
        uint16_t size;    // 配置大小

        // Modbus配置
        uint8_t modbus_slave_id;  // 从站地址
        uint32_t modbus_baudrate; // 波特率
        uint8_t modbus_timeout;   // 超时时间

        // 传感器配置
        int16_t temp_offset;         // 温度偏移(0.1°C)
        uint16_t temp_min_alarm;     // 温度下限报警
        uint16_t temp_max_alarm;     // 温度上限报警
        uint16_t humidity_min_alarm; // 湿度下限报警
        uint16_t humidity_max_alarm; // 湿度上限报警
        uint16_t voltage_min_alarm;  // 电压下限报警
        uint16_t voltage_max_alarm;  // 电压上限报警

        // 系统配置
        uint16_t sample_period; // 采样周期(ms)
        uint8_t led_enable;     // LED使能
        uint8_t buzzer_enable;  // 蜂鸣器使能

        uint8_t reserved[32]; // 保留字段
        uint16_t crc16;       // CRC16校验
    } __attribute__((packed)) storage_config_t;

    /**
     * @brief 存储统计信息
     */
    typedef struct
    {
        uint32_t total_writes;   // 总写入次数
        uint32_t total_reads;    // 总读取次数
        uint32_t total_erases;   // 总擦除次数
        uint32_t write_errors;   // 写入错误次数
        uint32_t read_errors;    // 读取错误次数
        uint32_t erase_errors;   // 擦除错误次数
        uint32_t crc_errors;     // CRC错误次数
        uint32_t config_writes;  // 配置写入次数
        uint32_t history_writes; // 历史数据写入次数
        uint32_t free_space;     // 剩余空间
    } storage_stats_t;

    // ============================================================================
    // 存储管理接口
    // ============================================================================

    /**
     * @brief 存储模块初始化
     * @return true: 成功, false: 失败
     */
    bool storage_init(void);

    /**
     * @brief 存储模块反初始化
     */
    void storage_deinit(void);

    /**
     * @brief 格式化存储区域
     * @param format_all true: 格式化所有区域, false: 仅格式化历史区
     * @return true: 成功, false: 失败
     */
    bool storage_format(bool format_all);

    /**
     * @brief 获取存储状态
     * @return 存储状态
     */
    storage_status_t storage_get_status(void);

    /**
     * @brief 获取存储统计信息
     * @param stats 统计信息结构体指针
     * @return true: 成功, false: 失败
     */
    bool storage_get_stats(storage_stats_t *stats);

    // ============================================================================
    // 配置参数管理接口
    // ============================================================================

    /**
     * @brief 读取配置参数
     * @param config 配置结构体指针
     * @return true: 成功, false: 失败
     */
    bool storage_read_config(storage_config_t *config);

    /**
     * @brief 写入配置参数
     * @param config 配置结构体指针
     * @return true: 成功, false: 失败
     */
    bool storage_write_config(const storage_config_t *config);

    /**
     * @brief 恢复默认配置
     * @return true: 成功, false: 失败
     */
    bool storage_reset_config(void);

    /**
     * @brief 备份配置参数
     * @return true: 成功, false: 失败
     */
    bool storage_backup_config(void);

    /**
     * @brief 从备份恢复配置
     * @return true: 成功, false: 失败
     */
    bool storage_restore_config(void);

    // ============================================================================
    // 历史数据管理接口
    // ============================================================================

    /**
     * @brief 写入传感器历史数据
     * @param temperature 温度 (0.1°C)
     * @param humidity 湿度 (0.1%RH)
     * @param voltage 电压 (mV)
     * @param sensor_status 传感器状态
     * @return true: 成功, false: 失败
     */
    bool storage_write_sensor_history(int16_t temperature, uint16_t humidity,
                                      uint16_t voltage, uint8_t sensor_status);

    /**
     * @brief 写入报警历史数据
     * @param alarm_type 报警类型
     * @param alarm_level 报警级别
     * @param alarm_value 报警值
     * @param alarm_duration 报警持续时间
     * @return true: 成功, false: 失败
     */
    bool storage_write_alarm_history(uint8_t alarm_type, uint8_t alarm_level,
                                     uint16_t alarm_value, uint32_t alarm_duration);

    /**
     * @brief 写入系统状态历史
     * @param uptime 运行时间
     * @param reboot_count 重启次数
     * @param error_code 错误代码
     * @return true: 成功, false: 失败
     */
    bool storage_write_status_history(uint32_t uptime, uint16_t reboot_count, uint8_t error_code);

    /**
     * @brief 读取最新的传感器历史数据
     * @param records 记录数组指针
     * @param count 要读取的记录数量
     * @return 实际读取的记录数量
     */
    uint16_t storage_read_sensor_history(storage_sensor_record_t *records, uint16_t count);

    /**
     * @brief 读取最新的报警历史数据
     * @param records 记录数组指针
     * @param count 要读取的记录数量
     * @return 实际读取的记录数量
     */
    uint16_t storage_read_alarm_history(storage_alarm_record_t *records, uint16_t count);

    /**
     * @brief 读取最新的状态历史数据
     * @param records 记录数组指针
     * @param count 要读取的记录数量
     * @return 实际读取的记录数量
     */
    uint16_t storage_read_status_history(storage_status_record_t *records, uint16_t count);

    /**
     * @brief 清除历史数据
     * @param type 数据类型 (0xFF: 清除所有类型)
     * @return true: 成功, false: 失败
     */
    bool storage_clear_history(uint8_t type);

    /**
     * @brief 获取历史数据记录数量
     * @param type 数据类型
     * @return 记录数量
     */
    uint16_t storage_get_history_count(uint8_t type);

    // ============================================================================
    // Flash底层操作接口
    // ============================================================================

    /**
     * @brief 擦除Flash扇区
     * @param address 地址
     * @return true: 成功, false: 失败
     */
    bool storage_flash_erase_sector(uint32_t address);

    /**
     * @brief 写入Flash数据
     * @param address 地址
     * @param data 数据指针
     * @param length 数据长度
     * @return true: 成功, false: 失败
     */
    bool storage_flash_write(uint32_t address, const uint8_t *data, uint16_t length);

    /**
     * @brief 读取Flash数据
     * @param address 地址
     * @param data 数据指针
     * @param length 数据长度
     * @return true: 成功, false: 失败
     */
    bool storage_flash_read(uint32_t address, uint8_t *data, uint16_t length);

    /**
     * @brief 验证Flash数据
     * @param address 地址
     * @param data 数据指针
     * @param length 数据长度
     * @return true: 验证成功, false: 验证失败
     */
    bool storage_flash_verify(uint32_t address, const uint8_t *data, uint16_t length);

    // ============================================================================
    // 工具函数接口
    // ============================================================================

    /**
     * @brief 计算CRC16校验
     * @param data 数据指针
     * @param length 数据长度
     * @return CRC16值
     */
    uint16_t storage_calculate_crc16(const uint8_t *data, uint16_t length);

    /**
     * @brief 检查数据完整性
     * @param header 记录头部指针
     * @param data 数据指针
     * @return true: 完整, false: 损坏
     */
    bool storage_check_integrity(const storage_header_t *header, const uint8_t *data);

    /**
     * @brief 获取空闲存储空间
     * @param type 存储区域类型
     * @return 空闲空间大小
     */
    uint32_t storage_get_free_space(uint8_t type);

    /**
     * @brief 存储碎片整理
     * @return true: 成功, false: 失败
     */
    bool storage_defragment(void);

    /**
     * @brief 打印存储状态信息 (调试用)
     */
    void storage_print_status(void);

    /**
     * @brief 打印存储统计信息 (调试用)
     */
    void storage_print_stats(void);

    // ============================================================================
    // 内联函数
    // ============================================================================

    /**
     * @brief 检查存储是否已初始化
     */
    static inline bool storage_is_initialized(void)
    {
        extern bool g_storage_initialized;
        return g_storage_initialized;
    }

    /**
     * @brief 检查数据类型是否有效
     */
    static inline bool storage_is_valid_type(uint8_t type)
    {
        return (type >= STORAGE_TYPE_CONFIG && type <= STORAGE_TYPE_LOG);
    }

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_H__ */

// ============================================================================
// 使用示例
// ============================================================================

#if 0
// 初始化存储模块
if (!storage_init()) {
    debug_printf("Storage init failed\n");
    return false;
}

// 读取配置
storage_config_t config;
if (storage_read_config(&config)) {
    debug_printf("Modbus slave ID: %d\n", config.modbus_slave_id);
}

// 写入传感器历史数据
storage_write_sensor_history(250, 650, 3300, 0); // 25.0°C, 65.0%RH, 3.3V

// 读取历史数据
storage_sensor_record_t records[10];
uint16_t count = storage_read_sensor_history(records, 10);
debug_printf("Read %d sensor records\n", count);

// 打印存储状态
storage_print_status();
#endif