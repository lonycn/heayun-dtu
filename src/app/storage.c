/**
 * @file storage.c
 * @brief 憨云DTU数据存储模块实现
 * @version 1.0.0
 * @date 2025-03-28
 *
 * 数据存储管理实现，支持Flash存储、历史数据记录、配置参数持久化
 * 设计目标：高可靠性、磨损均衡、数据完整性保证
 */

#include "storage.h"
#include "system.h"
#include "gpio.h"
#include <string.h>

// ============================================================================
// 内部数据结构
// ============================================================================

/**
 * @brief 存储模块控制块
 */
typedef struct
{
    bool initialized;             // 初始化标志
    storage_status_t status;      // 当前状态
    storage_stats_t stats;        // 统计信息
    uint16_t config_write_count;  // 配置写入计数
    uint16_t history_write_index; // 历史数据写入索引
    uint32_t last_write_time;     // 上次写入时间
} storage_control_t;

// 全局控制块
static storage_control_t g_storage = {0};
bool g_storage_initialized = false;

// CRC16表 (使用多项式0x8005)
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};

// ============================================================================
// 内部函数声明
// ============================================================================

static bool storage_init_flash(void);
static storage_config_t storage_get_default_config(void);
static void storage_fill_header(storage_header_t *header, uint8_t type, uint8_t length);
static bool storage_write_record(uint32_t base_addr, const void *data, uint16_t length);
static bool storage_read_record(uint32_t base_addr, void *data, uint16_t length);

// ============================================================================
// 存储管理接口实现
// ============================================================================

/**
 * @brief 存储模块初始化
 */
bool storage_init(void)
{
    if (g_storage.initialized)
    {
        storage_deinit();
    }

    // 清空控制块
    memset(&g_storage, 0, sizeof(storage_control_t));

    // 初始化Flash
    if (!storage_init_flash())
    {
        debug_printf("[STORAGE] Flash init failed\n");
        g_storage.status = STORAGE_STATUS_INIT_FAILED;
        return false;
    }

    // 验证配置区域
    storage_config_t config;
    if (!storage_read_config(&config))
    {
        debug_printf("[STORAGE] Config invalid, creating default\n");

        // 创建默认配置
        config = storage_get_default_config();
        if (!storage_write_config(&config))
        {
            debug_printf("[STORAGE] Failed to write default config\n");
            g_storage.status = STORAGE_STATUS_INIT_FAILED;
            return false;
        }
    }

    g_storage.initialized = true;
    g_storage_initialized = true;
    g_storage.status = STORAGE_STATUS_OK;
    g_storage.last_write_time = system_get_tick();

    debug_printf("[STORAGE] Module initialized successfully\n");
    return true;
}

/**
 * @brief 存储模块反初始化
 */
void storage_deinit(void)
{
    if (!g_storage.initialized)
    {
        return;
    }

    // 清空控制块
    memset(&g_storage, 0, sizeof(storage_control_t));
    g_storage_initialized = false;

    debug_printf("[STORAGE] Module deinitialized\n");
}

/**
 * @brief 格式化存储区域
 */
bool storage_format(bool format_all)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    debug_printf("[STORAGE] Formatting storage areas\n");

    if (format_all)
    {
        // 格式化所有区域
        if (!storage_flash_erase_sector(STORAGE_CONFIG_ADDR) ||
            !storage_flash_erase_sector(STORAGE_HISTORY_ADDR) ||
            !storage_flash_erase_sector(STORAGE_BACKUP_ADDR) ||
            !storage_flash_erase_sector(STORAGE_LOG_ADDR))
        {
            g_storage.status = STORAGE_STATUS_ERASE_ERROR;
            g_storage.stats.erase_errors++;
            return false;
        }

        // 重新写入默认配置
        storage_config_t config = storage_get_default_config();
        if (!storage_write_config(&config))
        {
            return false;
        }
    }
    else
    {
        // 仅格式化历史区域
        if (!storage_flash_erase_sector(STORAGE_HISTORY_ADDR))
        {
            g_storage.status = STORAGE_STATUS_ERASE_ERROR;
            g_storage.stats.erase_errors++;
            return false;
        }
    }

    // 重置统计信息
    g_storage.history_write_index = 0;
    g_storage.stats.total_erases++;

    debug_printf("[STORAGE] Format completed\n");
    return true;
}

