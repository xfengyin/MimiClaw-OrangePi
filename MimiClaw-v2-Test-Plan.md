# MimiClaw-OrangePi v2.0 测试验收标准

## 项目信息
- **仓库**: https://github.com/xfengyin/MimiClaw-OrangePi
- **版本**: v2.0 重构版
- **测试框架**: CMocka
- **覆盖率工具**: gcov + lcov
- **性能工具**: valgrind, wrk, systemd-analyze
- **CI**: GitHub Actions

---

## 一、模块测试

### 1.1 libmimi-core

#### 1.1.1 AI Agent 循环单元测试

**测试文件**: `tests/core/test_agent_loop.c`

```c
// 测试用例清单
- test_agent_loop_initialization      // Agent 循环初始化
- test_agent_loop_process_event     // 事件处理流程
- test_agent_loop_state_transition  // 状态转换正确性
- test_agent_loop_error_recovery    // 错误恢复机制
- test_agent_loop_graceful_shutdown // 优雅关闭
```

**关键测试点**:
- Agent 状态机完整性 (IDLE → PROCESSING → WAITING → IDLE)
- 事件队列处理 (FIFO 顺序)
- 超时处理机制
- 异常捕获与日志记录

#### 1.1.2 上下文管理测试 (多会话并发)

**测试文件**: `tests/core/test_context_manager.c`

```c
// 测试用例清单
- test_context_create_destroy       // 上下文创建/销毁
- test_context_session_isolation    // 会话隔离性
- test_context_concurrent_access    // 并发访问 (100 线程)
- test_context_memory_leak          // 内存泄漏检测
- test_context_serialization        // 上下文序列化/反序列化
```

**并发测试配置**:
```bash
# 使用 pthread 创建 100 个并发会话
./test_context_manager --threads=100 --iterations=1000
```

#### 1.1.3 工具调用测试 (Mock 外部 API)

**测试文件**: `tests/core/test_tool_invocation.c`

```c
// Mock 配置
- mock_http_client                  // HTTP 请求 Mock
- mock_database_connection          // 数据库连接 Mock
- mock_file_system                  // 文件系统 Mock
- mock_external_api_response        // 外部 API 响应 Mock

// 测试用例
- test_tool_register                // 工具注册
- test_tool_resolve                 // 工具解析
- test_tool_execute_success         // 执行成功
- test_tool_execute_failure         // 执行失败处理
- test_tool_timeout                 // 超时处理
```

---

### 1.2 libmimi-memory

#### 1.2.1 SQLite CRUD 操作测试

**测试文件**: `tests/memory/test_sqlite_crud.c`

```c
// 测试用例清单
- test_db_init                      // 数据库初始化
- test_insert_single_record         // 单条记录插入
- test_batch_insert                 // 批量插入 (1000 条)
- test_query_by_id                  // 按 ID 查询
- test_query_by_filter              // 条件查询
- test_update_record                // 记录更新
- test_delete_record                // 记录删除
- test_transaction_commit           // 事务提交
- test_transaction_rollback         // 事务回滚
```

**测试数据模型**:
```c
typedef struct {
    int64_t id;
    char session_id[64];
    char content[4096];
    int64_t timestamp;
    int type;  // 1=message, 2=tool_call, 3=system
} MemoryRecord;
```

#### 1.2.2 连接池压力测试 (100 并发)

**测试文件**: `tests/memory/test_connection_pool.c`

```c
// 压力测试配置
#define POOL_SIZE 10
#define CONCURRENT_THREADS 100
#define OPERATIONS_PER_THREAD 1000

// 测试用例
- test_pool_initialization            // 连接池初始化
- test_pool_exhaustion                // 连接池耗尽处理
- test_pool_connection_timeout        // 连接超时
- test_pool_concurrent_access         // 并发访问 (100 线程)
- test_pool_connection_recycle        // 连接回收
- test_pool_memory_usage              // 内存占用监控
```

