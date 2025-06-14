/**
 * @file unity.c
 * @brief 憨云DTU轻量级单元测试框架实现
 * @version 1.0
 * @date 2025-03-28
 */

#include "unity.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// =============================================================================
// 全局变量定义
// =============================================================================

unity_stats_t unity_stats;
unity_test_case_t unity_test_cases[UNITY_MAX_TEST_CASES];
uint32_t unity_test_count = 0;
uint32_t unity_current_test = 0;

// 测试状态
static bool unity_initialized = false;
static uint32_t unity_test_start_time = 0;

// =============================================================================
// 外部依赖函数(需要在具体平台实现)
// =============================================================================

// 这些函数需要在具体的嵌入式平台上实现
extern uint32_t HAL_GetTick(void);                           // 获取系统时间戳
extern void HAL_UART_Transmit(uint8_t *data, uint16_t size); // 串口输出
extern uint32_t HAL_GetFreeHeapSize(void);                   // 获取空闲堆大小

// =============================================================================
// 核心API函数实现
// =============================================================================

/**
 * @brief 初始化测试框架
 */
void unity_init(void)
{
    // 重置统计信息
    memset(&unity_stats, 0, sizeof(unity_stats_t));

    // 重置测试用例数组
    memset(unity_test_cases, 0, sizeof(unity_test_cases));

    // 重置计数器
    unity_test_count = 0;
    unity_current_test = 0;

    // 标记已初始化
    unity_initialized = true;

    printf("\n=== 憨云DTU测试框架初始化完成 ===\n");
    printf("最大测试用例数: %d\n", UNITY_MAX_TEST_CASES);
    printf("测试框架版本: 1.0\n\n");
}

/**
 * @brief 添加测试用例
 */
void unity_add_test(const char *name, void (*test_func)(void))
{
    if (!unity_initialized)
    {
        unity_init();
    }

    if (unity_test_count >= UNITY_MAX_TEST_CASES)
    {
        printf("错误: 测试用例数量超过最大限制 %d\n", UNITY_MAX_TEST_CASES);
        return;
    }

    unity_test_case_t *test_case = &unity_test_cases[unity_test_count];
    test_case->name = name;
    test_case->test_func = test_func;
    test_case->result = UNITY_RESULT_PASS;
    test_case->message[0] = '\0';
    test_case->line = 0;
    test_case->file = NULL;

    unity_test_count++;
}

/**
 * @brief 运行单个测试用例
 */
unity_result_t unity_run_test(unity_test_case_t *test_case)
{
    if (test_case == NULL || test_case->test_func == NULL)
    {
        return UNITY_RESULT_ERROR;
    }

    printf("运行测试: %s ... ", test_case->name);

    // 记录开始时间
    unity_test_start_time = unity_get_time();
    unity_current_test = (uint32_t)(test_case - unity_test_cases);

    // 重置测试结果
    test_case->result = UNITY_RESULT_PASS;
    test_case->message[0] = '\0';
    test_case->line = 0;
    test_case->file = NULL;

    // 执行测试函数
    test_case->test_func();

    // 检查测试结果
    switch (test_case->result)
    {
    case UNITY_RESULT_PASS:
        printf("通过\n");
        unity_stats.passed_tests++;
        break;

    case UNITY_RESULT_FAIL:
        printf("失败\n");
        if (test_case->file && test_case->line > 0)
        {
            printf("  位置: %s:%d\n", test_case->file, test_case->line);
        }
        if (test_case->message[0] != '\0')
        {
            printf("  消息: %s\n", test_case->message);
        }
        unity_stats.failed_tests++;
        break;

    case UNITY_RESULT_IGNORE:
        printf("忽略\n");
        unity_stats.ignored_tests++;
        break;

    case UNITY_RESULT_TIMEOUT:
        printf("超时\n");
        unity_stats.timeout_tests++;
        break;

    case UNITY_RESULT_ERROR:
        printf("错误\n");
        unity_stats.error_tests++;
        break;
    }

    return test_case->result;
}

/**
 * @brief 运行所有测试用例
 */
unity_stats_t unity_run_all_tests(void)
{
    if (!unity_initialized)
    {
        unity_init();
    }

    printf("\n=== 开始运行测试用例 ===\n");
    printf("总测试用例数: %d\n\n", unity_test_count);

    // 记录开始时间
    unity_stats.start_time = unity_get_time();
    unity_stats.total_tests = unity_test_count;

    // 运行所有测试用例
    for (uint32_t i = 0; i < unity_test_count; i++)
    {
        unity_run_test(&unity_test_cases[i]);
    }

    // 记录结束时间
    unity_stats.end_time = unity_get_time();

    // 打印测试报告
    unity_print_report();

    return unity_stats;
}

/**
 * @brief 打印测试报告
 */
