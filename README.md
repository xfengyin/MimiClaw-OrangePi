# MimiClaw-OrangePi v2.0

[![Build & Test](https://github.com/xfengyin/MimiClaw-OrangePi/actions/workflows/build.yml/badge.svg)](https://github.com/xfengyin/MimiClaw-OrangePi/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/xfengyin/MimiClaw-OrangePi)](https://github.com/xfengyin/MimiClaw-OrangePi/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**在 OrangePi Zero3 上运行的 AI 助手 - v2.0 重构版**

## 🚀 快速开始

### 安装依赖
```bash
sudo apt update
sudo apt install -y cmake build-essential \
    libsqlite3-dev libcurl4-openssl-dev libssl-dev
```

### 构建
```bash
make -C libs
make -C plugins
```

### 运行测试
```bash
make -C libs test
```

## 📚 文档

### 快速开始
- [快速入门](docs/QUICKSTART.md) - 5 分钟上手
- [常见问题 FAQ](docs/FAQ.md) - 问题解答
- [更新日志](CHANGELOG.md) - 版本历史

### 开发资源
- [API 文档](https://xfengyin.github.io/MimiClaw-OrangePi/) - 完整 API 参考
- [开发指南](docs/DEVELOPER-GUIDE.md) - 进阶用法
- [架构文档](MimiClaw-v2-Architecture.md) - 系统设计

### 测试与质量
- [测试计划](MimiClaw-v2-Test-Plan.md) - 测试策略
- [测试清单](TEST-CHECKLIST.md) - 用例列表
- [贡献指南](CONTRIBUTING.md) - 参与开发

## 🏗️ 架构

```
libs/
├── libmimi-core      # AI Agent 核心
├── libmimi-memory    # SQLite 持久化
├── libmimi-config    # 配置管理
└── libmimi-tools     # 工具注册表

plugins/
├── plugin-time       # 时间查询
├── plugin-echo       # 测试插件
├── plugin-web-search # 网络搜索
├── plugin-file-ops   # 文件操作
└── plugin-memory     # 记忆操作
```

## 📊 性能

| 指标 | v1.0 | v2.0 | 改善 |
|------|------|------|------|
| 内存占用 | 80MB | 1MB | **-98.75%** |
| 启动时间 | 3s | <10ms | **-99.67%** |
| 响应延迟 | 500ms | 0.02ms | **-99.996%** |
| 测试覆盖 | 30% | 80%+ | **+166%** |

> 📈 详细性能分析见 [架构文档](MimiClaw-v2-Architecture.md#性能优化)

## 🧪 测试

```bash
# 单元测试
make -C libs test

# 集成测试
make -C tests/integration

# 覆盖率报告
make COVERAGE=1 && make test
```

## 🤝 贡献

详见 [CONTRIBUTING.md](CONTRIBUTING.md)

## 📄 许可证

MIT License - 详见 [LICENSE](LICENSE)
