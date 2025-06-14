#!/bin/bash

# ================================================================
# 憨云 DTU - 调试版固件烧录工具
# ================================================================
# 文件: flash_debug.sh
# 功能: 编译并烧录极简调试版本，用于硬件问题诊断
# 目标: 新唐科技 NANO100B 微控制器
# 作者: 憨云 DTU 开发团队
# 日期: 2025-03-28
# ================================================================

set -e  # 遇到错误立即退出

# ================================================================
# 配置参数
# ================================================================

PROJECT_NAME="hua-cool-dtu-debug"
BUILD_DIR="build_debug"
FIRMWARE_NAME="${PROJECT_NAME}.bin"
ELF_NAME="${PROJECT_NAME}"
CMAKE_FILE="CMakeLists_debug.txt"

# 调试器配置
MCU_TYPE="NANO100SD3BN"
DEBUG_PORT="2331"

# ================================================================
# 颜色定义
# ================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# ================================================================
# 日志函数
# ================================================================

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
# 横幅显示
# ================================================================

print_banner() {
    echo -e "${CYAN}"
    echo "================================================================"
    echo "🚀 憨云 DTU - 调试版固件烧录工具 v1.0"
    echo "================================================================"
    echo "📱 目标设备: 新唐科技 NANO100B"
    echo "💾 Flash: 64KB | RAM: 8KB"
    echo "🔧 构建目录: ${BUILD_DIR}/"
    echo "📦 固件文件: ${FIRMWARE_NAME}"
    echo "🐛 调试模式: 极简硬件测试"
    echo "================================================================"
    echo -e "${NC}"
}

# ================================================================
# 环境检查
# ================================================================

