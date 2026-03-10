#!/bin/bash
# scripts/run-perf-tests.sh
# MimiClaw v2.0 性能测试脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
REPORT_DIR="$PROJECT_ROOT/reports"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 创建报告目录
mkdir -p "$REPORT_DIR"

# ============================================================================
# 测试 1: 内存占用测试
# ============================================================================

test_memory_usage() {
    echo_info "=== 测试 1: 内存占用测试 ==="
    
    local massif_out="/tmp/massif.out"
    local report_file="$REPORT_DIR/memory-test.md"
    
    # 清理旧文件
    rm -f "$massif_out"
    
    # 启动 Agent (带 valgrind)
    echo_info "启动 valgrind massif 测试..."
    
    valgrind --tool=massif \
             --massif-out-file="$massif_out" \
             --pages-as-heap=yes \
             --time-unit=B \
             "$BUILD_DIR/mimiclaw-agent" \
             --config="$PROJECT_ROOT/config/test.conf" &
    
    AGENT_PID=$!
    
    # 等待服务启动
    sleep 3
    
    # 模拟负载 (发送 100 个请求)
    echo_info "模拟负载测试..."
    for i in {1..100}; do
        curl -s -X POST http://localhost:8080/api/v1/chat \
            -H "Content-Type: application/json" \
            -d "{\"message\": \"Test message $i\", \"session_id\": \"perf_test\"}" \
            > /dev/null &
    done
    wait
    
    # 等待处理完成
    sleep 5
    
    # 停止 Agent
    kill $AGENT_PID 2>/dev/null || true
    wait $AGENT_PID 2>/dev/null || true
    
    # 分析结果
    if [ -f "$massif_out" ]; then
        # 提取峰值内存
        local peak_mem=$(grep -oP 'mem_heap_B=\K\d+' "$massif_out" | sort -n | tail -1)
        local peak_mem_mb=$((peak_mem / 1024 / 1024))
        
        echo_info "峰值内存占用：${peak_mem_mb}MB"
        
        # 生成报告
        cat > "$report_file" << EOF
# 内存占用测试报告

## 测试结果
- **峰值内存**: ${peak_mem_mb}MB
- **目标**: <50MB
- **状态**: $([ $peak_mem_mb -lt 50 ] && echo "✓ 通过" || echo "✗ 失败")

## 详细数据
- 峰值内存字节：$peak_mem_mem bytes
- 测试时间：$(date)

## massif 输出
\`\`\`
$(ms_print "$massif_out" | head -50)
\`\`\`
EOF
        
        # 验证结果
        if [ $peak_mem_mb -lt 50 ]; then
            echo_info "✓ 内存占用测试通过：${peak_mem_mb}MB < 50MB"
            return 0
        else
            echo_error "✗ 内存占用测试失败：${peak_mem_mb}MB > 50MB"
            return 1
        fi
    else
        echo_error "massif 输出文件不存在"
        return 1
    fi
}

# ============================================================================
# 测试 2: 启动时间测试
# ============================================================================

test_startup_time() {
    echo_info "=== 测试 2: 启动时间测试 ==="
    
    local report_file="$REPORT_DIR/startup-test.md"
    
    # 方法 1: systemd 服务分析 (如果可用)
    if command -v systemd-analyze &> /dev/null; then
        echo_info "使用 systemd-analyze 分析..."
        sudo systemctl restart mimiclaw-agent 2>/dev/null || true
        systemd-analyze blame | grep mimiclaw || true
    fi
    
    # 方法 2: 手动计时
    echo_info "手动计时测试..."
    
    local start_ns=$(date +%s%N)
    
    # 启动 Agent
    "$BUILD_DIR/mimiclaw-agent" --config="$PROJECT_ROOT/config/test.conf" &
    AGENT_PID=$!
    
    # 等待服务就绪 (轮询健康检查)
    local max_attempts=30
    local attempt=0
    while [ $attempt -lt $max_attempts ]; do
        if curl -s http://localhost:8080/health > /dev/null 2>&1; then
            break
        fi
        sleep 0.1
        attempt=$((attempt + 1))
    done
    
    local end_ns=$(date +%s%N)
    local elapsed_ms=$(( (end_ns - start_ns) / 1000000 ))
    
    # 停止 Agent
    kill $AGENT_PID 2>/dev/null || true
    wait $AGENT_PID 2>/dev/null || true
    
    echo_info "启动时间：${elapsed_ms}ms"
    
    # 生成报告
    cat > "$report_file" << EOF
# 启动时间测试报告

## 测试结果
- **启动时间**: ${elapsed_ms}ms
- **目标**: <1000ms
- **状态**: $([ $elapsed_ms -lt 1000 ] && echo "✓ 通过" || echo "✗ 失败")

## 测试详情
- 测试方法：健康检查轮询
- 轮询间隔：100ms
- 最大等待时间：3s
- 实际等待时间：$((attempt * 100))ms
EOF
    
    # 验证结果
    if [ $elapsed_ms -lt 1000 ]; then
        echo_info "✓ 启动时间测试通过：${elapsed_ms}ms < 1000ms"
        return 0
    else
        echo_error "✗ 启动时间测试失败：${elapsed_ms}ms > 1000ms"
        return 1
    fi
}

# ============================================================================
# 测试 3: 响应延迟测试
# ============================================================================

test_response_latency() {
    echo_info "=== 测试 3: 响应延迟测试 ==="
    
    local report_file="$REPORT_DIR/latency-test.md"
    
    # 启动 Agent
    "$BUILD_DIR/mimiclaw-agent" --config="$PROJECT_ROOT/config/test.conf" &
    AGENT_PID=$!
    sleep 2
    
    # wrk 压测配置
    local url="http://localhost:8080/api/v1/chat"
    local connections=100
    local threads=4
    local duration=60
    
    echo_info "运行 wrk 压测 (${connections} 连接，${threads} 线程，${duration}s)..."
    
    # 运行 wrk
    local wrk_output=$(wrk -t$threads -c$connections -d${duration}s \
        -H "Content-Type: application/json" \
        --latency \
        --timeout=2s \
        -s "$SCRIPT_DIR/wrk_chat.lua" \
        "$url" 2>&1)
    
    # 停止 Agent
    kill $AGENT_PID 2>/dev/null || true
    wait $AGENT_PID 2>/dev/null || true
    
    # 解析 wrk 输出
    local p50=$(echo "$wrk_output" | grep "50%" | awk '{print $2}' | tr -d 'ms')
    local p75=$(echo "$wrk_output" | grep "75%" | awk '{print $2}' | tr -d 'ms')
    local p90=$(echo "$wrk_output" | grep "90%" | awk '{print $2}' | tr -d 'ms')
    local p95=$(echo "$wrk_output" | grep "95%" | awk '{print $2}' | tr -d 'ms')
    local p99=$(echo "$wrk_output" | grep "99%" | awk '{print $2}' | tr -d 'ms')
    local avg=$(echo "$wrk_output" | grep "Latency" | awk '{print $2}')
    local req_sec=$(echo "$wrk_output" | grep "Req/Sec" | awk '{print $2}')
    
    echo_info "P95 延迟：${p95}ms"
    
    # 生成报告
    cat > "$report_file" << EOF
# 响应延迟测试报告

## 测试结果
- **P95 延迟**: ${p95}ms
- **目标**: <200ms
- **状态**: $(echo "$p95 < 200" | bc -l | grep -q 1 && echo "✓ 通过" || echo "✗ 失败")

## 延迟分布
| 百分位 | 延迟 (ms) |
|--------|-----------|
| 50%    | ${p50}    |
| 75%    | ${p75}    |
| 90%    | ${p90}    |
| 95%    | ${p95}    |
| 99%    | ${p99}    |
| 平均   | ${avg}    |

## 吞吐量
- **请求/秒**: ${req_sec}

## 测试配置
- URL: $url
- 并发连接：$connections
- 线程数：$threads
- 测试时长：${duration}s

## wrk 输出
\`\`\`
$wrk_output
\`\`\`
EOF
    
    # 验证结果
    if (( $(echo "$p95 < 200" | bc -l) )); then
        echo_info "✓ 响应延迟测试通过：P95=${p95}ms < 200ms"
        return 0
    else
        echo_error "✗ 响应延迟测试失败：P95=${p95}ms > 200ms"
        return 1
    fi
}

# ============================================================================
# 测试 4: 测试覆盖率测试
# ============================================================================

test_code_coverage() {
    echo_info "=== 测试 4: 测试覆盖率测试 ==="
    
    local report_file="$REPORT_DIR/coverage-test.md"
    local coverage_info="$BUILD_DIR/coverage.info"
    
    # 运行测试 (带覆盖率)
    echo_info "运行测试并收集覆盖率数据..."
    cd "$BUILD_DIR"
    ctest --output-on-failure
    
    # 生成覆盖率报告
    echo_info "生成覆盖率报告..."
    lcov --capture \
         --directory . \
         --output-file "$coverage_info" \
         --gcov-tool gcov \
         --exclude '*/tests/*' \
         --exclude '*/third_party/*' \
         --exclude '*/mocks/*' \
         --exclude '*/usr/*'
    
    # 生成 HTML 报告
    genhtml "$coverage_info" \
            --output-directory "$REPORT_DIR/coverage-html" \
            --title "MimiClaw v2.0 覆盖率报告" \
            --num-spaces 4
    
    # 提取覆盖率
    local line_coverage=$(lcov --summary "$coverage_info" | grep "lines..." | awk '{print $2}' | tr -d '%')
    local function_coverage=$(lcov --summary "$coverage_info" | grep "functions..." | awk '{print $2}' | tr -d '%')
    local branch_coverage=$(lcov --summary "$coverage_info" | grep "branches..." | awk '{print $2}' | tr -d '%')
    
    echo_info "行覆盖率：${line_coverage}%"
    
    # 生成报告
    cat > "$report_file" << EOF
# 测试覆盖率报告

## 测试结果
- **行覆盖率**: ${line_coverage}%
- **目标**: >80%
- **状态**: $(echo "$line_coverage > 80" | bc -l | grep -q 1 && echo "✓ 通过" || echo "✗ 失败")

## 覆盖率详情
| 类型 | 覆盖率 |
|------|--------|
| 行覆盖率 | ${line_coverage}% |
| 函数覆盖率 | ${function_coverage}% |
| 分支覆盖率 | ${branch_coverage}% |

## 模块覆盖率
| 模块 | 行覆盖率 | 目标 | 状态 |
|------|----------|------|------|
| libmimi-core | - | >85% | - |
| libmimi-memory | - | >85% | - |
| libmimi-config | - | >90% | - |
| libmimi-tools | - | >80% | - |

## HTML 报告
查看：$REPORT_DIR/coverage-html/index.html

## 覆盖率数据文件
- 原始数据：$coverage_info
EOF
    
    # 验证结果
    if (( $(echo "$line_coverage > 80" | bc -l) )); then
        echo_info "✓ 测试覆盖率测试通过：${line_coverage}% > 80%"
        return 0
    else
        echo_error "✗ 测试覆盖率测试失败：${line_coverage}% < 80%"
        return 1
    fi
}

# ============================================================================
# 主函数
# ============================================================================

main() {
    echo_info "=========================================="
    echo_info "MimiClaw v2.0 性能测试"
    echo_info "=========================================="
    
    local failed=0
    
    # 检查构建目录
    if [ ! -d "$BUILD_DIR" ]; then
        echo_error "构建目录不存在：$BUILD_DIR"
        echo_info "请先运行：cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON .. && make"
        exit 1
    fi
    
    # 运行所有测试
    test_memory_usage || failed=1
    test_startup_time || failed=1
    test_response_latency || failed=1
    test_code_coverage || failed=1
    
    # 总结
    echo_info "=========================================="
    if [ $failed -eq 0 ]; then
        echo_info "✓ 所有性能测试通过"
    else
        echo_error "✗ 部分性能测试失败"
    fi
    echo_info "=========================================="
    
    # 生成性能基准对比表
    echo_info "生成性能基准对比表..."
    "$SCRIPT_DIR/generate_benchmark.sh"
    
    exit $failed
}

# 运行主函数
main "$@"
