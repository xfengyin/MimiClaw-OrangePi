# 🧠 MimiClaw-OrangePi

### AI Agent × Edge Computing

MimiClaw AI Assistant adapted to **OrangePi** — exploring the boundary between cloud LLMs, local devices, and real-world hardware.

> **From cloud to edge: an AI agent that actually touches the real world.**

## 🎯 Vision

把 LLM Agent 从“聊天窗口”延伸到真实硬件：运行在 OrangePi 上的 AI 助手，通过工具调用（Tools / MCP）控制本地设备、读取传感器、执行任务，云端模型负责推理，边缘节点负责执行。

```text
                  Cloud LLM
                     ▲
                     │  HTTP / SSE
                     │
              ┌──────┴──────┐
              │   Agent     │
              │  (Go/Rust)  │
              └──────┬──────┘
                     │  Tools / MCP
              ┌──────▼──────┐
              │  OrangePi   │
              │ GPIO / USB  │
              │ Sensor      │
              └─────────────┘
```

## 📦 Planned Components

| Layer | Stack | Role |
|-------|-------|------|
| LLM | OpenAI-compatible API / Gemini | Reasoning & planning |
| Agent Runtime | Go | Loop, memory, tool dispatch |
| Tool System | MCP | File / shell / GPIO / sensor tools |
| Edge Runtime | Linux / OrangePi Zero3 | Tool execution on device |
| Frontend | Web / TUI | Local console & remote control |

## 🚧 Status

**Scaffold** — the project is being assembled. Code will land as the architecture stabilizes.

## 📄 License

MIT
