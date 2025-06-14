/**
 * @file unity.h
 * @brief 憨云DTU轻量级单元测试框架
 * @version 1.0
 * @date 2025-03-28
 *
 * 专为嵌入式环境设计的轻量级测试框架
 * 支持断言、测试用例管理、结果统计
 */

#ifndef UNITY_H
#define UNITY_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // =============================================================================
    // 测试框架配置
    // =============================================================================

#define UNITY_MAX_TEST_CASES 100 // 最大测试用例数
#define UNITY_MAX_TEST_NAME 64   // 测试名称最大长度
#define UNITY_MAX_MESSAGE 128    // 错误消息最大长度

    // =============================================================================
    // 测试结果枚举
    // =============================================================================

    typedef enum
    {
        UNITY_RESULT_PASS = 0, // 测试通过
        UNITY_RESULT_FAIL,     // 测试失败
        UNITY_RESULT_IGNORE,   // 测试忽略
        UNITY_RESULT_TIMEOUT,  // 测试超时
        UNITY_RESULT_ERROR     // 测试错误
    } unity_result_t;

    // =============================================================================
    // 测试统计结构
    // =============================================================================

    typedef struct
    {
        uint32_t total_tests;   // 总测试数
        uint32_t passed_tests;  // 通过测试数
        uint32_t failed_tests;  // 失败测试数
        uint32_t ignored_tests; // 忽略测试数
        uint32_t timeout_tests; // 超时测试数
        uint32_t error_tests;   // 错误测试数
        uint32_t start_time;    // 开始时间
        uint32_t end_time;      // 结束时间
    } unity_stats_t;

    // =============================================================================
    // 测试用例结构
    // =============================================================================

    typedef struct
    {
        const char *name;                // 测试名称
        void (*test_func)(void);         // 测试函数
        unity_result_t result;           // 测试结果
        char message[UNITY_MAX_MESSAGE]; // 错误消息
        uint32_t line;                   // 失败行号
        const char *file;                // 失败文件
    } unity_test_case_t;

    // =============================================================================
    // 全局变量声明
    // =============================================================================

    extern unity_stats_t unity_stats;
    extern unity_test_case_t unity_test_cases[UNITY_MAX_TEST_CASES];
    extern uint32_t unity_test_count;
    extern uint32_t unity_current_test;

    // =============================================================================
    // 核心API函数
    // =============================================================================

    /**
     * @brief 初始化测试框架
     */
    void unity_init(void);

    /**
     * @brief 运行所有测试用例
     * @return 测试结果统计
     */
    unity_stats_t unity_run_all_tests(void);

    /**
     * @brief 运行单个测试用例
     * @param test_case 测试用例
     * @return 测试结果
     */
    unity_result_t unity_run_test(unity_test_case_t *test_case);

    /**
     * @brief 添加测试用例
     * @param name 测试名称
     * @param test_func 测试函数
     */
    void unity_add_test(const char *name, void (*test_func)(void));

    /**
     * @brief 打印测试报告
     */
    void unity_print_report(void);

    /**
     * @brief 重置测试框架
     */
    void unity_reset(void);

    // =============================================================================
    // 断言宏定义
    // =============================================================================

