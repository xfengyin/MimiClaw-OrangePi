# MimiClaw-OrangePi 架构设计

## 概述

MimiClaw-OrangePi 是一个为 OrangePi Zero3 优化的 AI 助手，采用模块化设计，支持高并发和可扩展性。

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                      MimiClaw-OrangePi                       │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   Gateway   │  │    Agent    │  │   Memory    │         │
│  │   Layer     │  │    Loop     │  │   Store     │         │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │
│         │                │                │                │
│  ┌──────┴────────────────┴────────────────┴──────┐         │
│  │              Core Framework                    │         │
│  │  (Config, Logging, Utils, Event Loop)         │         │
│  └────────────────────────────────────────────────┘         │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      Hardware Layer                          │
│  OrangePi Zero3 (Allwinner H618, 4x Cortex-A53, 512MB-2GB)  │
└─────────────────────────────────────────────────────────────┘
```

## 模块说明

### Core (核心层)

- **main.c**: 程序入口，初始化所有子系统
- **config.c**: 配置管理，JSON 配置文件解析
- **logger.c**: 日志系统，支持多级别日志
- **utils.c**: 工具函数集合

### Agent (AI Agent 层)

- **agent_loop.c**: 主 Agent 循环，处理消息和工具调用
- **context_builder.c**: 构建 LLM 上下文
- **llm_client.c**: Anthropic API 客户端

### Memory (记忆层)

- **memory_store.c**: 长期记忆存储
- **session_manager.c**: 会话管理
- **file_storage.c**: 文件系统存储

### Gateway (网关层)

- **telegram_bot.c**: Telegram Bot 接口
- **websocket_server.c**: WebSocket 服务
- **http_server.c**: HTTP API 服务

### CLI (命令行层)

- **command_parser.c**: 命令解析器
- **interactive_shell.c**: 交互式 Shell

### Tools (工具层)

- **tool_registry.c**: 工具注册表
- **web_search.c**: 网络搜索工具
- **time_tool.c**: 时间工具
- **file_tool.c**: 文件操作工具

## 数据流

```
User Message
     │
     ▼
┌─────────────┐
│   Gateway   │
│ (Telegram)  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Agent Loop │
│  (Process)  │
└──────┬──────┘
       │
   ┌───┴───┐
   ▼       ▼
┌──────┐ ┌──────┐
│ LLM  │ │Tools │
│ API  │ │      │
└──┬───┘ └──┬───┘
   │        │
   └────┬───┘
        ▼
┌─────────────┐
│   Memory    │
│   Store     │
└─────────────┘
```

## 线程模型

OrangePi Zero3 有四核 CPU，我们采用多线程架构：

- **Main Thread**: 主事件循环
- **Network Thread**: 网络 I/O 处理
- **AI Thread**: LLM API 调用
- **Worker Threads**: 工具执行（可配置数量）

## 配置文件

```json
{
  "system": {
    "log_level": "info",
    "worker_threads": 2
  },
  "network": {
    "telegram": { "enabled": true },
    "websocket": { "port": 18789 },
    "http": { "port": 8080 }
  }
}
```

## 内存管理

- 使用 SQLite 存储会话和记忆
- 文件系统存储长期数据
- 内存缓存热点数据

## 扩展性

- 插件系统支持自定义工具
- Webhook 支持外部集成
- RESTful API 供第三方调用
