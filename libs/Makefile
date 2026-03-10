# MimiClaw v2.0 - 简单构建 Makefile (用于快速验证)
# 推荐使用 CMake 进行完整构建

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC -I./libmimi-core/include -I./libmimi-memory/include
LDFLAGS = -lsqlite3 -lpthread

# 目录
SRC_DIR = .
BUILD_DIR = build
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin
TEST_DIR = tests/benchmarks

# 源文件
MEMORY_SRC = libmimi-memory/src/memory.c
CORE_SRC = libmimi-core/src/core.c libmimi-core/src/mempool.c libmimi-core/src/async_io.c

# 目标
MEMORY_LIB = $(LIB_DIR)/libmimi-memory.so
CORE_LIB = $(LIB_DIR)/libmimi-core.a

.PHONY: all clean test

all: dirs $(MEMORY_LIB) $(CORE_LIB) benchmarks

dirs:
	@mkdir -p $(LIB_DIR) $(BIN_DIR)

# 构建共享库 libmimi-memory.so
$(MEMORY_LIB): $(MEMORY_SRC)
	@echo "编译 libmimi-memory.so..."
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDFLAGS)

# 构建静态库 libmimi-core.a
$(CORE_LIB): $(CORE_SRC)
	@echo "编译 libmimi-core.a..."
	$(CC) $(CFLAGS) -c $(CORE_SRC)
	ar rcs $@ *.o
	rm -f *.o

# 构建基准测试
benchmarks: $(MEMORY_LIB) $(CORE_LIB)
	@echo "编译基准测试程序..."
	$(CC) $(CFLAGS) -o $(BIN_DIR)/bench_memory $(TEST_DIR)/bench_memory.c -L$(LIB_DIR) -lmimi-memory -lmimi-core $(LDFLAGS) -Wl,-rpath,'$$ORIGIN/../lib'
	$(CC) $(CFLAGS) -o $(BIN_DIR)/bench_startup $(TEST_DIR)/test_startup.c -L$(LIB_DIR) -lmimi-memory $(LDFLAGS) -Wl,-rpath,'$$ORIGIN/../lib'
	$(CC) $(CFLAGS) -o $(BIN_DIR)/bench_latency $(TEST_DIR)/test_latency.c -L$(LIB_DIR) -lmimi-memory $(LDFLAGS) -Wl,-rpath,'$$ORIGIN/../lib'
	$(CC) $(CFLAGS) -o $(BIN_DIR)/bench_concurrent $(TEST_DIR)/test_concurrent.c -L$(LIB_DIR) -lmimi-memory $(LDFLAGS) -Wl,-rpath,'$$ORIGIN/../lib'

# 运行测试
test: benchmarks
	@echo ""
	@echo "=== 运行基准测试 ==="
	@echo ""
	@echo "1. 内存测试..."
	$(BIN_DIR)/bench_memory
	@echo ""
	@echo "2. 启动测试..."
	@$(BIN_DIR)/bench_startup && echo "✓ 启动成功"
	@echo ""
	@echo "3. 延迟测试..."
	$(BIN_DIR)/bench_latency
	@echo ""
	@echo "4. 并发测试..."
	$(BIN_DIR)/bench_concurrent

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o
