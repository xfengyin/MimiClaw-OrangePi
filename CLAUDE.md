# MimiClaw AI Collaboration Guide

This document helps AI assistants (Claude, GPT, etc.) understand and contribute to the MimiClaw project.

## Project Overview

**MimiClaw** is an ultra-lightweight AI assistant written in Go for OrangePi and embedded Linux devices.

## Key Technologies

- **Language**: Go 1.21+
- **Database**: SQLite (github.com/mattn/go-sqlite3)
- **Hardware**: PCA9685 I2C PWM controller
- **LLM**: OpenAI API, Ollama (local models)

## Code Architecture

### Package Structure

```
pkg/
├── agent/       # AI agent logic
├── config/      # Configuration management (YAML)
├── hardware/    # PCA9685 I2C PWM control
├── memory/      # SQLite persistent memory
├── providers/   # LLM provider abstraction
└── tools/       # Tool registry system
```

### Key Patterns

1. **Agent Pattern**: All agent operations go through `pkg/agent/agent.go`
2. **Hardware Abstraction**: Hardware layer in `pkg/hardware/` supports simulation mode
3. **Provider Interface**: `LLMProvider` interface for multiple LLM backends
4. **Memory**: SQLite-based with WAL mode for performance

### Common Tasks

#### Adding a New LLM Provider

Implement the `LLMProvider` interface in `pkg/providers/`:

```go
type LLMProvider interface {
    Chat(ctx context.Context, messages []Message) (string, error)
    Name() string
}
```

#### Adding a New Tool

Register in `cmd/cli/main.go`:

```go
reg.Register(tools.Tool{
    Name:        "my_tool",
    Description: "What it does",
    Handler: func(ctx context.Context, args map[string]interface{}) (interface{}, error) {
        return "result", nil
    },
})
```

#### Adding Hardware Support

Extend `pkg/hardware/claw.go` with new hardware interfaces.

### Build Commands

```bash
make deps      # Download dependencies
make build     # Build for current platform
make test      # Run tests
make fmt       # Format code
```

### Testing

- Unit tests: `make test`
- Manual testing: `make run` (demo mode)
- Hardware testing: Connect PCA9685 and run `mimiclaw demo`

### Configuration

All configuration is in `config/config.yaml`. No hardcoded values.

## Code Style

- Follow Go conventions (run `make fmt`)
- Add comments for all exported functions
- Keep functions under 50 lines when possible
- Use interfaces for abstraction

## Important Notes

1. **Hardware First**: Always check if hardware is available before use
2. **Simulated Mode**: Hardware can run in simulation for testing
3. **Error Handling**: Return meaningful errors, never panic
4. **Concurrency**: Use sync.RWMutex for shared state
