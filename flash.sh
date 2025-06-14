#!/bin/bash

# ================================================================
# 憨云 DTU - 固件烧录脚本 v2.0
# ================================================================
# 功能: 自动检测环境，编译固件，烧录到 NANO100B 微控制器
# 支持: J-Link, OpenOCD, 串口调试
# 平台: macOS (M4), Linux, Windows WSL
# 作者: 憨云 DTU 开发团队
# 日期: 2025-03-28
# ================================================================

set -e  # 遇到错误立即退出

# ================================================================
# 配置参数
# ================================================================

# 项目配置
PROJECT_NAME="hua-cool-dtu"
BUILD_DIR="build"
FIRMWARE_NAME="${PROJECT_NAME}.bin"
ELF_NAME="${PROJECT_NAME}.elf"
HEX_NAME="${PROJECT_NAME}.hex"

# 硬件配置
MCU_TYPE="NANO100SD3BN"
FLASH_START_ADDR="0x00000000"
FLASH_SIZE="64KB"
RAM_SIZE="8KB"

# 调试配置
SERIAL_BAUDRATE="115200"
DEBUG_PORT="3333"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# ================================================================
# 工具函数
# ================================================================

print_banner() {
    echo -e "${CYAN}"
    echo "================================================================"
    echo "🚀 憨云 DTU - 固件烧录工具 v2.0"
    echo "================================================================"
    echo -e "📱 目标设备: ${WHITE}新唐科技 ${MCU_TYPE}${CYAN}"
    echo -e "💾 Flash: ${WHITE}${FLASH_SIZE}${CYAN} | RAM: ${WHITE}${RAM_SIZE}${CYAN}"
    echo -e "🔧 构建目录: ${WHITE}${BUILD_DIR}/${CYAN}"
    echo -e "📦 固件文件: ${WHITE}${FIRMWARE_NAME}${CYAN}"
    echo "================================================================"
    echo -e "${NC}"
}

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_step() {
    echo -e "${PURPLE}[STEP]${NC} $1"
}

# ================================================================
# 环境检测
# ================================================================

check_dependencies() {
    log_step "检查开发环境依赖..."
    
    local missing_tools=()
    
    # 检查基础工具
    if ! command -v cmake &> /dev/null; then
        missing_tools+=("cmake")
    fi
    
    if ! command -v ninja &> /dev/null && ! command -v make &> /dev/null; then
        missing_tools+=("ninja 或 make")
    fi
    
    # 检查 ARM 工具链
    if ! command -v arm-none-eabi-gcc &> /dev/null; then
        missing_tools+=("arm-none-eabi-gcc")
    fi
    
    if ! command -v arm-none-eabi-objcopy &> /dev/null; then
        missing_tools+=("arm-none-eabi-objcopy")
    fi
    
    # 检查烧录工具
    local flash_tool_found=false
    if command -v openocd &> /dev/null; then
        flash_tool_found=true
        log_info "发现 OpenOCD: $(openocd --version 2>&1 | head -n1)"
    fi
    
    if command -v JLinkExe &> /dev/null; then
        flash_tool_found=true
        log_info "发现 J-Link: $(JLinkExe -? 2>&1 | grep "J-Link Commander" | head -n1)"
    fi
    
    if [ "$flash_tool_found" = false ]; then
        missing_tools+=("openocd 或 JLinkExe")
    fi
    
    # 报告缺失工具
    if [ ${#missing_tools[@]} -ne 0 ]; then
        log_error "缺少以下工具:"
        for tool in "${missing_tools[@]}"; do
            echo -e "  ${RED}✗${NC} $tool"
        done
        echo ""
        log_info "请运行以下命令安装 (macOS):"
        echo "  brew install cmake ninja arm-none-eabi-gcc openocd"
        echo "  或参考 docs/05-Mac开发环境搭建.md"
        exit 1
    fi
    
    log_success "所有依赖工具已安装 ✓"
}

detect_platform() {
    log_step "检测运行平台..."
    
    case "$(uname -s)" in
        Darwin*)
            PLATFORM="macOS"
            if [[ $(uname -m) == "arm64" ]]; then
                PLATFORM="macOS (Apple Silicon)"
            fi
            ;;
        Linux*)
            PLATFORM="Linux"
            ;;
        CYGWIN*|MINGW32*|MSYS*|MINGW*)
            PLATFORM="Windows"
            ;;
        *)
            PLATFORM="Unknown"
            ;;
    esac
    
    log_info "运行平台: ${PLATFORM}"
}

