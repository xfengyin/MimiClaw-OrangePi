# MimiClaw 插件开发指南

## 概述

本指南介绍如何为 MimiClaw 开发动态库插件。所有插件都编译为独立的 `.so` 文件，通过 `dlopen/dlsym/dlclose` 动态加载。

## 插件目录结构

标准插件目录结构：

```
plugin-xxx/
├── CMakeLists.txt
├── README.md
├── src/
│   └── xxx_plugin.c
└── include/
    └── xxx_plugin.h
```

### 文件说明

| 文件 | 说明 |
|------|------|
| `CMakeLists.txt` | CMake 构建配置 |
| `README.md` | 插件使用说明 |
| `src/xxx_plugin.c` | 插件实现 |
| `include/xxx_plugin.h` | 插件头文件（可选） |

## 插件接口规范

### 必须实现的接口

所有插件必须导出以下符号：

```c
const mimi_plugin_t* get_tool_plugin(void);
```

### 插件结构定义

```c
#include "mimi_tools.h"

// 插件元数据
typedef struct {
    const char *name;        // 插件名称（唯一标识）
    const char *version;     // 版本号（如 "1.0.0"）
    const char *description; // 描述
    const char *author;      // 作者
} mimi_plugin_meta_t;

// 插件接口
typedef struct {
    mimi_plugin_meta_t meta;
    int (*init)(mimi_tool_ctx_t *ctx);
    int (*exec)(mimi_tool_ctx_t *ctx, const char *input, char **output);
    int (*destroy)(mimi_tool_ctx_t *ctx);
} mimi_plugin_t;
```

### 生命周期函数

#### init - 初始化

```c
static int my_plugin_init(mimi_tool_ctx_t *ctx) {
    // 分配上下文
    my_ctx_t *my_ctx = calloc(1, sizeof(my_ctx_t));
    if (my_ctx == NULL) return -1;
    
    // 初始化资源（数据库连接、文件句柄等）
    // ...
    
    *(my_ctx_t**)ctx = my_ctx;
    return 0;  // 成功返回 0
}
```

#### exec - 执行

```c
static int my_plugin_exec(mimi_tool_ctx_t *ctx, const char *input, char **output) {
    // 1. 验证输入
    if (input == NULL || output == NULL) return -1;
    
    // 2. 处理输入（通常是 JSON）
    // ...
    
    // 3. 分配输出字符串（调用者负责释放）
    *output = strdup("{\"result\":\"success\"}");
    
    return (*output != NULL) ? 0 : -1;
}
```

#### destroy - 清理

```c
static int my_plugin_destroy(mimi_tool_ctx_t *ctx) {
    my_ctx_t *my_ctx = (my_ctx_t*)ctx;
    
    // 释放资源
    // ...
    
    free(my_ctx);
    return 0;
}
```

### 导出符号

```c
static const mimi_plugin_t my_plugin = {
    .meta = {
        .name = "my-plugin",
        .version = "1.0.0",
        .description = "My awesome plugin",
        .author = "Your Name"
    },
    .init = my_plugin_init,
    .exec = my_plugin_exec,
    .destroy = my_plugin_destroy
};

// 必须导出此符号
const mimi_plugin_t* get_tool_plugin(void) {
    return &my_plugin;
}
```

## CMakeLists.txt 模板

```cmake
cmake_minimum_required(VERSION 3.16)
project(plugin-xxx VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-tools/include
)

# 源文件
set(PLUGIN_SOURCES src/xxx_plugin.c)

# 创建共享库
add_library(plugin-xxx SHARED ${PLUGIN_SOURCES})

# 设置输出文件名
set_target_properties(plugin-xxx PROPERTIES
    OUTPUT_NAME "mimi-plugin-xxx"
    PREFIX "lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 链接依赖库（如有）
# target_link_libraries(plugin-xxx some_library)

# 安装规则
install(TARGETS plugin-xxx
    LIBRARY DESTINATION lib/mimiclaw/plugins
)
```

## 构建和测试流程

### 1. 创建插件目录

```bash
mkdir -p plugin-myplugin/{src,include}
cd plugin-myplugin
```

### 2. 编写代码

创建 `src/myplugin_plugin.c` 和 `CMakeLists.txt`。

### 3. 构建

```bash
mkdir -p build && cd build
cmake ..
make
```

### 4. 验证符号

```bash
# 检查导出符号
nm -D libmimi-plugin-myplugin.so | grep get_tool_plugin

# 检查依赖
ldd libmimi-plugin-myplugin.so
```

### 5. 测试加载

```c
#include <dlfcn.h>
#include <stdio.h>

int main() {
    void *handle = dlopen("./libmimi-plugin-myplugin.so", RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }
    
    typedef const mimi_plugin_t* (*get_plugin_fn)(void);
    get_plugin_fn get_plugin = dlsym(handle, "get_tool_plugin");
    
    if (!get_plugin) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    
    const mimi_plugin_t *plugin = get_plugin();
    printf("Plugin: %s v%s\n", plugin->meta.name, plugin->meta.version);
    
    dlclose(handle);
    return 0;
}
```

