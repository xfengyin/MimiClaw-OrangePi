# MimiClaw-OrangePi v2.0 测试基础设施

本目录包含 MimiClaw-OrangePi v2.0 重构项目的完整测试基础设施。

## 目录结构

```
tests/
├── CMakeLists.txt              # 测试构建配置
├── core/                       # libmimi-core 测试
│   ├── test_agent_loop.c       # AI Agent 循环测试
│   ├── test_context_manager.c  # 上下文管理测试
│   └── test_tool_invocation.c  # 工具调用测试
├── memory/                     # libmimi-memory 测试
│   ├── test_sqlite_crud.c      # SQLite CRUD 测试
│   ├── test_connection_pool.c  # 连接池压力测试
│   └── test_persistence.c      # 持久化验证测试
├── config/                     # libmimi-config 测试
│   ├── test_parser.c           # JSON/YAML 解析测试
│   ├── test_hot_reload.c       # 热重载测试
│   └── test_validation.c       # 配置验证测试
├── tools/                      # libmimi-tools 测试
│   ├── test_plugin_lifecycle.c # 插件生命周期测试
│   ├── test_plugin_sandbox.c   # 沙箱隔离测试
│   └── test_registry.c         # 注册表查询测试
├── integration/                # 集成测试
│   ├── test_telegram_bot.py    # Telegram Bot 测试
│   ├── test_cli.sh             # CLI 命令测试
│   └── test_websocket_stress.py # WebSocket 压力测试
├── perf/                       # 性能测试
│   ├── test_memory.c           # 内存占用测试
│   └── test_latency.c          # 响应延迟测试
└── mocks/                      # Mock 实现
    ├── mock_http_client.c      # HTTP 客户端 Mock
    ├── mock_database.c         # 数据库 Mock
    ├── mock_file_system.c      # 文件系统 Mock
    └── mock_external_api.c     # 外部 API Mock

scripts/
├── setup-test-env.sh           # 测试环境安装脚本
├── run-perf-tests.sh           # 性能测试执行脚本
├── run-integration-tests.sh    # 集成测试执行脚本
├── generate-coverage.sh        # 覆盖率报告生成脚本
├── generate_benchmark.sh       # 性能基准对比脚本
├── generate-test-report.sh     # 综合测试报告生成脚本
└── wrk_chat.lua                # wrk 压测脚本

.github/
└── workflows/
    └── test.yml                # GitHub Actions CI 配置

reports/
├── test-report.md              # 测试报告 (生成)
├── coverage-html/              # 覆盖率 HTML 报告 (生成)
├── performance-benchmark.csv   # 性能基准对比表 (生成)
└── performance-benchmark.png   # 性能对比图 (生成)
```

## 快速开始

### 1. 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
  cmake \
  gcc \
  g++ \
  libcmocka-dev \
  libsqlite3-dev \
  libcurl4-openssl-dev \
  libyaml-dev \
  valgrind \
  wrk \
  gcovr \
  lcov \
  python3 \
  python3-pip

# Python 依赖
pip3 install websockets requests pytest pandas matplotlib
```

### 2. 编译测试

```bash
mkdir -p build && cd build

# Debug 模式 (带覆盖率)
cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON ..
make -j$(nproc)
```

### 3. 运行测试

```bash
# 所有单元测试
ctest --output-on-failure

# 特定测试组
ctest -R "test_agent_loop" --verbose

# 性能测试
./scripts/run-perf-tests.sh

# 集成测试
./scripts/run-integration-tests.sh