# ================================================================
# 项目构建
# ================================================================

clean_build() {
    log_step "清理构建目录..."
    
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        log_info "已删除旧的构建目录"
    fi
    
    mkdir -p "${BUILD_DIR}"
    log_success "构建目录已准备就绪"
}

configure_project() {
    log_step "配置 CMake 项目..."
    
    cd "${BUILD_DIR}"
    
    # 选择构建系统
    local generator="Unix Makefiles"
    if command -v ninja &> /dev/null; then
        generator="Ninja"
        log_info "使用 Ninja 构建系统"
    else
        log_info "使用 Make 构建系统"
    fi
    
    # 配置项目
    cmake .. \
        -G "${generator}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
        -DTARGET_MCU=${MCU_TYPE} \
        -DENABLE_DEBUG=ON
    
    cd ..
    log_success "CMake 配置完成"
}

build_firmware() {
    log_step "编译固件..."
    
    cd "${BUILD_DIR}"
    
    # 开始编译
    local start_time=$(date +%s)
    
    if command -v ninja &> /dev/null && [ -f "build.ninja" ]; then
        ninja -v
    else
        make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    fi
    
    local end_time=$(date +%s)
    local build_time=$((end_time - start_time))
    
    cd ..
    
    # 检查编译结果
    if [ ! -f "${BUILD_DIR}/${FIRMWARE_NAME}" ]; then
        log_error "固件编译失败，未找到 ${FIRMWARE_NAME}"
        exit 1
    fi
    
    # 显示固件信息
    local firmware_size=$(stat -f%z "${BUILD_DIR}/${FIRMWARE_NAME}" 2>/dev/null || stat -c%s "${BUILD_DIR}/${FIRMWARE_NAME}")
    local firmware_size_kb=$((firmware_size / 1024))
    
    log_success "固件编译完成 (耗时: ${build_time}s)"
    log_info "固件大小: ${firmware_size} bytes (${firmware_size_kb} KB)"
    
    # 检查固件大小
    local max_size_kb=64
    if [ $firmware_size_kb -gt $max_size_kb ]; then
        log_warning "固件大小超过 Flash 容量 (${max_size_kb} KB)"
    fi
}

# ================================================================
# 固件烧录
# ================================================================

detect_debugger() {
    log_step "检测调试器连接..."
    
    # 检查 J-Link
    if command -v JLinkExe &> /dev/null; then
        # 创建临时 J-Link 脚本
        cat > /tmp/jlink_detect.jlink << EOF
connect
exit
EOF
        
        if JLinkExe -device ${MCU_TYPE} -if SWD -speed 4000 -CommanderScript /tmp/jlink_detect.jlink &>/dev/null; then
            DEBUGGER_TYPE="jlink"
            log_success "检测到 J-Link 调试器"
            rm -f /tmp/jlink_detect.jlink
            return 0
        fi
        rm -f /tmp/jlink_detect.jlink
    fi
    
    # 检查 OpenOCD 支持的调试器
    if command -v openocd &> /dev/null; then
        # 尝试常见的调试器配置
        local configs=("jlink.cfg" "stlink.cfg" "cmsis-dap.cfg")
        
        for config in "${configs[@]}"; do
            if openocd -f "interface/${config}" -f "target/nano100.cfg" -c "init; exit" &>/dev/null; then
                DEBUGGER_TYPE="openocd"
                DEBUGGER_CONFIG="${config}"
                log_success "检测到 OpenOCD 兼容调试器 (${config})"
                return 0
            fi
        done
    fi
    
    log_warning "未检测到调试器连接"
    return 1
}

