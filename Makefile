# MimiClaw-OrangePi Makefile
# Simple wrapper around CMake for convenience

.PHONY: all clean install build test package docker

# Default target
all: build

# Create build directory and configure
build:
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release
	@cd build && $(MAKE) -j$$(nproc)

# Debug build
debug:
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug
	@cd build && $(MAKE) -j$$(nproc)

# Clean build artifacts
clean:
	@rm -rf build
	@echo "Build directory cleaned"

# Install to system
install: build
	@cd build && sudo $(MAKE) install
	@echo "Installation complete"

# Run tests
test: build
	@cd build && ctest --output-on-failure

# Create package
package: build
	@cd build && cpack
	@echo "Package created in build/"

# Code formatting
format:
	@find src -name "*.c" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatted"

# Static analysis
check:
	@cppcheck --enable=all --suppress=missingIncludeSystem src/

# Docker build
docker:
	@docker build -t mimiclaw-orangepi:latest .

# Run in Docker
docker-run:
	@docker run -it --rm mimiclaw-orangepi:latest

# Help
help:
	@echo "MimiClaw-OrangePi Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build the project (default)"
	@echo "  make debug    - Build with debug symbols"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make install  - Install to system"
	@echo "  make test     - Run tests"
	@echo "  make package  - Create distribution package"
	@echo "  make format   - Format source code"
	@echo "  make check    - Run static analysis"
	@echo "  make docker   - Build Docker image"
	@echo "  make help     - Show this help"
