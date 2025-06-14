#!/bin/bash

# 憨云DTU测试运行脚本
# 版本: 1.0
# 日期: 2025-03-28

set -e  # 遇到错误时退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/tests"

echo -e "${BLUE}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                    憨云DTU测试运行脚本                        ║${NC}"
echo -e "${BLUE}║                  Hancloud DTU Test Runner                    ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# 函数：打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 函数：检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake 未安装，请先安装 CMake"
        exit 1
    fi
    
    # 检查编译器
    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        print_error "未找到C编译器 (gcc 或 clang)"
        exit 1
    fi
    
    print_success "依赖检查完成"
}

# 函数：清理构建目录
clean_build() {
    print_info "清理构建目录..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    mkdir -p "$BUILD_DIR"
    print_success "构建目录已清理"
}

# 函数：配置CMake
configure_cmake() {
    print_info "配置CMake..."
    cd "$BUILD_DIR"
    
    # 配置CMake，启用测试
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DENABLE_TESTING=ON \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          "$PROJECT_ROOT"
    
    if [ $? -eq 0 ]; then
        print_success "CMake配置完成"
    else
        print_error "CMake配置失败"
        exit 1
    fi
}

# 函数：编译项目
build_project() {
    print_info "编译项目..."
    cd "$BUILD_DIR"
    
    # 编译项目
    make -j$(nproc 2>/dev/null || echo 4)
    
    if [ $? -eq 0 ]; then
        print_success "项目编译完成"
    else
        print_error "项目编译失败"
        exit 1
    fi
}

# 函数：运行测试
run_tests() {
    print_info "运行测试..."
    cd "$BUILD_DIR"
    
    # 检查测试可执行文件是否存在
    if [ ! -f "./test_runner" ]; then
        print_error "测试可执行文件不存在: ./test_runner"
        print_info "尝试查找其他测试文件..."
        find . -name "*test*" -type f -executable
        exit 1
    fi
    
    # 运行测试
    echo ""
    print_info "开始执行测试..."
    echo ""
    
    ./test_runner "$@"
    TEST_RESULT=$?
    
    echo ""
    if [ $TEST_RESULT -eq 0 ]; then
        print_success "所有测试通过! 🎉"
    else
        print_error "测试失败，退出码: $TEST_RESULT"
    fi
    
    return $TEST_RESULT
}

# 函数：生成测试报告
generate_report() {
    print_info "生成测试报告..."
    
    REPORT_DIR="$PROJECT_ROOT/reports"
    mkdir -p "$REPORT_DIR"
    
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    REPORT_FILE="$REPORT_DIR/test_report_$TIMESTAMP.txt"
    
    # 运行测试并保存报告
    cd "$BUILD_DIR"
    if [ -f "./test_runner" ]; then
        ./test_runner --output "$REPORT_FILE" "$@"
        if [ -f "$REPORT_FILE" ]; then
            print_success "测试报告已保存到: $REPORT_FILE"
        fi
    fi
}

# 函数：显示帮助信息
show_help() {
    echo "憨云DTU测试运行脚本"
    echo ""
    echo "用法: $0 [选项] [测试参数]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示此帮助信息"
    echo "  -c, --clean         清理构建目录"
    echo "  -b, --build-only    仅编译，不运行测试"
    echo "  -t, --test-only     仅运行测试，不重新编译"
    echo "  -r, --report        生成测试报告"
    echo "  -v, --verbose       详细输出"
    echo ""
    echo "测试参数 (传递给test_runner):"
    echo "  --core              仅运行核心模块测试"
    echo "  --drivers           仅运行驱动模块测试"
    echo "  --apps              仅运行应用模块测试"
    echo "  --wireless          仅运行无线模块测试"
    echo "  --stop              失败时停止"
    echo ""
    echo "示例:"
    echo "  $0                  # 完整编译和测试"
    echo "  $0 --core           # 仅运行核心模块测试"
    echo "  $0 -r --drivers     # 运行驱动测试并生成报告"
    echo "  $0 -t               # 仅运行测试"
    echo ""
}

# 主函数
main() {
    local CLEAN_BUILD=false
    local BUILD_ONLY=false
    local TEST_ONLY=false
    local GENERATE_REPORT=false
    local VERBOSE=false
    local TEST_ARGS=()
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -b|--build-only)
                BUILD_ONLY=true
                shift
                ;;
            -t|--test-only)
                TEST_ONLY=true
                shift
                ;;
            -r|--report)
                GENERATE_REPORT=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            *)
                # 其他参数传递给测试运行器
                TEST_ARGS+=("$1")
                shift
                ;;
        esac
    done
    
    # 显示配置
    if [ "$VERBOSE" = true ]; then
        echo "=== 脚本配置 ==="
        echo "项目根目录: $PROJECT_ROOT"
        echo "构建目录: $BUILD_DIR"
        echo "测试目录: $TEST_DIR"
        echo "清理构建: $CLEAN_BUILD"
        echo "仅编译: $BUILD_ONLY"
        echo "仅测试: $TEST_ONLY"
        echo "生成报告: $GENERATE_REPORT"
        echo "测试参数: ${TEST_ARGS[*]}"
        echo ""
    fi
    
    # 检查依赖
    check_dependencies
    
    # 执行构建流程
    if [ "$TEST_ONLY" != true ]; then
        if [ "$CLEAN_BUILD" = true ]; then
            clean_build
        else
            mkdir -p "$BUILD_DIR"
        fi
        
        configure_cmake
        build_project
    fi
    
    # 执行测试
    if [ "$BUILD_ONLY" != true ]; then
        if [ "$GENERATE_REPORT" = true ]; then
            generate_report "${TEST_ARGS[@]}"
        else
            run_tests "${TEST_ARGS[@]}"
        fi
        
        TEST_RESULT=$?
        
        # 显示最终结果
        echo ""
        echo "=== 测试完成 ==="
        if [ $TEST_RESULT -eq 0 ]; then
            print_success "憨云DTU测试全部通过! 项目质量优秀! 🚀"
        else
            print_error "憨云DTU测试存在失败，请检查并修复问题"
        fi
        
        exit $TEST_RESULT
    fi
    
    print_success "构建完成"
}

# 运行主函数
main "$@" 