void unity_print_report(void)
{
    uint32_t duration = unity_stats.end_time - unity_stats.start_time;

    printf("\n=== 测试报告 ===\n");
    printf("总测试数:   %d\n", unity_stats.total_tests);
    printf("通过测试:   %d\n", unity_stats.passed_tests);
    printf("失败测试:   %d\n", unity_stats.failed_tests);
    printf("忽略测试:   %d\n", unity_stats.ignored_tests);
    printf("超时测试:   %d\n", unity_stats.timeout_tests);
    printf("错误测试:   %d\n", unity_stats.error_tests);
    printf("测试时间:   %d ms\n", duration);

    // 计算成功率
    if (unity_stats.total_tests > 0)
    {
        uint32_t success_rate = (unity_stats.passed_tests * 100) / unity_stats.total_tests;
        printf("成功率:     %d%%\n", success_rate);
    }

    // 测试结果总结
    if (unity_stats.failed_tests == 0 && unity_stats.error_tests == 0 && unity_stats.timeout_tests == 0)
    {
        printf("\n🎉 所有测试通过! 🎉\n");
    }
    else
    {
        printf("\n❌ 存在失败测试!\n");

        // 列出失败的测试用例
        printf("\n失败的测试用例:\n");
        for (uint32_t i = 0; i < unity_test_count; i++)
        {
            unity_test_case_t *test_case = &unity_test_cases[i];
            if (test_case->result != UNITY_RESULT_PASS && test_case->result != UNITY_RESULT_IGNORE)
            {
                printf("  - %s", test_case->name);
                if (test_case->message[0] != '\0')
                {
                    printf(" (%s)", test_case->message);
                }
                printf("\n");
            }
        }
    }

    printf("==================\n\n");
}

/**
 * @brief 重置测试框架
 */
void unity_reset(void)
{
    unity_initialized = false;
    unity_init();
}

// =============================================================================
// 内部函数实现
// =============================================================================

/**
 * @brief 测试失败处理
 */
void unity_fail(uint32_t line, const char *file, const char *format, ...)
{
    if (unity_current_test >= unity_test_count)
    {
        return;
    }

    unity_test_case_t *test_case = &unity_test_cases[unity_current_test];
    test_case->result = UNITY_RESULT_FAIL;
    test_case->line = line;
    test_case->file = file;

    // 格式化错误消息
    va_list args;
    va_start(args, format);
    vsnprintf(test_case->message, UNITY_MAX_MESSAGE, format, args);
    va_end(args);
}

/**
 * @brief 获取系统时间戳
 */
uint32_t unity_get_time(void)
{
// 在实际嵌入式系统中，这里应该调用HAL_GetTick()
// 这里使用简单的实现用于演示
#ifdef HAL_GetTick
    return HAL_GetTick();
#else
    // 简单的时间模拟(仅用于测试)
    static uint32_t fake_time = 0;
    return ++fake_time;
#endif
}

/**
 * @brief 测试超时检查
 */
bool unity_check_timeout(uint32_t timeout_ms)
{
    uint32_t current_time = unity_get_time();
    uint32_t elapsed = current_time - unity_test_start_time;
    return elapsed > timeout_ms;
}

/**
 * @brief 获取空闲内存大小
 */
uint32_t unity_get_free_memory(void)
{
#ifdef HAL_GetFreeHeapSize
    return HAL_GetFreeHeapSize();
#else
    // 简单的内存模拟(仅用于测试)
    return 4096; // 假设有4KB空闲内存
#endif
}

// =============================================================================
// 辅助函数
// =============================================================================

/**
 * @brief 打印测试用例列表
 */
void unity_list_tests(void)
{
    printf("\n=== 测试用例列表 ===\n");
    for (uint32_t i = 0; i < unity_test_count; i++)
    {
        printf("%d. %s\n", i + 1, unity_test_cases[i].name);
    }
    printf("总计: %d 个测试用例\n\n", unity_test_count);
}

/**
 * @brief 运行指定名称的测试用例
 */
unity_result_t unity_run_test_by_name(const char *test_name)
{
    for (uint32_t i = 0; i < unity_test_count; i++)
    {
        if (strcmp(unity_test_cases[i].name, test_name) == 0)
        {
            return unity_run_test(&unity_test_cases[i]);
        }
    }

    printf("错误: 找不到测试用例 '%s'\n", test_name);
    return UNITY_RESULT_ERROR;
}

/**
 * @brief 获取测试统计信息
 */
unity_stats_t unity_get_stats(void)
{
    return unity_stats;
}

/**
 * @brief 检查是否所有测试都通过
 */
bool unity_all_tests_passed(void)
{
    return (unity_stats.failed_tests == 0 &&
            unity_stats.error_tests == 0 &&
            unity_stats.timeout_tests == 0);
}

// =============================================================================
// 调试和诊断函数
// =============================================================================

/**
 * @brief 打印内存使用情况
 */
void unity_print_memory_usage(void)
{
    uint32_t free_memory = unity_get_free_memory();
    printf("\n=== 内存使用情况 ===\n");
    printf("空闲内存: %d 字节\n", free_memory);
    printf("测试框架内存占用: %d 字节\n",
           (uint32_t)(sizeof(unity_stats) + sizeof(unity_test_cases)));
    printf("==================\n\n");
}

/**
 * @brief 设置测试超时时间
 */
void unity_set_test_timeout(uint32_t timeout_ms)
{
    // 这里可以设置全局超时时间
    // 在实际实现中可以添加超时检查逻辑
    (void)timeout_ms; // 避免未使用警告
}

/**
 * @brief 测试框架自检
 */
bool unity_self_test(void)
{
    printf("=== 测试框架自检 ===\n");

    // 检查初始化状态
    if (!unity_initialized)
    {
        printf("❌ 框架未初始化\n");
        return false;
    }

    // 检查内存
    uint32_t free_mem = unity_get_free_memory();
    if (free_mem < 1024)
    {
        printf("⚠️  可用内存不足: %d 字节\n", free_mem);
    }

    // 检查时间函数
    uint32_t time1 = unity_get_time();
    uint32_t time2 = unity_get_time();
    if (time2 <= time1)
    {
        printf("⚠️  时间函数可能有问题\n");
    }

    printf("✅ 测试框架自检完成\n\n");
    return true;
}