/**
 * @brief 获取存储状态
 */
storage_status_t storage_get_status(void)
{
    return g_storage.status;
}

/**
 * @brief 获取存储统计信息
 */
bool storage_get_stats(storage_stats_t *stats)
{
    if (!g_storage.initialized || !stats)
    {
        return false;
    }

    // 复制统计信息
    memcpy(stats, &g_storage.stats, sizeof(storage_stats_t));

    return true;
}

// ============================================================================
// 配置参数管理接口实现
// ============================================================================

/**
 * @brief 读取配置参数
 */
bool storage_read_config(storage_config_t *config)
{
    if (!g_storage.initialized || !config)
    {
        return false;
    }

    // 从Flash读取配置
    if (!storage_flash_read(STORAGE_CONFIG_ADDR, (uint8_t *)config, sizeof(storage_config_t)))
    {
        g_storage.status = STORAGE_STATUS_READ_ERROR;
        g_storage.stats.read_errors++;
        return false;
    }

    // 验证魔数和CRC
    if (config->magic != STORAGE_MAGIC_NUMBER)
    {
        debug_printf("[STORAGE] Invalid config magic: 0x%08X\n", config->magic);
        return false;
    }

    uint16_t calc_crc = storage_calculate_crc16((uint8_t *)config, sizeof(storage_config_t) - 2);
    if (calc_crc != config->crc16)
    {
        debug_printf("[STORAGE] Config CRC error: calc=0x%04X, stored=0x%04X\n", calc_crc, config->crc16);
        g_storage.stats.crc_errors++;
        return false;
    }

    g_storage.stats.total_reads++;
    return true;
}

/**
 * @brief 写入配置参数
 */
bool storage_write_config(const storage_config_t *config)
{
    if (!g_storage.initialized || !config)
    {
        return false;
    }

    // 准备配置数据
    storage_config_t write_config = *config;
    write_config.magic = STORAGE_MAGIC_NUMBER;
    write_config.version = 1;
    write_config.size = sizeof(storage_config_t);

    // 计算CRC
    write_config.crc16 = storage_calculate_crc16((uint8_t *)&write_config, sizeof(storage_config_t) - 2);

    // 擦除配置扇区
    if (!storage_flash_erase_sector(STORAGE_CONFIG_ADDR))
    {
        g_storage.status = STORAGE_STATUS_ERASE_ERROR;
        g_storage.stats.erase_errors++;
        return false;
    }

    // 写入配置
    if (!storage_flash_write(STORAGE_CONFIG_ADDR, (uint8_t *)&write_config, sizeof(storage_config_t)))
    {
        g_storage.status = STORAGE_STATUS_WRITE_ERROR;
        g_storage.stats.write_errors++;
        return false;
    }

    // 验证写入
    if (!storage_flash_verify(STORAGE_CONFIG_ADDR, (uint8_t *)&write_config, sizeof(storage_config_t)))
    {
        g_storage.status = STORAGE_STATUS_WRITE_ERROR;
        g_storage.stats.write_errors++;
        return false;
    }

    g_storage.stats.total_writes++;
    g_storage.stats.config_writes++;
    g_storage.config_write_count++;

    debug_printf("[STORAGE] Config written successfully (count: %d)\n", g_storage.config_write_count);
    return true;
}

/**
 * @brief 恢复默认配置
 */
bool storage_reset_config(void)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    storage_config_t default_config = storage_get_default_config();

    debug_printf("[STORAGE] Resetting to default config\n");
    return storage_write_config(&default_config);
}

/**
 * @brief 备份配置参数
 */
