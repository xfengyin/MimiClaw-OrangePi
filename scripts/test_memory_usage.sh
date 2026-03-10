#!/bin/bash
# 内存占用测试脚本

set -e

BUILD_DIR="$(dirname "$0")/../libs/build"
TEST_PROG="$BUILD_DIR/bin/test_memory"

if [ ! -f "$TEST_PROG" ]; then
    echo "错误：测试程序不存在，请先编译"
    exit 1
fi

echo "=== 内存占用测试 ==="
echo ""

# 使用 /usr/bin/time 测量最大内存
echo "运行内存测试..."
/usr/bin/time -v "$TEST_PROG" 2>&1 | grep -E "(Maximum resident|Elapsed|Exit status)"

echo ""
echo "测试完成"
