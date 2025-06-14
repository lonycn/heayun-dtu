/**
 * @file test_modbus.c
 * @brief Modbus通信模块单元测试
 * @version 1.0
 * @date 2025-03-28
 */

#include "../../framework/unity.h"
#include "../../../inc/modbus.h"
#include <string.h>

// =============================================================================
// 测试夹具(Test Fixtures)
// =============================================================================

static modbus_config_t test_config;
static modbus_master_config_t master_config;
static modbus_slave_config_t slave_config;
static uint8_t test_buffer[256];

/**
 * @brief 测试前置设置
 */
TEST_SETUP()
{
    // 重置测试配置
    memset(&test_config, 0, sizeof(modbus_config_t));
    memset(&master_config, 0, sizeof(modbus_master_config_t));
    memset(&slave_config, 0, sizeof(modbus_slave_config_t));
    memset(test_buffer, 0, sizeof(test_buffer));

    // 设置默认测试配置
    test_config.mode = MODBUS_MODE_RTU;
    test_config.role = MODBUS_ROLE_SLAVE;
    test_config.slave_id = 1;
    test_config.baudrate = 9600;
    test_config.data_bits = 8;
    test_config.stop_bits = 1;
    test_config.parity = MODBUS_PARITY_NONE;
    test_config.timeout = 1000;

    // 主机配置
    master_config.timeout = 1000;
    master_config.retry_count = 3;
    master_config.inter_frame_delay = 10;

    // 从机配置
    slave_config.slave_id = 1;
    slave_config.holding_registers_start = 0;
    slave_config.holding_registers_count = 100;
    slave_config.input_registers_start = 0;
    slave_config.input_registers_count = 50;
    slave_config.coils_start = 0;
    slave_config.coils_count = 64;
    slave_config.discrete_inputs_start = 0;
    slave_config.discrete_inputs_count = 32;
}

/**
 * @brief 测试后置清理
 */
TEST_TEARDOWN()
{
    // 清理Modbus环境
    modbus_deinit();
}

// =============================================================================
// Modbus初始化测试
// =============================================================================

/**
 * @brief 测试Modbus初始化成功
 */
TEST_CASE(modbus_init_success)
{
    modbus_result_t result = modbus_init(&test_config);

    TEST_ASSERT_EQUAL(MODBUS_OK, result);

    // 验证初始化状态
    bool is_initialized = modbus_is_initialized();
    TEST_ASSERT_TRUE(is_initialized);

    // 验证配置
    modbus_config_t read_config;
    modbus_get_config(&read_config);
    TEST_ASSERT_EQUAL(test_config.mode, read_config.mode);
    TEST_ASSERT_EQUAL(test_config.role, read_config.role);
    TEST_ASSERT_EQUAL(test_config.slave_id, read_config.slave_id);
}

/**
 * @brief 测试空指针初始化
 */
TEST_CASE(modbus_init_null_config)
{
    modbus_result_t result = modbus_init(NULL);

    TEST_ASSERT_EQUAL(MODBUS_ERROR_INVALID_PARAM, result);
}

/**
 * @brief 测试无效配置初始化
 */
TEST_CASE(modbus_init_invalid_config)
{
    // 测试无效波特率
    test_config.baudrate = 0;
    modbus_result_t result1 = modbus_init(&test_config);
    TEST_ASSERT_EQUAL(MODBUS_ERROR_INVALID_PARAM, result1);

    // 测试无效从机ID
    test_config.baudrate = 9600;
    test_config.slave_id = 0; // 0是无效的从机ID
    modbus_result_t result2 = modbus_init(&test_config);
    TEST_ASSERT_EQUAL(MODBUS_ERROR_INVALID_PARAM, result2);
}

// =============================================================================
// Modbus主机模式测试
// =============================================================================

/**
 * @brief 测试主机模式初始化
 */
TEST_CASE(modbus_master_init)
{
    test_config.role = MODBUS_ROLE_MASTER;
    modbus_init(&test_config);

    modbus_result_t result = modbus_master_configure(&master_config);
    TEST_ASSERT_EQUAL(MODBUS_OK, result);

    // 验证主机状态
    modbus_master_status_t status = modbus_master_get_status();
    TEST_ASSERT_TRUE(status.initialized);
    TEST_ASSERT_EQUAL(MODBUS_MASTER_STATE_IDLE, status.state);
}

