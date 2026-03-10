# MimiClaw-OrangePi v2.0 架构设计文档

**版本：** 2.0.0  
**日期：** 2026-03-10  
**状态：** 重构中  
**目标硬件：** OrangePi Zero3 (四核 Cortex-A53, 512MB-2GB RAM)

---

## 1. 架构概述

### 1.1 设计目标

| 指标 | v1.0 当前 | v2.0 目标 | 优化幅度 |
|------|----------|----------|----------|
| 内存占用 | ~80MB | <50MB | -37.5% |
| 启动时间 | ~3s | <1s | -66.7% |
| 响应延迟 | ~500ms | <200ms | -60% |
| 测试覆盖 | ~30% | >80% | +166% |
| 模块耦合度 | 高 | 低 | 解耦 |

### 1.2 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                      MimiClaw-OrangePi v2.0                      │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │   CLI       │  │  Telegram   │  │  WebSocket  │             │
│  │   Interface │  │    Bot      │  │   Gateway   │             │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘             │
│         │                │                │                     │
│         └────────────────┼────────────────┘                     │
│                          │                                      │
│                  ┌───────▼────────┐                             │
│                  │  Command Router │                            │
│                  │  (mimi-gateway) │                            │
│                  └───────┬────────┘                             │
│                          │                                      │
│  ┌───────────────────────┼───────────────────────┐             │
│  │                    Core Layer                  │             │
│  │  ┌─────────────┐  ┌─────────────┐  ┌────────┐ │             │
│  │  │ AI Agent    │  │  Memory     │  │ Config │ │             │
│  │  │ (ReAct)     │  │  Manager    │  │ Manager│ │             │
│  │  │ libmimi-core│  │libmimi-mem  │  │libmimi-│ │             │
│  │  └──────┬──────┘  └──────┬──────┘  └───┬────┘ │             │
│  │         │                │               │     │             │
│  │  ┌──────▼────────────────▼───────────────▼────┐│             │
│  │  │           Tool Registry (Plugin)           ││             │
│  │  │              libmimi-tools                 ││             │
│  │  └─────────────────────┬──────────────────────┘│             │
│  └────────────────────────┼────────────────────────┘             │
│                           │                                       │
│  ┌────────────────────────▼────────────────────────┐             │
│  │              Infrastructure Layer                │             │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │             │
│  │  │ SQLite   │  │ libcurl  │  │ OpenSSL      │  │             │
│  │  │ (Memory) │  │ (HTTP)   │  │ (Security)   │  │             │
│  │  └──────────┘  └──────────┘  └──────────────┘  │             │
│  └─────────────────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. 模块详细设计

### 2.1 libmimi-core (核心库)

**职责：** AI Agent 循环、上下文管理、工具调用调度

**API 设计：**
```c
// 核心结构体
typedef struct {
    char *api_key;
    char *model;
    int max_tokens;
    float temperature;
} mimi_core_config_t;

typedef struct {
    mimi_core_config_t config;
    void *memory_handle;
    void *tool_registry;
    // 无全局状态，支持多实例
} mimi_core_ctx_t;

// 生命周期
int mimi_core_init(mimi_core_ctx_t *ctx, const mimi_core_config_t *config);
int mimi_core_destroy(mimi_core_ctx_t *ctx);

// 对话处理
int mimi_core_chat(mimi_core_ctx_t *ctx, const char *session_id, 
                   const char *user_message, char **response);

// 上下文管理
int mimi_core_set_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *context_key, const char *context_value);
int mimi_core_get_context(mimi_core_ctx_t *ctx, const char *session_id,
                          const char *context_key, char **context_value);
```

**重构重点：**
- [ ] 移除所有全局变量
- [ ] 实现上下文隔离（多会话并发）
- [ ] 工具调用异步化

---

### 2.2 libmimi-memory (记忆库)

**职责：** SQLite 持久化、会话管理、记忆检索

**API 设计：**
```c
// 连接池配置
typedef struct {
    const char *db_path;
    int pool_size;          // 连接池大小
    int max_idle_time;      // 最大空闲时间 (秒)
} mimi_mem_config_t;

typedef struct mimi_mem_pool mimi_mem_pool_t;  //  opaque

// 生命周期
mimi_mem_pool_t* mimi_mem_pool_create(const mimi_mem_config_t *config);
void mimi_mem_pool_destroy(mimi_mem_pool_t *pool);

// 会话操作
int mimi_mem_session_create(mimi_mem_pool_t *pool, const char *session_id);
int mimi_mem_session_delete(mimi_mem_pool_t *pool, const char *session_id);
int mimi_mem_session_list(mimi_mem_pool_t *pool, char ***sessions, int *count);

// 消息存储
int mimi_mem_message_append(mimi_mem_pool_t *pool, const char *session_id,
                            const char *role, const char *content, int64_t *msg_id);
int mimi_mem_message_query(mimi_mem_pool_t *pool, const char *session_id,
                           int limit, char ***messages, int *count);

// 记忆操作
int mimi_mem_write(mimi_mem_pool_t *pool, const char *key, const char *value);
int mimi_mem_read(mimi_mem_pool_t *pool, const char *key, char **value);
int mimi_mem_search(mimi_mem_pool_t *pool, const char *query, 
                    char ***results, int *count);
```

