# Mac M4 开发环境搭建指南

## 1. 概述

本文档指导如何在 Mac M4 (Apple Silicon) 平台上搭建 NANO100B 微控制器开发环境，支持编译、调试和烧录 Modbus 电箱控制系统项目。

## 2. 环境要求

### 2.1 硬件要求

- Mac M4 (Apple Silicon) 计算机
- USB Type-C 接口
- NANO100B 开发板
- SWD 调试器 (推荐 J-Link 或 ST-Link)
- USB 转串口模块 (用于 Modbus 通信调试)

### 2.2 软件要求

- macOS Sonoma 14.0 或更高版本
- Xcode Command Line Tools
- Homebrew 包管理器

## 3. 基础环境安装

### 3.1 安装 Homebrew

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 3.2 安装 Xcode Command Line Tools

```bash
xcode-select --install
```

### 3.3 安装 Git

```bash
brew install git
```

## 4. ARM 工具链安装

### 4.1 安装 ARM GCC 编译器

```bash
# 安装 ARM GCC 工具链
brew install --cask gcc-arm-embedded

# 或者使用 ARM 官方工具链
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2
tar -xjf gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2
sudo mv gcc-arm-none-eabi-10.3-2021.10 /usr/local/
```

### 4.2 配置环境变量

编辑 `~/.zshrc` 文件：

```bash
# ARM GCC 工具链路径
export ARM_GCC_PATH="/usr/local/gcc-arm-none-eabi-10.3-2021.10/bin"
export PATH="$ARM_GCC_PATH:$PATH"

# 项目环境变量
export NANO100_SDK_PATH="$HOME/development/nano100_sdk"
export PROJECT_ROOT="$HOME/development/modbus_ver3.4.2"
```

使配置生效：

```bash
source ~/.zshrc
```

### 4.3 验证安装

```bash
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
```

## 5. 开发工具安装

### 5.1 安装 CMake

```bash
brew install cmake
```

### 5.2 安装 Ninja 构建系统

```bash
brew install ninja
```

### 5.3 安装 OpenOCD (调试器支持)

```bash
brew install openocd
```

### 5.4 安装 J-Link 软件 (如果使用 J-Link 调试器)

从 SEGGER 官网下载并安装 J-Link Software Pack for macOS。

## 6. 集成开发环境

### 6.1 安装 Visual Studio Code

```bash
brew install --cask visual-studio-code
```

### 6.2 安装 VS Code 扩展

启动 VS Code 并安装以下扩展：

```bash
# 安装必要扩展
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension cortex-debug.cortex-debug
code --install-extension ms-vscode.vscode-serial-monitor
```

推荐扩展列表：

- **C/C++**: Microsoft 官方 C/C++ 支持
- **CMake Tools**: CMake 项目支持
- **Cortex-Debug**: ARM Cortex 调试支持
- **Serial Monitor**: 串口监控
- **Hex Editor**: 十六进制文件编辑
- **GitLens**: Git 增强工具

### 6.3 配置 VS Code

创建 `.vscode/settings.json` 文件：

```json
{
  "C_Cpp.default.compilerPath": "/usr/local/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc",
  "C_Cpp.default.intelliSenseMode": "gcc-arm",
  "C_Cpp.default.includePath": [
    "${workspaceFolder}/**",
    "/usr/local/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi/include/**",
    "${workspaceFolder}/EC_NANO100B_UART_Rx_Wakeup_PDMA_V1.00/Library/**"
  ],
  "C_Cpp.default.defines": [
    "__arm__",
    "__ARM_ARCH_6M__",
    "ARM_MATH_CM0",
    "NANO100_SERIES"
  ],
  "cmake.configureOnOpen": true,
  "cmake.preferredGenerators": ["Ninja"],
  "cortex-debug.armToolchainPath": "/usr/local/gcc-arm-none-eabi-10.3-2021.10/bin",
  "cortex-debug.openocdPath": "/usr/local/bin/openocd"
}
```

## 7. NANO100B SDK 安装

### 7.1 获取 SDK

