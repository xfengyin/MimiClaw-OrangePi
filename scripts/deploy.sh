#!/bin/bash
# ============================================================================
# MimiClaw-OrangePi 部署脚本
# 
# 用法: ./scripts/deploy.sh [options]
# 
# 选项:
#   -e, --env ENV       环境: production, staging, development
#   -h, --host HOST     目标主机
#   -u, --user USER    SSH 用户
#   -p, --production   部署到生产环境 (默认)
#   -d, --debug        调试模式
#   --skip-build       跳过构建
#   --skip-tests       跳过测试
# ============================================================================

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认配置
ENV="production"
HOST=""
USER="orangepi"
SKIP_BUILD=false
SKIP_TESTS=false
DEBUG=false

# 目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
DEPLOY_DIR="/opt/mimiclaw"

# ============================================================================
# 函数定义
# ============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "MimiClaw-OrangePi 部署脚本"
    echo ""
    echo "用法: $0 [options]"
    echo ""
    echo "选项:"
    echo "  -e, --env ENV       环境: production, staging, development"
    echo "  -h, --host HOST     目标主机"
    echo "  -u, --user USER    SSH 用户 (默认: orangepi)"
    echo "  -p, --production   部署到生产环境"
    echo "  -d, --debug        调试模式"
    echo "  --skip-build       跳过构建"
    echo "  --skip-tests       跳过测试"
    echo "  -h, --help         显示帮助"
    echo ""
    echo "示例:"
    echo "  $0 -h 192.168.1.100           # 部署到指定主机"
    echo "  $0 -e staging                 # 部署到测试环境"
    echo "  $0 --skip-tests               # 跳过测试"
}

# 解析参数
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -e|--env)
                ENV="$2"
                shift 2
                ;;
            -h|--host)
                HOST="$2"
                shift 2
                ;;
            -u|--user)
                USER="$2"
                shift 2
                ;;
            -p|--production)
                ENV="production"
                shift
                ;;
            -d|--debug)
                DEBUG=true
                shift
                ;;
            --skip-build)
                SKIP_BUILD=true
                shift
                ;;
            --skip-tests)
                SKIP_TESTS=true
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                log_error "未知参数: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# 检查依赖
check_dependencies() {
    log_info "检查依赖..."
    
    local deps=("cmake" "make" "gcc")
    for dep in "${deps[@]}"; do
        if ! command -v $dep &> /dev/null; then
            log_error "缺少依赖: $dep"
            exit 1
        fi
    done
    
    log_success "依赖检查完成"
}

# 构建项目
build_project() {
    if [ "$SKIP_BUILD" = true ]; then
        log_warn "跳过构建"
        return
    fi
    
    log_info "构建项目..."
    
    cd "$PROJECT_DIR"
    
    # 清理旧构建
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    
    # CMake 配置
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    # 编译
    cmake --build build -j$(nproc)
    
    log_success "构建完成"
}

# 运行测试
run_tests() {
    if [ "$SKIP_TESTS" = true ]; then
        log_warn "跳过测试"
        return
    fi
    
    log_info "运行测试..."
    
    cd "$PROJECT_DIR"
    
    # 单元测试
    make -C libs test
    
    log_success "测试完成"
}

# 部署到本地
deploy_local() {
    log_info "部署到本地系统..."
    
    # 创建安装目录
    sudo mkdir -p "$DEPLOY_DIR"
    
    # 复制构建产物
    sudo cp -r "$BUILD_DIR"/* "$DEPLOY_DIR/"
    
    # 复制配置
    if [ -f "$PROJECT_DIR/apps/mimi-gateway/config/gateway.json" ]; then
        sudo cp "$PROJECT_DIR/apps/mimi-gateway/config/gateway.json" "$DEPLOY_DIR/config/"
    fi
    
    # 创建符号链接
    sudo ln -sf "$DEPLOY_DIR/bin/mimi-gateway" /usr/local/bin/mimi-gateway
    
    log_success "本地部署完成"
}

# 部署到远程主机
deploy_remote() {
    if [ -z "$HOST" ]; then
        log_error "未指定目标主机"
        exit 1
    fi
    
    log_info "部署到 $USER@$HOST..."
    
    # 创建远程目录
    ssh "$USER@$HOST" "mkdir -p $DEPLOY_DIR"
    
    # 复制文件
    rsync -avz --exclude='build' --exclude='.git' --exclude='*.o' \
        "$PROJECT_DIR/" "$USER@$HOST:$DEPLOY_DIR/"
    
    # 远程构建
    ssh "$USER@$HOST" "cd $DEPLOY_DIR && make -C libs"
    
    log_success "远程部署完成"
}

# 启动服务
start_service() {
    log_info "启动服务..."
    
    if [ -n "$HOST" ]; then
        ssh "$USER@$HOST" "sudo systemctl restart mimiclaw"
    else
        sudo systemctl restart mimiclaw || true
    fi
    
    log_success "服务已启动"
}

# ============================================================================
# 主程序
# ============================================================================

main() {
    parse_args "$@"
    
    echo "============================================"
    echo "  MimiClaw-OrangePi 部署脚本"
    echo "============================================"
    echo ""
    echo "环境: $ENV"
    echo "目标: ${HOST:-localhost}"
    echo "构建: ${SKIP_BUILD:+跳过}"
    echo "测试: ${SKIP_TESTS:+跳过}"
    echo ""
    
    # 检查依赖
    check_dependencies
    
    # 构建
    build_project
    
    # 测试
    run_tests
    
    # 部署
    if [ -z "$HOST" ]; then
        deploy_local
    else
        deploy_remote
    fi
    
    # 启动
    start_service
    
    echo ""
    echo "============================================"
    log_success "部署完成!"
    echo "============================================"
}

main "$@"