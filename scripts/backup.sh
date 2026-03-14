#!/bin/bash
# ============================================================================
# MimiClaw-OrangePi 备份脚本
# 
# 用法: ./scripts/backup.sh [options]
# 
# 选项:
#   -o, --output DIR    输出目录 (默认: ./backups)
#   -k, --keep N        保留最近 N 个备份 (默认: 7)
#   --include-db        包含数据库备份
#   --include-config   包含配置文件
# ============================================================================

set -e

# 颜色
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 配置
OUTPUT_DIR="./backups"
KEEP=7
INCLUDE_DB=true
INCLUDE_CONFIG=true
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_NAME="mimiclaw_backup_$TIMESTAMP"

# 项目目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -k|--keep)
            KEEP="$2"
            shift 2
            ;;
        --include-db)
            INCLUDE_DB=true
            shift
            ;;
        --no-db)
            INCLUDE_DB=false
            shift
            ;;
        --include-config)
            INCLUDE_CONFIG=true
            shift
            ;;
        -h|--help)
            echo "用法: $0 [options]"
            echo "  -o, --output DIR   输出目录"
            echo "  -k, --keep N       保留备份数"
            echo "  --include-db       包含数据库"
            exit 0
            ;;
        *) shift ;;
    esac
done

echo "============================================"
echo "  MimiClaw-OrangePi 备份"
echo "============================================"
echo ""

# 创建备份目录
mkdir -p "$OUTPUT_DIR"
BACKUP_PATH="$OUTPUT_DIR/$BACKUP_NAME"
mkdir -p "$BACKUP_PATH"

cd "$PROJECT_DIR"

log_info "备份项目文件..."

# 备份源码
tar -czf "$BACKUP_PATH/source.tar.gz" \
    --exclude='.git' \
    --exclude='build' \
    --exclude='*.o' \
    --exclude='*.so' \
    --exclude='*.a' \
    --exclude='coverage*' \
    --exclude='*.log' \
    .

log_success "源码备份完成"

# 备份数据库
if [ "$INCLUDE_DB" = true ]; then
    log_info "备份数据库..."
    if [ -f "mimi.db" ]; then
        cp mimi.db "$BACKUP_PATH/mimi.db"
        log_success "数据库备份完成"
    else
        log_warn "数据库文件不存在，跳过"
    fi
fi

# 备份配置
if [ "$INCLUDE_CONFIG" = true ]; then
    log_info "备份配置文件..."
    mkdir -p "$BACKUP_PATH/config"
    
    for conf in apps/*/config/*.json apps/*/config/*.json.in; do
        if [ -f "$conf" ]; then
            cp "$conf" "$BACKUP_PATH/config/" 2>/dev/null || true
        fi
    done
    
    if [ -f ".env" ]; then
        cp .env "$BACKUP_PATH/config/" 2>/dev/null || true
    fi
    
    log_success "配置备份完成"
fi

# 创建清单
cat > "$BACKUP_PATH/manifest.txt" << EOF
MimiClaw-OrangePi Backup Manifest
================================
Backup Date: $(date)
Version: $(git describe --tags --always 2>/dev/null || echo "unknown")
Hostname: $(hostname)
User: $(whoami)

Included:
- Source code: source.tar.gz
$(if [ "$INCLUDE_DB" = true ]; then echo "- Database: mimi.db"; fi)
$(if [ "$INCLUDE_CONFIG" = true ]; then echo "- Config: config/"; fi)
EOF

# 创建压缩包
log_info "创建压缩包..."
cd "$OUTPUT_DIR"
tar -czf "${BACKUP_NAME}.tar.gz" "$BACKUP_NAME"
rm -rf "$BACKUP_NAME"

log_success "备份完成: ${BACKUP_NAME}.tar.gz"

# 清理旧备份
log_info "清理旧备份 (保留 $KEEP 个)..."
cd "$OUTPUT_DIR"
ls -t mimiclaw_backup_*.tar.gz 2>/dev/null | tail -n +$((KEEP + 1)) | xargs -r rm -f

echo ""
log_success "备份完成!"
echo "备份位置: $OUTPUT_DIR/${BACKUP_NAME}.tar.gz"
echo "备份大小: $(du -h "${BACKUP_NAME}.tar.gz" | cut -f1)"