/**
 * @file test_gpio.c
 * @brief GPIO驱动模块单元测试
 * @version 1.0
 * @date 2025-03-28
 */

#include "../../framework/unity.h"
#include "../../../inc/gpio.h"
#include <string.h>

// =============================================================================
// 测试夹具(Test Fixtures)
// =============================================================================

static gpio_config_t test_config;
static gpio_pin_t test_pins[] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7};

/**
 * @brief 测试前置设置
 */
TEST_SETUP()
{
    // 重置测试配置
    memset(&test_config, 0, sizeof(gpio_config_t));

    // 设置默认测试配置
    test_config.port = GPIO_PORT_A;
    test_config.pin = GPIO_PIN_0;
    test_config.mode = GPIO_MODE_OUTPUT;
    test_config.pull = GPIO_PULL_NONE;
    test_config.speed = GPIO_SPEED_MEDIUM;
    test_config.output_type = GPIO_OUTPUT_PUSH_PULL;
}

/**
 * @brief 测试后置清理
 */
TEST_TEARDOWN()
{
    // 清理所有测试引脚
    for (int i = 0; i < sizeof(test_pins) / sizeof(test_pins[0]); i++)
    {
        gpio_deinit(GPIO_PORT_A, test_pins[i]);
        gpio_deinit(GPIO_PORT_B, test_pins[i]);
    }
}

// =============================================================================
// GPIO初始化测试
// =============================================================================

/**
 * @brief 测试GPIO初始化成功
 */
TEST_CASE(gpio_init_success)
{
    gpio_result_t result = gpio_init(&test_config);

    TEST_ASSERT_EQUAL(GPIO_OK, result);

    // 验证引脚状态
    bool is_initialized = gpio_is_initialized(test_config.port, test_config.pin);
    TEST_ASSERT_TRUE(is_initialized);
}

/**
 * @brief 测试空指针初始化
 */
TEST_CASE(gpio_init_null_config)
{
    gpio_result_t result = gpio_init(NULL);

    TEST_ASSERT_EQUAL(GPIO_ERROR_INVALID_PARAM, result);
}

/**
 * @brief 测试无效端口初始化
 */
TEST_CASE(gpio_init_invalid_port)
{
    test_config.port = 0xFF; // 无效端口

    gpio_result_t result = gpio_init(&test_config);

    TEST_ASSERT_EQUAL(GPIO_ERROR_INVALID_PARAM, result);
}

/**
 * @brief 测试无效引脚初始化
 */
TEST_CASE(gpio_init_invalid_pin)
{
    test_config.pin = 0xFF; // 无效引脚

    gpio_result_t result = gpio_init(&test_config);

    TEST_ASSERT_EQUAL(GPIO_ERROR_INVALID_PARAM, result);
}

/**
 * @brief 测试重复初始化
 */
TEST_CASE(gpio_init_already_initialized)
{
    // 第一次初始化
    gpio_result_t result1 = gpio_init(&test_config);
    TEST_ASSERT_EQUAL(GPIO_OK, result1);

    // 第二次初始化应该成功(重新配置)
    gpio_result_t result2 = gpio_init(&test_config);
    TEST_ASSERT_EQUAL(GPIO_OK, result2);
}

// =============================================================================
// GPIO输出测试
// =============================================================================

/**
 * @brief 测试GPIO输出高电平
 */
TEST_CASE(gpio_set_output_high)
{
    test_config.mode = GPIO_MODE_OUTPUT;
    gpio_init(&test_config);

    gpio_result_t result = gpio_set_output(test_config.port, test_config.pin, GPIO_STATE_HIGH);
    TEST_ASSERT_EQUAL(GPIO_OK, result);

    // 读取输出状态
    gpio_state_t state = gpio_get_output(test_config.port, test_config.pin);
    TEST_ASSERT_EQUAL(GPIO_STATE_HIGH, state);
}

/**
 * @brief 测试GPIO输出低电平
 */