**监控指标**:
- 平均等待时间 < 10ms
- 最大等待时间 < 100ms
- 连接泄漏检测

#### 1.2.3 数据持久化验证 (重启后数据完整)

**测试文件**: `tests/memory/test_persistence.c`

```c
// 测试流程
1. 插入 N 条测试数据 (N=1000)
2. 计算数据校验和 (CRC32)
3. 重启数据库/进程
4. 重新加载数据
5. 验证校验和匹配
6. 验证记录数量一致

// 测试用例
- test_persistence_write_read         // 写入后读取验证
- test_persistence_after_restart      // 重启后数据完整
- test_persistence_crash_recovery     // 崩溃恢复
- test_persistence_wal_mode           // WAL 模式验证
```

---

### 1.3 libmimi-config

#### 1.3.1 JSON/YAML 解析测试

**测试文件**: `tests/config/test_parser.c`

```c
// JSON 测试用例
- test_json_parse_simple              // 简单 JSON 解析
- test_json_parse_nested              // 嵌套 JSON 解析
- test_json_parse_array               // 数组解析
- test_json_parse_unicode             // Unicode 字符处理
- test_json_parse_invalid             // 非法 JSON 拒绝

// YAML 测试用例
- test_yaml_parse_simple              // 简单 YAML 解析
- test_yaml_parse_nested              // 嵌套 YAML 解析
- test_yaml_parse_anchor              // 锚点引用
- test_yaml_parse_invalid             // 非法 YAML 拒绝
```

**测试配置文件示例**:
```json
{
  "agent": {
    "name": "MimiClaw",
    "version": "2.0",
    "max_sessions": 100,
    "timeout_ms": 30000
  },
  "memory": {
    "db_path": "/var/lib/mimiclaw/memory.db",
    "pool_size": 10,
    "cache_enabled": true
  },
  "tools": {
    "enabled": ["search", "calculator", "weather"],
    "sandbox": true
  }
}
```

#### 1.3.2 热重载测试 (文件修改后自动生效)

**测试文件**: `tests/config/test_hot_reload.c`

```c
// 测试流程
1. 加载初始配置
2. 启动文件监控 (inotify)
3. 修改配置文件
4. 触发重载信号
5. 验证新配置生效
6. 验证旧连接不受影响

// 测试用例
- test_reload_json_file               // JSON 文件热重载
- test_reload_yaml_file               // YAML 文件热重载
- test_reload_invalid_config          // 非法配置拒绝重载
- test_reload_concurrent_requests     // 重载期间并发请求处理
- test_reload_rollback                // 重载失败回滚
```

#### 1.3.3 配置验证测试 (非法配置拒绝加载)

**测试文件**: `tests/config/test_validation.c`

```c
// 非法配置测试
- test_validation_negative_timeout    // 负数超时值拒绝
- test_validation_invalid_path        // 非法路径拒绝
- test_validation_missing_required    // 缺少必填字段拒绝
- test_validation_type_mismatch       // 类型不匹配拒绝
- test_validation_range_exceeded      // 超出范围拒绝
- test_validation_duplicate_keys      // 重复键名拒绝
```

---

### 1.4 libmimi-tools

#### 1.4.1 插件加载/卸载测试

**测试文件**: `tests/tools/test_plugin_lifecycle.c`

```c
// 测试用例清单
- test_plugin_load_valid              // 加载有效插件
- test_plugin_load_invalid            // 加载无效插件 (拒绝)
- test_plugin_unload                  // 卸载插件
- test_plugin_reload                  // 重载插件
- test_plugin_load_order              // 插件加载顺序
- test_plugin_dependency_resolution   // 依赖解析
```

**插件元数据格式**:
```json
{
  "name": "weather_plugin",
  "version": "1.0.0",
  "author": "MimiClaw Team",
  "description": "天气查询插件",
  "entry_point": "weather_init",
  "dependencies": ["libcurl", "libjson"],
  "permissions": ["network"]
}
```

#### 1.4.2 插件沙箱隔离测试

