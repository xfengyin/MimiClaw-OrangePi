#!/bin/bash
# 并发会话测试脚本

set -e

BUILD_DIR="$(dirname "$0")/../libs/build"
TEST_PROG="$BUILD_DIR/bin/test_concurrent"

if [ ! -f "$TEST_PROG" ]; then
    echo "错误：测试程序不存在，请先编译"
    exit 1
fi

echo "=== 并发会话测试 ==="
echo ""

"$TEST_PROG"

echo ""
echo "测试完成"