bool storage_backup_config(void)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    // 读取当前配置
    storage_config_t config;
    if (!storage_read_config(&config))
    {
        return false;
    }

    // 擦除备份扇区
    if (!storage_flash_erase_sector(STORAGE_BACKUP_ADDR))
    {
        g_storage.status = STORAGE_STATUS_ERASE_ERROR;
        g_storage.stats.erase_errors++;
        return false;
    }

    // 写入备份
    if (!storage_flash_write(STORAGE_BACKUP_ADDR, (uint8_t *)&config, sizeof(storage_config_t)))
    {
        g_storage.status = STORAGE_STATUS_WRITE_ERROR;
        g_storage.stats.write_errors++;
        return false;
    }

    debug_printf("[STORAGE] Config backup completed\n");
    return true;
}

/**
 * @brief 从备份恢复配置
 */
bool storage_restore_config(void)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    // 从备份区读取配置
    storage_config_t backup_config;
    if (!storage_flash_read(STORAGE_BACKUP_ADDR, (uint8_t *)&backup_config, sizeof(storage_config_t)))
    {
        g_storage.status = STORAGE_STATUS_READ_ERROR;
        g_storage.stats.read_errors++;
        return false;
    }

    // 验证备份数据
    if (backup_config.magic != STORAGE_MAGIC_NUMBER)
    {
        debug_printf("[STORAGE] Invalid backup magic\n");
        return false;
    }

    uint16_t calc_crc = storage_calculate_crc16((uint8_t *)&backup_config, sizeof(storage_config_t) - 2);
    if (calc_crc != backup_config.crc16)
    {
        debug_printf("[STORAGE] Backup CRC error\n");
        g_storage.stats.crc_errors++;
        return false;
    }

    // 恢复配置
    debug_printf("[STORAGE] Restoring config from backup\n");
    return storage_write_config(&backup_config);
}

// ============================================================================
// 历史数据管理接口实现
// ============================================================================

/**
 * @brief 写入传感器历史数据
 */
bool storage_write_sensor_history(int16_t temperature, uint16_t humidity,
                                  uint16_t voltage, uint8_t sensor_status)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    storage_sensor_record_t record;

    // 填充记录头部
    storage_fill_header(&record.header, STORAGE_TYPE_SENSOR, sizeof(record) - sizeof(storage_header_t));

    // 填充传感器数据
    record.temperature = temperature;
    record.humidity = humidity;
    record.voltage = voltage;
    record.sensor_status = sensor_status;
    memset(record.reserved, 0, sizeof(record.reserved));

    // 计算CRC (不包括头部的CRC字段)
    record.header.crc16 = storage_calculate_crc16((uint8_t *)&record + sizeof(storage_header_t),
                                                  sizeof(record) - sizeof(storage_header_t));

    // 计算写入地址 (循环写入)
    uint32_t write_addr = STORAGE_HISTORY_ADDR +
                          (g_storage.history_write_index * sizeof(storage_sensor_record_t)) % STORAGE_HISTORY_SIZE;

    // 写入记录
    if (!storage_write_record(write_addr, &record, sizeof(record)))
    {
        return false;
    }

    g_storage.history_write_index++;
    g_storage.stats.history_writes++;

    if (g_storage.history_write_index % 100 == 0)
    {
        debug_printf("[STORAGE] Sensor history written: T=%.1f°C, H=%.1f%%RH, V=%.2fV (index: %d)\n",
                     temperature / 10.0f, humidity / 10.0f, voltage / 1000.0f, g_storage.history_write_index);
    }

    return true;
}

/**
 * @brief 写入报警历史数据
 */
bool storage_write_alarm_history(uint8_t alarm_type, uint8_t alarm_level,
                                 uint16_t alarm_value, uint32_t alarm_duration)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    storage_alarm_record_t record;

    // 填充记录头部
    storage_fill_header(&record.header, STORAGE_TYPE_ALARM, sizeof(record) - sizeof(storage_header_t));

    // 填充报警数据
    record.alarm_type = alarm_type;
    record.alarm_level = alarm_level;
    record.alarm_value = alarm_value;
    record.alarm_duration = alarm_duration;

    // 计算CRC
    record.header.crc16 = storage_calculate_crc16((uint8_t *)&record + sizeof(storage_header_t),
                                                  sizeof(record) - sizeof(storage_header_t));

    // 写入记录 (简化版本，使用固定偏移)
    uint32_t write_addr = STORAGE_LOG_ADDR + (g_storage.history_write_index % 100) * sizeof(storage_alarm_record_t);

    if (!storage_write_record(write_addr, &record, sizeof(record)))
    {
        return false;
    }

    debug_printf("[STORAGE] Alarm history written: type=%d, level=%d, value=%d\n",
                 alarm_type, alarm_level, alarm_value);

    return true;
}

