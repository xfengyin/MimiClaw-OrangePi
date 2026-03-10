# MimiClaw Plugins - Makefile Build System
# Pure Makefile build (no CMake required)
# Suitable for embedded systems like OrangePi

.PHONY: all clean test install libs

# 编译器设置
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -fPIC -I../libs/libmimi-tools/include
LDFLAGS = -shared

# 目录
BUILD_DIR = build
LIBS_DIR = ../libs/build/lib
PLUGINS_SRC = .

# 插件列表
PLUGINS = plugin-time plugin-echo plugin-web-search plugin-file-ops plugin-memory

# 库文件
LIBMIMI_TOOLS = $(LIBS_DIR)/libmimi-tools.a
LIBMIMI_MEMORY = $(LIBS_DIR)/libmimi-memory.a
LIBMIMI_CORE = $(LIBS_DIR)/libmimi-core.a

# 系统库
SYSLIBS_TIME = 
SYSLIBS_ECHO = 
SYSLIBS_WEBSEARCH = -lcurl
SYSLIBS_FILEOPS = 
SYSLIBS_MEMORY = -lsqlite3

# 默认目标
all: $(BUILD_DIR) plugins

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 构建所有插件
plugins: $(PLUGINS)

# 插件构建规则
plugin-time: $(BUILD_DIR)/libmimi-plugin-time.so

plugin-echo: $(BUILD_DIR)/libmimi-plugin-echo.so

plugin-web-search: $(BUILD_DIR)/libmimi-plugin-web-search.so

plugin-file-ops: $(BUILD_DIR)/libmimi-plugin-file-ops.so

plugin-memory: $(BUILD_DIR)/libmimi-plugin-memory.so

# Time 插件
$(BUILD_DIR)/libmimi-plugin-time.so: plugin-time/src/time_plugin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(SYSLIBS_TIME)
	@echo "✓ Built libmimi-plugin-time.so"

# Echo 插件
$(BUILD_DIR)/libmimi-plugin-echo.so: plugin-echo/src/echo_plugin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(SYSLIBS_ECHO)
	@echo "✓ Built libmimi-plugin-echo.so"

# Web Search 插件
$(BUILD_DIR)/libmimi-plugin-web-search.so: plugin-web-search/src/web_search_plugin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(SYSLIBS_WEBSEARCH)
	@echo "✓ Built libmimi-plugin-web-search.so"

# File Ops 插件
$(BUILD_DIR)/libmimi-plugin-file-ops.so: plugin-file-ops/src/file_ops_plugin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(SYSLIBS_FILEOPS)
	@echo "✓ Built libmimi-plugin-file-ops.so"

# Memory 插件 (需要链接静态库)
$(BUILD_DIR)/libmimi-plugin-memory.so: plugin-memory/src/memory_plugin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -Wl,--whole-archive $(LIBMIMI_MEMORY) $(LIBMIMI_CORE) -Wl,--no-whole-archive $(SYSLIBS_MEMORY)
	@echo "✓ Built libmimi-plugin-memory.so"

# 构建依赖库
libs:
	@echo "=== Building MimiClaw Core Libraries ==="
	$(MAKE) -C ../libs
	@echo "✓ Libraries built"

# 运行测试
test: all
	@echo "=== Running Plugin Tests ==="
	@echo "Note: Full test suite requires test framework"
	@echo "Testing plugin loading..."
	@for plugin in $(BUILD_DIR)/*.so; do \
		if [ -f "$$plugin" ]; then \
			echo "  Checking $$plugin..."; \
			nm -D "$$plugin" | grep -q get_tool_plugin && echo "    ✓ Exports get_tool_plugin" || echo "    ✗ Missing get_tool_plugin"; \
		fi; \
	done
	@echo "✓ Tests complete"

# 安装插件
install: all
	@echo "=== Installing Plugins ==="
	@mkdir -p ../lib/mimiclaw/plugins
	@cp $(BUILD_DIR)/*.so ../lib/mimiclaw/plugins/
	@echo "✓ Plugins installed to ../lib/mimiclaw/plugins/"

# 清理
clean:
	@echo "=== Cleaning build artifacts ==="
	@rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

# 帮助
help:
	@echo "MimiClaw Plugins Build System (Makefile)"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build all plugins (default)"
	@echo "  libs    - Build core libraries first"
	@echo "  test    - Build and run basic tests"
	@echo "  install - Build and install plugins"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build all plugins"
	@echo "  make libs && make # Build libs then plugins"
	@echo "  make test         # Run tests"
	@echo "  make clean        # Clean build"

# 打印信息
info:
	@echo "MimiClaw Plugins Build Configuration:"
	@echo "  CC:        $(CC)"
	@echo "  CFLAGS:    $(CFLAGS)"
	@echo "  LDFLAGS:   $(LDFLAGS)"
	@echo "  BUILD_DIR: $(BUILD_DIR)"
	@echo "  LIBS_DIR:  $(LIBS_DIR)"
	@echo "  PLUGINS:   $(PLUGINS)"
