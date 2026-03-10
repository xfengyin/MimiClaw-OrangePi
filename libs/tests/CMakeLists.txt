cmake_minimum_required(VERSION 3.16)

# 测试：核心库
add_executable(test_core test_core.c)
target_link_libraries(test_core PRIVATE mimi-core)
add_test(NAME test_core COMMAND test_core)

# 测试：内存库
add_executable(test_memory test_memory.c)
target_link_libraries(test_memory PRIVATE mimi-memory ${SQLite3_LIBRARIES})
add_test(NAME test_memory COMMAND test_memory)

# 测试：配置库
add_executable(test_config test_config.c)
target_link_libraries(test_config PRIVATE mimi-config)
add_test(NAME test_config COMMAND test_config)

# 测试：工具库
add_executable(test_tools test_tools.c)
target_link_libraries(test_tools PRIVATE mimi-tools ${CMAKE_DL_LIBS})
add_test(NAME test_tools COMMAND test_tools)

# =============================================================================
# 性能基准测试 (Phase 3)
# =============================================================================

# 内存池测试
add_executable(bench_memory benchmarks/bench_memory.c)
target_link_libraries(bench_memory PRIVATE mimi-core mimi-memory ${SQLite3_LIBRARIES})

# 启动时间测试
add_executable(bench_startup benchmarks/test_startup.c)
target_link_libraries(bench_startup PRIVATE mimi-memory ${SQLite3_LIBRARIES})

# 延迟测试
add_executable(bench_latency benchmarks/test_latency.c)
target_link_libraries(bench_latency PRIVATE mimi-memory ${SQLite3_LIBRARIES})

# 并发测试
add_executable(bench_concurrent benchmarks/test_concurrent.c)
target_link_libraries(bench_concurrent PRIVATE mimi-memory ${SQLite3_LIBRARIES} pthread)

# 安装到 bin 目录
install(TARGETS bench_memory bench_startup bench_latency bench_concurrent
    RUNTIME DESTINATION bin
)
