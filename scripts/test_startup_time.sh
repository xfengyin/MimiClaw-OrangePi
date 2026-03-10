#!/bin/bash
# 启动时间测试脚本

set -e

BUILD_DIR="$(dirname "$0")/../libs/build"
TEST_PROG="$BUILD_DIR/bin/test_startup"

if [ ! -f "$TEST_PROG" ]; then
    echo "错误：测试程序不存在，请先编译"
    exit 1
fi

echo "=== 启动时间测试 ==="
echo ""

# 运行 10 次取平均
RUNS=10
TOTAL=0

echo "运行 $RUNS 次启动测试..."

for i in $(seq 1 $RUNS); do
    START=$(date +%s%N)
    "$TEST_PROG" > /dev/null 2>&1
    END=$(date +%s%N)
    ELAPSED=$(( (END - START) / 1000000 ))  # 毫秒
    TOTAL=$((TOTAL + ELAPSED))
    echo "  运行 $i: ${ELAPSED}ms"
done

AVG=$((TOTAL / RUNS))

echo ""
echo "结果:"
echo "  平均启动时间：${AVG}ms"
echo "  目标：<1000ms"

if [ $AVG -lt 1000 ]; then
    echo "  状态：✅ 达标"
else
    echo "  状态：❌ 未达标"
fi