TEST_CASE(gpio_set_output_low)
{
    test_config.mode = GPIO_MODE_OUTPUT;
    gpio_init(&test_config);

    gpio_result_t result = gpio_set_output(test_config.port, test_config.pin, GPIO_STATE_LOW);
    TEST_ASSERT_EQUAL(GPIO_OK, result);

    // 读取输出状态
    gpio_state_t state = gpio_get_output(test_config.port, test_config.pin);
    TEST_ASSERT_EQUAL(GPIO_STATE_LOW, state);
}

/**
 * @brief 测试GPIO翻转输出
 */
TEST_CASE(gpio_toggle_output)
{
    test_config.mode = GPIO_MODE_OUTPUT;
    gpio_init(&test_config);

    // 设置初始状态为低
    gpio_set_output(test_config.port, test_config.pin, GPIO_STATE_LOW);
    gpio_state_t initial_state = gpio_get_output(test_config.port, test_config.pin);

    // 翻转输出
    gpio_result_t result = gpio_toggle_output(test_config.port, test_config.pin);
    TEST_ASSERT_EQUAL(GPIO_OK, result);

    // 验证状态已翻转
    gpio_state_t new_state = gpio_get_output(test_config.port, test_config.pin);
    TEST_ASSERT_NOT_EQUAL(initial_state, new_state);
}

/**
 * @brief 测试非输出模式设置输出
 */
TEST_CASE(gpio_set_output_invalid_mode)
{
    test_config.mode = GPIO_MODE_INPUT; // 输入模式
    gpio_init(&test_config);

    gpio_result_t result = gpio_set_output(test_config.port, test_config.pin, GPIO_STATE_HIGH);
    TEST_ASSERT_EQUAL(GPIO_ERROR_INVALID_MODE, result);
}

// =============================================================================
// GPIO输入测试
// =============================================================================

/**
 * @brief 测试GPIO输入读取
 */
TEST_CASE(gpio_get_input)
{
    test_config.mode = GPIO_MODE_INPUT;
    test_config.pull = GPIO_PULL_UP; // 上拉，默认读取高电平
    gpio_init(&test_config);

    gpio_state_t state = gpio_get_input(test_config.port, test_config.pin);

    // 上拉模式下应该读取到高电平
    TEST_ASSERT_EQUAL(GPIO_STATE_HIGH, state);
}

/**
 * @brief 测试GPIO输入下拉
 */
TEST_CASE(gpio_get_input_pull_down)
{
    test_config.mode = GPIO_MODE_INPUT;
    test_config.pull = GPIO_PULL_DOWN; // 下拉，默认读取低电平
    gpio_init(&test_config);

    gpio_state_t state = gpio_get_input(test_config.port, test_config.pin);

    // 下拉模式下应该读取到低电平
    TEST_ASSERT_EQUAL(GPIO_STATE_LOW, state);
}

/**
 * @brief 测试GPIO输入无上下拉
 */
TEST_CASE(gpio_get_input_no_pull)
{
    test_config.mode = GPIO_MODE_INPUT;
    test_config.pull = GPIO_PULL_NONE; // 无上下拉
    gpio_init(&test_config);

    gpio_state_t state = gpio_get_input(test_config.port, test_config.pin);

    // 无上下拉模式下状态不确定，但应该是有效值
    TEST_ASSERT_TRUE(state == GPIO_STATE_HIGH || state == GPIO_STATE_LOW);
}

// =============================================================================
// GPIO多引脚操作测试
// =============================================================================

/**
 * @brief 测试GPIO端口写入
 */
TEST_CASE(gpio_write_port)
{
    // 初始化端口A的多个引脚为输出模式
    for (int i = 0; i < 4; i++)
    {
        test_config.pin = test_pins[i];
        test_config.mode = GPIO_MODE_OUTPUT;
        gpio_init(&test_config);
    }

    uint16_t port_value = 0x0F; // 低4位为1
    gpio_result_t result = gpio_write_port(GPIO_PORT_A, port_value);
    TEST_ASSERT_EQUAL(GPIO_OK, result);

    // 验证各引脚状态
    for (int i = 0; i < 4; i++)
    {
        gpio_state_t state = gpio_get_output(GPIO_PORT_A, test_pins[i]);
        TEST_ASSERT_EQUAL(GPIO_STATE_HIGH, state);
    }
}