**优化措施：**
- [ ] SQLite WAL 模式 (Write-Ahead Logging)
- [ ] 预编译语句 (sqlite3_prepare_v2)
- [ ] 连接池复用，避免频繁打开/关闭
- [ ] 事务批量写入

---

### 2.3 libmimi-config (配置库)

**职责：** 配置解析、验证、热重载

**API 设计：**
```c
typedef struct {
    const char *config_path;
    int watch_enabled;      // 是否启用文件监听
    void (*on_reload)(void *user_data, int status);
    void *user_data;
} mimi_config_options_t;

typedef struct mimi_config mimi_config_t;  // opaque

// 生命周期
mimi_config_t* mimi_config_load(const char *path);
void mimi_config_free(mimi_config_t *cfg);

// 读取配置
int mimi_config_get_string(mimi_config_t *cfg, const char *key, char **value);
int mimi_config_get_int(mimi_config_t *cfg, const char *key, int *value);
int mimi_config_get_bool(mimi_config_t *cfg, const char *key, int *value);

// 热重载
int mimi_config_watch(mimi_config_t *cfg, const mimi_config_options_t *opts);
int mimi_config_reload(mimi_config_t *cfg);

// 验证
int mimi_config_validate(mimi_config_t *cfg, char **error_msg);
```

**功能特性：**
- [ ] 支持 JSON 和 YAML 格式
- [ ] inotify 文件监听 (Linux)
- [ ] 配置验证 schema
- [ ] 原子写入 (避免读取半写入文件)

---

### 2.4 libmimi-tools (工具插件库)

**职责：** 工具注册表、插件加载、沙箱隔离

**API 设计：**
```c
// 插件接口 (所有工具必须实现)
typedef struct {
    const char *name;
    const char *description;
    int (*init)(void *ctx);
    int (*exec)(void *ctx, const char *input, char **output);
    int (*destroy)(void *ctx);
} mimi_tool_plugin_t;

// 工具注册表
typedef struct mimi_tool_registry mimi_tool_registry_t;

mimi_tool_registry_t* mimi_registry_create(void);
void mimi_registry_destroy(mimi_tool_registry_t *reg);

// 插件加载
int mimi_registry_load_plugin(mimi_tool_registry_t *reg, const char *so_path);
int mimi_registry_unload_plugin(mimi_tool_registry_t *reg, const char *name);

// 工具调用
int mimi_registry_exec(mimi_tool_registry_t *reg, const char *tool_name,
                       const char *input, char **output);

// 查询
int mimi_registry_list(mimi_tool_registry_t *reg, char ***tools, int *count);
const mimi_tool_plugin_t* mimi_registry_get(mimi_tool_registry_t *reg, 
                                             const char *name);
```

**内置工具迁移：**
| 工具名 | 功能 | 迁移状态 |
|--------|------|----------|
| web_search | Brave/Perplexity 搜索 | ⏳ 待迁移 |
| time | 获取当前时间/日期 | ⏳ 待迁移 |
| file_ops | 文件读写 | ⏳ 待迁移 |
| memory_read | 读取记忆 | ⏳ 待迁移 |
| memory_write | 写入记忆 | ⏳ 待迁移 |

---

### 2.5 mimi-gateway (网关服务)

**职责：** 多协议接入、命令路由、会话管理

**接入协议：**
- CLI (命令行)
- Telegram Bot
- WebSocket API

**命令路由表：**
```
/chat <message>          → mimi_core_chat()
/memory read <key>       → mimi_mem_read()
/memory write <k> <v>    → mimi_mem_write()
/session list            → mimi_mem_session_list()
/session clear           → mimi_mem_session_delete()
/config show             → mimi_config_get_*()
/tool list               → mimi_registry_list()
```

---

## 3. 构建系统

### 3.1 目录结构

