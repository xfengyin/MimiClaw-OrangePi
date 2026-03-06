# MimiClaw-OrangePi: OrangePi Zero3 上的 AI 助手

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build Status](https://github.com/xfengyin/MimiClaw-OrangePi/actions/workflows/build.yml/badge.svg)](https://github.com/xfengyin/MimiClaw-OrangePi/actions)
[![Release](https://img.shields.io/github/v/release/xfengyin/MimiClaw-OrangePi)](https://github.com/xfengyin/MimiClaw-OrangePi/releases)

**[English](README.md) | [中文](README_CN.md)**

<p align="center">
  <img src="assets/banner.png" alt="MimiClaw-OrangePi" width="480" />
</p>

**在 OrangePi Zero3 上运行的 AI 助手。轻量级、高效、开源。**

MimiClaw-OrangePi 将 MimiClaw AI 助手移植到 OrangePi Zero3 开发板，利用其更强大的处理能力（四核 Cortex-A53）和更大的内存（512MB/1GB/2GB），提供更流畅的 AI 对话体验。

## 认识 MimiClaw-OrangePi

- **🚀 强大** — 四核 ARM Cortex-A53，比 ESP32-S3 更强大的处理能力
- **💾 充足** — 512MB/1GB/2GB 内存，支持更大上下文和更复杂的任务
- **🔧 灵活** — 完整的 Linux 环境，易于扩展和定制
- **📡 连接** — 内置 WiFi + 以太网，稳定网络连接
- **💰 经济** — OrangePi Zero3 仅 ¥89 起，性价比极高

## 硬件要求

- **OrangePi Zero3** (512MB/1GB/2GB RAM)
- **MicroSD 卡** (16GB+, Class 10)
- **USB 电源** (5V/2A)
- **可选**: USB 转 TTL 串口模块（用于调试）

## 快速开始

### 1. 准备系统

下载并烧录 OrangePi OS (Debian) 到 MicroSD 卡：

```bash
# 使用官方镜像
wget https://drive.google.com/file/d/...
# 或使用 balenaEtcher 烧录
```

### 2. 安装依赖

```bash
# 更新系统
sudo apt update && sudo apt upgrade -y

# 安装必要依赖
sudo apt install -y git cmake build-essential libcurl4-openssl-dev \
    libjson-c-dev libsqlite3-dev libssl-dev

# 安装 Python 依赖（用于工具脚本）
pip3 install -r requirements.txt
```

### 3. 克隆并构建

```bash
git clone https://github.com/xfengyin/MimiClaw-OrangePi.git
cd MimiClaw-OrangePi

# 配置
make menuconfig  # 或使用默认配置

# 构建
make -j$(nproc)

# 安装
sudo make install
```

### 4. 配置

```bash
# 复制配置模板
sudo cp config/mimi_config.example.json /etc/mimiclaw/config.json

# 编辑配置
sudo nano /etc/mimiclaw/config.json
```

配置项：
```json
{
  "wifi": {
    "ssid": "你的WiFi名称",
    "password": "你的WiFi密码"
  },
  "telegram": {
    "bot_token": "你的Telegram Bot Token"
  },
  "anthropic": {
    "api_key": "你的Anthropic API Key",
    "model": "claude-sonnet-4-5-20250929"
  },
  "proxy": {
    "enabled": false,
    "host": "",
    "port": 0
  },
  "memory": {
    "storage_path": "/var/lib/mimiclaw/memory",
    "max_sessions": 100
  }
}
```

### 5. 启动服务

```bash
# 启动 MimiClaw 服务
sudo systemctl enable mimiclaw
sudo systemctl start mimiclaw

# 查看状态
sudo systemctl status mimiclaw

# 查看日志
sudo journalctl -u mimiclaw -f
```

## 使用方法

### Telegram 交互

1. 在 Telegram 中搜索你的 Bot
2. 发送 `/start` 开始对话
3. 直接发送消息与 AI 助手聊天

### CLI 命令

```bash
# 查看帮助
mimiclaw --help

# 配置管理
mimiclaw config show      # 显示当前配置
mimiclaw config set wifi.ssid "NewSSID"
mimiclaw config set wifi.password "NewPassword"

# 内存管理
mimiclaw memory read      # 查看 AI 的记忆
mimiclaw memory write "新的记忆内容"

# 会话管理
mimiclaw session list     # 列出所有会话
mimiclaw session clear    # 清除会话历史

# 系统控制
mimiclaw restart          # 重启服务
mimiclaw status           # 查看运行状态
```

## 功能特性

### 核心功能
- **🤖 AI 对话** — 基于 Claude API 的智能对话
- **🧠 长期记忆** — 自动保存和检索对话历史
- **🔍 网络搜索** — 集成 Brave Search，获取实时信息
- **⏰ 时间管理** — 获取当前时间和日期
- **📁 文件操作** — 读取和管理本地文件

### 高级功能
- **🌐 WebSocket 网关** — 端口 18789，支持 WebSocket 客户端连接
- **☁️ OTA 更新** — 远程固件更新，无需物理访问
- **🔒 安全代理** — HTTP CONNECT 隧道，适配受限网络
- **🛠️ 工具调用** — ReAct Agent 循环，支持自定义工具

## 团队协作

本项目采用**多分支协作模式**，支持团队成员并行开发：

### 分支策略

```
main          # 稳定分支，用于发布
├── develop   # 开发分支，日常开发
├── feature/* # 功能分支
├── bugfix/*  # 修复分支
└── release/* # 发布分支
```

### 开发工作流

1. **Fork 仓库** 或创建功能分支
2. **开发功能** 在 feature/* 分支上
3. **提交 PR** 到 develop 分支
4. **代码审查** 通过后合并
5. **自动构建** CI/CD 生成测试固件
6. **发布** 合并到 main 分支后自动发布

### 贡献指南

详见 [CONTRIBUTING.md](CONTRIBUTING.md)

## 项目结构

```
MimiClaw-OrangePi/
├── src/                    # 源代码
│   ├── core/              # 核心模块
│   ├── agent/             # AI Agent 循环
│   ├── memory/            # 记忆管理
│   ├── gateway/           # 网关服务
│   ├── cli/               # 命令行接口
│   └── tools/             # 工具集
├── config/                # 配置文件
├── scripts/               # 构建和工具脚本
├── tests/                 # 测试代码
├── docs/                  # 文档
├── .github/workflows/     # CI/CD 配置
└── Makefile              # 构建系统
```

## 自动构建

本项目使用 **GitHub Actions** 自动构建：

- **每次 Push** — 自动编译并运行测试
- **PR 合并** — 生成预发布固件
- **Tag 发布** — 自动构建并发布正式版本

构建产物：
- `mimiclaw-orangepi-v{VERSION}.deb` — Debian 安装包
- `mimiclaw-orangepi-v{VERSION}.img` — 完整系统镜像
- `mimiclaw-orangepi-v{VERSION}.tar.gz` — 源码分发包

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

## 致谢

- 原项目 [MimiClaw](https://github.com/memovai/mimiclaw) by memovai
- [OrangePi](http://www.orangepi.org/) 社区
- [Anthropic](https://www.anthropic.com/) Claude API

## 支持与联系

- 💬 **Discussions**: [GitHub Discussions](https://github.com/xfengyin/MimiClaw-OrangePi/discussions)
- 🐛 **Issues**: [GitHub Issues](https://github.com/xfengyin/MimiClaw-OrangePi/issues)
- 📧 **Email**: xfengyin@gmail.com

---

<p align="center">
  Made with ❤️ for the OrangePi community
</p>