/**
 * @brief 测试GPIO端口读取
 */
TEST_CASE(gpio_read_port)
{
    // 初始化端口A的多个引脚为输入模式
    for (int i = 0; i < 4; i++)
    {
        test_config.port = GPIO_PORT_A;
        test_config.pin = test_pins[i];
        test_config.mode = GPIO_MODE_INPUT;
        test_config.pull = (i % 2 == 0) ? GPIO_PULL_UP : GPIO_PULL_DOWN;
        gpio_init(&test_config);
    }

    uint16_t port_value = gpio_read_port(GPIO_PORT_A);

    // 验证读取值的合理性
    TEST_ASSERT_NOT_EQUAL(0xFFFF, port_value); // 不应该是全1

    // 验证各引脚状态
    for (int i = 0; i < 4; i++)
    {
        gpio_state_t expected = (i % 2 == 0) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
        gpio_state_t actual = gpio_get_input(GPIO_PORT_A, test_pins[i]);
        TEST_ASSERT_EQUAL(expected, actual);
    }
}

// =============================================================================
// GPIO中断测试
// =============================================================================

static volatile bool interrupt_triggered = false;
static volatile gpio_port_t interrupt_port;
static volatile gpio_pin_t interrupt_pin;

/**
 * @brief GPIO中断回调函数
 */
void test_gpio_interrupt_callback(gpio_port_t port, gpio_pin_t pin)
{
    interrupt_triggered = true;
    interrupt_port = port;
    interrupt_pin = pin;
}

/**
 * @brief 测试GPIO中断配置
 */
TEST_CASE(gpio_interrupt_config)
{
    test_config.mode = GPIO_MODE_INPUT;
    test_config.pull = GPIO_PULL_UP;
    gpio_init(&test_config);

    gpio_interrupt_config_t int_config = {
        .port = test_config.port,
        .pin = test_config.pin,
        .trigger = GPIO_TRIGGER_FALLING,
        .callback = test_gpio_interrupt_callback,
        .priority = GPIO_INTERRUPT_PRIORITY_MEDIUM};

    gpio_result_t result = gpio_configure_interrupt(&int_config);
    TEST_ASSERT_EQUAL(GPIO_OK, result);

    // 验证中断是否已配置
    bool is_configured = gpio_is_interrupt_configured(test_config.port, test_config.pin);
    TEST_ASSERT_TRUE(is_configured);
}

/**
 * @brief 测试GPIO中断使能/禁用
 */
TEST_CASE(gpio_interrupt_enable_disable)
{
    test_config.mode = GPIO_MODE_INPUT;
    gpio_init(&test_config);

    gpio_interrupt_config_t int_config = {
        .port = test_config.port,
        .pin = test_config.pin,
        .trigger = GPIO_TRIGGER_RISING,
        .callback = test_gpio_interrupt_callback,
        .priority = GPIO_INTERRUPT_PRIORITY_HIGH};

    gpio_configure_interrupt(&int_config);

    // 使能中断
    gpio_result_t result1 = gpio_enable_interrupt(test_config.port, test_config.pin);
    TEST_ASSERT_EQUAL(GPIO_OK, result1);

    bool is_enabled = gpio_is_interrupt_enabled(test_config.port, test_config.pin);
    TEST_ASSERT_TRUE(is_enabled);

    // 禁用中断
    gpio_result_t result2 = gpio_disable_interrupt(test_config.port, test_config.pin);
    TEST_ASSERT_EQUAL(GPIO_OK, result2);

    is_enabled = gpio_is_interrupt_enabled(test_config.port, test_config.pin);
    TEST_ASSERT_FALSE(is_enabled);
}

/**
 * @brief 测试GPIO中断触发(模拟)
 */