**测试文件**: `tests/tools/test_plugin_sandbox.c`

```c
// 沙箱隔离测试
- test_sandbox_memory_isolation       // 内存隔离
- test_sandbox_file_system_restriction // 文件系统限制
- test_sandbox_network_restriction     // 网络访问限制
- test_sandbox_system_call_restriction // 系统调用限制
- test_sandbox_resource_limits         // 资源限制 (CPU/内存)
- test_sandbox_escape_attempt          // 沙箱逃逸尝试 (应失败)
```

**沙箱配置**:
```c
// seccomp-bpf 规则
- 禁止: execve, ptrace, mount
- 允许: read, write, open, close, socket (受限)

// 资源限制
- RLIMIT_AS: 64MB
- RLIMIT_CPU: 5s
- RLIMIT_NOFILE: 100
```

#### 1.4.3 工具注册表查询测试

**测试文件**: `tests/tools/test_registry.c`

```c
// 测试用例清单
- test_registry_register_tool         // 注册工具
- test_registry_unregister_tool       // 注销工具
- test_registry_query_by_name         // 按名称查询
- test_registry_query_by_category     // 按类别查询
- test_registry_list_all              // 列出所有工具
- test_registry_duplicate_register    // 重复注册拒绝
- test_registry_query_not_found       // 查询不存在工具
```

---

## 二、性能测试

### 2.1 内存占用测试

**目标**: <50MB

**工具**: valgrind --tool=massif

**测试脚本**:
```bash
#!/bin/bash
# tests/perf/test_memory.sh

valgrind --tool=massif \
         --massif-out-file=/tmp/massif.out \
         --pages-as-heap=yes \
         ./mimiclaw-agent --config=test.conf

# 分析结果
ms_print /tmp/massif.out > /tmp/memory_report.txt

# 验证
PEAK_MEM=$(grep -oP 'mem_heap_B=\K\d+' /tmp/massif.out | sort -n | tail -1)
if [ $PEAK_MEM -lt 52428800 ]; then  # 50MB
    echo "✓ 内存占用测试通过: $(($PEAK_MEM / 1024 / 1024))MB"
else
    echo "✗ 内存占用测试失败: $(($PEAK_MEM / 1024 / 1024))MB > 50MB"
    exit 1
fi
```

**测试场景**:
- 空载状态内存基线
- 10 个并发会话
- 100 个并发会话
- 长时间运行 (24 小时) 内存泄漏检测

### 2.2 启动时间测试

**目标**: <1s

**工具**: systemd-analyze blame

**测试脚本**:
```bash
#!/bin/bash
# tests/perf/test_startup.sh

# 方法 1: systemd 服务分析
systemctl restart mimiclaw-agent
systemd-analyze blame | grep mimiclaw

# 方法 2: 手动计时
START=$(date +%s%N)
./mimiclaw-agent --config=test.conf &
AGENT_PID=$!

# 等待服务就绪
until curl -s http://localhost:8080/health > /dev/null; do
    sleep 0.1
done

END=$(date +%s%N)
ELAPSED=$(( (END - START) / 1000000 ))  # 毫秒

if [ $ELAPSED -lt 1000 ]; then
    echo "✓ 启动时间测试通过: ${ELAPSED}ms"
else
    echo "✗ 启动时间测试失败: ${ELAPSED}ms > 1000ms"
    kill $AGENT_PID
    exit 1
fi

kill $AGENT_PID
```

**优化检查点**:
- 延迟加载非必要模块
- 配置文件预编译
- 数据库连接池预创建
- 插件异步加载

### 2.3 响应延迟测试

**目标**: <200ms (P95)

**工具**: wrk

