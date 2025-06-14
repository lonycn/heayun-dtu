/**
 * @file test_system.c
 * @brief 系统核心模块单元测试
 * @version 1.0
 * @date 2025-03-28
 */

#include "../../framework/unity.h"
#include "../../../inc/system.h"
#include <string.h>

// =============================================================================
// 测试夹具(Test Fixtures)
// =============================================================================

static system_config_t test_config;
static system_status_t test_status;

/**
 * @brief 测试前置设置
 */
TEST_SETUP()
{
    // 重置测试配置
    memset(&test_config, 0, sizeof(system_config_t));
    memset(&test_status, 0, sizeof(system_status_t));

    // 设置默认测试配置
    test_config.cpu_freq = SYSTEM_CPU_FREQ_48MHZ;
    test_config.watchdog_timeout = 5000;
    test_config.debug_level = SYSTEM_DEBUG_INFO;
    test_config.auto_sleep = true;
    test_config.sleep_timeout = 30000;
}

/**
 * @brief 测试后置清理
 */
TEST_TEARDOWN()
{
    // 清理测试环境
    system_deinit();
}

// =============================================================================
// 系统初始化测试
// =============================================================================

/**
 * @brief 测试系统初始化
 */
TEST_CASE(system_init_success)
{
    system_result_t result = system_init(&test_config);

    TEST_ASSERT_EQUAL(SYSTEM_OK, result);

    // 验证系统状态
    system_status_t status = system_get_status();
    TEST_ASSERT_EQUAL(SYSTEM_STATE_RUNNING, status.state);
    TEST_ASSERT_TRUE(status.initialized);
}

/**
 * @brief 测试空指针初始化
 */
TEST_CASE(system_init_null_config)
{
    system_result_t result = system_init(NULL);

    TEST_ASSERT_EQUAL(SYSTEM_ERROR_INVALID_PARAM, result);
}

/**
 * @brief 测试重复初始化
 */
TEST_CASE(system_init_already_initialized)
{
    // 第一次初始化
    system_result_t result1 = system_init(&test_config);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result1);

    // 第二次初始化应该返回错误
    system_result_t result2 = system_init(&test_config);
    TEST_ASSERT_EQUAL(SYSTEM_ERROR_ALREADY_INITIALIZED, result2);
}

// =============================================================================
// 系统时钟测试
// =============================================================================

/**
 * @brief 测试系统时钟获取
 */
TEST_CASE(system_get_tick_monotonic)
{
    system_init(&test_config);

    uint32_t tick1 = system_get_tick();
    uint32_t tick2 = system_get_tick();

    // 时钟应该是单调递增的
    TEST_ASSERT_GREATER_THAN(tick1, tick2);
}

/**
 * @brief 测试系统运行时间
 */
TEST_CASE(system_get_runtime)
{
    system_init(&test_config);

    uint32_t runtime1 = system_get_runtime();

    // 模拟一些延时
    system_delay(10);

    uint32_t runtime2 = system_get_runtime();

    // 运行时间应该增加
    TEST_ASSERT_GREATER_THAN(runtime1, runtime2);
    TEST_ASSERT_WITHIN(5, 10, runtime2 - runtime1); // 允许5ms误差
}

// =============================================================================
// 系统延时测试
// =============================================================================

/**
 * @brief 测试系统延时功能
 */
TEST_CASE(system_delay_accuracy)
{
    system_init(&test_config);

    PERFORMANCE_TEST_START();

    system_delay(100); // 延时100ms

    PERFORMANCE_TEST_END(120); // 允许20ms误差
}

/**
 * @brief 测试零延时
 */
TEST_CASE(system_delay_zero)
{
    system_init(&test_config);

    uint32_t start_time = system_get_tick();
    system_delay(0);
    uint32_t end_time = system_get_tick();

    // 零延时应该立即返回
    TEST_ASSERT_WITHIN(2, 0, end_time - start_time);
}

// =============================================================================
// 系统状态测试
// =============================================================================

/**
 * @brief 测试系统状态获取
 */
TEST_CASE(system_get_status_valid)
{
    system_init(&test_config);

    system_status_t status = system_get_status();

    TEST_ASSERT_TRUE(status.initialized);
    TEST_ASSERT_EQUAL(SYSTEM_STATE_RUNNING, status.state);
    TEST_ASSERT_GREATER_THAN(0, status.uptime);
    TEST_ASSERT_GREATER_THAN(0, status.free_memory);
}

/**
 * @brief 测试系统重置
 */
TEST_CASE(system_reset)
{
    system_init(&test_config);

    // 获取重置前状态
    system_status_t status_before = system_get_status();
    TEST_ASSERT_TRUE(status_before.initialized);

    // 执行软重置
    system_result_t result = system_reset(SYSTEM_RESET_SOFT);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result);

    // 验证重置后状态
    system_status_t status_after = system_get_status();
    TEST_ASSERT_FALSE(status_after.initialized);
}

// =============================================================================
// 系统配置测试
// =============================================================================

/**
 * @brief 测试CPU频率设置
 */
TEST_CASE(system_set_cpu_freq)
{
    system_init(&test_config);

    // 测试设置不同频率
    system_result_t result1 = system_set_cpu_freq(SYSTEM_CPU_FREQ_24MHZ);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result1);

    system_result_t result2 = system_set_cpu_freq(SYSTEM_CPU_FREQ_48MHZ);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result2);

    // 测试无效频率
    system_result_t result3 = system_set_cpu_freq(0xFF);
    TEST_ASSERT_EQUAL(SYSTEM_ERROR_INVALID_PARAM, result3);
}

