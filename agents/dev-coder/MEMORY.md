# MEMORY.md - Dev-Coder 记忆库

# 项目代码信息

## 核心模块

### libmimi-core
- **位置:** `libs/libmimi-core/`
- **功能:** AI Agent 核心、上下文管理、工具调度
- **关键文件:**
  - `src/core.c` - 核心实现
  - `include/mimi_core.h` - 头文件

### libmimi-memory
- **位置:** `libs/libmimi-memory/`
- **功能:** SQLite 持久化、连接池
- **关键文件:**
  - `src/memory.c` - 内存操作
  - `src/pool.c` - 连接池

### libmimi-config
- **位置:** `libs/libmimi-config/`
- **功能:** 配置解析、热重载

### libmimi-tools
- **位置:** `libs/libmimi-tools/`
- **功能:** 插件注册表、动态加载

## 插件系统

| 插件 | 功能 | 状态 |
|------|------|------|
| plugin-time | 时间查询 | ✅ |
| plugin-echo | 测试插件 | ✅ |
| plugin-web-search | 网络搜索 | ✅ |
| plugin-file-ops | 文件操作 | ✅ |
| plugin-memory | 记忆操作 | ✅ |

## 构建命令

```bash
# 编译所有库
make -C libs

# 编译插件
make -C plugins

# 运行测试
make -C libs test

# 覆盖率
make COVERAGE=1 && make -C libs test
```

## 代码规范 (C99)

### 命名规则
```c
// 函数: mimi_<模块>_<动作>
int mimi_core_init(mimi_core_ctx_t *ctx, const mimi_core_config_t *config);

// 变量: snake_case
char *config_path;
int max_sessions;

// 常量: MIMI_<模块>_<名称>
#define MIMI_MAX_BUFFER 4096

// 类型: mimi_<模块>_t
typedef struct mimi_core_ctx mimi_core_ctx_t;
```

### 错误处理
```c
int function_example(void) {
    if (!context) return -1;
    // ... 实现
    return 0;  // 成功
}
```

### 注释风格
```c
/**
 * @brief 函数简短描述
 * 
 * @param param1 参数1说明
 * @param param2 参数2说明
 * @return 0 成功, -1 失败
 */
int function_example(int param1, const char *param2);
```

## 用户偏好 (Kk)

- **偏好语言:** 中文
- **沟通风格:** 直接、专业
- **常用命令:**
  - `make -C libs && make -C plugins` - 快速开始
  - `make -C libs test` - 运行测试

## 常见问题

**Q: 遇到编译错误怎么办?**
> 1. 检查 CMakeLists.txt 依赖
> 2. 检查头文件路径
> 3. 查看 BUILD_LOG.md

**Q: 测试失败怎么办?**
> 1. 查看测试日志
> 2. 定位失败用例
> 3. 修复后重新测试

# ============================================================================
# 本文件由 Dev-Coder 维护
# 记录代码规范、技术细节、常见问题