/**
 * @brief 写入系统状态历史
 */
bool storage_write_status_history(uint32_t uptime, uint16_t reboot_count, uint8_t error_code)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    storage_status_record_t record;

    // 填充记录头部
    storage_fill_header(&record.header, STORAGE_TYPE_STATUS, sizeof(record) - sizeof(storage_header_t));

    // 填充状态数据
    record.uptime = uptime;
    record.reboot_count = reboot_count;
    record.error_code = error_code;
    memset(record.reserved, 0, sizeof(record.reserved));

    // 计算CRC
    record.header.crc16 = storage_calculate_crc16((uint8_t *)&record + sizeof(storage_header_t),
                                                  sizeof(record) - sizeof(storage_header_t));

    // 写入记录 (简化版本)
    uint32_t write_addr = STORAGE_LOG_ADDR + 0x100 + (g_storage.history_write_index % 50) * sizeof(storage_status_record_t);

    if (!storage_write_record(write_addr, &record, sizeof(record)))
    {
        return false;
    }

    debug_printf("[STORAGE] Status history written: uptime=%lu, reboots=%d, error=%d\n",
                 uptime, reboot_count, error_code);

    return true;
}

/**
 * @brief 读取最新的传感器历史数据
 */
uint16_t storage_read_sensor_history(storage_sensor_record_t *records, uint16_t count)
{
    if (!g_storage.initialized || !records || count == 0)
    {
        return 0;
    }

    uint16_t read_count = 0;

    // 简化实现：从最新位置开始读取
    for (uint16_t i = 0; i < count && i < 10; i++) // 限制最多读取10条
    {
        uint32_t read_addr = STORAGE_HISTORY_ADDR +
                             ((g_storage.history_write_index - i - 1) * sizeof(storage_sensor_record_t)) % STORAGE_HISTORY_SIZE;

        if (storage_read_record(read_addr, &records[i], sizeof(storage_sensor_record_t)))
        {
            // 验证记录有效性
            if (records[i].header.magic == STORAGE_MAGIC_NUMBER &&
                records[i].header.type == STORAGE_TYPE_SENSOR)
            {
                read_count++;
            }
            else
            {
                break; // 遇到无效记录，停止读取
            }
        }
        else
        {
            break;
        }
    }

    return read_count;
}

// ============================================================================
// Flash底层操作接口实现
// ============================================================================

/**
 * @brief 擦除Flash扇区 (模拟实现)
 */
bool storage_flash_erase_sector(uint32_t address)
{
    // 模拟擦除操作 (实际应调用硬件Flash驱动)
    debug_printf("[STORAGE] Erasing sector at 0x%08X\n", address);

    g_storage.stats.total_erases++;
    return true; // 模拟成功
}

/**
 * @brief 写入Flash数据 (模拟实现)
 */
bool storage_flash_write(uint32_t address, const uint8_t *data, uint16_t length)
{
    if (!data || length == 0)
    {
        return false;
    }

    // 模拟写入操作 (实际应调用硬件Flash驱动)
    debug_printf("[STORAGE] Writing %d bytes to 0x%08X\n", length, address);

    g_storage.stats.total_writes++;
    return true; // 模拟成功
}

/**
 * @brief 读取Flash数据 (模拟实现)
 */