```
MimiClaw-OrangePi/
├── CMakeLists.txt              # 顶层构建配置
├── libs/
│   ├── libmimi-core/
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── core.c
│   │   │   ├── agent.c
│   │   │   └── context.c
│   │   └── include/
│   │       └── mimi_core.h
│   ├── libmimi-memory/
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── memory.c
│   │   │   └── pool.c
│   │   └── include/
│   │       └── mimi_memory.h
│   ├── libmimi-config/
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── config.c
│   │   │   └── watch.c
│   │   └── include/
│   │       └── mimi_config.h
│   └── libmimi-tools/
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── registry.c
│       │   └── loader.c
│       └── include/
│           └── mimi_tools.h
├── apps/
│   └── mimi-gateway/
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── main.c
│       │   ├── cli.c
│       │   ├── telegram.c
│       │   └── websocket.c
│       └── config/
│           └── mimi_config.example.json
├── plugins/                    # 工具插件 (动态库)
│   ├── plugin-web-search/
│   ├── plugin-time/
│   └── plugin-file-ops/
├── tests/
│   ├── CMakeLists.txt
│   ├── test_core.c
│   ├── test_memory.c
│   ├── test_config.c
│   └── test_tools.c
└── scripts/
    ├── build.sh
    ├── test.sh
    └── install.sh
```

### 3.2 CMake 配置示例

**顶层 CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.16)
project(MimiClaw-OrangePi VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 选项
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_PLUGINS "Build tool plugins" ON)

# 依赖
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# 子目录
add_subdirectory(libs/libmimi-core)
add_subdirectory(libs/libmimi-memory)
add_subdirectory(libs/libmimi-config)
add_subdirectory(libs/libmimi-tools)
add_subdirectory(apps/mimi-gateway)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()
```

---

## 4. 性能优化方案

### 4.1 内存池

```c
typedef struct {
    void *blocks;
    size_t block_size;
    size_t block_count;
    size_t free_index;
} mimi_memory_pool_t;

mimi_memory_pool_t* mimi_pool_create(size_t block_size, size_t count);
void* mimi_pool_alloc(mimi_memory_pool_t *pool);
void mimi_pool_free(mimi_memory_pool_t *pool, void *ptr);
void mimi_pool_destroy(mimi_memory_pool_t *pool);
```

**使用场景：**
- 消息缓冲区 (固定大小 4KB)
- 会话上下文 (固定大小 2KB)
- HTTP 响应缓冲 (固定大小 16KB)

### 4.2 SQLite 优化

```c
// 启用 WAL 模式
sqlite3_exec(db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);

// 预编译语句
sqlite3_stmt *stmt;
sqlite3_prepare_v2(db, "INSERT INTO messages (...) VALUES (...)", -1, &stmt, NULL);

// 事务批量写入
sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
// ... 批量插入 ...
sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
```

### 4.3 异步 I/O

**方案选择：**
- **libuv** - 跨平台，功能完整，但依赖较多
- **epoll** - Linux 原生，轻量，但仅支持 Linux

**推荐：** OrangePi 使用 epoll (减少依赖)

---

## 5. 测试策略

### 5.1 单元测试 (CMocka)

```c
#include <cmocka.h>

static void test_core_init(void **state) {
    mimi_core_ctx_t ctx;
    mimi_core_config_t config = {.api_key = "test", .model = "claude"};
    
    int ret = mimi_core_init(&ctx, &config);
    assert_int_equal(ret, 0);
    
    mimi_core_destroy(&ctx);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_core_init),
        // ...
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

### 5.2 覆盖率目标

| 模块 | 行覆盖率 | 分支覆盖率 |
|------|---------|-----------|
| libmimi-core | >80% | >70% |
| libmimi-memory | >85% | >75% |
| libmimi-config | >80% | >70% |
| libmimi-tools | >75% | >65% |

---

## 6. 迁移计划

### Phase 1: 架构解耦 (Week 1-2)
- [ ] 创建 libs/ 目录结构
- [ ] 迁移 libmimi-core
- [ ] 迁移 libmimi-memory
- [ ] 迁移 libmimi-config
- [ ] 更新 CMakeLists.txt

### Phase 2: 插件系统 (Week 3)
- [ ] 实现 libmimi-tools 框架
- [ ] 迁移现有工具为插件
- [ ] 测试动态加载

### Phase 3: 性能优化 (Week 4)
- [ ] 实现内存池
- [ ] SQLite 优化 (WAL + 预编译)
- [ ] 异步 I/O 改造

### Phase 4: 测试与文档 (Week 5)
- [ ] 编写单元测试
- [ ] 性能基准测试
- [ ] API 文档 (Doxygen)
- [ ] 迁移报告

---

## 7. 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| 内存池实现复杂 | 高 | 先使用 jemalloc，后优化 |
| 插件沙箱隔离困难 | 中 | v1.0 先不做沙箱，v2.1 再加 |
| 异步 I/O 改造工作量大 | 高 | 先优化同步 I/O，异步作为可选 |
| 测试覆盖率难达标 | 中 | 优先覆盖核心路径，边缘 case 后续补充 |

---

## 8. 参考资源

- [SQLite Performance Tuning](https://www.sqlite.org/np1queryprob.html)
- [CMocka Documentation](https://api.cmocka.org/)
- [Linux epoll Tutorial](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [CMake Best Practices](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1)

---

**文档维护：** Dev-Planner  
**最后更新：** 2026-03-10