check_dependencies() {
    log_step "检查开发环境依赖..."
    
    local missing_tools=()
    
    # 检查基本工具
    if ! command -v cmake &> /dev/null; then
        missing_tools+=("cmake")
    fi
    
    if ! command -v arm-none-eabi-gcc &> /dev/null; then
        missing_tools+=("arm-none-eabi-gcc")
    fi
    
    if ! command -v arm-none-eabi-objcopy &> /dev/null; then
        missing_tools+=("arm-none-eabi-objcopy")
    fi
    
    # 检查调试器
    local debugger_found=false
    if command -v JLinkExe &> /dev/null; then
        log_info "发现 J-Link: $(JLinkExe -CommanderScript /dev/null 2>&1 | grep "J-Link Commander" | head -1)"
        debugger_found=true
    fi
    
    if command -v openocd &> /dev/null; then
        log_info "发现 OpenOCD: $(openocd --version 2>&1 | head -1)"
        debugger_found=true
    fi
    
    if [ "$debugger_found" = false ]; then
        missing_tools+=("JLink或OpenOCD")
    fi
    
    # 检查缺失工具
    if [ ${#missing_tools[@]} -ne 0 ]; then
        log_error "缺少以下工具:"
        for tool in "${missing_tools[@]}"; do
            echo "  - $tool"
        done
        exit 1
    fi
    
    log_success "所有依赖工具已安装 ✓"
}

# ================================================================
# 构建函数
# ================================================================

clean_build() {
    log_step "清理构建目录..."
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        log_success "构建目录已清理"
    else
        log_info "构建目录不存在，跳过清理"
    fi
}

configure_project() {
    log_step "配置 CMake 项目..."
    
    # 创建构建目录
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    # 配置项目 (复制调试版CMake文件)
    cp "../${CMAKE_FILE}" "../CMakeLists.txt.backup" 2>/dev/null || true
    cp "../${CMAKE_FILE}" "../CMakeLists.txt"
    
    cmake -DCMAKE_BUILD_TYPE=Release \
          -G "Unix Makefiles" \
          ..
    
    cd ..
    log_success "CMake 配置完成"
}

build_firmware() {
    log_step "编译调试固件..."
    
    cd "${BUILD_DIR}"
    
    # 编译项目
    local start_time=$(date +%s)
    make -j$(nproc 2>/dev/null || echo 4)
    local end_time=$(date +%s)
    local build_time=$((end_time - start_time))
    
    cd ..
    
    # 检查编译结果
    if [ -f "${BUILD_DIR}/${FIRMWARE_NAME}" ]; then
        local firmware_size=$(stat -f%z "${BUILD_DIR}/${FIRMWARE_NAME}" 2>/dev/null || stat -c%s "${BUILD_DIR}/${FIRMWARE_NAME}")
        local firmware_kb=$((firmware_size / 1024))
        log_success "调试固件编译完成 (耗时: ${build_time}s)"
        log_info "固件大小: ${firmware_size} bytes (${firmware_kb} KB)"
    else
        log_error "固件编译失败"
        exit 1
    fi
}

# ================================================================
# 烧录函数
# ================================================================

detect_debugger() {
    log_step "检测调试器连接..."
    
    # 优先使用 J-Link
    if command -v JLinkExe &> /dev/null; then
        log_success "检测到 J-Link 调试器"
        return 0
    fi
    
    # 备用 OpenOCD
    if command -v openocd &> /dev/null; then
        log_success "检测到 OpenOCD 调试器"
        return 1
    fi
    
    log_error "未检测到支持的调试器"
    exit 1
}

flash_with_jlink() {
    log_step "使用 J-Link 烧录调试固件..."
    
    # 创建 J-Link 脚本
    cat > jlink_flash_debug.jlink << EOF
connect
reset
halt
loadbin ${BUILD_DIR}/${FIRMWARE_NAME}, 0x00000000
verifybin ${BUILD_DIR}/${FIRMWARE_NAME}, 0x00000000
reset
go
exit
EOF
    
    # 执行烧录
    JLinkExe -device ${MCU_TYPE} -if SWD -speed 4000 -CommanderScript jlink_flash_debug.jlink
    
    # 清理临时文件
    rm -f jlink_flash_debug.jlink
    
    log_success "J-Link 烧录完成"
}

flash_with_openocd() {
    log_step "使用 OpenOCD 烧录调试固件..."
    
    openocd -f interface/jlink.cfg \
            -f scripts/nano100b.cfg \
            -c "program ${BUILD_DIR}/${FIRMWARE_NAME} 0x00000000 verify reset exit"
    
    log_success "OpenOCD 烧录完成"
}

flash_firmware() {
    detect_debugger
    local debugger_type=$?
    
    case $debugger_type in
        0)
            flash_with_jlink
            ;;
        1)
            flash_with_openocd
            ;;
        *)
            log_error "未知调试器类型"
            exit 1
            ;;
    esac
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
    echo "  -b, --build         仅编译调试固件"
    echo "  -f, --flash         仅烧录调试固件"
    echo "  -a, --all           完整流程 (清理+编译+烧录)"
    echo ""
    echo "调试固件功能:"
    echo "  - 启动时快速闪烁5次 (LED + 蜂鸣器)"
    echo "  - 主循环交替闪烁 PA0/PA1 LED"
    echo "  - 每4个周期蜂鸣器响一次"
    echo "  - 极简GPIO操作，便于硬件问题诊断"
    echo ""
    echo "示例:"
    echo "  $0                  # 编译并烧录调试固件"
    echo "  $0 --all            # 完整流程"
    echo "  $0 --build          # 仅编译"
    echo "  $0 --flash          # 仅烧录"
}

main() {
    print_banner
    
    # 解析命令行参数
    local do_clean=false
    local do_build=true
    local do_flash=true
    
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
            -a|--all)
                do_clean=true
                do_build=true
                do_flash=true
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
    check_dependencies
    
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
            log_error "未找到调试固件文件，请先编译"
            exit 1
        fi
        flash_firmware
    fi
    
    # 完成提示
    echo ""
    log_success "🎉 调试固件操作完成！"
    
    if [ "$do_flash" = true ]; then
        echo ""
        log_info "📱 调试固件已烧录到设备"
        log_info "🔍 预期行为:"
        echo "  1. 启动时: LED和蜂鸣器快速闪烁5次"
        echo "  2. 运行时: PA0和PA1 LED交替闪烁"
        echo "  3. 每4个周期: 蜂鸣器响一次"
        echo ""
        log_info "💡 如果没有任何反应，可能的问题:"
        echo "  - 硬件连接问题 (电源、GPIO连接)"
        echo "  - 时钟配置问题"
        echo "  - 启动代码问题"
        echo "  - 寄存器地址错误"
    fi
}

# 运行主程序
main "$@" 