# MimiClaw Plugins

Phase 2 插件系统 - 将内置工具迁移为独立动态库插件。

## 目录结构

```
plugins/
├── CMakeLists.txt              # 顶层构建配置
├── README.md                   # 本文件
├── PLUGIN-DEV-GUIDE.md         # 插件开发指南
├── plugin-time/                # 时间插件
├── plugin-echo/                # 回显插件（测试用）
├── plugin-web-search/          # 网络搜索插件
├── plugin-file-ops/            # 文件操作插件
├── plugin-memory/              # 记忆操作插件
├── tests/                      # 插件测试
└── build/                      # 构建输出目录
```

## 快速开始

### 构建所有插件

```bash
cd plugins
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### 构建单个插件

```bash
cd plugins/plugin-time
mkdir -p build && cd build
cmake ..
make
```

### 安装插件

```bash
cd plugins/build
make install
```

插件将安装到 `lib/mimiclaw/plugins/` 目录。

## 插件列表

| 插件名 | 功能 | 依赖 |
|--------|------|------|
| time | 获取当前日期时间 | 无 |
| echo | 回显输入（测试用） | 无 |
| web-search | 网络搜索 | libcurl, API Key |
| file-ops | 文件读写删除 | 无 |
| memory | 记忆读写 | libmimi-memory |

## 使用插件

### 在应用中加载插件

```c
#include "mimi_tools.h"

int main() {
    // 创建注册表
    mimi_tool_registry_t *reg = mimi_registry_create();
    
    // 加载单个插件
    mimi_registry_load_plugin(reg, "./build/libmimi-plugin-time.so");
    
    // 或加载目录下所有插件
    mimi_registry_load_plugins_dir(reg, "./lib/mimiclaw/plugins");
    
    // 执行插件
    char *output = NULL;
    mimi_registry_exec(reg, "time", "now", &output);
    printf("Result: %s\n", output);
    free(output);
    
    // 清理
    mimi_registry_destroy(reg);
    return 0;
}
```

### 插件输入输出示例

#### time 插件
```bash
# 输入
"now" | "date" | "time" | "unix"

# 输出
{"timestamp":1710057600,"iso":"2026-03-10T10:00:00+08:00","human":"2026-03-10 10:00:00"}
```

#### echo 插件
```bash
# 输入
"Hello World"

# 输出
{"echo":"Hello World"}
```

#### file-ops 插件
```bash
# 输入
{"op":"read","path":"test.txt"}

# 输出
{"content":"file content...","success":true,"path":"/path/to/test.txt"}
```

## 配置

### web-search 插件

需要配置 API Key：

```bash
export BRAVE_SEARCH_API_KEY=your_api_key_here
```

或在 MimiClaw 配置文件中设置。

### memory 插件

可配置数据库路径：

```bash
export MIMI_MEMORY_DB=~/.mimiclaw/memory.db
```

## 测试

运行所有插件测试：

```bash
cd plugins/build
ctest --output-on-failure
```

## 故障排除

### 插件加载失败

1. 检查 `.so` 文件是否存在
2. 检查符号：`nm -D libmimi-plugin-xxx.so | grep get_tool_plugin`
3. 检查依赖：`ldd libmimi-plugin-xxx.so`

### 段错误

1. 使用 valgrind 检查内存问题
2. 确保插件正确初始化上下文
3. 检查输入参数是否为 NULL

## 开发新插件

参考 [PLUGIN-DEV-GUIDE.md](PLUGIN-DEV-GUIDE.md)

## 技术细节

- **标准:** C99
- **动态库:** Linux .so (ELF)
- **加载 API:** dlopen/dlsym/dlclose
- **线程安全:** 是（每个插件独立上下文）
- **内存管理:** 插件负责分配，调用者负责释放输出

## License

MIT License - MimiClaw Project