/**
 * @brief 测试读取保持寄存器
 */
TEST_CASE(modbus_read_holding_registers)
{
    test_config.role = MODBUS_ROLE_MASTER;
    modbus_init(&test_config);
    modbus_master_configure(&master_config);

    uint16_t registers[10];
    modbus_result_t result = modbus_read_holding_registers(1, 0, 10, registers);

    // 在没有实际从机的情况下，应该返回超时错误
    TEST_ASSERT_EQUAL(MODBUS_ERROR_TIMEOUT, result);
}

/**
 * @brief 测试写入单个寄存器
 */
TEST_CASE(modbus_write_single_register)
{
    test_config.role = MODBUS_ROLE_MASTER;
    modbus_init(&test_config);
    modbus_master_configure(&master_config);

    modbus_result_t result = modbus_write_single_register(1, 0, 0x1234);

    // 在没有实际从机的情况下，应该返回超时错误
    TEST_ASSERT_EQUAL(MODBUS_ERROR_TIMEOUT, result);
}

/**
 * @brief 测试写入多个寄存器
 */
TEST_CASE(modbus_write_multiple_registers)
{
    test_config.role = MODBUS_ROLE_MASTER;
    modbus_init(&test_config);
    modbus_master_configure(&master_config);

    uint16_t registers[] = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    modbus_result_t result = modbus_write_multiple_registers(1, 0, 4, registers);

    // 在没有实际从机的情况下，应该返回超时错误
    TEST_ASSERT_EQUAL(MODBUS_ERROR_TIMEOUT, result);
}

/**
 * @brief 测试读取线圈
 */
TEST_CASE(modbus_read_coils)
{
    test_config.role = MODBUS_ROLE_MASTER;
    modbus_init(&test_config);
    modbus_master_configure(&master_config);

    uint8_t coils[8]; // 64个线圈，8字节
    modbus_result_t result = modbus_read_coils(1, 0, 64, coils);

    // 在没有实际从机的情况下，应该返回超时错误
    TEST_ASSERT_EQUAL(MODBUS_ERROR_TIMEOUT, result);
}

/**
 * @brief 测试写入单个线圈
 */
TEST_CASE(modbus_write_single_coil)
{
    test_config.role = MODBUS_ROLE_MASTER;
    modbus_init(&test_config);
    modbus_master_configure(&master_config);

    modbus_result_t result = modbus_write_single_coil(1, 0, true);

    // 在没有实际从机的情况下，应该返回超时错误
    TEST_ASSERT_EQUAL(MODBUS_ERROR_TIMEOUT, result);
}

// =============================================================================
// Modbus从机模式测试
// =============================================================================

/**
 * @brief 测试从机模式初始化
 */
TEST_CASE(modbus_slave_init)
{
    test_config.role = MODBUS_ROLE_SLAVE;
    modbus_init(&test_config);

    modbus_result_t result = modbus_slave_configure(&slave_config);
    TEST_ASSERT_EQUAL(MODBUS_OK, result);

    // 验证从机状态
    modbus_slave_status_t status = modbus_slave_get_status();
    TEST_ASSERT_TRUE(status.initialized);
    TEST_ASSERT_EQUAL(MODBUS_SLAVE_STATE_LISTENING, status.state);
}

/**
 * @brief 测试从机寄存器设置
 */
TEST_CASE(modbus_slave_set_holding_register)
{
    test_config.role = MODBUS_ROLE_SLAVE;
    modbus_init(&test_config);
    modbus_slave_configure(&slave_config);

    // 设置保持寄存器值
    modbus_result_t result1 = modbus_slave_set_holding_register(0, 0x1234);
    TEST_ASSERT_EQUAL(MODBUS_OK, result1);

    modbus_result_t result2 = modbus_slave_set_holding_register(1, 0x5678);
    TEST_ASSERT_EQUAL(MODBUS_OK, result2);

    // 读取保持寄存器值
    uint16_t value1 = modbus_slave_get_holding_register(0);
    uint16_t value2 = modbus_slave_get_holding_register(1);

    TEST_ASSERT_EQUAL(0x1234, value1);
    TEST_ASSERT_EQUAL(0x5678, value2);
}