```bash
# 创建开发目录
mkdir -p ~/development/nano100_sdk
cd ~/development/nano100_sdk

# 下载新唐 NANO100B SDK (需要从新唐官网获取)
# 或使用项目中的库文件
cp -r $PROJECT_ROOT/EC_NANO100B_UART_Rx_Wakeup_PDMA_V1.00/Library/* .
```

### 7.2 SDK 目录结构

```
nano100_sdk/
├── Device/
│   └── Nuvoton/
│       └── Nano100Series/
│           ├── Include/
│           └── Source/
├── CMSIS/
│   ├── Include/
│   └── Lib/
└── StdDriver/
    ├── inc/
    └── src/
```

## 8. 项目构建配置

### 8.1 创建 CMakeLists.txt

在项目根目录创建 `CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.20)

# 设置工具链
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

project(modbus_control_system C ASM)

# 编译选项
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# MCU 配置
set(MCU_FLAGS "-mcpu=cortex-m0 -mthumb -mfloat-abi=soft")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -Wall -Wextra -Os -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${MCU_FLAGS} -Wl,--gc-sections -Wl,-Map=output.map")

# 定义宏
add_definitions(
    -DNANO100_SERIES
    -DARM_MATH_CM0
    -D__arm__
    -D__ARM_ARCH_6M__
)

# 包含目录
include_directories(
    EC_NANO100B_UART_Rx_Wakeup_PDMA_V1.00/Library/Device/Nuvoton/Nano100Series/Include
    EC_NANO100B_UART_Rx_Wakeup_PDMA_V1.00/Library/CMSIS/Include
    EC_NANO100B_UART_Rx_Wakeup_PDMA_V1.00/Library/StdDriver/inc
    modbus_ver3.4.2(3283)/App
)

# 源文件
file(GLOB_RECURSE SOURCES
    "modbus_ver3.4.2(3283)/App/*.c"
    "EC_NANO100B_UART_Rx_Wakeup_PDMA_V1.00/Library/Device/Nuvoton/Nano100Series/Source/*.c"
    "EC_NANO100B_UART_Rx_Wakeup_PDMA_V1.00/Library/StdDriver/src/*.c"
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接文件 (需要根据实际情况调整)
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "-T ${CMAKE_SOURCE_DIR}/nano100_flash.ld"
)

# 生成二进制文件
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
)
```

### 8.2 创建链接脚本

创建 `nano100_flash.ld` 文件：

```ld
MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 64K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 8K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data : AT(_etext)
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
```

## 9. 调试配置

### 9.1 OpenOCD 配置

创建 `openocd.cfg` 文件：

```tcl
# NANO100B OpenOCD 配置
source [find interface/jlink.cfg]
source [find target/nano100.cfg]

# 设置适配器速度
adapter speed 1000

# 复位配置
reset_config srst_only

# 初始化
init
reset halt
```

### 9.2 VS Code 调试配置

创建 `.vscode/launch.json` 文件：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug NANO100B",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "openocd",
      "configFiles": ["openocd.cfg"],
      "executable": "${workspaceFolder}/build/modbus_control_system",
      "device": "NANO100BN",
      "interface": "swd",
      "runToEntryPoint": "main",
      "svdFile": "${workspaceFolder}/nano100b.svd"
    }
  ]
}
```

## 10. 构建和编译

### 10.1 命令行构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake -G Ninja ..

# 编译项目
ninja

# 或使用 make
# make -j$(nproc)
```

### 10.2 VS Code 构建

1. 按 `Cmd+Shift+P` 打开命令面板
2. 输入 "CMake: Configure"
3. 选择编译器工具链
4. 按 `Cmd+Shift+P` 然后输入 "CMake: Build"

## 11. 烧录和调试

### 11.1 使用 OpenOCD 烧录

```bash
# 启动 OpenOCD
openocd -f openocd.cfg

# 在另一个终端连接 telnet
telnet localhost 4444

# 烧录命令
> reset halt
> flash write_image erase build/modbus_control_system.hex
> reset run
> exit
```

### 11.2 使用 J-Link 烧录

