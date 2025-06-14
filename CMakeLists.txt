cmake_minimum_required(VERSION 3.16)

# ================================================================
# 憨云DTU项目配置 (完整版)
# ================================================================

# 设置工具链 (必须在project之前)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake)

project(hua-cool-dtu VERSION 1.0.0 LANGUAGES C ASM)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ================================================================
# 编译器和链接器设置
# ================================================================

# 目标芯片设置
set(MCU_ARCH "cortex-m0")
set(MCU_FLAGS "-mcpu=${MCU_ARCH} -mthumb -mfloat-abi=soft")

# 编译标志
set(CMAKE_C_FLAGS "${MCU_FLAGS} -fdata-sections -ffunction-sections -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -g -O2")

# 链接标志
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -T${CMAKE_SOURCE_DIR}/nano100b.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -nostdlib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage")

# ================================================================
# 源文件配置
# ================================================================

# 完整版本源文件
set(SOURCES
    # 核心系统文件
    src/core/main.c
    src/core/system.c
    src/core/startup.c
    
    # 驱动文件 (如果存在)
    # src/drivers/gpio.c
    # src/drivers/uart.c
    # src/drivers/adc.c
    # src/drivers/i2c.c
    # src/drivers/spi.c
    # src/drivers/timer.c
    # src/drivers/pwm.c
    
    # 应用文件 (如果存在)
    # src/app/modbus.c
    # src/app/lora.c
    # src/app/sensor.c
    # src/app/display.c
    # src/app/storage.c
)

# 检查源文件是否存在，只添加存在的文件
set(VERIFIED_SOURCES)
foreach(source ${SOURCES})
    if(EXISTS ${CMAKE_SOURCE_DIR}/${source})
        list(APPEND VERIFIED_SOURCES ${source})
    endif()
endforeach()

# 如果没有找到main.c，使用调试版本
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/src/core/main.c)
    message(WARNING "未找到 src/core/main.c，使用调试版本")
    set(VERIFIED_SOURCES
        src/core/main_debug.c
        src/core/startup.c
    )
endif()

# ================================================================
# 包含目录
# ================================================================

# 添加头文件目录
include_directories(
    inc
    src/core
    src/drivers
    src/app
)

# ================================================================
# 目标配置
# ================================================================

# 创建可执行文件
add_executable(${PROJECT_NAME} ${VERIFIED_SOURCES})

# 设置输出文件名
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "hua-cool-dtu")

# ================================================================
# 构建后处理
# ================================================================

# 生成二进制文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMENT "生成二进制文件: ${PROJECT_NAME}.bin"
)

# 生成十六进制文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMENT "生成十六进制文件: ${PROJECT_NAME}.hex"
)

# 显示文件大小信息
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "显示文件大小信息"
)

# ================================================================
# 调试信息输出
# ================================================================

message(STATUS "")
message(STATUS "===== 憨云DTU构建配置 (完整版) =====")
message(STATUS "项目: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "目标芯片: ${MCU_ARCH}")
message(STATUS "CPU架构: ${MCU_FLAGS}")
message(STATUS "编译标志: ${CMAKE_C_FLAGS}")
message(STATUS "链接标志: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "源文件: ${VERIFIED_SOURCES}")
message(STATUS "工具链: ${CMAKE_C_COMPILER}")
message(STATUS "===================================")
message(STATUS "") 