#!/bin/bash
# 响应延迟测试脚本

set -e

BUILD_DIR="$(dirname "$0")/../libs/build"
TEST_PROG="$BUILD_DIR/bin/test_latency"

if [ ! -f "$TEST_PROG" ]; then
    echo "错误：测试程序不存在，请先编译"
    exit 1
fi

echo "=== 响应延迟测试 ==="
echo ""

"$TEST_PROG"

echo ""
echo "测试完成"
