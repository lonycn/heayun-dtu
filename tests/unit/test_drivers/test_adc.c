/**
 * @file test_adc.c
 * @brief ADC驱动模块单元测试
 * @version 1.0
 * @date 2025-03-28
 */

#include "../../framework/unity.h"
#include "../../../inc/adc.h"
#include <string.h>

// 测试夹具
static adc_config_t test_config;

TEST_SETUP()
{
    memset(&test_config, 0, sizeof(adc_config_t));
    test_config.channel = ADC_CHANNEL_0;
    test_config.resolution = ADC_RESOLUTION_12BIT;
    test_config.sampling_time = ADC_SAMPLING_TIME_MEDIUM;
    test_config.reference = ADC_REFERENCE_VCC;
}

TEST_TEARDOWN()
{
    adc_deinit();
}

TEST_CASE(adc_init_success)
{
    adc_result_t result = adc_init(&test_config);
    TEST_ASSERT_EQUAL(ADC_OK, result);

    bool is_initialized = adc_is_initialized();
    TEST_ASSERT_TRUE(is_initialized);
}

TEST_CASE(adc_read_single_channel)
{
    adc_init(&test_config);

    uint16_t value;
    adc_result_t result = adc_read_single(ADC_CHANNEL_0, &value);

    TEST_ASSERT_EQUAL(ADC_OK, result);
    TEST_ASSERT_GREATER_THAN(0, value);
    TEST_ASSERT_LESS_THAN(4096, value); // 12位ADC最大值
}

TEST_CASE(adc_read_voltage)
{
    adc_init(&test_config);

    float voltage;
    adc_result_t result = adc_read_voltage(ADC_CHANNEL_0, &voltage);

    TEST_ASSERT_EQUAL(ADC_OK, result);
    TEST_ASSERT_GREATER_THAN(0.0f, voltage);
    TEST_ASSERT_LESS_THAN(3.3f, voltage); // VCC参考电压
}

void run_adc_tests(void)
{
    printf("\n=== 运行ADC驱动模块测试 ===\n");

    RUN_TEST(adc_init_success);
    RUN_TEST(adc_read_single_channel);
    RUN_TEST(adc_read_voltage);

    printf("ADC驱动模块测试用例已添加完成\n");
}