/**
 * @brief 测试调试级别设置
 */
TEST_CASE(system_set_debug_level)
{
    system_init(&test_config);

    // 测试设置不同调试级别
    system_result_t result1 = system_set_debug_level(SYSTEM_DEBUG_ERROR);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result1);

    system_result_t result2 = system_set_debug_level(SYSTEM_DEBUG_DEBUG);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result2);

    // 测试无效级别
    system_result_t result3 = system_set_debug_level(0xFF);
    TEST_ASSERT_EQUAL(SYSTEM_ERROR_INVALID_PARAM, result3);
}

// =============================================================================
// 系统电源管理测试
// =============================================================================

/**
 * @brief 测试系统睡眠
 */
TEST_CASE(system_sleep)
{
    system_init(&test_config);

    uint32_t start_time = system_get_tick();

    // 进入睡眠模式
    system_result_t result = system_sleep(SYSTEM_SLEEP_LIGHT, 100);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result);

    uint32_t end_time = system_get_tick();

    // 验证睡眠时间
    TEST_ASSERT_WITHIN(20, 100, end_time - start_time);
}

/**
 * @brief 测试系统唤醒
 */
TEST_CASE(system_wakeup)
{
    system_init(&test_config);

    // 设置唤醒源
    system_result_t result1 = system_set_wakeup_source(SYSTEM_WAKEUP_TIMER | SYSTEM_WAKEUP_GPIO);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result1);

    // 进入睡眠
    system_result_t result2 = system_sleep(SYSTEM_SLEEP_DEEP, 1000);
    TEST_ASSERT_EQUAL(SYSTEM_OK, result2);

    // 验证唤醒后状态
    system_status_t status = system_get_status();
    TEST_ASSERT_EQUAL(SYSTEM_STATE_RUNNING, status.state);
}

// =============================================================================
// 系统错误处理测试
// =============================================================================

/**
 * @brief 测试错误处理
 */
TEST_CASE(system_error_handling)
{
    system_init(&test_config);

    // 模拟系统错误
    system_report_error(SYSTEM_ERROR_HARDWARE_FAULT, "Test error");

    // 获取错误信息
    system_error_info_t error_info = system_get_last_error();
    TEST_ASSERT_EQUAL(SYSTEM_ERROR_HARDWARE_FAULT, error_info.code);
    TEST_ASSERT_EQUAL_STRING("Test error", error_info.message);
}

/**
 * @brief 测试错误计数
 */
TEST_CASE(system_error_count)
{
    system_init(&test_config);

    uint32_t initial_count = system_get_error_count();

    // 报告几个错误
    system_report_error(SYSTEM_ERROR_TIMEOUT, "Timeout 1");
    system_report_error(SYSTEM_ERROR_TIMEOUT, "Timeout 2");
    system_report_error(SYSTEM_ERROR_INVALID_PARAM, "Invalid param");

    uint32_t final_count = system_get_error_count();
    TEST_ASSERT_EQUAL(initial_count + 3, final_count);
}

// =============================================================================
// 内存管理测试
// =============================================================================

/**
 * @brief 测试内存使用监控
 */
TEST_CASE(system_memory_monitoring)
{
    system_init(&test_config);

    MEMORY_TEST_START();

    // 获取内存信息
    system_memory_info_t mem_info = system_get_memory_info();

    TEST_ASSERT_GREATER_THAN(0, mem_info.total_size);
    TEST_ASSERT_GREATER_THAN(0, mem_info.free_size);
    TEST_ASSERT_LESS_THAN(mem_info.total_size, mem_info.free_size);
    TEST_ASSERT_WITHIN(100, mem_info.total_size - mem_info.free_size, mem_info.used_size);

    MEMORY_TEST_END(0); // 不应该有内存泄漏
}

// =============================================================================
// 性能测试
// =============================================================================

/**
 * @brief 测试系统调用性能
 */
TEST_CASE(system_performance)
{
    system_init(&test_config);

    // 测试system_get_tick性能
    PERFORMANCE_TEST_START();
    for (int i = 0; i < 1000; i++)
    {
        system_get_tick();
    }
    PERFORMANCE_TEST_END(10); // 1000次调用应该在10ms内完成

    // 测试system_get_status性能
    PERFORMANCE_TEST_START();
    for (int i = 0; i < 100; i++)
    {
        system_get_status();
    }
    PERFORMANCE_TEST_END(5); // 100次调用应该在5ms内完成
}

// =============================================================================
// 测试运行器
// =============================================================================

/**
 * @brief 运行所有系统测试
 */
void run_system_tests(void)
{
    printf("\n=== 运行系统核心模块测试 ===\n");

    // 添加所有测试用例
    RUN_TEST(system_init_success);
    RUN_TEST(system_init_null_config);
    RUN_TEST(system_init_already_initialized);

    RUN_TEST(system_get_tick_monotonic);
    RUN_TEST(system_get_runtime);

    RUN_TEST(system_delay_accuracy);
    RUN_TEST(system_delay_zero);

    RUN_TEST(system_get_status_valid);
    RUN_TEST(system_reset);

    RUN_TEST(system_set_cpu_freq);
    RUN_TEST(system_set_debug_level);

    RUN_TEST(system_sleep);
    RUN_TEST(system_wakeup);

    RUN_TEST(system_error_handling);
    RUN_TEST(system_error_count);

    RUN_TEST(system_memory_monitoring);
    RUN_TEST(system_performance);

    printf("系统核心模块测试用例已添加完成\n");
}