**测试脚本**:
```bash
#!/bin/bash
# tests/perf/test_latency.sh

# wrk 压测配置
URL="http://localhost:8080/api/v1/chat"
CONNECTIONS=100
THREADS=4
DURATION=60s

# 运行压测
wrk -t$THREADS -c$CONNECTIONS -d$DURATION \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer test_token" \
    --latency \
    --timeout=2s \
    --post.lua <<EOF
    wrk.method = "POST"
    wrk.body = '{"message": "test", "session_id": "test_123"}'
    wrk.headers["Content-Type"] = "application/json"
EOF

# 验证 P95 延迟
P95=$(wrk_output | grep "95%" | awk '{print $2}')
if (( $(echo "$P95 < 200" | bc -l) )); then
    echo "✓ 响应延迟测试通过: P95=${P95}ms"
else
    echo "✗ 响应延迟测试失败: P95=${P95}ms > 200ms"
    exit 1
fi
```

**测试场景**:
- 简单问答 (无工具调用)
- 复杂问答 (带工具调用)
- 多轮对话 (上下文累积)
- 高并发场景 (1000 QPS)

### 2.4 测试覆盖率测试

**目标**: >80%

**工具**: gcov + lcov

**测试脚本**:
```bash
#!/bin/bash
# tests/perf/test_coverage.sh

# 清理旧数据
rm -rf coverage/
mkdir -p coverage

# 编译 (带覆盖率标志)
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCODE_COVERAGE=ON \
      ..
make clean
make

# 运行所有测试
ctest --output-on-failure

# 生成覆盖率报告
lcov --capture \
     --directory . \
     --output-file coverage.info \
     --gcov-tool gcov \
     --exclude '*/tests/*' \
     --exclude '*/third_party/*' \
     --exclude '*/mocks/*'

# 生成 HTML 报告
genhtml coverage.info \
        --output-directory coverage/html \
        --title "MimiClaw v2.0 覆盖率报告" \
        --num-spaces 4

# 提取覆盖率
LINE_COVERAGE=$(lcov --summary coverage.info | grep "lines..." | awk '{print $2}' | tr -d '%')

if (( $(echo "$LINE_COVERAGE > 80" | bc -l) )); then
    echo "✓ 测试覆盖率测试通过: ${LINE_COVERAGE}%"
else
    echo "✗ 测试覆盖率测试失败: ${LINE_COVERAGE}% < 80%"
    exit 1
fi
```

**覆盖率要求细分**:
- libmimi-core: >85%
- libmimi-memory: >85%
- libmimi-config: >90%
- libmimi-tools: >80%

---

## 三、集成测试

### 3.1 Telegram Bot 端到端测试

**测试文件**: `tests/integration/test_telegram_bot.c`

**测试流程**:
```
1. 启动 MimiClaw Agent
2. 启动 Telegram Bot 网关
3. 模拟用户发送消息
4. 验证 Agent 处理逻辑
5. 验证响应发送回 Telegram
6. 验证消息持久化
```

**测试用例**:
```c
- test_telegram_message_receive        // 接收消息
- test_telegram_message_process        // 处理消息
- test_telegram_message_respond        // 发送响应
- test_telegram_command_handling       // 命令处理 (/start, /help)
- test_telegram_file_upload            // 文件上传
- test_telegram_group_chat             // 群聊支持
- test_telegram_concurrent_users       // 多用户并发 (100 用户)
```

**测试工具**:
- Telegram Bot API Mock
- Python telebot 库 (自动化测试脚本)

### 3.2 CLI 命令全量测试

**测试文件**: `tests/integration/test_cli.sh`

**命令清单**:
```bash
# 基础命令
mimiclaw version              # 版本信息
mimiclaw status               # 服务状态
mimiclaw config show          # 显示配置
mimiclaw config reload        # 重载配置

# 会话管理
mimiclaw session list         # 列出会话
mimiclaw session create       # 创建会话
mimiclaw session delete       # 删除会话
mimiclaw session export       # 导出会话

# 消息管理
mimiclaw message send         # 发送消息
mimiclaw message history      # 查看历史
mimiclaw message delete       # 删除消息

# 工具管理
mimiclaw tool list            # 列出工具
mimiclaw tool enable          # 启用工具
mimiclaw tool disable         # 禁用工具
mimiclaw tool install         # 安装插件

# 性能诊断
mimiclaw diag memory          # 内存诊断
mimiclaw diag performance     # 性能诊断
mimiclaw diag logs            # 日志查看
```

