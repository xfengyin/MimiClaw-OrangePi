# MEMORY.md - Dev-Planner 记忆库

# 项目核心信息

## 项目概述
- **名称:** MimiClaw-OrangePi
- **类型:** AI 助手 (C 语言)
- **目标硬件:** OrangePi Zero3
- **版本:** v2.0

## 架构技术栈

| 模块 | 技术 | 说明 |
|------|------|------|
| libmimi-core | C | AI Agent 核心 (ReAct 模式) |
| libmimi-memory | C + SQLite | 记忆持久化 |
| libmimi-config | C | 配置管理 |
| libmimi-tools | C | 工具插件系统 |
| mimi-gateway | C | 多协议网关 |

## 构建系统
- **构建工具:** CMake + Make
- **依赖:** SQLite3, libcurl, OpenSSL
- **测试框架:** cmocka

## 用户信息
- **Name:** Kk
- **Timezone:** GMT+8 (深圳)
- **偏好:** Coding & 自媒体创作

## 开发规范

### 代码风格
- C99 标准
- 命名: snake_case
- 错误处理: 返回 0 成功, -1 失败

### 提交规范
```
# 提交信息格式
<模块>: <描述>

# 示例
core: 添加异步 IO 支持
memory: 优化连接池
```

# 重要里程碑
- v2.0 重构 - 内存优化 98.75%, 启动提升 99.67%
- 架构文档完成
- 网关服务实现 (CLI + Telegram + WebSocket)

# 待处理任务
# (在此记录团队待办事项)

# ============================================================================
# 团队 Agent 配置
# ============================================================================

## Dev-Coder
- 配置文件: agents/dev-coder/
- 代码规范: C99, 覆盖率 > 80%

## Dev-Tester  
- 配置文件: agents/dev-tester/
- 测试标准: 覆盖率 > 80%, 无阻塞 Bug

## Media-Creator
- 配置文件: agents/media-creator/
- 内容渠道: GitHub, 博客, 社交媒体

# ============================================================================

# 本文件由 Dev-Planner 维护
# 记录项目重要信息和团队协作数据