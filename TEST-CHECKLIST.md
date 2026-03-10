# MimiClaw-OrangePi v2.0 测试执行清单

## 快速开始

### 1. 环境准备

```bash
# 安装测试依赖
./scripts/setup-test-env.sh

# 验证安装
cmake --version
gcc --version
cmocka --version
```

### 2. 编译项目

```bash
# Debug 模式 (带覆盖率)
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON ..
make -j$(nproc)

# Release 模式 (性能测试)
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 3. 运行测试

```bash
# 运行所有单元测试
cd build
ctest --output-on-failure

# 运行特定测试组
ctest -R "test_agent_loop" --verbose
ctest -R "test_sqlite" --verbose

# 运行性能测试
./scripts/run-perf-tests.sh

# 运行集成测试
./scripts/run-integration-tests.sh

# 生成覆盖率报告
make coverage
```

---

## 测试清单

### 模块测试

#### libmimi-core
- [ ] AI Agent 循环单元测试
  - [ ] 初始化测试
  - [ ] 事件处理测试
  - [ ] 状态转换测试
  - [ ] 错误恢复测试
  - [ ] 优雅关闭测试
  - [ ] 并发会话测试 (100 线程)
  - [ ] 超时处理测试
  - [ ] 内存泄漏检测

- [ ] 上下文管理测试
  - [ ] 创建/销毁测试
  - [ ] 会话隔离测试
  - [ ] 并发访问测试 (100 线程)
  - [ ] 内存泄漏检测
  - [ ] 序列化测试

- [ ] 工具调用测试
  - [ ] 工具注册测试
  - [ ] 工具解析测试
  - [ ] 执行成功测试
  - [ ] 执行失败测试
  - [ ] 超时处理测试
  - [ ] Mock API 测试

#### libmimi-memory
- [ ] SQLite CRUD 操作测试
  - [ ] 数据库初始化
  - [ ] 单条记录插入
  - [ ] 批量插入 (1000 条)
  - [ ] 按 ID 查询
  - [ ] 条件查询
  - [ ] 记录更新
  - [ ] 记录删除
  - [ ] 事务提交
  - [ ] 事务回滚

- [ ] 连接池压力测试
  - [ ] 连接池初始化
  - [ ] 连接池耗尽处理
  - [ ] 连接超时测试
  - [ ] 并发访问测试 (100 线程)
  - [ ] 连接回收测试
  - [ ] 内存占用监控

- [ ] 数据持久化验证
  - [ ] 写入后读取验证
  - [ ] 重启后数据完整
  - [ ] 崩溃恢复测试
  - [ ] WAL 模式验证

#### libmimi-config
- [ ] JSON/YAML 解析测试
  - [ ] 简单 JSON 解析
  - [ ] 嵌套 JSON 解析
  - [ ] 数组解析
  - [ ] Unicode 字符处理
  - [ ] 非法 JSON 拒绝
  - [ ] 简单 YAML 解析
  - [ ] 嵌套 YAML 解析
  - [ ] 锚点引用
  - [ ] 非法 YAML 拒绝

- [ ] 热重载测试
  - [ ] JSON 文件热重载
  - [ ] YAML 文件热重载
  - [ ] 非法配置拒绝重载
  - [ ] 重载期间并发请求处理
  - [ ] 重载失败回滚

- [ ] 配置验证测试
  - [ ] 负数超时值拒绝
  - [ ] 非法路径拒绝
  - [ ] 缺少必填字段拒绝
  - [ ] 类型不匹配拒绝
  - [ ] 超出范围拒绝
  - [ ] 重复键名拒绝

#### libmimi-tools
- [ ] 插件加载/卸载测试
  - [ ] 加载有效插件
  - [ ] 加载无效插件 (拒绝)
  - [ ] 卸载插件
  - [ ] 重载插件
  - [ ] 插件加载顺序
  - [ ] 依赖解析

- [ ] 插件沙箱隔离测试
  - [ ] 内存隔离
  - [ ] 文件系统限制
  - [ ] 网络访问限制
  - [ ] 系统调用限制
  - [ ] 资源限制 (CPU/内存)
  - [ ] 沙箱逃逸尝试 (应失败)

- [ ] 工具注册表查询测试
  - [ ] 注册工具
  - [ ] 注销工具
  - [ ] 按名称查询
  - [ ] 按类别查询
  - [ ] 列出所有工具
  - [ ] 重复注册拒绝
  - [ ] 查询不存在工具

---

### 性能测试

| 指标 | 目标 | 测试方法 | 状态 |
|------|------|----------|------|
| 内存占用 | <50MB | valgrind --tool=massif | [ ] |
| 启动时间 | <1s | systemd-analyze / 手动计时 | [ ] |
| 响应延迟 (P95) | <200ms | wrk 压测 | [ ] |
| 测试覆盖率 | >80% | gcov + lcov | [ ] |

#### 内存占用测试
- [ ] 空载状态内存基线
- [ ] 10 个并发会话
- [ ] 100 个并发会话
- [ ] 长时间运行 (24 小时) 内存泄漏检测

#### 启动时间测试
- [ ] systemd 服务分析
- [ ] 手动计时测试
- [ ] 优化检查点验证

#### 响应延迟测试
- [ ] 简单问答 (无工具调用)
- [ ] 复杂问答 (带工具调用)
- [ ] 多轮对话 (上下文累积)
- [ ] 高并发场景 (1000 QPS)

#### 测试覆盖率测试
- [ ] libmimi-core: >85%
- [ ] libmimi-memory: >85%
- [ ] libmimi-config: >90%
- [ ] libmimi-tools: >80%

---

### 集成测试

- [ ] Telegram Bot 端到端测试
  - [ ] 接收消息
  - [ ] 处理消息
  - [ ] 发送响应
  - [ ] 命令处理 (/start, /help)
  - [ ] 文件上传
  - [ ] 群聊支持
  - [ ] 多用户并发 (100 用户)

- [ ] CLI 命令全量测试
  - [ ] 版本命令
  - [ ] 状态命令
  - [ ] 配置管理
  - [ ] 会话管理
  - [ ] 消息管理
  - [ ] 工具管理
  - [ ] 性能诊断

- [ ] WebSocket 网关压力测试
  - [ ] 1000 并发连接
  - [ ] 每连接 100 条消息
  - [ ] P95 延迟 <200ms
  - [ ] P99 延迟 <500ms

---

## 输出物清单

### 1. 测试报告 (Markdown)
- [ ] 测试概况
- [ ] 测试结果汇总
- [ ] 模块测试详情
- [ ] 性能测试结果
- [ ] 已知问题清单
- [ ] 测试结论

**文件位置**: `reports/test-report.md`

### 2. 覆盖率报告 (HTML)
- [ ] 总览页面
- [ ] libmimi-core 覆盖率
- [ ] libmimi-memory 覆盖率
- [ ] libmimi-config 覆盖率
- [ ] libmimi-tools 覆盖率

**文件位置**: `reports/coverage-html/index.html`

### 3. 性能基准对比表
- [ ] v1.0 vs v2.0 对比
- [ ] 可视化图表
- [ ] 改进幅度计算

**文件位置**: `reports/performance-benchmark.csv`

### 4. Bug 清单 (GitHub Issues)
- [ ] P0 严重问题
- [ ] P1 主要问题
- [ ] P2 一般问题
- [ ] P3 轻微问题

**文件位置**: GitHub Issues

---

## CI/CD 状态

### GitHub Actions 工作流

- [ ] 单元测试 (每次推送/PR)
- [ ] 性能测试 (通过单元测试后)
- [ ] 集成测试 (通过单元测试后)
- [ ] 生成测试报告 (所有测试完成后)

**工作流文件**: `.github/workflows/test.yml`

---

## 通过标准

### 必须满足 (P0)
- ✅ 所有单元测试通过 (100%)
- ✅ 内存占用 <50MB
- ✅ 启动时间 <1s
- ✅ 响应延迟 P95 <200ms
- ✅ 测试覆盖率 >80%
- ✅ 无 P0 级别 Bug

### 建议满足 (P1)
- ✅ 所有集成测试通过
- ✅ 性能指标优于 v1.0 版本
- ✅ 文档完整

---

## 常见问题

### Q1: 测试失败怎么办？
```bash
# 查看详细错误
ctest --output-on-failure --verbose

# 运行单个测试
ctest -R test_name --verbose

# 查看日志
cat build/Testing/Temporary/LastTest.log
```

### Q2: 覆盖率不足怎么办？
```bash
# 查看未覆盖的代码
genhtml coverage.info --output-directory coverage-report
# 打开 coverage-report/index.html 查看

# 针对未覆盖的函数编写测试
```

### Q3: 性能不达标怎么办？
```bash
# 使用 valgrind 分析
valgrind --tool=callgrind ./mimiclaw-agent

# 使用 perf 分析
perf record -g ./mimiclaw-agent
perf report

# 查看性能报告
cat reports/performance-benchmark.csv
```

---

## 联系方式

- **项目负责人**: xfengyin
- **项目仓库**: https://github.com/xfengyin/MimiClaw-OrangePi
- **问题反馈**: GitHub Issues

---

_文档版本：v1.0_
_创建日期：2026-03-10_
_维护者：Dev-Planner_
