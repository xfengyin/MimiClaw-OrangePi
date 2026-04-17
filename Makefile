# Copyright 2024 MimiClaw Authors
# SPDX-License-Identifier: MIT

.PHONY: build build-linux-arm build-linux-arm64 build-all clean test deps fmt lint

# Build variables
BINARY_NAME=mimiclaw
VERSION?=dev
COMMIT?=$(shell git rev-parse --short HEAD 2>/dev/null || echo "none")
BUILD_TIME=$(shell date -u '+%Y-%m-%d_%H:%M:%S')
LDFLAGS=-ldflags "-X main.version=$(VERSION) -X main.commit=$(COMMIT) -X main.date=$(BUILD_TIME)"

# Go parameters
GOCMD=go
GOBUILD=$(GOCMD) build
GOCLEAN=$(GOCMD) clean
GOTEST=$(GOCMD) test
GOGET=$(GOCMD) get
GOMOD=$(GOCMD) mod

# Directories
BUILD_DIR=build
CMD_DIR=cmd/cli

# Default target
all: build

# Download dependencies
deps:
	$(GOMOD) download
	$(GOMOD) tidy

# Build for current platform
build:
	mkdir -p $(BUILD_DIR)
	CGO_ENABLED=1 $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME) $(CMD_DIR)

# Build for Linux ARM (32-bit) - OrangePi Zero 2 / Zero 2W
build-linux-arm:
	mkdir -p $(BUILD_DIR)
	GOOS=linux GOARCH=arm GOARM=7 CGO_ENABLED=1 CC=arm-linux-gnueabihf-gcc $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME)-linux-arm $(CMD_DIR)

# Build for Linux ARM64 - OrangePi Zero 3 / Raspberry Pi 64-bit
build-linux-arm64:
	mkdir -p $(BUILD_DIR)
	GOOS=linux GOARCH=arm64 CGO_ENABLED=1 CC=aarch64-linux-gnu-gcc $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME)-linux-arm64 $(CMD_DIR)

# Build for Linux x86_64
build-linux-x86:
	mkdir -p $(BUILD_DIR)
	GOOS=linux GOARCH=amd64 $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME)-linux-x86_64 $(CMD_DIR)

# Build for macOS ARM64 (Apple Silicon)
build-darwin-arm64:
	mkdir -p $(BUILD_DIR)
	GOOS=darwin GOARCH=arm64 $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME)-darwin-arm64 $(CMD_DIR)

# Build for Windows
build-windows:
	mkdir -p $(BUILD_DIR)
	GOOS=windows GOARCH=amd64 $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME).exe $(CMD_DIR)

# Build for all platforms
build-all: build-linux-arm build-linux-arm64 build-linux-x86 build-darwin-arm64 build-windows
	@echo "Build complete!"
	@ls -la $(BUILD_DIR)/

# Build for Raspberry Pi Zero 2 W
build-pi-zero:
	@echo "Building for Raspberry Pi Zero 2 W..."
	mkdir -p $(BUILD_DIR)
	# 32-bit (Raspberry Pi OS 32-bit)
	GOOS=linux GOARCH=arm GOARM=7 CGO_ENABLED=1 CC=arm-linux-gnueabihf-gcc $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME)-rpi-zero-armv6l $(CMD_DIR)
	# 64-bit (Raspberry Pi OS 64-bit)
	GOOS=linux GOARCH=arm64 CGO_ENABLED=1 CC=aarch64-linux-gnu-gcc $(GOBUILD) $(LDFLAGS) -o $(BUILD_DIR)/$(BINARY_NAME)-rpi-zero-arm64 $(CMD_DIR)
	@echo "Raspberry Pi Zero 2 W builds complete!"

# Test
test:
	$(GOTEST) -v ./...

# Run
run: build
	$(BUILD_DIR)/$(BINARY_NAME) demo

# Clean
clean:
	$(GOCLEAN)
	rm -rf $(BUILD_DIR)

# Format code
fmt:
	$(GOCMD) fmt ./...

# Lint code
lint:
	golangci-lint run || echo "Install golangci-lint: go install github.com/golangci/golangci-lint/cmd/golangci-lint@latest"

# Generate mocks (if needed)
mocks:
	@echo "Generating mocks..."
	# Add mockgen commands here if needed

# Docker build
docker-build:
	docker build -t mimiclaw:latest .

# Install
install: build
	sudo install -m 755 $(BUILD_DIR)/$(BINARY_NAME) /usr/local/bin/mimiclaw

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/mimiclaw

# Help
help:
	@echo "MimiClaw Build System"
	@echo ""
	@echo "Targets:"
	@echo "  build              - Build for current platform"
	@echo "  build-linux-arm    - Build for Linux ARM 32-bit"
	@echo "  build-linux-arm64  - Build for Linux ARM 64-bit"
	@echo "  build-all          - Build for all platforms"
	@echo "  build-pi-zero      - Build for Raspberry Pi Zero 2 W"
	@echo "  test               - Run tests"
	@echo "  clean              - Clean build artifacts"
	@echo "  fmt                - Format code"
	@echo "  lint               - Lint code"
	@echo "  install            - Install to /usr/local/bin"
