#!/bin/bash
#===============================================================================
# MimiClaw Build Script
# Build MimiClaw for OrangePi and other platforms
#===============================================================================

set -e

VERSION="${1:-dev}"
BUILD_DIR="build"
PROJECT_NAME="MimiClaw-OrangePi"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo_info() { echo -e "${YELLOW}[INFO]${NC} $1"; }
echo_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
echo_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Detect platform
detect_platform() {
    if [ -f /proc/device-tree/model ]; then
        MODEL=$(cat /proc/device-tree/model)
        if [[ "$MODEL" == *"Zero 3"* ]]; then
            echo "orangepi-zero3"
        elif [[ "$MODEL" == *"Zero 2"* ]]; then
            echo "orangepi-zero2"
        elif [[ "$MODEL" == *"Raspberry Pi"* ]]; then
            echo "rpi"
        else
            echo "linux"
        fi
    else
        echo "linux"
    fi
}

# Build for current platform
build_current() {
    echo_info "Building for current platform..."
    
    mkdir -p "$BUILD_DIR"
    
    # Detect Go installation
    if ! command -v go &> /dev/null; then
        echo_error "Go not found. Installing..."
        sudo apt update && sudo apt install -y golang-go
    fi
    
    # Get commit hash
    COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "none")
    BUILD_TIME=$(date -u '+%Y-%m-%d_%H:%M:%S')
    
    # Build
    CGO_ENABLED=1 go build \
        -ldflags "-X main.version=$VERSION -X main.commit=$COMMIT -X main.date=$BUILD_TIME" \
        -o "$BUILD_DIR/mimiclaw" \
        ./cmd/cli
    
    echo_success "Build complete: $BUILD_DIR/mimiclaw"
    ls -lh "$BUILD_DIR/mimiclaw"
}

# Build for ARM 64-bit
build_arm64() {
    echo_info "Building for Linux ARM64..."
    
    mkdir -p "$BUILD_DIR"
    
    GOOS=linux GOARCH=arm64 CGO_ENABLED=1 CC=aarch64-linux-gnu-gcc \
        go build \
        -ldflags "-X main.version=$VERSION" \
        -o "$BUILD_DIR/mimiclaw-linux-arm64" \
        ./cmd/cli
    
    echo_success "ARM64 build complete: $BUILD_DIR/mimiclaw-linux-arm64"
}

# Main
main() {
    echo "========================================"
    echo "  MimiClaw Build Script"
    echo "  Version: $VERSION"
    echo "========================================"
    echo ""
    
    PLATFORM=$(detect_platform)
    echo_info "Detected platform: $PLATFORM"
    
    case "${1:-current}" in
        current)
            build_current
            ;;
        arm64)
            build_arm64
            ;;
        all)
            build_current
            build_arm64
            ;;
        *)
            build_current
            ;;
    esac
    
    echo ""
    echo_success "Build completed!"
}

main "$@"