**测试脚本**:
```bash
#!/bin/bash
# tests/integration/test_cli_full.sh

set -e

echo "=== CLI 全量测试 ==="

# 版本命令
output=$(mimiclaw version)
[[ $output =~ "MimiClaw v2.0" ]] || exit 1

# 状态命令
output=$(mimiclaw status)
[[ $output =~ "running" ]] || exit 1

# 会话管理
SESSION_ID=$(mimiclaw session create | jq -r '.session_id')
mimiclaw session list | grep $SESSION_ID || exit 1
mimiclaw message send --session $SESSION_ID "Hello" || exit 1
mimiclaw session delete $SESSION_ID || exit 1

echo "✓ CLI 全量测试通过"
```

### 3.3 WebSocket 网关压力测试

**测试文件**: `tests/integration/test_websocket_stress.py`

**测试配置**:
```python
# 压力测试参数
CONCURRENT_CONNECTIONS = 1000
MESSAGES_PER_CONNECTION = 100
MESSAGE_SIZE = 1024  # bytes
TEST_DURATION = 300  # seconds
```

**测试脚本**:
```python
#!/usr/bin/env python3
# tests/integration/test_websocket_stress.py

import asyncio
import websockets
import time
import statistics

async def websocket_client(session_id, message_count):
    """单个 WebSocket 客户端"""
    uri = "ws://localhost:8080/ws"
    async with websockets.connect(uri) as websocket:
        for i in range(message_count):
            msg = f"{{'session_id': '{session_id}', 'message': 'test {i}'}}"
            start = time.time()
            await websocket.send(msg)
            response = await websocket.recv()
            latency = (time.time() - start) * 1000
            yield latency

async def stress_test():
    """压力测试主函数"""
    num_connections = 1000
    messages_per_conn = 100
    
    tasks = [
        websocket_client(f"session_{i}", messages_per_conn)
        for i in range(num_connections)
    ]
    
    start_time = time.time()
    all_latencies = []
    
    for coro in asyncio.as_completed(tasks):
        latencies = await coro
        all_latencies.extend(latencies)
    
    total_time = time.time() - start_time
    
    # 统计结果
    print(f"总连接数：{num_connections}")
    print(f"总消息数：{num_connections * messages_per_conn}")
    print(f"总耗时：{total_time:.2f}s")
    print(f"平均延迟：{statistics.mean(all_latencies):.2f}ms")
    print(f"P95 延迟：{statistics.quantiles(all_latencies, n=100)[94]:.2f}ms")
    print(f"P99 延迟：{statistics.quantiles(all_latencies, n=100)[98]:.2f}ms")
    
    # 验证
    p95 = statistics.quantiles(all_latencies, n=100)[94]
    if p95 < 200:
        print("✓ WebSocket 压力测试通过")
        return 0
    else:
        print(f"✗ WebSocket 压力测试失败: P95={p95}ms > 200ms")
        return 1

if __name__ == "__main__":
    exit(asyncio.run(stress_test()))
```

---

## 四、输出物

### 4.1 测试报告 (Markdown)

**模板**: `reports/test-report-template.md`

