# 共享文档索引

> 团队共享的文档和资源位置

---

## 核心文档

### 项目配置
| 文档 | 位置 | 说明 |
|------|------|------|
| README | `./README.md` | 项目首页 |
| CHANGELOG | `./CHANGELOG.md` | 版本历史 |
| LICENSE | `./LICENSE` | 许可证 |
| AGENTS.md | `./AGENTS.md` | 团队配置 |
| SOUL.md | `./SOUL.md` | 角色定义 |

### 架构文档
| 文档 | 位置 | 说明 |
|------|------|------|
| 架构设计 | `./MimiClaw-v2-Architecture.md` | 架构详解 |
| 测试计划 | `./MimiClaw-v2-Test-Plan.md` | 测试策略 |
| 开发指南 | `./docs/DEVELOPER-GUIDE.md` | 开发文档 |
| 快速入门 | `./docs/QUICKSTART.md` | 入门指南 |
| FAQ | `./docs/FAQ.md` | 常见问题 |

### API 文档
| 文档 | 位置 | 说明 |
|------|------|------|
| 核心库 | `./libs/libmimi-core/include/mimi_core.h` | API |
| 内存库 | `./libs/libmimi-memory/include/mimi_memory.h` | API |
| 配置库 | `./libs/libmimi-config/include/mimi_config.h` | API |
| 工具库 | `./libs/libmimi-tools/include/mimi_tools.h` | API |
| 网关 | `./apps/mimi-gateway/include/gateway.h` | API |

---

## Agent 配置

### Dev-Coder
| 文档 | 位置 |
|------|------|
| SOUL | `agents/dev-coder/SOUL.md` |
| IDENTITY | `agents/dev-coder/IDENTITY.md` |
| AGENTS | `agents/dev-coder/AGENTS.md` |
| USER | `agents/dev-coder/USER.md` |
| MEMORY | `agents/dev-coder/MEMORY.md` |
| HEARTBEAT | `agents/dev-coder/HEARTBEAT.md` |

### Dev-Tester
| 文档 | 位置 |
|------|------|
| SOUL | `agents/dev-tester/SOUL.md` |
| IDENTITY | `agents/dev-tester/IDENTITY.md` |
| AGENTS | `agents/dev-tester/AGENTS.md` |
| USER | `agents/dev-tester/USER.md` |
| MEMORY | `agents/dev-tester/MEMORY.md` |
| HEARTBEAT | `agents/dev-tester/HEARTBEAT.md` |

### Media-Creator
| 文档 | 位置 |
|------|------|
| SOUL | `agents/media-creator/SOUL.md` |
| IDENTITY | `agents/media-creator/IDENTITY.md` |
| AGENTS | `agents/media-creator/AGENTS.md` |
| USER | `agents/media-creator/USER.md` |
| MEMORY | `agents/media-creator/MEMORY.md` |
| HEARTBEAT | `agents/media-creator/HEARTBEAT.md` |

---

## 示例代码

| 示例 | 位置 |
|------|------|
| 核心使用 | `libs/examples/example_core.c` |
| 内存使用 | `libs/examples/example_memory.c` |
| 配置使用 | `libs/examples/example_config.c` |
| 工具使用 | `libs/examples/example_tools.c` |
| 基础聊天 | `docs/examples/example_basic_chat.c` |
| 插件开发 | `plugins/PLUGIN-DEV-GUIDE.md` |

---

## 构建产物

| 产物 | 位置 |
|------|------|
| 库文件 | `libs/build/lib/*.so` |
| 插件 | `plugins/build/*.so` |
| 测试 | `libs/build/bin/*_test` |
| 覆盖率 | `coverage-report/` |

---

## 外部资源

| 资源 | 链接 |
|------|------|
| GitHub | https://github.com/xfengyin/MimiClaw-OrangePi |
| 文档站 | https://xfengyin.github.io/MimiClaw-OrangePi/ |
| Releases | https://github.com/xfengyin/MimiClaw-OrangePi/releases |

---

## 快速查找命令

```bash
# 查找所有 .md 文件
find . -name "*.md" -type f

# 查找 API 头文件
find . -name "*.h" -path "*/include/*"

# 查找示例代码
find . -name "example_*.c"
```

---

# 本文件由 Dev-Planner 维护
# 团队共享文档索引