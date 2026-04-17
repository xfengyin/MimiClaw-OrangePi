# Contributing to MimiClaw

Thank you for your interest in contributing to MimiClaw!

## How to Contribute

### Reporting Bugs

1. Check existing issues first
2. Create a new issue with:
   - Clear title and description
   - Steps to reproduce
   - Expected vs actual behavior
   - Environment details (OS, Go version, hardware)

### Suggesting Features

1. Open a discussion first
2. Describe the use case
3. Explain why it would benefit the project

### Pull Requests

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes
4. Run tests: `make test`
5. Commit with clear messages
6. Push and create PR

### Code Style

- Follow Go conventions (run `make fmt`)
- Add comments for exported functions
- Keep functions small and focused

### Development Setup

```bash
# Clone
git clone https://github.com/xfengyin/MimiClaw-OrangePi.git
cd MimiClaw-OrangePi

# Install dependencies
make deps

# Build
make build

# Test
make test
```

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
