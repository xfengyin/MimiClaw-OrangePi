# MimiClaw - Ultra-Lightweight AI Assistant for OrangePi

<p align="center">
  <img src="assets/logo.png" alt="MimiClaw" width="256">
</p>

<p align="center">
  <strong>Go</strong> · <strong>&lt;10MB RAM</strong> · <strong>&lt;1s Boot</strong> · <strong>Hardware Control</strong>
</p>

<p align="center">
  <a href="https://github.com/xfengyin/MimiClaw-OrangePi/actions">
    <img src="https://img.shields.io/github/actions/workflow/status/xfengyin/MimiClaw-OrangePi/build.yml" alt="Build">
  </a>
  <a href="https://github.com/xfengyin/MimiClaw-OrangePi/releases">
    <img src="https://img.shields.io/github/v/release/xfengyin/MimiClaw-OrangePi" alt="Release">
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/License-MIT-green" alt="License">
  </a>
  <img src="https://img.shields.io/badge/Go-1.21+-00ADD8?logo=go" alt="Go">
</p>

---

**MimiClaw** is an ultra-lightweight AI assistant written in **Go** for OrangePi and other embedded Linux devices. It features hardware control (robotic claw), persistent memory, and modular architecture.

> Inspired by [PicoClaw](https://github.com/sipeed/picoclaw) and built for OrangePi hardware.

## Features

| Feature | Description |
|---------|-------------|
| ⚡ **Ultra-fast** | &lt;1s cold start, &lt;10MB RAM footprint |
| 🤖 **AI Native** | Supports OpenAI, Ollama, and local models |
| 🦾 **Hardware Control** | I2C PWM control for robotic claws (PCA9685) |
| 💾 **Persistent Memory** | SQLite-based memory with search |
| 🔌 **Tool System** | Extensible tool registry |
| 🌐 **Multi-platform** | OrangePi, Raspberry Pi, x86, ARM |
| 📦 **Single Binary** | No dependencies, easy deployment |

## Hardware Requirements

| Component | Specification | Recommended |
|-----------|--------------|-------------|
| **Board** | Orange Pi Zero 3 (1GB+) / Zero 2W | Zero 3 4GB |
| **Storage** | 16GB+ microSD card | Class 10 / U3 |
| **Network** | Ethernet or WiFi | WiFi |
| **Power** | 5V/2A USB-C | Official adapter |

### Optional: Robotic Claw

| Component | Specification |
|-----------|--------------|
| **Servos** | SG90 or MG90S (3x) |
| **Controller** | PCA9685 I2C PWM Board |
| **Power** | 5V/2A separate supply |

## Quick Start

### Download Binary

```bash
# For OrangePi Zero 3 (64-bit)
wget https://github.com/xfengyin/MimiClaw-OrangePi/releases/latest/download/mimiclaw-linux-arm64
chmod +x mimiclaw-linux-arm64
sudo mv mimiclaw-linux-arm64 /usr/local/bin/mimiclaw

# For OrangePi Zero 2 / Zero 2W (32-bit)
wget https://github.com/xfengyin/MimiClaw-OrangePi/releases/latest/download/mimiclaw-linux-arm
chmod +x mimiclaw-linux-arm
sudo mv mimiclaw-linux-arm /usr/local/bin/mimiclaw
```

### Build from Source

```bash
# Install Go 1.21+
sudo apt update
sudo apt install -y golang-go

# Clone repository
git clone https://github.com/xfengyin/MimiClaw-OrangePi.git
cd MimiClaw-OrangePi

# Build
make build

# Run
./mimiclaw demo
```

### Run

```bash
# Demo mode
mimiclaw demo

# Interactive mode
mimiclaw agent

# With custom config
mimiclaw --config /path/to/config.yaml
```

## Configuration

Create `config.yaml` in current directory or `~/.mimiclaw/config.yaml`:

```yaml
agent:
  name: "MimiClaw"
  model: "gpt-4o-mini"
  max_tokens: 512
  temperature: 0.7
  system_prompt: "You are MimiClaw, an AI assistant on OrangePi."

memory:
  type: "sqlite"
  path: "./data/memory.db"
  vector_dim: 384
  max_memory: 1000

hardware:
  enabled: true
  i2c_bus: 0
  i2c_address: 0x40
  pwm_frequency: 50
  simulated: false  # Set true if no hardware

provider:
  type: "openai"      # or "ollama"
  api_key: "your-key"
  model: "gpt-4o-mini"
  base_url: "https://api.openai.com/v1"

logging:
  level: "info"
  file: "./logs/mimiclaw.log"
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `OPENAI_API_KEY` | OpenAI API key |
| `OLLAMA_BASE_URL` | Ollama server URL |
| `MIMICLAW_CONFIG` | Config file path |

## Hardware Control

MimiClaw supports robotic claw control via PCA9685 I2C PWM controller.

### Wiring

```
PCA9685          OrangePi Zero 3
--------         -------------
VCC (5V)   ──→   5V (Pin 2/4)
GND        ──→   GND (Pin 6/9/14/20)
SDA        ──→   SDA (Pin 3 - I2C0)
SCL        ──→   SCL (Pin 5 - I2C0)
```

### CLI Commands

```
/open       - Open the claw
/close      - Close the claw
/grip 90    - Set gripper to 90 degrees
/status     - Show hardware status
```

## Project Structure

```
MimiClaw-OrangePi/
├── cmd/
│   └── cli/
│       └── main.go          # Entry point
├── pkg/
│   ├── agent/               # AI agent
│   ├── config/             # Configuration
│   ├── hardware/            # Hardware control (PCA9685)
│   ├── memory/              # SQLite memory
│   ├── providers/           # LLM providers
│   └── tools/              # Tool registry
├── config/                  # Example configs
├── scripts/                 # Build/deploy scripts
├── hardware/                # Hardware docs
├── docs/                    # Documentation
├── Makefile
├── go.mod
└── README.md
```

## Performance

| Metric | Value |
|--------|-------|
| **Memory Usage** | &lt;10MB (without LLM) |
| **Startup Time** | &lt;1s |
| **Binary Size** | ~8MB |
| **Dependencies** | None (static build) |

## Supported Boards

- ✅ Orange Pi Zero 3 (1GB/1.5GB/2GB/4GB)
- ✅ Orange Pi Zero 2W
- ✅ Orange Pi Zero 2
- ✅ Raspberry Pi Zero 2 W
- ✅ Standard x86_64 Linux

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

---

<p align="center">
  Built with ❤️ for OrangePi
</p>
