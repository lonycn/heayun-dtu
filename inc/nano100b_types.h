/**
 * ================================================================
 * 憨云 DTU - NANO100B 类型定义
 * ================================================================
 * 文件: nano100b_types.h
 * 功能: 基本数据类型定义，替代 stdint.h
 * 目标: 新唐科技 NANO100B 微控制器
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

#ifndef NANO100B_TYPES_H
#define NANO100B_TYPES_H

// 基本整数类型定义
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

// 布尔类型 (避免C23关键字冲突)
typedef uint8_t boolean_t;
#define FALSE 0
#define TRUE 1

// 为了兼容性，也定义小写版本
#define false FALSE
#define true TRUE

// 空指针定义
#ifndef NULL
#define NULL ((void *)0)
#endif

// 最大值和最小值定义
#define UINT8_MAX 255U
#define INT8_MAX 127
#define INT8_MIN (-128)
#define UINT16_MAX 65535U
#define INT16_MAX 32767
#define INT16_MIN (-32768)
#define UINT32_MAX 4294967295UL
#define INT32_MAX 2147483647L
#define INT32_MIN (-2147483648L)

// 大小类型
typedef uint32_t size_t;

// 函数返回状态
typedef enum
{
    STATUS_OK = 0,
    STATUS_ERROR = 1,
    STATUS_BUSY = 2,
    STATUS_TIMEOUT = 3,
    STATUS_INVALID_PARAM = 4
} status_t;

// 位操作宏
#define BIT(n) (1UL << (n))
#define SET_BIT(reg, bit) ((reg) |= BIT(bit))
#define CLEAR_BIT(reg, bit) ((reg) &= ~BIT(bit))
#define READ_BIT(reg, bit) (((reg) >> (bit)) & 1UL)
#define TOGGLE_BIT(reg, bit) ((reg) ^= BIT(bit))

// 寄存器操作宏
#define REG8(addr) (*(volatile uint8_t *)(addr))
#define REG16(addr) (*(volatile uint16_t *)(addr))
#define REG32(addr) (*(volatile uint32_t *)(addr))

// 内存对齐宏
#define ALIGN_4(x) (((x) + 3) & ~3)
#define ALIGN_8(x) (((x) + 7) & ~7)

// 数组大小宏
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// 最小值和最大值宏
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 编译器属性
#define WEAK __attribute__((weak))
#define PACKED __attribute__((packed))
#define ALIGNED(n) __attribute__((aligned(n)))
#define SECTION(s) __attribute__((section(s)))

#endif /* NANO100B_TYPES_H */