TEST_CASE(gpio_interrupt_trigger)
{
    interrupt_triggered = false;

    test_config.mode = GPIO_MODE_INPUT;
    gpio_init(&test_config);

    gpio_interrupt_config_t int_config = {
        .port = test_config.port,
        .pin = test_config.pin,
        .trigger = GPIO_TRIGGER_BOTH,
        .callback = test_gpio_interrupt_callback,
        .priority = GPIO_INTERRUPT_PRIORITY_HIGH};

    gpio_configure_interrupt(&int_config);
    gpio_enable_interrupt(test_config.port, test_config.pin);

    // 模拟中断触发
    gpio_simulate_interrupt(test_config.port, test_config.pin);

    // 验证中断是否被触发
    TEST_ASSERT_TRUE(interrupt_triggered);
    TEST_ASSERT_EQUAL(test_config.port, interrupt_port);
    TEST_ASSERT_EQUAL(test_config.pin, interrupt_pin);
}

// =============================================================================
// GPIO配置测试
// =============================================================================

/**
 * @brief 测试GPIO模式配置
 */
TEST_CASE(gpio_configure_mode)
{
    gpio_init(&test_config);

    // 测试各种模式
    gpio_result_t result1 = gpio_configure_mode(test_config.port, test_config.pin, GPIO_MODE_INPUT);
    TEST_ASSERT_EQUAL(GPIO_OK, result1);

    gpio_result_t result2 = gpio_configure_mode(test_config.port, test_config.pin, GPIO_MODE_OUTPUT);
    TEST_ASSERT_EQUAL(GPIO_OK, result2);

    gpio_result_t result3 = gpio_configure_mode(test_config.port, test_config.pin, GPIO_MODE_ALTERNATE);
    TEST_ASSERT_EQUAL(GPIO_OK, result3);

    gpio_result_t result4 = gpio_configure_mode(test_config.port, test_config.pin, GPIO_MODE_ANALOG);
    TEST_ASSERT_EQUAL(GPIO_OK, result4);
}

/**
 * @brief 测试GPIO上下拉配置
 */
TEST_CASE(gpio_configure_pull)
{
    test_config.mode = GPIO_MODE_INPUT;
    gpio_init(&test_config);

    // 测试各种上下拉配置
    gpio_result_t result1 = gpio_configure_pull(test_config.port, test_config.pin, GPIO_PULL_NONE);
    TEST_ASSERT_EQUAL(GPIO_OK, result1);

    gpio_result_t result2 = gpio_configure_pull(test_config.port, test_config.pin, GPIO_PULL_UP);
    TEST_ASSERT_EQUAL(GPIO_OK, result2);

    gpio_result_t result3 = gpio_configure_pull(test_config.port, test_config.pin, GPIO_PULL_DOWN);
    TEST_ASSERT_EQUAL(GPIO_OK, result3);
}

/**
 * @brief 测试GPIO速度配置
 */
TEST_CASE(gpio_configure_speed)
{
    test_config.mode = GPIO_MODE_OUTPUT;
    gpio_init(&test_config);

    // 测试各种速度配置
    gpio_result_t result1 = gpio_configure_speed(test_config.port, test_config.pin, GPIO_SPEED_LOW);
    TEST_ASSERT_EQUAL(GPIO_OK, result1);

    gpio_result_t result2 = gpio_configure_speed(test_config.port, test_config.pin, GPIO_SPEED_MEDIUM);
    TEST_ASSERT_EQUAL(GPIO_OK, result2);

    gpio_result_t result3 = gpio_configure_speed(test_config.port, test_config.pin, GPIO_SPEED_HIGH);
    TEST_ASSERT_EQUAL(GPIO_OK, result3);

    gpio_result_t result4 = gpio_configure_speed(test_config.port, test_config.pin, GPIO_SPEED_VERY_HIGH);
    TEST_ASSERT_EQUAL(GPIO_OK, result4);
}

// =============================================================================
// GPIO状态查询测试
// =============================================================================

/**
 * @brief 测试GPIO状态查询
 */
TEST_CASE(gpio_get_config)
{
    gpio_init(&test_config);

    gpio_config_t read_config;
    gpio_result_t result = gpio_get_config(test_config.port, test_config.pin, &read_config);

    TEST_ASSERT_EQUAL(GPIO_OK, result);
    TEST_ASSERT_EQUAL(test_config.port, read_config.port);
    TEST_ASSERT_EQUAL(test_config.pin, read_config.pin);
    TEST_ASSERT_EQUAL(test_config.mode, read_config.mode);
    TEST_ASSERT_EQUAL(test_config.pull, read_config.pull);
    TEST_ASSERT_EQUAL(test_config.speed, read_config.speed);
}

