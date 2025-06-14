/**
 * @file test_sensor.c
 * @brief 传感器管理模块单元测试
 * @version 1.0
 * @date 2025-03-28
 */

#include "../../framework/unity.h"
#include "../../../inc/sensor.h"
#include <string.h>

// 测试夹具
static sensor_config_t test_config;

TEST_SETUP()
{
    memset(&test_config, 0, sizeof(sensor_config_t));
    test_config.dht22_pin = GPIO_PIN_2;
    test_config.voltage_channel = ADC_CHANNEL_1;
    test_config.sample_interval = 1000;
    test_config.filter_enabled = true;
    test_config.filter_window_size = 5;
}

TEST_TEARDOWN()
{
    sensor_deinit();
}

TEST_CASE(sensor_init_success)
{
    sensor_result_t result = sensor_init(&test_config);
    TEST_ASSERT_EQUAL(SENSOR_OK, result);

    bool is_initialized = sensor_is_initialized();
    TEST_ASSERT_TRUE(is_initialized);
}

TEST_CASE(sensor_read_temperature)
{
    sensor_init(&test_config);

    float temperature;
    sensor_result_t result = sensor_read_temperature(&temperature);

    TEST_ASSERT_EQUAL(SENSOR_OK, result);
    TEST_ASSERT_GREATER_THAN(-40.0f, temperature);
    TEST_ASSERT_LESS_THAN(80.0f, temperature);
}

TEST_CASE(sensor_read_humidity)
{
    sensor_init(&test_config);

    float humidity;
    sensor_result_t result = sensor_read_humidity(&humidity);

    TEST_ASSERT_EQUAL(SENSOR_OK, result);
    TEST_ASSERT_GREATER_THAN(0.0f, humidity);
    TEST_ASSERT_LESS_THAN(100.0f, humidity);
}

TEST_CASE(sensor_read_voltage)
{
    sensor_init(&test_config);

    float voltage;
    sensor_result_t result = sensor_read_voltage(&voltage);

    TEST_ASSERT_EQUAL(SENSOR_OK, result);
    TEST_ASSERT_GREATER_THAN(0.0f, voltage);
    TEST_ASSERT_LESS_THAN(30.0f, voltage);
}

void run_sensor_tests(void)
{
    printf("\n=== 运行传感器管理模块测试 ===\n");

    RUN_TEST(sensor_init_success);
    RUN_TEST(sensor_read_temperature);
    RUN_TEST(sensor_read_humidity);
    RUN_TEST(sensor_read_voltage);

    printf("传感器管理模块测试用例已添加完成\n");
}