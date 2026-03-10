# 贡献指南

## 开发环境设置

### 依赖安装
```bash
sudo apt update
sudo apt install -y git cmake build-essential \
    libsqlite3-dev libcurl4-openssl-dev libssl-dev \
    libjson-c-dev
```

### 克隆项目
```bash
git clone https://github.com/xfengyin/MimiClaw-OrangePi.git
cd MimiClaw-OrangePi
```

### 构建
```bash
make -C libs
make -C plugins
make -C tests test
```

## 开发流程

### 1. Fork 项目
在 GitHub 上点击 Fork 按钮

### 2. 创建分支
```bash
git checkout -b feature/your-feature-name
```

### 3. 开发功能
- 编写代码
- 编写测试
- 确保测试通过

### 4. 提交代码
```bash
git add .
git commit -m "feat: add your feature description"
git push origin feature/your-feature-name
```

### 5. 创建 Pull Request
在 GitHub 上创建 PR，描述你的更改

## 代码规范

### C 语言标准
- C99
- 编译选项：`-std=c99 -Wall -Wextra -Wpedantic`

### 提交信息格式
```
feat: 新功能
fix: 修复 bug
docs: 文档更新
style: 代码格式
refactor: 重构
test: 测试
chore: 构建/工具
```

## 测试要求
- 新功能必须包含测试
- 所有测试必须通过
- 覆盖率不得降低