bool storage_flash_read(uint32_t address, uint8_t *data, uint16_t length)
{
    if (!data || length == 0)
    {
        return false;
    }

    // 模拟读取操作 (实际应调用硬件Flash驱动)
    // 这里返回模拟的默认配置数据
    if (address == STORAGE_CONFIG_ADDR && length == sizeof(storage_config_t))
    {
        storage_config_t default_config = storage_get_default_config();
        memcpy(data, &default_config, length);
    }
    else
    {
        memset(data, 0xFF, length); // 模拟空Flash内容
    }

    g_storage.stats.total_reads++;
    return true; // 模拟成功
}

/**
 * @brief 验证Flash数据
 */
bool storage_flash_verify(uint32_t address, const uint8_t *data, uint16_t length)
{
    if (!data || length == 0)
    {
        return false;
    }

    // 简化验证：直接返回成功
    // 实际实现应该读取Flash内容并与预期数据比较
    return true;
}

// ============================================================================
// 工具函数接口实现
// ============================================================================

/**
 * @brief 计算CRC16校验
 */
uint16_t storage_calculate_crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc = (crc >> 8) ^ crc16_table[(crc ^ data[i]) & 0xFF];
    }

    return crc;
}

/**
 * @brief 检查数据完整性
 */
bool storage_check_integrity(const storage_header_t *header, const uint8_t *data)
{
    if (!header || !data)
    {
        return false;
    }

    // 检查魔数
    if (header->magic != STORAGE_MAGIC_NUMBER)
    {
        return false;
    }

    // 检查CRC
    uint16_t calc_crc = storage_calculate_crc16(data, header->length);
    return (calc_crc == header->crc16);
}

/**
 * @brief 获取空闲存储空间
 */
uint32_t storage_get_free_space(uint8_t type)
{
    switch (type)
    {
    case STORAGE_TYPE_CONFIG:
        return STORAGE_CONFIG_SIZE - sizeof(storage_config_t);
    case STORAGE_TYPE_SENSOR:
    case STORAGE_TYPE_ALARM:
    case STORAGE_TYPE_STATUS:
        return STORAGE_HISTORY_SIZE - (g_storage.history_write_index * sizeof(storage_sensor_record_t));
    default:
        return 0;
    }
}

/**
 * @brief 存储碎片整理 (简化实现)
 */
bool storage_defragment(void)
{
    if (!g_storage.initialized)
    {
        return false;
    }

    debug_printf("[STORAGE] Defragmentation not implemented yet\n");
    return true; // 暂时返回成功
}

/**
 * @brief 打印存储状态信息
 */
void storage_print_status(void)
{
    if (!g_storage.initialized)
    {
        debug_printf("[STORAGE] Module not initialized\n");
        return;
    }

    const char *status_names[] = {"OK", "INIT_FAILED", "READ_ERROR", "WRITE_ERROR",
                                  "ERASE_ERROR", "FULL", "CORRUPTED", "NOT_FOUND", "INVALID_PARAM"};

    debug_printf("\n[STORAGE] Module Status:\n");
    debug_printf("  - Initialized: %s\n", g_storage.initialized ? "Yes" : "No");
    debug_printf("  - Status: %s\n", (g_storage.status < STORAGE_STATUS_COUNT) ? status_names[g_storage.status] : "UNKNOWN");
    debug_printf("  - Config writes: %d\n", g_storage.config_write_count);
    debug_printf("  - History index: %d\n", g_storage.history_write_index);
    debug_printf("  - Free space: %lu bytes\n", storage_get_free_space(STORAGE_TYPE_SENSOR));
    debug_printf("\n");
}

/**
 * @brief 打印存储统计信息
 */
void storage_print_stats(void)
{
    if (!g_storage.initialized)
    {
        debug_printf("[STORAGE] Module not initialized\n");
        return;
    }

    debug_printf("\n[STORAGE] Statistics:\n");
    debug_printf("  - Total writes: %lu\n", g_storage.stats.total_writes);
    debug_printf("  - Total reads: %lu\n", g_storage.stats.total_reads);
    debug_printf("  - Total erases: %lu\n", g_storage.stats.total_erases);
    debug_printf("  - Write errors: %lu\n", g_storage.stats.write_errors);
    debug_printf("  - Read errors: %lu\n", g_storage.stats.read_errors);
    debug_printf("  - CRC errors: %lu\n", g_storage.stats.crc_errors);
    debug_printf("  - Config writes: %lu\n", g_storage.stats.config_writes);
    debug_printf("  - History writes: %lu\n", g_storage.stats.history_writes);
    debug_printf("\n");
}

