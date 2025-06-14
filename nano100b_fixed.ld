/**
 * ================================================================
 * 憨云 DTU - NANO100B 链接脚本 (修复版)
 * ================================================================
 * 文件: nano100b_fixed.ld
 * 功能: 定义内存布局和段分配，明确栈和堆配置
 * 目标: 新唐科技 NANO100B 微控制器
 * 架构: ARM Cortex-M0 (64KB Flash, 8KB RAM)
 * 作者: 憨云 DTU 开发团队
 * 日期: 2025-03-28
 * ================================================================
 */

/* 栈和堆大小定义 */
_stack_size = 0x800;  /* 2KB 栈空间 */
_heap_size = 0x400;   /* 1KB 堆空间 */

/* Linker script to configure memory regions. */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x10000   /* 64k */
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0x2000    /*  8k */
}

/* 入口点 */
ENTRY(Reset_Handler)

SECTIONS
{
	.text :
	{
		KEEP(*(.vectors))
		*(.text*)

		KEEP(*(.init))
		KEEP(*(.fini))

		*(.rodata*)

		KEEP(*(.eh_frame*))
	} > FLASH

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	__exidx_end = .;

	__etext = .;

	.data : AT (__etext)
	{
		__data_start__ = .;
		*(vtable)
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	/* 堆配置 */
	.heap (NOLOAD):
	{
		. = ALIGN(8);
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		. = . + _heap_size;
		. = ALIGN(8);
		__HeapLimit = .;
	} > RAM

	/* 栈配置 */
	.stack (NOLOAD):
	{
		. = ALIGN(8);
		_stack_start = .;
		. = . + _stack_size;
		. = ALIGN(8);
		_stack_end = .;
	} > RAM

	/* 设置栈顶和栈底 */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackLimit = __StackTop - _stack_size;
	PROVIDE(__stack = __StackTop);

	/* 检查内存使用是否超出限制 */
	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
	
	/* 内存使用统计 */
	_ram_used = _stack_end - ORIGIN(RAM);
	_ram_free = LENGTH(RAM) - _ram_used;
} 