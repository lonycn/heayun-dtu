# ================================================================
# ARM None EABI 工具链配置文件
# ================================================================
# 目标: ARM Cortex-M0 (NANO100B)
# 工具链: arm-none-eabi-gcc
# ================================================================

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链前缀
set(TOOLCHAIN_PREFIX arm-none-eabi-)

# 编译器设置
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

# 目标芯片配置
set(CPU_FLAGS "-mcpu=cortex-m0 -mthumb -mfloat-abi=soft")

# 编译器标志
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")

# 链接器标志 - 使用 nostdlib
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -T${CMAKE_SOURCE_DIR}/nano100b.ld -Wl,--gc-sections -nostdlib")

# 搜索路径设置
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 禁用编译器检查
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1) 