/**
 * @brief 测试从机输入寄存器设置
 */
TEST_CASE(modbus_slave_set_input_register)
{
    test_config.role = MODBUS_ROLE_SLAVE;
    modbus_init(&test_config);
    modbus_slave_configure(&slave_config);

    // 设置输入寄存器值
    modbus_result_t result = modbus_slave_set_input_register(0, 0xABCD);
    TEST_ASSERT_EQUAL(MODBUS_OK, result);

    // 读取输入寄存器值
    uint16_t value = modbus_slave_get_input_register(0);
    TEST_ASSERT_EQUAL(0xABCD, value);
}

/**
 * @brief 测试从机线圈设置
 */
TEST_CASE(modbus_slave_set_coil)
{
    test_config.role = MODBUS_ROLE_SLAVE;
    modbus_init(&test_config);
    modbus_slave_configure(&slave_config);

    // 设置线圈状态
    modbus_result_t result1 = modbus_slave_set_coil(0, true);
    TEST_ASSERT_EQUAL(MODBUS_OK, result1);

    modbus_result_t result2 = modbus_slave_set_coil(1, false);
    TEST_ASSERT_EQUAL(MODBUS_OK, result2);

    // 读取线圈状态
    bool state1 = modbus_slave_get_coil(0);
    bool state2 = modbus_slave_get_coil(1);

    TEST_ASSERT_TRUE(state1);
    TEST_ASSERT_FALSE(state2);
}

/**
 * @brief 测试从机离散输入设置
 */
TEST_CASE(modbus_slave_set_discrete_input)
{
    test_config.role = MODBUS_ROLE_SLAVE;
    modbus_init(&test_config);
    modbus_slave_configure(&slave_config);

    // 设置离散输入状态
    modbus_result_t result = modbus_slave_set_discrete_input(0, true);
    TEST_ASSERT_EQUAL(MODBUS_OK, result);

    // 读取离散输入状态
    bool state = modbus_slave_get_discrete_input(0);
    TEST_ASSERT_TRUE(state);
}

// =============================================================================
// Modbus协议测试
// =============================================================================

/**
 * @brief 测试CRC计算
 */
TEST_CASE(modbus_crc_calculation)
{
    uint8_t data[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
    uint16_t expected_crc = 0x840A; // 预期的CRC值

    uint16_t calculated_crc = modbus_calculate_crc(data, sizeof(data));

    TEST_ASSERT_EQUAL(expected_crc, calculated_crc);
}

/**
 * @brief 测试CRC验证
 */
TEST_CASE(modbus_crc_verification)
{
    uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x84}; // 包含CRC的完整帧

    bool is_valid = modbus_verify_crc(frame, sizeof(frame));

    TEST_ASSERT_TRUE(is_valid);

    // 测试错误的CRC
    frame[7] = 0x85; // 修改CRC
    is_valid = modbus_verify_crc(frame, sizeof(frame));
    TEST_ASSERT_FALSE(is_valid);
}

/**
 * @brief 测试帧构建
 */
TEST_CASE(modbus_build_frame)
{
    uint8_t frame[256];
    uint16_t frame_length;

    // 构建读取保持寄存器的请求帧
    modbus_result_t result = modbus_build_read_holding_registers_request(
        1, // 从机ID
        0, // 起始地址
        1, // 寄存器数量
        frame,
        &frame_length);

    TEST_ASSERT_EQUAL(MODBUS_OK, result);
    TEST_ASSERT_EQUAL(8, frame_length); // RTU模式下应该是8字节

    // 验证帧内容
    TEST_ASSERT_EQUAL(0x01, frame[0]); // 从机ID
    TEST_ASSERT_EQUAL(0x03, frame[1]); // 功能码
    TEST_ASSERT_EQUAL(0x00, frame[2]); // 起始地址高字节
    TEST_ASSERT_EQUAL(0x00, frame[3]); // 起始地址低字节
    TEST_ASSERT_EQUAL(0x00, frame[4]); // 寄存器数量高字节
    TEST_ASSERT_EQUAL(0x01, frame[5]); // 寄存器数量低字节

    // 验证CRC
    bool crc_valid = modbus_verify_crc(frame, frame_length);
    TEST_ASSERT_TRUE(crc_valid);
}