```bash
# 启动 J-Link Commander
JLinkExe

# J-Link 命令
J-Link> connect
J-Link> device NANO100BN
J-Link> si swd
J-Link> speed 1000
J-Link> loadfile build/modbus_control_system.hex
J-Link> r
J-Link> g
J-Link> exit
```

## 12. 串口通信调试

### 12.1 安装串口工具

```bash
# 安装 minicom
brew install minicom

# 或安装 screen (系统自带)
# screen /dev/tty.usbserial-* 9600
```

### 12.2 查找串口设备

```bash
# 列出所有串口设备
ls /dev/tty.*

# 常见的 USB 转串口设备
# /dev/tty.usbserial-*
# /dev/tty.usbmodem*
```

### 12.3 Modbus 通信测试

使用 VS Code 的 Serial Monitor 扩展或命令行工具：

```bash
# 使用 minicom
minicom -D /dev/tty.usbserial-A5XK3RJT -b 9600

# 使用 screen
screen /dev/tty.usbserial-A5XK3RJT 9600
```

## 13. 常见问题解决

### 13.1 编译错误

**问题**: `arm-none-eabi-gcc: command not found`

**解决**:

```bash
# 检查工具链安装
which arm-none-eabi-gcc

# 重新安装工具链
brew reinstall gcc-arm-embedded

# 检查环境变量
echo $PATH
```

### 13.2 调试器连接问题

**问题**: OpenOCD 无法连接调试器

**解决**:

```bash
# 检查调试器连接
lsusb

# 重启 OpenOCD 服务
killall openocd
openocd -f openocd.cfg

# 检查权限 (可能需要 sudo)
sudo openocd -f openocd.cfg
```

### 13.3 串口权限问题

**问题**: 无法访问串口设备

**解决**:

```bash
# 添加用户到 dialout 组 (Linux)
# macOS 通常不需要

# 检查设备权限
ls -l /dev/tty.usbserial-*

# 使用 sudo (如果必要)
sudo minicom -D /dev/tty.usbserial-A5XK3RJT -b 9600
```

## 14. 自动化脚本

### 14.1 构建脚本

创建 `build.sh` 脚本：

```bash
#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}开始构建 Modbus 控制系统...${NC}"

# 创建构建目录
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# CMake 配置
echo -e "${YELLOW}配置项目...${NC}"
if ! cmake -G Ninja ..; then
    echo -e "${RED}CMake 配置失败${NC}"
    exit 1
fi

# 编译
echo -e "${YELLOW}编译项目...${NC}"
if ! ninja; then
    echo -e "${RED}编译失败${NC}"
    exit 1
fi

echo -e "${GREEN}构建完成！${NC}"
echo -e "${GREEN}生成文件:${NC}"
ls -la *.bin *.hex *.elf 2>/dev/null || echo "未找到输出文件"
```

### 14.2 烧录脚本

创建 `flash.sh` 脚本：

```bash
#!/bin/bash

BINARY_FILE="build/modbus_control_system.hex"

if [ ! -f "$BINARY_FILE" ]; then
    echo "错误: 找不到二进制文件 $BINARY_FILE"
    echo "请先运行 ./build.sh"
    exit 1
fi

echo "开始烧录 $BINARY_FILE ..."

# 使用 OpenOCD 烧录
openocd -f openocd.cfg -c "
    init
    reset halt
    flash write_image erase $BINARY_FILE
    reset run
    shutdown
"

echo "烧录完成！"
```

## 15. 开发工作流

### 15.1 典型开发流程

```bash
# 1. 克隆项目
git clone <project-repo>
cd modbus_ver3.4.2

# 2. 配置环境
./setup_env.sh

# 3. 构建项目
./build.sh

# 4. 烧录程序
./flash.sh

# 5. 调试 (VS Code)
# 按 F5 启动调试

# 6. 串口监控
minicom -D /dev/tty.usbserial-* -b 9600
```

### 15.2 代码管理

```bash
# 创建功能分支
git checkout -b feature/new-sensor-support

# 提交更改
git add .
git commit -m "add: 新增传感器支持"

# 推送分支
git push origin feature/new-sensor-support
```

这个开发环境配置完成后，您就可以在 Mac M4 平台上高效地开发、调试和部署 NANO100B 项目了。