#define TEST_ASSERT(condition)                                               \
    do                                                                       \
    {                                                                        \
        if (!(condition))                                                    \
        {                                                                    \
            unity_fail(__LINE__, __FILE__, "Assertion failed: " #condition); \
            return;                                                          \
        }                                                                    \
    } while (0)

#define TEST_ASSERT_TRUE(condition) \
    TEST_ASSERT((condition) == true)

#define TEST_ASSERT_FALSE(condition) \
    TEST_ASSERT((condition) == false)

#define TEST_ASSERT_EQUAL(expected, actual)                                                        \
    do                                                                                             \
    {                                                                                              \
        if ((expected) != (actual))                                                                \
        {                                                                                          \
            unity_fail(__LINE__, __FILE__, "Expected %d, got %d", (int)(expected), (int)(actual)); \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual)                                                            \
    do                                                                                                     \
    {                                                                                                      \
        if ((expected) == (actual))                                                                        \
        {                                                                                                  \
            unity_fail(__LINE__, __FILE__, "Expected not %d, but got %d", (int)(expected), (int)(actual)); \
            return;                                                                                        \
        }                                                                                                  \
    } while (0)

#define TEST_ASSERT_NULL(pointer) \
    TEST_ASSERT((pointer) == NULL)

#define TEST_ASSERT_NOT_NULL(pointer) \
    TEST_ASSERT((pointer) != NULL)

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                                               \
    do                                                                                           \
    {                                                                                            \
        if (strcmp((expected), (actual)) != 0)                                                   \
        {                                                                                        \
            unity_fail(__LINE__, __FILE__, "Expected \"%s\", got \"%s\"", (expected), (actual)); \
            return;                                                                              \
        }                                                                                        \
    } while (0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, size)                \
    do                                                                  \
    {                                                                   \
        if (memcmp((expected), (actual), (size)) != 0)                  \
        {                                                               \
            unity_fail(__LINE__, __FILE__, "Memory comparison failed"); \
            return;                                                     \
        }                                                               \
    } while (0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual)                                                   \
    do                                                                                                \
    {                                                                                                 \
        if ((actual) <= (threshold))                                                                  \
        {                                                                                             \
            unity_fail(__LINE__, __FILE__, "Expected > %d, got %d", (int)(threshold), (int)(actual)); \
            return;                                                                                   \
        }                                                                                             \
    } while (0)

#define TEST_ASSERT_LESS_THAN(threshold, actual)                                                      \
    do                                                                                                \
    {                                                                                                 \
        if ((actual) >= (threshold))                                                                  \
        {                                                                                             \
            unity_fail(__LINE__, __FILE__, "Expected < %d, got %d", (int)(threshold), (int)(actual)); \
            return;                                                                                   \
        }                                                                                             \
    } while (0)

#define TEST_ASSERT_WITHIN(delta, expected, actual)                                                                     \
    do                                                                                                                  \
    {                                                                                                                   \
        int diff = (int)(actual) - (int)(expected);                                                                     \
        if (diff < 0)                                                                                                   \
            diff = -diff;                                                                                               \
        if (diff > (int)(delta))                                                                                        \
        {                                                                                                               \
            unity_fail(__LINE__, __FILE__, "Expected %d +/- %d, got %d", (int)(expected), (int)(delta), (int)(actual)); \
            return;                                                                                                     \
        }                                                                                                               \
    } while (0)

    // =============================================================================
    // 测试用例定义宏
    // =============================================================================

#define TEST_CASE(name) \
    void test_##name(void)

#define RUN_TEST(name) \
    unity_add_test(#name, test_##name)

#define TEST_SETUP() \
    void test_setup(void)

#define TEST_TEARDOWN() \
    void test_teardown(void)

    // =============================================================================
    // 内部函数声明
    // =============================================================================

    /**
     * @brief 测试失败处理
     * @param line 行号
     * @param file 文件名
     * @param format 格式字符串
     * @param ... 参数
     */
    void unity_fail(uint32_t line, const char *file, const char *format, ...);

    /**
     * @brief 获取系统时间戳
     * @return 时间戳(ms)
     */
    uint32_t unity_get_time(void);

    /**
     * @brief 测试超时检查
     * @param timeout_ms 超时时间(ms)
     * @return true-超时, false-未超时
     */
    bool unity_check_timeout(uint32_t timeout_ms);

    // =============================================================================
    // 性能测试宏
    // =============================================================================

#define PERFORMANCE_TEST_START() \
    uint32_t perf_start_time = unity_get_time()

#define PERFORMANCE_TEST_END(max_time_ms)                                                                         \
    do                                                                                                            \
    {                                                                                                             \
        uint32_t perf_end_time = unity_get_time();                                                                \
        uint32_t perf_duration = perf_end_time - perf_start_time;                                                 \
        if (perf_duration > (max_time_ms))                                                                        \
        {                                                                                                         \
            unity_fail(__LINE__, __FILE__, "Performance test failed: %dms > %dms", perf_duration, (max_time_ms)); \
            return;                                                                                               \
        }                                                                                                         \
    } while (0)

    // =============================================================================
    // 内存测试宏
    // =============================================================================

#define MEMORY_TEST_START() \
    uint32_t mem_start = unity_get_free_memory()

#define MEMORY_TEST_END(max_leak_bytes)                                                 \
    do                                                                                  \
    {                                                                                   \
        uint32_t mem_end = unity_get_free_memory();                                     \
        int32_t mem_leak = (int32_t)mem_start - (int32_t)mem_end;                       \
        if (mem_leak > (int32_t)(max_leak_bytes))                                       \
        {                                                                               \
            unity_fail(__LINE__, __FILE__, "Memory leak detected: %d bytes", mem_leak); \
            return;                                                                     \
        }                                                                               \
    } while (0)

    /**
     * @brief 获取空闲内存大小
     * @return 空闲内存字节数
     */
    uint32_t unity_get_free_memory(void);

#ifdef __cplusplus
}
#endif

#endif // UNITY_H