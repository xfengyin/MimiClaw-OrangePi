# MimiClaw - 超轻量级 AI 助手 for OrangePi

<p align="center">
  <img src="assets/logo.png" alt="MimiClaw" width="256">
</p>

<p align="center">
  <strong>Go 语言</strong> · <strong>&lt;10MB 内存</strong> · <strong>&lt;1秒启动</strong> · <strong>硬件控制</strong>
</p>

---

**MimiClaw** 是一款用 **Go 语言** 编写的超轻量级 AI 助手，专为香橙派（OrangePi）和其他嵌入式 Linux 设备设计。支持机械爪控制、持久化记忆和模块化架构。

> 灵感来自 [PicoClaw](https://github.com/sipeed/picoclaw)，专为 OrangePi 硬件打造。

## 特性

| 特性 | 描述 |
|------|------|
| ⚡ **极速启动** | &lt;1秒冷启动，&lt;10MB 内存占用 |
| 🤖 **原生 AI** | 支持 OpenAI、Ollama、本地模型 |
| 🦾 **硬件控制** | I2C PWM 控制机械爪（PCA9685） |
| 💾 **持久记忆** | SQLite 存储，支持搜索 |
| 🔌 **工具系统** | 可扩展工具注册表 |
| 🌐 **多平台** | OrangePi、树莓派、x86、ARM |
| 📦 **单文件** | 无依赖，易部署 |

## 硬件需求

| 组件 | 规格 | 推荐 |
|------|------|------|
| **开发板** | 香橙派 Zero 3 (1GB+) / Zero 2W | Zero 3 4GB |
| **存储** | 16GB+ microSD 卡 | Class 10 / U3 |
| **网络** | 以太网或 WiFi | WiFi |
| **电源** | 5V/2A USB-C | 官方适配器 |

### 可选：机械爪

| 组件 | 规格 |
|------|------|
| **舵机** | SG90 或 MG90S (3个) |
| **控制器** | PCA9685 I2C PWM 驱动板 |
| **电源** | 5V/2A 外置电源 |

## 快速开始

### 下载二进制文件

```bash
# 香橙派 Zero 3 (64位)
wget https://github.com/xfengyin/MimiClaw-OrangePi/releases/latest/download/mimiclaw-linux-arm64
chmod +x mimiclaw-linux-arm64
sudo mv mimiclaw-linux-arm64 /usr/local/bin/mimiclaw

# 香橙派 Zero 2 / Zero 2W (32位)
wget https://github.com/xfengyin/MimiClaw-OrangePi/releases/latest/download/mimiclaw-linux-arm
chmod +x mimiclaw-linux-arm
sudo mv mimiclaw-linux-arm /usr/local/bin/mimiclaw
```

### 从源码编译

```bash
# 安装 Go 1.21+
sudo apt update
sudo apt install -y golang-go

# 克隆仓库
git clone https://github.com/xfengyin/MimiClaw-OrangePi.git
cd MimiClaw-OrangePi

# 编译
make build

# 运行
./mimiclaw demo
```

### 运行

```bash
# 演示模式
mimiclaw demo

# 交互模式
mimiclaw agent

# 自定义配置
mimiclaw --config /path/to/config.yaml
```

## 配置

在当前目录或 `~/.mimiclaw/config.yaml` 创建 `config.yaml`：

```yaml
agent:
  name: "MimiClaw"
  model: "gpt-4o-mini"
  max_tokens: 512
  temperature: 0.7
  system_prompt: "你是运行在香橙派上的 AI 助手 MimiClaw。"

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
  simulated: false  # 无硬件时设为 true

provider:
  type: "openai"      # 或 "ollama"
  api_key: "你的密钥"
  model: "gpt-4o-mini"
  base_url: "https://api.openai.com/v1"

logging:
  level: "info"
  file: "./logs/mimiclaw.log"
```

## 硬件控制

MimiClaw 支持通过 PCA9685 I2C PWM 控制器控制机械爪。

### 接线图

```
PCA9685          香橙派 Zero 3
--------         -------------
VCC (5V)   ──→   5V (引脚 2/4)
GND        ──→   GND (引脚 6/9/14/20)
SDA        ──→   SDA (引脚 3 - I2C0)
SCL        ──→   SCL (引脚 5 - I2C0)
```

### CLI 命令

```
/open       - 打开爪子
/close      - 关闭爪子
/grip 90    - 设置夹爪角度为 90 度
/status     - 显示硬件状态
```

## 项目结构

```
MimiClaw-OrangePi/
├── cmd/
│   └── cli/
│       └── main.go          # 程序入口
├── pkg/
│   ├── agent/               # AI 代理
│   ├── config/              # 配置管理
│   ├── hardware/            # 硬件控制 (PCA9685)
│   ├── memory/              # SQLite 记忆
│   ├── providers/           # LLM 提供商
│   └── tools/              # 工具注册表
├── config/                  # 示例配置
├── scripts/                 # 构建/部署脚本
├── hardware/                # 硬件文档
├── docs/                    # 使用文档
├── Makefile
├── go.mod
└── README.md
```

## 性能指标

| 指标 | 数值 |
|------|------|
| **内存占用** | &lt;10MB（不含 LLM） |
| **启动时间** | &lt;1秒 |
| **二进制大小** | ~8MB |
| **依赖项** | 无（静态编译） |

## 支持的开发板

- ✅ 香橙派 Zero 3 (1GB/1.5GB/2GB/4GB)
- ✅ 香橙派 Zero 2W
- ✅ 香橙派 Zero 2
- ✅ 树莓派 Zero 2 W
- ✅ 标准 x86_64 Linux

## 许可证

MIT 许可证 - 详见 [LICENSE](LICENSE)。

## 贡献

详见 [CONTRIBUTING.md](CONTRIBUTING.md)。

---

<p align="center">
  用 ❤️ 为香橙派打造
</p>
