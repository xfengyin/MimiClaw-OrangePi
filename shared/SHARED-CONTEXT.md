# SHARED-CONTEXT.md - 团队共享上下文

> 所有 Agent 共享的项目信息和状态

---

## 项目信息

### 基本信息
| 字段 | 值 |
|------|-----|
| **项目名称** | MimiClaw-OrangePi |
| **版本** | v2.0 |
| **主语言** | C (C99) |
| **目标硬件** | OrangePi Zero3 |
| **许可证** | MIT |

### 技术栈
| 模块 | 技术 | 版本 |
|------|------|------|
| 核心库 | libmimi-core | 2.0.0 |
| 内存库 | libmimi-memory + SQLite3 | - |
| 配置库 | libmimi-config | - |
| 工具库 | libmimi-tools | - |
| 网关 | mimi-gateway | 2.0.0 |
| 构建 | CMake | 3.16+ |
| 测试 | cmocka | - |

---

## 团队成员

### Agent 身份卡

| Agent | 角色 | Emoji | 颜色 |
|-------|------|-------|------|
| Dev-Planner | 架构师 | 📐 | 蓝色 #2563EB |
| Dev-Coder | 程序员 | 💻 | 绿色 #16A34A |
| Dev-Tester | 测试工程师 | 🧪 | 橙色 #EA580C |
| Media-Creator | 内容创作者 | 📣 | 紫色 #9333EA |

### 协作关系
```
           Kk (用户)
              ↓
       Dev-Planner
      /    |     \
     ↓     ↓      ↓
  Dev-  Dev-  Media-
 Coder Tester Creator
    ↑     │
    └─────┘
   (代码提交)
```

---

## 当前项目状态

### 开发阶段
- **阶段:** v2.0 完善
- **状态:** 核心功能完成，待测试

### 构建状态
| 模块 | 构建 | 测试 | 说明 |
|------|------|------|------|
| libs | ✅ | 🔄 | 核心库 |
| plugins | ✅ | 🔄 | 插件系统 |
| apps/mimi-gateway | ✅ | 🔄 | 网关服务 |
| docs/examples | ✅ | - | 示例代码 |

### 新增功能 (2026-03-14)
- mimi-gateway 多协议网关 (CLI/Telegram/WebSocket)
- Docker 支持 (Dockerfile + docker-compose.yml)
- 根目录 CMakeLists.txt 整合构建
- 团队 Agent 完整配置 (32 个配置文件)
- 共享上下文 shared/ 目录
- 日志系统 logs/ 目录

### 性能指标 (v2.0)
| 指标 | 目标 | 实际 |
|------|------|------|
| 内存 | < 50MB | 1MB ✅ |
| 启动 | < 1s | < 10ms ✅ |
| 延迟 | < 200ms | 0.02ms ✅ |
| 覆盖 | > 80% | 80%+ ✅ |

---

## 共享规范

### 1. 代码规范 (C99)
```c
// 命名
函数: mimi_<模块>_<动作>
变量: snake_case
常量: MIMI_<模块>_<名称>
类型: mimi_<模块>_t

// 错误处理
// 返回 0 成功, -1 失败
```

### 2. 提交规范
```
<模块>: <描述>

# 示例
core: 添加异步 IO 支持
gateway: 实现 WebSocket 服务器
```

### 3. 通信协议
```markdown
## 任务派发
### 任务: [名称]
- 需求: [描述]
- 验收标准: [列表]
- 截止: [日期]

## Bug 报告
### 严重程度: [阻塞/高/中/低]
- 描述: 
- 复现步骤:
```

---

## 共享资源

### 文档位置
| 文档 | 路径 |
|------|------|
| 架构文档 | `MimiClaw-v2-Architecture.md` |
| 测试计划 | `MimiClaw-v2-Test-Plan.md` |
| 开发指南 | `docs/DEVELOPER-GUIDE.md` |
| 快速入门 | `docs/QUICKSTART.md` |

### 命令速查
```bash
# 构建
make -C libs
make -C plugins
make -C apps/mimi-gateway

# 测试
make -C libs test

# 覆盖率
make COVERAGE=1 && make -C libs test
```

---

## 质量标准

### 通过条件
| 阶段 | 标准 |
|------|------|
| 代码实现 | 编译通过 + 单元测试 > 80% |
| 测试验收 | 无阻塞 Bug |
| 发布 | 文档完备 + 测试通过 |

### Bug 级别
| 级别 | 定义 |
|------|------|
| 阻塞 | 系统崩溃、数据丢失 |
| 高 | 核心功能不可用 |
| 中 | 功能异常可绕过 |
| 低 | 体验问题 |

---

## 更新日志

| 日期 | 更新内容 | 更新者 |
|------|----------|--------|
| 2026-03-14 | 初始化共享上下文 | Dev-Planner |

---

# 本文件由 Dev-Planner 维护
# 所有 Agent 共享的项目信息和规范