```markdown
# MimiClaw-OrangePi v2.0 测试报告

## 测试概况
- 测试日期: YYYY-MM-DD
- 测试版本: v2.0.x
- 测试环境: OrangePi 5 / Ubuntu 22.04
- 测试人员: [自动化测试]

## 测试结果汇总
| 测试类别 | 通过 | 失败 | 跳过 | 通过率 |
|----------|------|------|------|--------|
| 模块测试 | 45   | 2    | 0    | 95.7%  |
| 性能测试 | 4    | 0    | 0    | 100%   |
| 集成测试 | 3    | 0    | 0    | 100%   |
| **总计** | **52** | **2** | **0** | **96.3%** |

## 模块测试详情

### libmimi-core
- ✓ AI Agent 循环单元测试 (15/15)
- ✓ 上下文管理测试 (12/12)
- ✗ 工具调用测试 (8/10) - 2 个失败用例

### libmimi-memory
...

## 性能测试结果

| 指标 | 目标 | 实测 | 状态 |
|------|------|------|------|
| 内存占用 | <50MB | 42MB | ✓ |
| 启动时间 | <1s | 0.65s | ✓ |
| 响应延迟 (P95) | <200ms | 145ms | ✓ |
| 测试覆盖率 | >80% | 87.3% | ✓ |

## 已知问题

### 严重问题 (P0)
无

### 一般问题 (P1)
1. [Issue #123] 工具调用超时处理不完善
2. [Issue #124] 高并发下连接池等待时间偶尔超标

## 测试结论
□ 通过，可以发布
□ 通过，但有已知问题
□ 不通过，需要修复

签字: ___________
日期: ___________
```

### 4.2 覆盖率报告 (HTML)

**生成命令**:
```bash
# 生成 HTML 覆盖率报告
genhtml coverage.info \
        --output-directory reports/coverage-html \
        --title "MimiClaw v2.0 代码覆盖率报告" \
        --show-function-coverage \
        --show-branch-coverage \
        --highlight \
        --legend
```

**报告结构**:
```
reports/coverage-html/
├── index.html          # 总览页面
├── libmimi-core/       # core 模块覆盖率
├── libmimi-memory/     # memory 模块覆盖率
├── libmimi-config/     # config 模块覆盖率
└── libmimi-tools/      # tools 模块覆盖率
```

### 4.3 性能基准对比表

**文件**: `reports/performance-benchmark.csv`

```csv
测试项，v1.0,v2.0,改进幅度，目标，状态
内存占用 (MB),78,42,-46%,<50MB,✓
启动时间 (s),2.3,0.65,-72%,<1s,✓
响应延迟 P95 (ms),350,145,-59%,<200ms,✓
响应延迟 P99 (ms),580,198,-66%,-,✓
吞吐量 (QPS),120,450,+275%,-,✓
测试覆盖率 (%),62,87.3,+41%,>80%,✓
```

**可视化脚本**:
```python
#!/usr/bin/env python3
# scripts/generate_benchmark_chart.py

import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('reports/performance-benchmark.csv')

# 创建对比图
fig, axes = plt.subplots(2, 2, figsize=(12, 10))

# 内存占用对比
axes[0, 0].bar(['v1.0', 'v2.0'], [df['v1.0'][0], df['v2.0'][0]], color=['red', 'green'])
axes[0, 0].set_title('内存占用对比 (MB)')
axes[0, 0].axhline(y=50, color='orange', linestyle='--', label='目标 (<50MB)')

# 启动时间对比
axes[0, 1].bar(['v1.0', 'v2.0'], [df['v1.0'][1], df['v2.0'][1]], color=['red', 'green'])
axes[0, 1].set_title('启动时间对比 (s)')
axes[0, 1].axhline(y=1, color='orange', linestyle='--', label='目标 (<1s)')

# 响应延迟对比
axes[1, 0].bar(['v1.0', 'v2.0'], [df['v1.0'][2], df['v2.0'][2]], color=['red', 'green'])
axes[1, 0].set_title('响应延迟 P95 对比 (ms)')
axes[1, 0].axhline(y=200, color='orange', linestyle='--', label='目标 (<200ms)')

# 覆盖率对比
axes[1, 1].bar(['v1.0', 'v2.0'], [df['v1.0'][5], df['v2.0'][5]], color=['red', 'green'])
axes[1, 1].set_title('测试覆盖率对比 (%)')
axes[1, 1].axhline(y=80, color='orange', linestyle='--', label='目标 (>80%)')

plt.tight_layout()
plt.savefig('reports/performance-benchmark.png', dpi=300)
plt.show()
```