flash_with_jlink() {
    log_step "使用 J-Link 烧录固件..."
    
    # 创建 J-Link 烧录脚本
    cat > /tmp/jlink_flash.jlink << EOF
connect
reset
halt
loadbin ${BUILD_DIR}/${FIRMWARE_NAME}, ${FLASH_START_ADDR}
verifybin ${BUILD_DIR}/${FIRMWARE_NAME}, ${FLASH_START_ADDR}
reset
go
exit
EOF
    
    # 执行烧录
    if JLinkExe -device ${MCU_TYPE} -if SWD -speed 4000 -CommanderScript /tmp/jlink_flash.jlink; then
        log_success "J-Link 烧录完成"
        rm -f /tmp/jlink_flash.jlink
        return 0
    else
        log_error "J-Link 烧录失败"
        rm -f /tmp/jlink_flash.jlink
        return 1
    fi
}

flash_with_openocd() {
    log_step "使用 OpenOCD 烧录固件..."
    
    # 检查配置文件
    local target_cfg="scripts/nano100b.cfg"
    if [ ! -f "${target_cfg}" ]; then
        log_warning "未找到 ${target_cfg}，使用默认配置"
        target_cfg="target/nano100.cfg"
    fi
    
    # 执行烧录
    local openocd_cmd="openocd -f interface/${DEBUGGER_CONFIG} -f ${target_cfg}"
    openocd_cmd+=" -c \"init; reset halt; flash write_image erase ${BUILD_DIR}/${FIRMWARE_NAME} ${FLASH_START_ADDR}; verify_image ${BUILD_DIR}/${FIRMWARE_NAME} ${FLASH_START_ADDR}; reset run; exit\""
    
    if eval $openocd_cmd; then
        log_success "OpenOCD 烧录完成"
        return 0
    else
        log_error "OpenOCD 烧录失败"
        return 1
    fi
}

flash_firmware() {
    log_step "开始烧录固件..."
    
    if ! detect_debugger; then
        log_error "无法检测到调试器，请检查连接"
        log_info "支持的调试器: J-Link, ST-Link, CMSIS-DAP"
        exit 1
    fi
    
    case $DEBUGGER_TYPE in
        "jlink")
            flash_with_jlink
            ;;
        "openocd")
            flash_with_openocd
            ;;
        *)
            log_error "不支持的调试器类型: $DEBUGGER_TYPE"
            exit 1
            ;;
    esac
}

# ================================================================
# 调试功能
# ================================================================

start_debug_session() {
    log_step "启动调试会话..."
    
    case $DEBUGGER_TYPE in
        "jlink")
            log_info "启动 J-Link GDB 服务器..."
            JLinkGDBServer -device ${MCU_TYPE} -if SWD -speed 4000 -port ${DEBUG_PORT} &
            ;;
        "openocd")
            log_info "启动 OpenOCD GDB 服务器..."
            openocd -f interface/${DEBUGGER_CONFIG} -f scripts/nano100b.cfg &
            ;;
    esac
    
    sleep 2
    log_success "GDB 服务器已启动，端口: ${DEBUG_PORT}"
    log_info "可以使用以下命令连接:"
    echo "  arm-none-eabi-gdb ${BUILD_DIR}/${ELF_NAME}"
    echo "  (gdb) target remote localhost:${DEBUG_PORT}"
}