/**
 * @brief 测试GPIO信息获取
 */
TEST_CASE(gpio_get_info)
{
    gpio_init(&test_config);

    gpio_info_t info;
    gpio_result_t result = gpio_get_info(test_config.port, test_config.pin, &info);

    TEST_ASSERT_EQUAL(GPIO_OK, result);
    TEST_ASSERT_TRUE(info.initialized);
    TEST_ASSERT_EQUAL(test_config.mode, info.mode);
    TEST_ASSERT_EQUAL(test_config.pull, info.pull);
}

// =============================================================================
// 性能测试
// =============================================================================

/**
 * @brief 测试GPIO操作性能
 */
TEST_CASE(gpio_performance)
{
    test_config.mode = GPIO_MODE_OUTPUT;
    gpio_init(&test_config);

    // 测试输出设置性能
    PERFORMANCE_TEST_START();
    for (int i = 0; i < 1000; i++)
    {
        gpio_set_output(test_config.port, test_config.pin, GPIO_STATE_HIGH);
        gpio_set_output(test_config.port, test_config.pin, GPIO_STATE_LOW);
    }
    PERFORMANCE_TEST_END(50); // 2000次操作应该在50ms内完成

    // 测试翻转性能
    PERFORMANCE_TEST_START();
    for (int i = 0; i < 1000; i++)
    {
        gpio_toggle_output(test_config.port, test_config.pin);
    }
    PERFORMANCE_TEST_END(20); // 1000次翻转应该在20ms内完成
}

// =============================================================================
// 错误处理测试
// =============================================================================

/**
 * @brief 测试GPIO错误处理
 */
TEST_CASE(gpio_error_handling)
{
    // 测试未初始化引脚操作
    gpio_result_t result1 = gpio_set_output(GPIO_PORT_A, GPIO_PIN_15, GPIO_STATE_HIGH);
    TEST_ASSERT_EQUAL(GPIO_ERROR_NOT_INITIALIZED, result1);

    // 测试无效参数
    gpio_result_t result2 = gpio_get_config(0xFF, 0xFF, NULL);
    TEST_ASSERT_EQUAL(GPIO_ERROR_INVALID_PARAM, result2);
}

// =============================================================================
// 测试运行器
// =============================================================================

/**
 * @brief 运行所有GPIO测试
 */
void run_gpio_tests(void)
{
    printf("\n=== 运行GPIO驱动模块测试 ===\n");

    // 初始化测试
    RUN_TEST(gpio_init_success);
    RUN_TEST(gpio_init_null_config);
    RUN_TEST(gpio_init_invalid_port);
    RUN_TEST(gpio_init_invalid_pin);
    RUN_TEST(gpio_init_already_initialized);

    // 输出测试
    RUN_TEST(gpio_set_output_high);
    RUN_TEST(gpio_set_output_low);
    RUN_TEST(gpio_toggle_output);
    RUN_TEST(gpio_set_output_invalid_mode);

    // 输入测试
    RUN_TEST(gpio_get_input);
    RUN_TEST(gpio_get_input_pull_down);
    RUN_TEST(gpio_get_input_no_pull);

    // 多引脚操作测试
    RUN_TEST(gpio_write_port);
    RUN_TEST(gpio_read_port);

    // 中断测试
    RUN_TEST(gpio_interrupt_config);
    RUN_TEST(gpio_interrupt_enable_disable);
    RUN_TEST(gpio_interrupt_trigger);

    // 配置测试
    RUN_TEST(gpio_configure_mode);
    RUN_TEST(gpio_configure_pull);
    RUN_TEST(gpio_configure_speed);

    // 状态查询测试
    RUN_TEST(gpio_get_config);
    RUN_TEST(gpio_get_info);

    // 性能测试
    RUN_TEST(gpio_performance);

    // 错误处理测试
    RUN_TEST(gpio_error_handling);

    printf("GPIO驱动模块测试用例已添加完成\n");
}