/**
 * @brief 测试帧解析
 */
TEST_CASE(modbus_parse_frame)
{
    // 读取保持寄存器的响应帧
    uint8_t response[] = {0x01, 0x03, 0x02, 0x12, 0x34, 0x79, 0x14};

    modbus_frame_info_t frame_info;
    modbus_result_t result = modbus_parse_frame(response, sizeof(response), &frame_info);

    TEST_ASSERT_EQUAL(MODBUS_OK, result);
    TEST_ASSERT_EQUAL(0x01, frame_info.slave_id);
    TEST_ASSERT_EQUAL(0x03, frame_info.function_code);
    TEST_ASSERT_EQUAL(2, frame_info.data_length);
    TEST_ASSERT_EQUAL(0x1234, *(uint16_t *)frame_info.data);
}

// =============================================================================
// Modbus异常处理测试
// =============================================================================

/**
 * @brief 测试异常响应构建
 */
TEST_CASE(modbus_build_exception_response)
{
    uint8_t response[256];
    uint16_t response_length;

    modbus_result_t result = modbus_build_exception_response(
        1,                                 // 从机ID
        0x03,                              // 原功能码
        MODBUS_EXCEPTION_ILLEGAL_FUNCTION, // 异常码
        response,
        &response_length);

    TEST_ASSERT_EQUAL(MODBUS_OK, result);
    TEST_ASSERT_EQUAL(5, response_length); // 异常响应长度

    // 验证异常响应内容
    TEST_ASSERT_EQUAL(0x01, response[0]);                              // 从机ID
    TEST_ASSERT_EQUAL(0x83, response[1]);                              // 异常功能码 (0x03 | 0x80)
    TEST_ASSERT_EQUAL(MODBUS_EXCEPTION_ILLEGAL_FUNCTION, response[2]); // 异常码

    // 验证CRC
    bool crc_valid = modbus_verify_crc(response, response_length);
    TEST_ASSERT_TRUE(crc_valid);
}

/**
 * @brief 测试异常响应解析
 */
TEST_CASE(modbus_parse_exception_response)
{
    uint8_t exception_response[] = {0x01, 0x83, 0x02, 0xC0, 0xF1}; // 非法数据地址异常

    modbus_frame_info_t frame_info;
    modbus_result_t result = modbus_parse_frame(exception_response, sizeof(exception_response), &frame_info);

    TEST_ASSERT_EQUAL(MODBUS_OK, result);
    TEST_ASSERT_EQUAL(0x01, frame_info.slave_id);
    TEST_ASSERT_EQUAL(0x83, frame_info.function_code); // 异常功能码
    TEST_ASSERT_TRUE(frame_info.is_exception);
    TEST_ASSERT_EQUAL(MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS, frame_info.exception_code);
}

// =============================================================================
// Modbus通信测试
// =============================================================================

/**
 * @brief 测试数据发送
 */
TEST_CASE(modbus_send_data)
{
    test_config.role = MODBUS_ROLE_MASTER;
    modbus_init(&test_config);

    uint8_t data[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x84};

    modbus_result_t result = modbus_send_data(data, sizeof(data));

    // 在测试环境中，发送应该成功
    TEST_ASSERT_EQUAL(MODBUS_OK, result);
}

/**
 * @brief 测试数据接收
 */
TEST_CASE(modbus_receive_data)
{
    test_config.role = MODBUS_ROLE_SLAVE;
    modbus_init(&test_config);

    uint8_t buffer[256];
    uint16_t received_length;

    // 在没有实际数据的情况下，应该返回超时
    modbus_result_t result = modbus_receive_data(buffer, sizeof(buffer), &received_length, 100);

    TEST_ASSERT_EQUAL(MODBUS_ERROR_TIMEOUT, result);
}

// =============================================================================
// 性能测试
// =============================================================================

/**
 * @brief 测试CRC计算性能
 */
