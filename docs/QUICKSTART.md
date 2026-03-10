# MimiClaw Quick Start Guide

Welcome to MimiClaw! This guide will help you get up and running in minutes.

## Table of Contents

1. [Installation](#installation)
2. [Configuration](#configuration)
3. [Your First AI Conversation](#your-first-ai-conversation)
4. [Using Plugins](#using-plugins)
5. [Common Issues](#common-issues)

---

## Installation

### Prerequisites

- GCC or Clang compiler
- CMake 3.16+
- SQLite3 development libraries
- Linux/macOS (Windows via WSL)

### Build from Source

```bash
# Clone the repository
cd MimiClaw

# Build libraries
cd libs
make clean && make

# Build plugins
cd ../plugins
make clean && make

# Verify build
ls build/lib/
# Should show: libmimi-core.a libmimi-memory.a libmimi-config.a libmimi-tools.a
```

### Verify Installation

```bash
# Run basic tests
cd libs/tests
gcc -I../libmimi-core/include test_core.c -L../build/lib -lmimi-core -o test_core
./test_core
```

---

## Configuration

### Create Configuration File

Create `config.json` in your project directory:

```json
{
  "core": {
    "api_key": "your-api-key-here",
    "model": "claude-3-5-sonnet",
    "max_tokens": 1000,
    "temperature": 0.7,
    "timeout_ms": 30000
  },
  "memory": {
    "db_path": "./mimi.db",
    "pool_size": 4,
    "enable_wal": true
  },
  "plugins": {
    "directory": "./plugins/build",
    "auto_load": true
  }
}
```

### Environment Variables

Alternatively, set via environment:

```bash
export MIMI_API_KEY="your-api-key-here"
export MIMI_MODEL="claude-3-5-sonnet"
export MIMI_DB_PATH="./mimi.db"
```

---

## Your First AI Conversation

### Basic Example

```c
#include <stdio.h>
#include "mimi_core.h"

int main() {
    // Configure
    mimi_core_config_t config = {
        .api_key = "your-api-key",
        .model = "claude-3-5-sonnet",
        .max_tokens = 1000,
        .temperature = 0.7f,
        .timeout_ms = 30000
    };
    
    // Initialize
    mimi_core_ctx_t *ctx;
    if (mimi_core_init(&ctx, &config) != MIMI_CORE_OK) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }
    
    // Chat
    mimi_core_response_t response;
    if (mimi_core_chat(ctx, "session-1", "Hello! What can you do?", &response) == MIMI_CORE_OK) {
        printf("AI: %s\n", response.content);
        mimi_core_response_free(&response);
    }
    
    // Cleanup
    mimi_core_destroy(ctx);
    return 0;
}
```

### Compile and Run

```bash
gcc -I./libs/libmimi-core/include \
    -I./libs/libmimi-memory/include \
    -I./libs/libmimi-config/include \
    -I./libs/libmimi-tools/include \
    example.c \
    -L./libs/build/lib \
    -lmimi-core -lmimi-memory -lmimi-config -lmimi-tools \
    -lsqlite3 -lpthread \
    -o example

./example
```

### Expected Output

```
AI: Hello! I'm an AI assistant powered by MimiClaw. I can help you with:
- Answering questions
- Writing code
- Analyzing documents
- And much more!

What would you like to do today?
```

---

## Using Plugins

### Load Built-in Plugins

```c
#include "mimi_tools.h"

// Create registry
mimi_tool_registry_t *tools = mimi_registry_create();

// Register built-in tools
mimi_tools_register_time(tools);
mimi_tools_register_echo(tools);

// Load plugin from .so file
mimi_registry_load_plugin(tools, "./plugins/build/libmimi-plugin-web-search.so");

// Execute a tool
char *output;
mimi_registry_exec(tools, "time", "iso", &output);
printf("Current time: %s\n", output);
free(output);

// Cleanup
mimi_registry_destroy(tools);
```

### Available Plugins

| Plugin | Description | Input Example |
|--------|-------------|---------------|
| `time` | Get current time/date | `"iso"`, `"timestamp"`, `"date"` |
| `echo` | Echo input (testing) | Any string |
| `websearch` | Search the web | Search query |
| `fileops` | File operations | `"read|/path/to/file"` |
| `memory` | Memory operations | `"store|key|value"` |

---

## Common Issues

### Issue: "Failed to initialize"

**Cause**: Invalid API key or network issue

**Solution**:
```bash
# Check API key
echo $MIMI_API_KEY

# Test connectivity
curl -I https://api.anthropic.com
```

### Issue: "Database locked"

**Cause**: Multiple processes accessing the same database

**Solution**:
```json
{
  "memory": {
    "pool_size": 1,
    "enable_wal": true
  }
}
```

### Issue: "Plugin not found"

**Cause**: Plugin .so file not in expected location

**Solution**:
```bash
# Check plugin exists
ls plugins/build/*.so

# Use absolute path
mimi_registry_load_plugin(reg, "/full/path/to/plugin.so");
```

### Issue: "Segmentation fault"

**Cause**: Using freed memory or NULL pointers

**Solution**:
- Always check return values
- Free responses after use
- Don't use context after `mimi_core_destroy()`

---

## Next Steps

- Read the [Developer Guide](DEVELOPER-GUIDE.md) for advanced usage
- Check [API Reference](API-REFERENCE.md) for detailed function documentation
- Explore [examples](examples/) for more code samples

---

**Need Help?**  
Check the [GitHub Issues](https://github.com/mimiclaw/mimiclaw/issues) or join our community!