### 4.4 Bug 清单 (GitHub Issues)

**Issue 模板**: `.github/ISSUE_TEMPLATES/bug-report.md`

```markdown
---
name: Bug 报告
about: 报告测试中发现的问题
title: '[BUG] '
labels: bug, testing
assignees: ''
---

## Bug 描述
清晰简洁地描述 bug

## 复现步骤
1. 执行 '...'
2. 点击 '....'
3. 看到错误 '....'

## 期望行为
清晰简洁地描述期望发生什么

## 截图
如果适用，添加截图帮助解释问题

## 环境信息
- OS: [e.g. Ubuntu 22.04]
- MimiClaw 版本：[e.g. v2.0.1]
- 测试框架版本：[e.g. CMocka 1.1.5]

## 日志
```
相关日志输出
```

## 严重程度
- [ ] P0 - 阻塞发布
- [ ] P1 - 严重问题
- [ ] P2 - 一般问题
- [ ] P3 - 轻微问题

## 关联测试用例
- 测试文件：`tests/xxx/test_xxx.c`
- 测试函数：`test_xxx_xxx`
```

**Bug 清单示例**:

| Issue # | 标题 | 模块 | 严重程度 | 状态 |
|---------|------|------|----------|------|
| #123 | 工具调用超时处理不完善 | libmimi-core | P1 | Open |
| #124 | 高并发下连接池等待时间偶尔超标 | libmimi-memory | P2 | Open |
| #125 | YAML 配置锚点引用解析错误 | libmimi-config | P2 | Fixed |
| #126 | 插件沙箱文件访问限制绕过 | libmimi-tools | P0 | Fixed |

---

## 五、CI/CD 配置

### 5.1 GitHub Actions 工作流

**文件**: `.github/workflows/test.yml`

```yaml
name: MimiClaw v2.0 测试

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  unit-test:
    runs-on: ubuntu-22.04
    
    steps:
    - uses: actions/checkout@v3
    
    - name: 安装依赖
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          cmake \
          libcmocka-dev \
          libsqlite3-dev \
          libcurl4-openssl-dev \
          libyaml-dev \
          gcovr \
          lcov
    
    - name: 配置 (带覆盖率)
      run: |
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Debug \
              -DCODE_COVERAGE=ON \
              ..
    
    - name: 编译
      run: |
        cd build
        make -j$(nproc)
    
    - name: 运行单元测试
      run: |
        cd build
        ctest --output-on-failure
    
    - name: 生成覆盖率报告
      run: |
        cd build
        lcov --capture \
             --directory . \
             --output-file coverage.info \
             --exclude '*/tests/*' \
             --exclude '*/third_party/*'
        
        genhtml coverage.info \
                --output-directory coverage-report \
                --title "MimiClaw v2.0"
    
    - name: 上传覆盖率报告
      uses: actions/upload-artifact@v3
      with:
        name: coverage-report
        path: build/coverage-report/
    
    - name: 验证覆盖率
      run: |
        COVERAGE=$(lcov --summary build/coverage.info | grep "lines..." | awk '{print $2}' | tr -d '%')
        if (( $(echo "$COVERAGE < 80" | bc -l) )); then
          echo "覆盖率不足：${COVERAGE}% < 80%"
          exit 1
        fi
        echo "✓ 覆盖率：${COVERAGE}%"

  performance-test:
    runs-on: ubuntu-22.04
    needs: unit-test
    
    steps:
    - uses: actions/checkout@v3
    
    - name: 安装性能测试工具
      run: |
        sudo apt-get install -y \
          valgrind \
          wrk \
          python3-pip
        pip3 install websockets
    
    - name: 编译 (Release)
      run: |
        mkdir -p build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j$(nproc)
    
    - name: 内存占用测试
      run: |
        cd build
        valgrind --tool=massif \
                 --massif-out-file=/tmp/massif.out \
                 ./mimiclaw-agent --config=test.conf &
        AGENT_PID=$!
        sleep 5
        kill $AGENT_PID
        
        PEAK_MEM=$(grep -oP 'mem_heap_B=\K\d+' /tmp/massif.out | sort -n | tail -1)
        if [ $PEAK_MEM -lt 52428800 ]; then
          echo "✓ 内存占用：$(($PEAK_MEM / 1024 / 1024))MB"
        else
          echo "✗ 内存占用超标：$(($PEAK_MEM / 1024 / 1024))MB"
          exit 1
        fi
    
    - name: 响应延迟测试
      run: |
        cd build
        ./mimiclaw-agent --config=test.conf &
        AGENT_PID=$!
        sleep 2
        
        wrk -t4 -c100 -d30s \
            --latency \
            http://localhost:8080/api/v1/health
        
        kill $AGENT_PID

  integration-test:
    runs-on: ubuntu-22.04
    needs: unit-test
    
    services:
      telegram-mock:
        image: python:3.10
        ports:
          - 8081:8081
    
    steps:
    - uses: actions/checkout@v3
    
    - name: 运行集成测试
      run: |
        ./scripts/run-integration-tests.sh
    
    - name: 上传测试报告
      uses: actions/upload-artifact@v3
      with:
        name: test-report
        path: reports/test-report.md
```