TEST_CASE(modbus_crc_performance)
{
    uint8_t data[256];
    for (int i = 0; i < 256; i++)
    {
        data[i] = (uint8_t)i;
    }

    PERFORMANCE_TEST_START();

    for (int i = 0; i < 1000; i++)
    {
        modbus_calculate_crc(data, sizeof(data));
    }

    PERFORMANCE_TEST_END(100); // 1000次CRC计算应该在100ms内完成
}

/**
 * @brief 测试帧构建性能
 */
TEST_CASE(modbus_frame_build_performance)
{
    uint8_t frame[256];
    uint16_t frame_length;

    PERFORMANCE_TEST_START();

    for (int i = 0; i < 1000; i++)
    {
        modbus_build_read_holding_registers_request(1, i % 100, 1, frame, &frame_length);
    }

    PERFORMANCE_TEST_END(50); // 1000次帧构建应该在50ms内完成
}

// =============================================================================
// 内存测试
// =============================================================================

/**
 * @brief 测试内存使用
 */
TEST_CASE(modbus_memory_usage)
{
    MEMORY_TEST_START();

    // 初始化Modbus
    modbus_init(&test_config);
    modbus_slave_configure(&slave_config);

    // 执行一些操作
    for (int i = 0; i < 100; i++)
    {
        modbus_slave_set_holding_register(i % 100, i);
        modbus_slave_get_holding_register(i % 100);
    }

    // 清理
    modbus_deinit();

    MEMORY_TEST_END(0); // 不应该有内存泄漏
}

// =============================================================================
// 错误处理测试
// =============================================================================

/**
 * @brief 测试错误处理
 */
TEST_CASE(modbus_error_handling)
{
    // 测试未初始化操作
    modbus_result_t result1 = modbus_read_holding_registers(1, 0, 1, NULL);
    TEST_ASSERT_EQUAL(MODBUS_ERROR_NOT_INITIALIZED, result1);

    // 初始化后测试无效参数
    modbus_init(&test_config);

    modbus_result_t result2 = modbus_read_holding_registers(0, 0, 1, NULL); // 无效从机ID
    TEST_ASSERT_EQUAL(MODBUS_ERROR_INVALID_PARAM, result2);

    uint16_t registers[10];
    modbus_result_t result3 = modbus_read_holding_registers(1, 0, 0, registers); // 无效数量
    TEST_ASSERT_EQUAL(MODBUS_ERROR_INVALID_PARAM, result3);
}

// =============================================================================
// 测试运行器
// =============================================================================

/**
 * @brief 运行所有Modbus测试
 */
void run_modbus_tests(void)
{
    printf("\n=== 运行Modbus通信模块测试 ===\n");

    // 初始化测试
    RUN_TEST(modbus_init_success);
    RUN_TEST(modbus_init_null_config);
    RUN_TEST(modbus_init_invalid_config);

    // 主机模式测试
    RUN_TEST(modbus_master_init);
    RUN_TEST(modbus_read_holding_registers);
    RUN_TEST(modbus_write_single_register);
    RUN_TEST(modbus_write_multiple_registers);
    RUN_TEST(modbus_read_coils);
    RUN_TEST(modbus_write_single_coil);

    // 从机模式测试
    RUN_TEST(modbus_slave_init);
    RUN_TEST(modbus_slave_set_holding_register);
    RUN_TEST(modbus_slave_set_input_register);
    RUN_TEST(modbus_slave_set_coil);
    RUN_TEST(modbus_slave_set_discrete_input);

    // 协议测试
    RUN_TEST(modbus_crc_calculation);
    RUN_TEST(modbus_crc_verification);
    RUN_TEST(modbus_build_frame);
    RUN_TEST(modbus_parse_frame);

    // 异常处理测试
    RUN_TEST(modbus_build_exception_response);
    RUN_TEST(modbus_parse_exception_response);

    // 通信测试
    RUN_TEST(modbus_send_data);
    RUN_TEST(modbus_receive_data);

    // 性能测试
    RUN_TEST(modbus_crc_performance);
    RUN_TEST(modbus_frame_build_performance);

    // 内存测试
    RUN_TEST(modbus_memory_usage);

    // 错误处理测试
    RUN_TEST(modbus_error_handling);

    printf("Modbus通信模块测试用例已添加完成\n");
}