## 调试技巧

### 1. 使用 valgrind 检查内存泄漏

```bash
valgrind --leak-check=full --show-leak-kinds=all \
    ./test_loader ./libmimi-plugin-xxx.so
```

### 2. 启用调试输出

在插件中添加：

```c
#include <stdio.h>

#define DEBUG_PRINTF(fmt, ...) \
    fprintf(stderr, "[my-plugin] " fmt "\n", ##__VA_ARGS__)
```

### 3. 使用 GDB 调试

```bash
gdb --args ./test_loader ./libmimi-plugin-xxx.so
(gdb) break get_tool_plugin
(gdb) run
```

### 4. 检查段错误

```bash
# 启用 core dump
ulimit -c unlimited

# 分析 core 文件
gdb ./test_loader core
(gdb) bt
```

## 最佳实践

### 1. 内存管理

- **规则:** 谁分配，谁释放
- **输出字符串:** 插件分配，调用者释放
- **上下文:** init 分配，destroy 释放

```c
// ✓ 正确
*output = strdup("result");  // 调用者 free()

// ✗ 错误
static char buf[256];
*output = buf;  // 静态缓冲区，不安全
```

### 2. 错误处理

```c
static int my_exec(mimi_tool_ctx_t *ctx, const char *input, char **output) {
    if (input == NULL) {
        *output = strdup("{\"error\":\"null input\"}");
        return 0;  // 返回 JSON 错误，而不是错误码
    }
    
    // 处理失败也返回 JSON 错误
    if (some_operation_failed) {
        *output = strdup("{\"error\":\"operation failed\"}");
        return 0;
    }
    
    return 0;
}
```

### 3. 输入验证

```c
// 验证 JSON 输入
if (input == NULL || strlen(input) == 0) {
    *output = strdup("{\"error\":\"empty input\"}");
    return 0;
}

// 验证必需字段
char *field = json_get_string(input, "required_field");
if (field == NULL) {
    *output = strdup("{\"error\":\"missing required_field\"}");
    return 0;
}
```

### 4. 线程安全

- 每个插件调用有独立上下文
- 避免全局状态
- 如需共享资源，使用互斥锁

```c
typedef struct {
    pthread_mutex_t lock;
    // 共享数据...
} my_ctx_t;

static int my_exec(mimi_tool_ctx_t *ctx, ...) {
    my_ctx_t *mctx = (my_ctx_t*)ctx;
    pthread_mutex_lock(&mctx->lock);
    // 临界区...
    pthread_mutex_unlock(&mctx->lock);
    return 0;
}
```

### 5. 配置管理

从环境变量或配置文件读取敏感信息：

```c
const char *api_key = getenv("MY_PLUGIN_API_KEY");
if (api_key == NULL) {
    api_key = config_get("my_plugin.api_key");
}
```

### 6. 日志记录

使用标准错误输出：

```c
fprintf(stderr, "[my-plugin] Loaded configuration from %s\n", config_path);
```

## 输入输出格式规范

### JSON 输入

```json
{
  "op": "operation_name",
  "key": "value",
  ...
}
```

### JSON 输出

成功：
```json
{
  "success": true,
  "result": "...",
  ...
}
```

失败：
```json
{
  "success": false,
  "error": "error message"
}
```

## 依赖管理

### 系统库

在 CMakeLists.txt 中声明：

```cmake
find_package(CURL REQUIRED)
target_link_libraries(plugin-xxx ${CURL_LIBRARIES})
```

### MimiClaw 库

```cmake
target_link_libraries(plugin-xxx 
    ${CMAKE_SOURCE_DIR}/libs/build/libmimi-memory.a
)
```

## 发布插件

1. 更新 `README.md` 使用说明
2. 确保所有测试通过
3. 检查内存泄漏
4. 更新版本号
5. 提交到插件目录

## 常见问题

### Q: 插件加载失败 "undefined symbol"

**A:** 确保导出 `get_tool_plugin` 符号：

```bash
nm -D libmimi-plugin-xxx.so | grep get_tool_plugin
```

### Q: 段错误在 destroy 时发生

**A:** 检查上下文是否正确分配和转换：

```c
// init 中
my_ctx_t *ctx = calloc(1, sizeof(my_ctx_t));
*(my_ctx_t**)out_ctx = ctx;

// destroy 中
my_ctx_t *ctx = (my_ctx_t*)in_ctx;
```

### Q: 内存泄漏

**A:** 使用 valgrind 检查：

```bash
valgrind --leak-check=full ./test_program
```

确保：
- 所有 `malloc/strdup` 都有对应的 `free`
- 输出字符串由调用者释放
- 上下文在 destroy 中释放

## 示例代码

完整示例参考：
- `plugin-time/` - 简单插件示例
- `plugin-echo/` - 测试插件示例
- `plugin-file-ops/` - 中等复杂度示例
- `plugin-web-search/` - 带外部依赖示例

---

_Happy Plugin Development! 🚀_