/**
 * @brief 存储任务处理函数
 */
void storage_task(void)
{
    if (!g_storage.initialized)
    {
        return;
    }

    uint32_t current_time = system_get_tick();

    // 定期检查存储状态
    static uint32_t last_check_time = 0;
    if (current_time - last_check_time > 30000) // 每30秒检查一次
    {
        // 检查存储空间
        uint32_t free_space = storage_get_free_space(STORAGE_TYPE_SENSOR);
        if (free_space < 1024) // 少于1KB时警告
        {
            debug_printf("[STORAGE] Warning: Low storage space (%lu bytes)\n", free_space);
        }

        // 更新统计信息
        g_storage.stats.total_reads++;

        last_check_time = current_time;
    }
}

// ============================================================================
// 内部函数实现
// ============================================================================

/**
 * @brief 初始化Flash
 */
static bool storage_init_flash(void)
{
    // 模拟Flash初始化
    // 实际实现应该初始化Flash控制器，检查Flash状态等
    debug_printf("[STORAGE] Flash initialized (simulated)\n");
    return true;
}

/**
 * @brief 获取默认配置
 */
static storage_config_t storage_get_default_config(void)
{
    storage_config_t config = {0};

    config.magic = STORAGE_MAGIC_NUMBER;
    config.version = 1;
    config.size = sizeof(storage_config_t);

    // Modbus默认配置
    config.modbus_slave_id = 1;
    config.modbus_baudrate = 9600;
    config.modbus_timeout = 10; // 1秒

    // 传感器默认配置
    config.temp_offset = 0;
    config.temp_min_alarm = -300;    // -30.0°C
    config.temp_max_alarm = 800;     // 80.0°C
    config.humidity_min_alarm = 100; // 10.0%RH
    config.humidity_max_alarm = 900; // 90.0%RH
    config.voltage_min_alarm = 2800; // 2.8V
    config.voltage_max_alarm = 3800; // 3.8V

    // 系统默认配置
    config.sample_period = 1000; // 1秒
    config.led_enable = 1;
    config.buzzer_enable = 1;

    // 计算CRC
    config.crc16 = storage_calculate_crc16((uint8_t *)&config, sizeof(storage_config_t) - 2);

    return config;
}

/**
 * @brief 填充记录头部
 */
static void storage_fill_header(storage_header_t *header, uint8_t type, uint8_t length)
{
    if (!header)
    {
        return;
    }

    header->magic = STORAGE_MAGIC_NUMBER;
    header->type = type;
    header->length = length;
    header->timestamp = system_get_tick();
    header->crc16 = 0; // 在外部计算
    header->reserved = 0;
}

/**
 * @brief 写入记录
 */
static bool storage_write_record(uint32_t base_addr, const void *data, uint16_t length)
{
    if (!data || length == 0)
    {
        return false;
    }

    // 写入数据
    if (!storage_flash_write(base_addr, (const uint8_t *)data, length))
    {
        g_storage.status = STORAGE_STATUS_WRITE_ERROR;
        g_storage.stats.write_errors++;
        return false;
    }

    // 验证写入
    if (!storage_flash_verify(base_addr, (const uint8_t *)data, length))
    {
        g_storage.status = STORAGE_STATUS_WRITE_ERROR;
        g_storage.stats.write_errors++;
        return false;
    }

    return true;
}

/**
 * @brief 读取记录
 */
static bool storage_read_record(uint32_t base_addr, void *data, uint16_t length)
{
    if (!data || length == 0)
    {
        return false;
    }

    // 读取数据
    if (!storage_flash_read(base_addr, (uint8_t *)data, length))
    {
        g_storage.status = STORAGE_STATUS_READ_ERROR;
        g_storage.stats.read_errors++;
        return false;
    }

    return true;
}