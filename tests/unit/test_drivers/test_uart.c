/**
 * @file test_uart.c
 * @brief UART驱动模块单元测试
 * @version 1.0
 * @date 2025-03-28
 */

#include "../../framework/unity.h"
#include "../../../inc/uart.h"
#include <string.h>

// 测试夹具
static uart_config_t test_config;

TEST_SETUP()
{
    memset(&test_config, 0, sizeof(uart_config_t));
    test_config.port = UART_PORT_1;
    test_config.baudrate = 9600;
    test_config.data_bits = 8;
    test_config.stop_bits = 1;
    test_config.parity = UART_PARITY_NONE;
    test_config.flow_control = UART_FLOW_CONTROL_NONE;
}

TEST_TEARDOWN()
{
    uart_deinit(UART_PORT_1);
}

TEST_CASE(uart_init_success)
{
    uart_result_t result = uart_init(&test_config);
    TEST_ASSERT_EQUAL(UART_OK, result);

    bool is_initialized = uart_is_initialized(test_config.port);
    TEST_ASSERT_TRUE(is_initialized);
}

TEST_CASE(uart_send_data)
{
    uart_init(&test_config);

    uint8_t data[] = "Hello UART";
    uart_result_t result = uart_send_data(test_config.port, data, strlen((char *)data));

    TEST_ASSERT_EQUAL(UART_OK, result);
}

TEST_CASE(uart_receive_data)
{
    uart_init(&test_config);

    uint8_t buffer[64];
    uint16_t received_length;

    uart_result_t result = uart_receive_data(test_config.port, buffer, sizeof(buffer), &received_length, 100);

    // 在没有实际数据的情况下应该超时
    TEST_ASSERT_EQUAL(UART_ERROR_TIMEOUT, result);
}

void run_uart_tests(void)
{
    printf("\n=== 运行UART驱动模块测试 ===\n");

    RUN_TEST(uart_init_success);
    RUN_TEST(uart_send_data);
    RUN_TEST(uart_receive_data);

    printf("UART驱动模块测试用例已添加完成\n");
}