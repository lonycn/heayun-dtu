/**
 * @file test_runner.c
 * @brief 憨云DTU主测试运行器
 * @version 1.0
 * @date 2025-03-28
 *
 * 整合所有测试模块，提供完整的测试执行和报告功能
 */

#include "framework/unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// =============================================================================
// 测试模块声明
// =============================================================================

// 核心模块测试
extern void run_system_tests(void);

// 驱动模块测试
extern void run_gpio_tests(void);
extern void run_uart_tests(void);
extern void run_adc_tests(void);

// 应用模块测试
extern void run_modbus_tests(void);
extern void run_sensor_tests(void);
extern void run_storage_tests(void);
extern void run_alarm_tests(void);

// 无线模块测试
extern void run_lora_tests(void);
extern void run_mqtt_tests(void);
extern void run_4g_tests(void);
extern void run_bluetooth_tests(void);

// 系统模块测试
extern void run_power_tests(void);
extern void run_config_tests(void);

// =============================================================================
// 测试套件定义
// =============================================================================

typedef struct
{
    const char *name;        // 测试套件名称
    void (*run_tests)(void); // 测试运行函数
    bool enabled;            // 是否启用
    int priority;            // 优先级(1-10, 1最高)
} test_suite_t;

// 定义所有测试套件
static test_suite_t test_suites[] = {
    // 核心模块测试 (最高优先级)
    {"系统核心模块", run_system_tests, true, 1},

    // 驱动模块测试
    {"GPIO驱动", run_gpio_tests, true, 2},
    {"UART驱动", run_uart_tests, true, 2},
    {"ADC驱动", run_adc_tests, true, 2},

    // 应用模块测试
    {"Modbus通信", run_modbus_tests, true, 3},
    {"传感器管理", run_sensor_tests, true, 3},
    {"数据存储", run_storage_tests, true, 3},
    {"报警系统", run_alarm_tests, true, 3},

    // 无线模块测试
    {"LoRa通信", run_lora_tests, true, 4},
    {"MQTT通信", run_mqtt_tests, true, 4},
    {"4G通信", run_4g_tests, true, 4},
    {"蓝牙通信", run_bluetooth_tests, true, 4},

    // 系统模块测试
    {"功耗管理", run_power_tests, true, 5},
    {"配置管理", run_config_tests, true, 5},
};

#define TEST_SUITE_COUNT (sizeof(test_suites) / sizeof(test_suites[0]))

// =============================================================================
// 测试配置
// =============================================================================

typedef struct
{
    bool run_all_tests;      // 运行所有测试
    bool run_core_only;      // 仅运行核心测试
    bool run_drivers_only;   // 仅运行驱动测试
    bool run_apps_only;      // 仅运行应用测试
    bool run_wireless_only;  // 仅运行无线测试
    bool verbose_output;     // 详细输出
    bool stop_on_failure;    // 失败时停止
    int max_test_time;       // 最大测试时间(秒)
    const char *output_file; // 输出文件
} test_config_t;

static test_config_t test_config = {
    .run_all_tests = true,
    .run_core_only = false,
    .run_drivers_only = false,
    .run_apps_only = false,
    .run_wireless_only = false,
    .verbose_output = true,
    .stop_on_failure = false,
    .max_test_time = 300, // 5分钟
    .output_file = NULL};

// =============================================================================
// 测试统计
// =============================================================================

typedef struct
{
    int total_suites;            // 总测试套件数
    int passed_suites;           // 通过的测试套件数
    int failed_suites;           // 失败的测试套件数
    int skipped_suites;          // 跳过的测试套件数
    time_t start_time;           // 开始时间
    time_t end_time;             // 结束时间
    unity_stats_t overall_stats; // 总体测试统计
} test_runner_stats_t;

static test_runner_stats_t runner_stats;

// =============================================================================
// 辅助函数
// =============================================================================

/**
 * @brief 打印测试横幅
 */