monitor_serial() {
    log_step "监控串口输出..."
    
    # 检测串口设备
    local serial_devices=()
    
    case $PLATFORM in
        "macOS"*)
            serial_devices=($(ls /dev/tty.usbserial-* 2>/dev/null || true))
            serial_devices+=($(ls /dev/tty.usbmodem* 2>/dev/null || true))
            ;;
        "Linux")
            serial_devices=($(ls /dev/ttyUSB* 2>/dev/null || true))
            serial_devices+=($(ls /dev/ttyACM* 2>/dev/null || true))
            ;;
    esac
    
    if [ ${#serial_devices[@]} -eq 0 ]; then
        log_warning "未检测到串口设备"
        return 1
    fi
    
    local serial_device="${serial_devices[0]}"
    log_info "使用串口设备: ${serial_device}"
    
    # 启动串口监控
    if command -v screen &> /dev/null; then
        log_info "启动 screen 监控 (按 Ctrl+A, K 退出)"
        screen "${serial_device}" ${SERIAL_BAUDRATE}
    elif command -v minicom &> /dev/null; then
        log_info "启动 minicom 监控"
        minicom -D "${serial_device}" -b ${SERIAL_BAUDRATE}
    else
        log_warning "未找到串口监控工具 (screen 或 minicom)"
        return 1
    fi
}

# ================================================================
# 主程序
# ================================================================

show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示帮助信息"
    echo "  -c, --clean         清理构建目录"
    echo "  -b, --build         仅编译固件"
    echo "  -f, --flash         仅烧录固件"
    echo "  -d, --debug         启动调试会话"
    echo "  -m, --monitor       监控串口输出"
    echo "  -a, --all           完整流程 (清理+编译+烧录)"
    echo "  -t, --test          编译并烧录调试版本 (极简测试)"
    echo "  --check             检查环境依赖"
    echo ""
    echo "示例:"
    echo "  $0                  # 编译并烧录"
    echo "  $0 --all            # 完整流程"
    echo "  $0 --build          # 仅编译"
    echo "  $0 --flash          # 仅烧录"
    echo "  $0 --test           # 调试版本测试"
    echo "  $0 --debug          # 启动调试"
    echo "  $0 --monitor        # 监控串口"
}

main() {
    print_banner
    
    # 解析命令行参数
    local do_clean=false
    local do_build=true
    local do_flash=true
    local do_debug=false
    local do_monitor=false
    local check_only=false
    local test_mode=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                do_clean=true
                shift
                ;;
            -b|--build)
                do_build=true
                do_flash=false
                shift
                ;;
            -f|--flash)
                do_build=false
                do_flash=true
                shift
                ;;
            -d|--debug)
                do_debug=true
                shift
                ;;
            -m|--monitor)
                do_monitor=true
                shift
                ;;
            -a|--all)
                do_clean=true
                do_build=true
                do_flash=true
                shift
                ;;
            --check)
                check_only=true
                shift
                ;;
            *)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 检查环境
    detect_platform
    check_dependencies
    
    if [ "$check_only" = true ]; then
        log_success "环境检查完成"
        exit 0
    fi
    
    # 执行操作
    if [ "$do_clean" = true ]; then
        clean_build
    fi
    
    if [ "$do_build" = true ]; then
        if [ ! -d "${BUILD_DIR}" ]; then
            mkdir -p "${BUILD_DIR}"
        fi
        configure_project
        build_firmware
    fi
    
    if [ "$do_flash" = true ]; then
        if [ ! -f "${BUILD_DIR}/${FIRMWARE_NAME}" ]; then
            log_error "未找到固件文件，请先编译"
            exit 1
        fi
        flash_firmware
    fi
    
    if [ "$do_debug" = true ]; then
        start_debug_session
    fi
    
    if [ "$do_monitor" = true ]; then
        monitor_serial
    fi
    
    # 完成提示
    echo ""
    log_success "🎉 操作完成！"
    
    if [ "$do_flash" = true ]; then
        echo ""
        log_info "📱 设备应该已经开始运行新固件"
        log_info "💡 可以使用以下命令进行调试:"
        echo "  ./flash.sh --monitor    # 监控串口输出"
        echo "  ./flash.sh --debug      # 启动调试会话"
    fi
}

# 运行主程序
main "$@" 