---

## 六、测试执行清单

### 6.1 本地开发环境测试

```bash
# 1. 准备环境
./scripts/setup-test-env.sh

# 2. 编译 (Debug + 覆盖率)
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON ..
make -j$(nproc)

# 3. 运行所有测试
ctest --output-on-failure

# 4. 生成覆盖率报告
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-report

# 5. 性能测试
./scripts/run-perf-tests.sh

# 6. 集成测试
./scripts/run-integration-tests.sh
```

### 6.2 CI 环境测试

```bash
# GitHub Actions 自动执行
# 参考 .github/workflows/test.yml
```

### 6.3 发布前验证

```bash
# 发布前完整测试流程
./scripts/release-validation.sh

# 包含:
# - 全量单元测试
# - 性能基准测试
# - 集成测试
# - 覆盖率验证 (>80%)
# - 生成测试报告
# - 生成 Bug 清单
```

---

## 七、附录

### 7.1 测试环境要求

**硬件**:
- CPU: 4 核心以上
- 内存：8GB 以上
- 存储：50GB 可用空间

**软件**:
- OS: Ubuntu 22.04 LTS / OrangePi OS
- CMake: >= 3.20
- GCC: >= 11.0
- CMocka: >= 1.1.5
- SQLite: >= 3.35
- Python: >= 3.10 (集成测试)

### 7.2 依赖安装脚本

```bash
#!/bin/bash
# scripts/setup-test-env.sh

set -e

echo "=== 安装测试环境依赖 ==="

# 系统依赖
sudo apt-get update
sudo apt-get install -y \
    cmake \
    gcc \
    g++ \
    libcmocka-dev \
    libsqlite3-dev \
    libcurl4-openssl-dev \
    libyaml-dev \
    libssl-dev \
    valgrind \
    wrk \
    gcovr \
    lcov \
    python3 \
    python3-pip \
    jq

# Python 依赖
pip3 install \
    websockets \
    requests \
    pandas \
    matplotlib

echo "✓ 测试环境准备完成"
```

### 7.3 快速参考

| 测试类型 | 命令 | 预期时间 |
|----------|------|----------|
| 单元测试 | `ctest --output-on-failure` | 2-5 分钟 |
| 覆盖率 | `./scripts/generate-coverage.sh` | 1-2 分钟 |
| 性能测试 | `./scripts/run-perf-tests.sh` | 5-10 分钟 |
| 集成测试 | `./scripts/run-integration-tests.sh` | 10-15 分钟 |
| 全量测试 | `./scripts/run-all-tests.sh` | 20-30 分钟 |

---

_文档版本：v1.0_
_创建日期：2026-03-10_
_维护者：Dev-Planner_
