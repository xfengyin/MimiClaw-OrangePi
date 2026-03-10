# MimiClaw-OrangePi v2.0 - 项目配置

**版本：** 2.0.0  
**日期：** 2026-03-10  
**状态：** 开发中

---

## GitHub 仓库配置

### 仓库信息
- **仓库：** https://github.com/xfengyin/MimiClaw-OrangePi
- **分支：** `main` (稳定) / `develop` (开发)
- **凭证：** `~/.config/mimiclaw/github-credentials` (权限 600)

### 环境变量
```bash
# 加载凭证
export GITHUB_TOKEN=$(cat ~/.config/mimiclaw/github-credentials | grep GITHUB_TOKEN | cut -d= -f2)
export GITHUB_REPO=xfengyin/MimiClaw-OrangePi
```

### Git 操作
```bash
# 克隆仓库
git clone https://oauth2:${GITHUB_TOKEN}@github.com/${GITHUB_REPO}.git

# 或配置凭证助手后直接使用
git clone https://github.com/${GITHUB_REPO}.git
```

---

## CI/CD 配置

### GitHub Secrets
在仓库 Settings → Secrets and variables → Actions 中添加：

| Secret Name | Value |
|-------------|-------|
| `GH_PAT` | (你的 Personal Access Token) |
| `GH_REPO` | `xfengyin/MimiClaw-OrangePi` |

### GitHub Actions 工作流
```yaml
# .github/workflows/build.yml
name: Build

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake build-essential libsqlite3-dev libcurl4-openssl-dev libssl-dev
      
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make -j$(nproc)
      
      - name: Test
        run: |
          cd build
          ctest --output-on-failure
```

---

## 本地开发配置

### 依赖安装 (OrangePi/Debian)
```bash
sudo apt update
sudo apt install -y git cmake build-essential \
    libsqlite3-dev libcurl4-openssl-dev libssl-dev \
    libjson-c-dev valgrind gcovr
```

### 构建项目
```bash
cd /home/node/.openclaw/workspace-dev-planner

# 编译库
cd libs && make -j$(nproc)

# 编译插件
cd ../plugins && make -j$(nproc)

# 运行测试
cd ../tests && make test
```

---

## 代码规范

### C 语言标准
- **标准：** C99
- **编译选项：** `-std=c99 -Wall -Wextra -Wpedantic`
- **格式化工具：** `clang-format`

### 提交规范
```
feat: 新功能
fix: 修复 bug
docs: 文档更新
style: 代码格式
refactor: 重构
test: 测试
chore: 构建/工具
```

### 分支策略
```
main          # 稳定分支
├── develop   # 开发分支
├── feature/* # 功能分支
├── bugfix/*  # 修复分支
└── release/* # 发布分支
```

---

## 项目结构
```
MimiClaw-OrangePi/
├── libs/                    # 核心库
│   ├── libmimi-core/
│   ├── libmimi-memory/
│   ├── libmimi-config/
│   └── libmimi-tools/
├── plugins/                 # 插件
│   ├── plugin-time/
│   ├── plugin-echo/
│   ├── plugin-web-search/
│   ├── plugin-file-ops/
│   └── plugin-memory/
├── tests/                   # 测试
├── docs/                    # 文档
├── scripts/                 # 脚本
└── examples/                # 示例
```

---

## 下一步

1. **推送代码到 GitHub**
   ```bash
   cd /home/node/.openclaw/workspace-dev-planner
   git init
   git remote add origin https://github.com/xfengyin/MimiClaw-OrangePi.git
   git add .
   git commit -m "feat: v2.0 架构重构 - Phase 1-4 完成"
   git push -u origin main
   ```

2. **配置 GitHub Actions**
   - 复制 `.github/workflows/build.yml`
   - 添加 Secrets

3. **设置保护分支**
   - main 分支要求 PR 审查
   - 要求 CI 通过

---

**配置完成！** 🎉