static void print_test_banner(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    憨云DTU测试框架 v1.0                      ║\n");
    printf("║                  Hancloud DTU Test Framework                 ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  项目: 憨云DTU - 工业级数据传输单元                          ║\n");
    printf("║  版本: Phase 7 - 测试和质保                                 ║\n");
    printf("║  日期: 2025-03-28                                           ║\n");
    printf("║  架构师: 智商250+程序员                                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

/**
 * @brief 打印测试配置
 */
static void print_test_config(void)
{
    printf("=== 测试配置 ===\n");
    printf("运行模式: ");
    if (test_config.run_all_tests)
        printf("全部测试\n");
    else if (test_config.run_core_only)
        printf("仅核心模块\n");
    else if (test_config.run_drivers_only)
        printf("仅驱动模块\n");
    else if (test_config.run_apps_only)
        printf("仅应用模块\n");
    else if (test_config.run_wireless_only)
        printf("仅无线模块\n");
    else
        printf("自定义\n");

    printf("详细输出: %s\n", test_config.verbose_output ? "是" : "否");
    printf("失败停止: %s\n", test_config.stop_on_failure ? "是" : "否");
    printf("最大时间: %d 秒\n", test_config.max_test_time);
    printf("输出文件: %s\n", test_config.output_file ? test_config.output_file : "无");
    printf("\n");
}

/**
 * @brief 打印测试套件列表
 */
static void print_test_suites(void)
{
    printf("=== 测试套件列表 ===\n");
    printf("序号  状态  优先级  测试套件名称\n");
    printf("----  ----  ------  ------------\n");

    for (int i = 0; i < TEST_SUITE_COUNT; i++)
    {
        printf("%2d    %s    %d       %s\n",
               i + 1,
               test_suites[i].enabled ? "启用" : "禁用",
               test_suites[i].priority,
               test_suites[i].name);
    }
    printf("\n");
}

/**
 * @brief 检查是否应该运行测试套件
 */
static bool should_run_suite(int suite_index)
{
    test_suite_t *suite = &test_suites[suite_index];

    if (!suite->enabled)
    {
        return false;
    }

    if (test_config.run_all_tests)
    {
        return true;
    }

    // 根据配置决定是否运行
    if (test_config.run_core_only && suite->priority == 1)
    {
        return true;
    }

    if (test_config.run_drivers_only && suite->priority == 2)
    {
        return true;
    }

    if (test_config.run_apps_only && suite->priority == 3)
    {
        return true;
    }

    if (test_config.run_wireless_only && suite->priority == 4)
    {
        return true;
    }

    return false;
}

/**
 * @brief 运行单个测试套件
 */
static bool run_test_suite(int suite_index)
{
    test_suite_t *suite = &test_suites[suite_index];

    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║ 运行测试套件: %-47s ║\n", suite->name);
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    // 记录开始时间
    time_t suite_start_time = time(NULL);

    // 重置Unity统计
    unity_reset();

    // 运行测试套件
    if (suite->run_tests)
    {
        suite->run_tests();
    }

    // 运行所有测试用例
    unity_stats_t suite_stats = unity_run_all_tests();

    // 记录结束时间
    time_t suite_end_time = time(NULL);
    int suite_duration = (int)(suite_end_time - suite_start_time);

    // 更新总体统计
    runner_stats.overall_stats.total_tests += suite_stats.total_tests;
    runner_stats.overall_stats.passed_tests += suite_stats.passed_tests;
    runner_stats.overall_stats.failed_tests += suite_stats.failed_tests;
    runner_stats.overall_stats.ignored_tests += suite_stats.ignored_tests;
    runner_stats.overall_stats.timeout_tests += suite_stats.timeout_tests;
    runner_stats.overall_stats.error_tests += suite_stats.error_tests;

    // 判断套件是否通过
    bool suite_passed = unity_all_tests_passed();

    if (suite_passed)
    {
        runner_stats.passed_suites++;
        printf("\n✅ 测试套件 '%s' 通过 (用时: %d秒)\n", suite->name, suite_duration);
    }
    else
    {
        runner_stats.failed_suites++;
        printf("\n❌ 测试套件 '%s' 失败 (用时: %d秒)\n", suite->name, suite_duration);

        if (test_config.stop_on_failure)
        {
            printf("⚠️  配置为失败时停止，终止测试运行\n");
            return false;
        }
    }

    return true;
}

/**
 * @brief 打印最终测试报告
 */
static void print_final_report(void)
{
    int total_duration = (int)(runner_stats.end_time - runner_stats.start_time);

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                        最终测试报告                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    printf("\n=== 测试套件统计 ===\n");
    printf("总测试套件:   %d\n", runner_stats.total_suites);
    printf("通过套件:     %d\n", runner_stats.passed_suites);
    printf("失败套件:     %d\n", runner_stats.failed_suites);
    printf("跳过套件:     %d\n", runner_stats.skipped_suites);

    printf("\n=== 测试用例统计 ===\n");
    printf("总测试用例:   %d\n", runner_stats.overall_stats.total_tests);
    printf("通过用例:     %d\n", runner_stats.overall_stats.passed_tests);
    printf("失败用例:     %d\n", runner_stats.overall_stats.failed_tests);
    printf("忽略用例:     %d\n", runner_stats.overall_stats.ignored_tests);
    printf("超时用例:     %d\n", runner_stats.overall_stats.timeout_tests);
    printf("错误用例:     %d\n", runner_stats.overall_stats.error_tests);

    printf("\n=== 时间统计 ===\n");
    printf("开始时间:     %s", ctime(&runner_stats.start_time));
    printf("结束时间:     %s", ctime(&runner_stats.end_time));
    printf("总用时:       %d 秒 (%d分%d秒)\n",
           total_duration, total_duration / 60, total_duration % 60);

    // 计算成功率
    if (runner_stats.overall_stats.total_tests > 0)
    {
        int success_rate = (runner_stats.overall_stats.passed_tests * 100) /
                           runner_stats.overall_stats.total_tests;
        printf("成功率:       %d%%\n", success_rate);
    }

    // 总结
    printf("\n=== 测试结果 ===\n");
    if (runner_stats.failed_suites == 0 && runner_stats.overall_stats.failed_tests == 0)
    {
        printf("🎉 所有测试通过! 憨云DTU质量优秀! 🎉\n");
        printf("✅ 项目已准备好进入生产阶段\n");
    }
    else
    {
        printf("❌ 存在失败测试，需要修复后重新测试\n");
        printf("⚠️  建议优先修复失败的测试用例\n");
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  感谢使用憨云DTU测试框架 - 让质量成为习惯                    ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

/**
 * @brief 保存测试报告到文件
 */
static void save_test_report(const char *filename)
{
    if (!filename)
        return;

    FILE *file = fopen(filename, "w");
    if (!file)
    {
        printf("⚠️  无法创建报告文件: %s\n", filename);
        return;
    }

    fprintf(file, "憨云DTU测试报告\n");
    fprintf(file, "================\n\n");

    fprintf(file, "测试时间: %s", ctime(&runner_stats.start_time));
    fprintf(file, "测试用时: %d 秒\n\n", (int)(runner_stats.end_time - runner_stats.start_time));

    fprintf(file, "测试套件统计:\n");
    fprintf(file, "- 总套件: %d\n", runner_stats.total_suites);
    fprintf(file, "- 通过: %d\n", runner_stats.passed_suites);
    fprintf(file, "- 失败: %d\n", runner_stats.failed_suites);
    fprintf(file, "- 跳过: %d\n\n", runner_stats.skipped_suites);

    fprintf(file, "测试用例统计:\n");
    fprintf(file, "- 总用例: %d\n", runner_stats.overall_stats.total_tests);
    fprintf(file, "- 通过: %d\n", runner_stats.overall_stats.passed_tests);
    fprintf(file, "- 失败: %d\n", runner_stats.overall_stats.failed_tests);
    fprintf(file, "- 忽略: %d\n", runner_stats.overall_stats.ignored_tests);
    fprintf(file, "- 超时: %d\n", runner_stats.overall_stats.timeout_tests);
    fprintf(file, "- 错误: %d\n\n", runner_stats.overall_stats.error_tests);

    if (runner_stats.overall_stats.total_tests > 0)
    {
        int success_rate = (runner_stats.overall_stats.passed_tests * 100) /
                           runner_stats.overall_stats.total_tests;
        fprintf(file, "成功率: %d%%\n\n", success_rate);
    }

    fprintf(file, "测试结果: %s\n",
            (runner_stats.failed_suites == 0 && runner_stats.overall_stats.failed_tests == 0)
                ? "通过"
                : "失败");

    fclose(file);
    printf("✅ 测试报告已保存到: %s\n", filename);
}

// =============================================================================
// 命令行参数处理
// =============================================================================

/**
 * @brief 打印使用帮助
 */
static void print_usage(const char *program_name)
{
    printf("憨云DTU测试运行器 v1.0\n\n");
    printf("用法: %s [选项]\n\n", program_name);
    printf("选项:\n");
    printf("  -a, --all           运行所有测试 (默认)\n");
    printf("  -c, --core          仅运行核心模块测试\n");
    printf("  -d, --drivers       仅运行驱动模块测试\n");
    printf("  -p, --apps          仅运行应用模块测试\n");
    printf("  -w, --wireless      仅运行无线模块测试\n");
    printf("  -v, --verbose       详细输出 (默认)\n");
    printf("  -q, --quiet         简洁输出\n");
    printf("  -s, --stop          失败时停止\n");
    printf("  -t, --timeout SEC   设置最大测试时间(秒)\n");
    printf("  -o, --output FILE   保存报告到文件\n");
    printf("  -l, --list          列出所有测试套件\n");
    printf("  -h, --help          显示此帮助信息\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s                  # 运行所有测试\n", program_name);
    printf("  %s -c               # 仅运行核心模块测试\n", program_name);
    printf("  %s -d -s            # 运行驱动测试，失败时停止\n", program_name);
    printf("  %s -o report.txt    # 运行测试并保存报告\n", program_name);
    printf("\n");
}

/**
 * @brief 解析命令行参数
 */
static bool parse_arguments(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0)
        {
            test_config.run_all_tests = true;
        }
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--core") == 0)
        {
            test_config.run_all_tests = false;
            test_config.run_core_only = true;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--drivers") == 0)
        {
            test_config.run_all_tests = false;
            test_config.run_drivers_only = true;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--apps") == 0)
        {
            test_config.run_all_tests = false;
            test_config.run_apps_only = true;
        }
        else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--wireless") == 0)
        {
            test_config.run_all_tests = false;
            test_config.run_wireless_only = true;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            test_config.verbose_output = true;
        }
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0)
        {
            test_config.verbose_output = false;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stop") == 0)
        {
            test_config.stop_on_failure = true;
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--timeout") == 0)
        {
            if (i + 1 < argc)
            {
                test_config.max_test_time = atoi(argv[++i]);
            }
            else
            {
                printf("错误: -t 选项需要参数\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0)
        {
            if (i + 1 < argc)
            {
                test_config.output_file = argv[++i];
            }
            else
            {
                printf("错误: -o 选项需要参数\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0)
        {
            print_test_suites();
            return false;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_usage(argv[0]);
            return false;
        }
        else
        {
            printf("错误: 未知选项 '%s'\n", argv[i]);
            print_usage(argv[0]);
            return false;
        }
    }

    return true;
}

// =============================================================================
// 主函数
// =============================================================================

/**
 * @brief 主测试运行器入口
 */
int main(int argc, char *argv[])
{
    // 解析命令行参数
    if (!parse_arguments(argc, argv))
    {
        return 0;
    }

    // 打印横幅
    print_test_banner();

    // 初始化测试框架
    unity_init();

    // 打印配置信息
    if (test_config.verbose_output)
    {
        print_test_config();
        print_test_suites();
    }

    // 初始化统计信息
    memset(&runner_stats, 0, sizeof(runner_stats));
    runner_stats.start_time = time(NULL);

    // 运行测试套件
    printf("🚀 开始运行测试...\n");

    for (int i = 0; i < TEST_SUITE_COUNT; i++)
    {
        if (should_run_suite(i))
        {
            runner_stats.total_suites++;

            if (!run_test_suite(i))
            {
                break; // 失败时停止
            }
        }
        else
        {
            runner_stats.skipped_suites++;
            if (test_config.verbose_output)
            {
                printf("⏭️  跳过测试套件: %s\n", test_suites[i].name);
            }
        }
    }

    // 记录结束时间
    runner_stats.end_time = time(NULL);

    // 打印最终报告
    print_final_report();

    // 保存报告到文件
    if (test_config.output_file)
    {
        save_test_report(test_config.output_file);
    }

    // 返回退出码
    return (runner_stats.failed_suites == 0 && runner_stats.overall_stats.failed_tests == 0) ? 0 : 1;
}