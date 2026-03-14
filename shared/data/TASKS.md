# 共享数据 - 任务队列

> 所有 Agent 共享的任务列表和状态

---

## 活跃任务

### 待处理任务

| ID | 任务 | 分配给 | 优先级 | 状态 |
|----|------|--------|--------|------|
| T001 | Telegram Bot 连接测试 | Dev-Coder | 中 | 待开始 |
| T002 | WebSocket 压力测试 | Dev-Tester | 中 | 待开始 |

### 进行中任务

| ID | 任务 | 分配给 | 进度 | 状态 |
|----|------|--------|------|------|
| - | 无 | - | - | - |

### 已完成任务

| ID | 任务 | 完成者 | 日期 |
|----|------|--------|------|
| T004 | 单元测试覆盖补充 | Dev-Planner | 2026-03-14 |
| T005 | 根目录 CMakeLists.txt | Dev-Planner | 2026-03-14 |
| T006 | apps/CMakeLists.txt | Dev-Planner | 2026-03-14 |
| T007 | pkg-config 文件 | Dev-Planner | 2026-03-14 |
| T008 | README 更新 (Docker/网关) | Dev-Planner | 2026-03-14 |
| T009 | mimi-gateway 框架实现 | Dev-Planner | 2026-03-14 |
| T010 | 团队 Agent 配置 | Dev-Planner | 2026-03-14 |
| T011 | 共享上下文 shared/ | Dev-Planner | 2026-03-14 |
| T012 | 日志系统 logs/ | Dev-Planner | 2026-03-14 |
| T013 | .gitignore 添加 | Dev-Planner | 2026-03-14 |
| T014 | Gateway 单元测试 | Dev-Planner | 2026-03-14 |
| T015 | Gateway 性能测试 | Dev-Planner | 2026-03-14 |
| T016 | 部署脚本 deploy.sh | Dev-Planner | 2026-03-14 |
| T017 | 备份脚本 backup.sh | Dev-Planner | 2026-03-14 |

---

## 任务模板

### 新任务格式
```markdown
### TXXX: [任务名称]
- **描述:** [详细描述]
- **分配给:** [Agent]
- **优先级:** [高/中/低]
- **验收标准:**
  - [标准 1]
  - [标准 2]
```

---

## 优先级规则

| 优先级 | 定义 | 处理时限 |
|--------|------|----------|
| **高** | 阻塞发布 | 24h |
| **中** | 重要功能 | 72h |
| **低** | 优化改进 | 待定 |

---

# 本文件由 Dev-Planner 维护