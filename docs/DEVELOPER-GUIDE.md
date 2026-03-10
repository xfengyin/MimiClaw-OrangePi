# MimiClaw Developer Guide

This guide covers architecture, building, plugin development, and debugging.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Build System](#build-system)
3. [Plugin Development](#plugin-development)
4. [Debugging](#debugging)
5. [Testing](#testing)
6. [Performance Tuning](#performance-tuning)

---

## Architecture Overview

### Directory Structure

```
MimiClaw/
├── libs/
│   ├── libmimi-core/      # Core AI agent loop
│   ├── libmimi-memory/    # SQLite persistence
│   ├── libmimi-config/    # Configuration management
│   ├── libmimi-tools/     # Plugin registry
│   ├── examples/          # Usage examples
│   └── tests/             # Unit tests
├── plugins/
│   ├── plugin-time/       # Time plugin
│   ├── plugin-echo/       # Echo plugin
│   ├── plugin-web-search/ # Web search plugin
│   ├── plugin-file-ops/   # File operations plugin
│   ├── plugin-memory/     # Memory plugin
│   └── tests/             # Plugin tests
├── docs/
│   ├── QUICKSTART.md
│   ├── DEVELOPER-GUIDE.md
│   └── api/               # Generated API docs
└── tests/
    ├── unit/              # Unit tests
    └── integration/       # Integration tests
```

### Module Dependencies

```
libmimi-core
    ├── libmimi-memory (optional)
    └── libmimi-tools (optional)

libmimi-memory
    └── sqlite3

libmimi-config
    └── (none)

libmimi-tools
    └── (none, plugins are dynamic)
```

### Data Flow

```
User Input
    ↓
libmimi-core (agent loop)
    ↓
libmimi-tools (plugin execution)
    ↓
Plugin (time/echo/websearch/etc.)
    ↓
Response
    ↓
libmimi-memory (persistence)
```

---

## Build System

### Makefile Targets

```bash
# Build all libraries
cd libs && make

# Build with debug symbols
cd libs && make DEBUG=1

# Build with coverage
cd libs && make COVERAGE=1

# Run tests
cd libs && make test

# Clean build
cd libs && make clean

# Install to system
cd libs && sudo make install
```

### CMake Build

```bash
cd libs
mkdir build && cd build
cmake ..
make
make install
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `DEBUG=1` | Enable debug symbols | Off |
| `COVERAGE=1` | Enable code coverage | Off |
| `PREFIX=/path` | Install prefix | `/usr/local` |
| `STATIC=1` | Build static libs | On |

---

## Plugin Development

### Plugin Structure

```
plugin-mytool/
├── CMakeLists.txt
├── src/
│   └── mytool_plugin.c
└── include/
    └── mytool_plugin.h (optional)
```

### Plugin Interface

```c
#include "mimi_tools.h"

// Plugin metadata
static mimi_plugin_meta_t meta = {
    .name = "mytool",
    .version = "1.0.0",
    .description = "My custom tool",
    .author = "Your Name"
};

// Tool context (optional)
typedef struct {
    int initialized;
    void *custom_data;
} mytool_ctx_t;

// Initialize
static int mytool_init(mimi_tool_ctx_t *ctx) {
    mytool_ctx_t *myctx = (mytool_ctx_t *)ctx;
    myctx->initialized = 1;
    return 0;
}

// Execute
static int mytool_exec(mimi_tool_ctx_t *ctx, const char *input, char **output) {
    (void)ctx;
    
    // Process input
    *output = malloc(256);
    snprintf(*output, 256, "Processed: %s", input ? input : "NULL");
    
    return 0;
}

// Destroy
static int mytool_destroy(mimi_tool_ctx_t *ctx) {
    mytool_ctx_t *myctx = (mytool_ctx_t *)ctx;
    myctx->initialized = 0;
    return 0;
}

// Plugin interface
static mimi_plugin_t plugin = {
    .meta = meta,
    .init = mytool_init,
    .exec = mytool_exec,
    .destroy = mytool_destroy
};

// Export symbol
const mimi_plugin_t* get_tool_plugin(void) {
    return &plugin;
}
```

### CMakeLists.txt Template

```cmake
cmake_minimum_required(VERSION 3.16)
project(mimi-plugin-mytool C)

set(CMAKE_C_STANDARD 11)

add_library(mimi-plugin-mytool SHARED
    src/mytool_plugin.c
)

target_include_directories(mimi-plugin-mytool PRIVATE
    ../../libs/libmimi-tools/include
)

set_target_properties(mimi-plugin-mytool PROPERTIES
    PREFIX "lib"
    SUFFIX ".so"
)
```

### Build Plugin

```bash
cd plugins/plugin-mytool
mkdir build && cd build
cmake ..
make

# Test plugin
cd ../../tests
gcc -I../../libs/libmimi-tools/include \
    test_mytool.c \
    -L../../libs/build/lib -lmimi-tools \
    -o test_mytool
./test_mytool
```

---

## Debugging

### GDB Setup

```bash
# Compile with debug symbols
make DEBUG=1

# Run with GDB
gdb ./example
(gdb) break mimi_core_chat
(gdb) run
(gdb) backtrace
```

### Valgrind Memory Check

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./example
```

### Common Memory Issues

1. **Double free**: Check response is freed only once
2. **Use after free**: Don't use context after destroy
3. **Memory leak**: Always free allocated strings

### Logging

Enable debug logging:

```c
// In your code
#define DEBUG 1
#ifdef DEBUG
    printf("[DEBUG] %s:%d - %s\n", __FILE__, __LINE__, "message");
#endif
```

---

## Testing

### Unit Tests

```bash
cd libs/tests
gcc -I../libmimi-core/include test_core.c -L../build/lib -lmimi-core -o test_core
./test_core
```

### Integration Tests

```bash
cd tests/integration
./test_chat_flow.sh
./test_plugin_chain.sh
./test_concurrent_sessions.sh
./test_config_hot_reload.sh
```

### Coverage Report

```bash
# Build with coverage
make COVERAGE=1

# Run tests
make test

# Generate report
gcov -r
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage-report

# Open in browser
open coverage-report/index.html
```

---

## Performance Tuning

### Memory Pool Sizing

```json
{
  "memory": {
    "pool_size": 8,      // Increase for high concurrency
    "max_idle_time": 300 // Reduce to free connections faster
  }
}
```

### Batch Operations

```c
// Instead of multiple writes
for (int i = 0; i < 100; i++) {
    mimi_mem_write(pool, key[i], value[i]);  // Slow
}

// Use transactions (if supported)
mimi_mem_begin_transaction(pool);
for (int i = 0; i < 100; i++) {
    mimi_mem_write(pool, key[i], value[i]);
}
mimi_mem_commit_transaction(pool);  // Fast
```

### Caching

```c
// Cache frequently accessed data
static char *cached_config = NULL;

if (!cached_config) {
    cached_config = load_config();
}
use_config(cached_config);
```

### Profiling

```bash
# Profile with perf
perf record -g ./example
perf report

# Profile with gprof
gcc -pg example.c -o example
./example
gprof example gmon.out
```

---

## Best Practices

1. **Always check return values**
2. **Free all allocated memory**
3. **Use connection pooling for database**
4. **Validate user input**
5. **Handle errors gracefully**
6. **Log important events**
7. **Write tests for new features**
8. **Document public APIs**

---

## Contributing

1. Fork the repository
2. Create a feature branch
3. Write tests
4. Submit a pull request

For more details, see [CONTRIBUTING.md](../CONTRIBUTING.md).
