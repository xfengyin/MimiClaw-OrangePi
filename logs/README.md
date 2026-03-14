# 团队日志索引

> 所有 Agent 的每日工作日志汇总

## 日志目录结构

```
logs/
├── DAILY.md              # Dev-Planner 日志
├── dev-coder/
│   └── DAILY.md         # Dev-Coder 日志
├── dev-tester/
│   └── DAILY.md         # Dev-Tester 日志
└── media-creator/
    └── DAILY.md         # Media-Creator 日志
```

## 日志对应 Agent

| Agent | 日志文件 | 更新频率 | 记录内容 |
|-------|----------|----------|----------|
| Dev-Planner | `DAILY.md` | 每日 | 规划决策、团队状态、任务分配 |
| Dev-Coder | `dev-coder/DAILY.md` | 任务完成 | 代码开发、问题记录 |
| Dev-Tester | `dev-tester/DAILY.md` | 测试执行 | 测试结果、Bug 追踪 |
| Media-Creator | `media-creator/DAILY.md` | 内容发布 | 内容发布、互动数据 |

## 今日工作汇总

### Dev-Planner
- 项目分析与优化
- mimi-gateway 实现
- 团队 Agent 配置完成

### Dev-Coder
- (待激活)

### Dev-Tester
- (待激活)

### Media-Creator
- (待激活)

---

## 日志格式规范

### Dev-Planner 格式
```markdown
## YYYY-MM-DD (星期)

### 工作内容
- [x] 已完成任务

### 决策记录
| 决策 | 原因 |

### 待处理
- [ ] 任务列表

### 团队状态
| Agent | 状态 |
```

### Dev-Coder 格式
```markdown
## YYYY-MM-DD (星期)

### 今日任务
- [ ] 任务列表

### 任务执行
- 状态:
- 模块:
- 耗时:

### 代码统计
| 指标 | 今日 | 累计 |
```

### Dev-Tester 格式
```markdown
## YYYY-MM-DD (星期)

### 测试执行
- 覆盖率: X%

### Bug 追踪
| 严重程度 | 描述 | 状态 |
```

### Media-Creator 格式
```markdown
## YYYY-MM-DD (星期)

### 内容发布
- [ ] 发布列表

### 社区互动
- 回答: X 个

### 数据统计
| 指标 | 今日 | 累计 |
```

---

## 快速查看命令

```bash
# 查看今日 Dev-Planner 日志
cat logs/DAILY.md | grep -A 20 "$(date +%Y-%m-%d)"

# 查看所有 Agent 今日状态
for log in logs/DAILY.md logs/*/DAILY.md; do
    echo "=== $log ==="
    grep -A 10 "$(date +%Y-%m-%d)" "$log" 2>/dev/null
done
```

---

## 归档规则

- 每月 1 日自动归档
- 移至 `logs/archive/YYYY-MM/`
- 保留最近 3 个月活跃日志
- 超过 1 年的日志可删除

---

# 本文件由 Dev-Planner 维护
# 团队日志统一入口