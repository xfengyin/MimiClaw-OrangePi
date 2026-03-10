# MimiClaw v2.0 - API Documentation

## Overview

MimiClaw is a lightweight AI agent framework designed for embedded systems and edge devices. It provides a modular architecture with pluggable tools, efficient memory management, and hot-reloadable configuration.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      MimiClaw Core                          │
├─────────────────┬─────────────────┬─────────────────────────┤
│  libmimi-core   │ libmimi-memory  │   libmimi-config        │
│  - Agent Loop   │  - SQLite Pool  │   - JSON Parser         │
│  - Context Mgmt │  - Session Mgmt │   - Hot Reload          │
│  - Chat API     │  - KV Store     │   - Validation          │
├─────────────────┴─────────────────┴─────────────────────────┤
│                    libmimi-tools                            │
│  - Plugin Registry  - Dynamic Loading  - Tool Execution     │
├─────────────────────────────────────────────────────────────┤
│                      Plugins                                │
│  time  |  echo  |  websearch  |  fileops  |  memory         │
└─────────────────────────────────────────────────────────────┘
```

## Modules

### libmimi-core
Core AI agent functionality including:
- **Context Management**: Session-based conversation context
- **Chat API**: Unified interface for AI model interaction
- **Session Management**: Create, delete, and list sessions

### libmimi-memory
SQLite-based persistence layer:
- **Connection Pooling**: Efficient database connection reuse
- **Session Storage**: Persistent conversation history
- **Key-Value Store**: General-purpose memory storage

### libmimi-config
Configuration management:
- **JSON Parsing**: Load configuration from files or strings
- **Hot Reload**: Automatic configuration updates without restart
- **Validation**: Ensure required configuration keys exist

### libmimi-tools
Plugin system for extensibility:
- **Tool Registry**: Central registration of all tools
- **Plugin Loading**: Dynamic loading of .so plugins
- **Tool Execution**: Unified execution interface

## Quick Start

```c
#include "mimi_core.h"
#include "mimi_memory.h"
#include "mimi_config.h"
#include "mimi_tools.h"

int main() {
    // Initialize core
    mimi_core_config_t core_config = {
        .api_key = "your-api-key",
        .model = "claude-3-5-sonnet",
        .max_tokens = 1000,
        .temperature = 0.7f,
        .timeout_ms = 30000
    };
    
    mimi_core_ctx_t *core;
    mimi_core_init(&core, &core_config);
    
    // Initialize memory
    mimi_mem_config_t mem_config = {
        .db_path = "./mimi.db",
        .pool_size = 4,
        .enable_wal = 1
    };
    
    mimi_mem_pool_t *memory = mimi_mem_pool_create(&mem_config);
    
    // Initialize tools
    mimi_tool_registry_t *tools = mimi_registry_create();
    mimi_tools_register_time(tools);
    mimi_tools_register_echo(tools);
    
    // Chat
    mimi_core_response_t response;
    mimi_core_chat(core, "session-1", "Hello!", &response);
    printf("AI: %s\n", response.content);
    
    // Cleanup
    mimi_core_response_free(&response);
    mimi_core_destroy(core);
    mimi_mem_pool_destroy(memory);
    mimi_registry_destroy(tools);
    
    return 0;
}
```

## Version History

### v2.0.0 (2026-03-10)
- Complete modular architecture
- SQLite connection pooling
- Hot-reloadable configuration
- Plugin system with 5 built-in plugins
- Comprehensive test suite

### v1.0.0 (2026-02-01)
- Initial release
- Basic core functionality
- Simple memory storage

## License

Copyright (c) 2026 MimiClaw Project  
Licensed under MIT License
