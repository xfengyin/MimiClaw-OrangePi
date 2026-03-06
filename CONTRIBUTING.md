# Contributing to MimiClaw-OrangePi

感谢您对 MimiClaw-OrangePi 项目的关注！我们欢迎各种形式的贡献。

## 开发流程

### 1. Fork 仓库

点击 GitHub 右上角的 "Fork" 按钮，创建您自己的仓库副本。

### 2. 克隆仓库

```bash
git clone https://github.com/YOUR_USERNAME/MimiClaw-OrangePi.git
cd MimiClaw-OrangePi
```

### 3. 创建分支

```bash
# 创建功能分支
git checkout -b feature/your-feature-name

# 或创建修复分支
git checkout -b bugfix/issue-description
```

### 4. 开发

- 编写代码
- 添加测试
- 更新文档

### 5. 提交更改

```bash
git add .
git commit -m "feat: add new feature"
```

提交信息格式：
- `feat:` 新功能
- `fix:` 修复问题
- `docs:` 文档更新
- `style:` 代码格式
- `refactor:` 重构
- `test:` 测试
- `chore:` 构建/工具

### 6. 推送并创建 PR

```bash
git push origin feature/your-feature-name
```

然后在 GitHub 上创建 Pull Request。

## 代码规范

### C 代码风格

- 使用 4 空格缩进
- 函数名使用小写加下划线
- 常量使用大写加下划线
- 注释使用 /* */ 风格

示例：
```c
/* Calculate factorial of n */
int calculate_factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * calculate_factorial(n - 1);
}
```

### 文件组织

```
src/
├── core/          # 核心功能
├── agent/         # AI Agent
├── memory/        # 记忆管理
├── gateway/       # 网关服务
├── cli/           # 命令行
└── tools/         # 工具集
```

## 测试

运行测试：
```bash
make test
```

添加新测试到 `tests/` 目录。

## 文档

- 更新 README.md 如果需要
- 为公共 API 添加文档注释
- 更新 CHANGELOG.md

## 问题报告

使用 GitHub Issues 报告问题，请包含：
- 问题描述
- 复现步骤
- 预期行为
- 实际行为
- 环境信息（OrangePi 型号、系统版本等）

## 联系方式

- Discussions: https://github.com/xfengyin/MimiClaw-OrangePi/discussions
- Issues: https://github.com/xfengyin/MimiClaw-OrangePi/issues

## 许可证

通过提交代码，您同意将其授权给项目使用（MIT 许可证）。