# 生成覆盖率报告
make coverage
```

## 测试框架

### CMocka (单元测试)

- **版本**: >= 1.1.5
- **文档**: https://cmocka.org/
- **示例**:
  ```c
  static void test_example(void **state) {
      assert_int_equal(2 + 2, 4);
  }
  
  int main(void) {
      const struct CMUnitTest tests[] = {
          cmocka_unit_test(test_example),
      };
      return cmocka_run_group_tests(tests, NULL, NULL);
  }
  ```

### wrk (性能测试)

- **版本**: >= 4.1.0
- **文档**: https://github.com/wg/wrk
- **示例**:
  ```bash
  wrk -t4 -c100 -d30s --latency http://localhost:8080/api/v1/chat
  ```

### valgrind (内存分析)

- **版本**: >= 3.18.0
- **文档**: https://valgrind.org/
- **示例**:
  ```bash
  valgrind --tool=massif --massif-out-file=massif.out ./mimiclaw-agent
  ms_print massif.out
  ```

### gcov/lcov (覆盖率)

- **版本**: gcov >= 11.0, lcov >= 1.15
- **文档**: http://ltp.sourceforge.net/coverage/lcov.php
- **示例**:
  ```bash
  lcov --capture --directory . --output-file coverage.info
  genhtml coverage.info --output-directory coverage-report
  ```

## 测试覆盖要求

| 模块 | 最低覆盖率 | 目标覆盖率 |
|------|-----------|-----------|
| libmimi-core | 85% | 90% |
| libmimi-memory | 85% | 90% |
| libmimi-config | 90% | 95% |
| libmimi-tools | 80% | 85% |
| **总计** | **80%** | **85%** |

## 性能要求

| 指标 | 目标 | 测试方法 |
|------|------|----------|
| 内存占用 | <50MB | valgrind --tool=massif |
| 启动时间 | <1s | systemd-analyze / 手动计时 |
| 响应延迟 (P95) | <200ms | wrk 压测 |
| 响应延迟 (P99) | <500ms | wrk 压测 |
| 吞吐量 | >400 QPS | wrk 压测 |

## CI/CD 集成

GitHub Actions 工作流自动执行以下测试：

1. **单元测试** - 每次推送/PR 触发
2. **性能测试** - 通过单元测试后触发
3. **集成测试** - 通过单元测试后触发
4. **生成报告** - 所有测试完成后生成综合报告

工作流配置：`.github/workflows/test.yml`

## 编写新测试

### 单元测试模板

```c
/**
 * @file test_xxx.c
 * @brief XXX 模块测试
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

// Setup/Teardown
static int setup_xxx(void **state) {
    // 初始化
    return 0;
}

static int teardown_xxx(void **state) {
    // 清理
    return 0;
}

// 测试用例
static void test_xxx_example(void **state) {
    assert_int_equal(2 + 2, 4);
}

// 主函数
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(
            test_xxx_example,
            setup_xxx, teardown_xxx
        ),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

### 集成测试模板 (Python)

```python
#!/usr/bin/env python3
"""
集成测试示例
"""

import requests
import unittest

class TestTelegramBot(unittest.TestCase):
    
    def setUp(self):
        self.base_url = "http://localhost:8080"
    
    def test_message_receive(self):
        response = requests.post(
            f"{self.base_url}/api/v1/chat",
            json={"message": "Hello", "session_id": "test"}
        )
        self.assertEqual(response.status_code, 200)

if __name__ == "__main__":
    unittest.main()
```

## 调试技巧

### 1. 单个测试调试

```bash
# GDB 调试
gdb --args ./mimiclaw-tests --group=core --test=agent_loop

# 详细输出
ctest -R test_name --verbose --output-on-failure
```

### 2. 内存泄漏检测

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./mimiclaw-tests
```

### 3. 性能分析

```bash
# CPU 性能分析
perf record -g ./mimiclaw-agent
perf report

# 内存分析
valgrind --tool=massif ./mimiclaw-agent
ms_print massif.out
```

## 常见问题

### Q: 测试编译失败？
```bash
# 清理构建
rm -rf build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make clean
make -j$(nproc)
```

### Q: 测试运行失败？
```bash
# 查看详细错误
ctest --output-on-failure --verbose

# 查看日志
cat build/Testing/Temporary/LastTest.log
```

### Q: 覆盖率报告为空？
```bash
# 确保使用 Debug 模式编译
cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON ..

# 运行测试后生成报告
ctest
make coverage
```

## 参考文档

- [MimiClaw v2.0 测试计划](../MimiClaw-v2-Test-Plan.md)
- [测试执行清单](../TEST-CHECKLIST.md)
- [CMocka 官方文档](https://cmocka.org/)
- [GitHub Actions 文档](https://docs.github.com/en/actions)

---

_维护者：Dev-Planner_
_创建日期：2026-03-10_
