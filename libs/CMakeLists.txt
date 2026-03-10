cmake_minimum_required(VERSION 3.16)
project(MimiClaw-OrangePi VERSION 2.0.0 LANGUAGES C)

message(STATUS "Building MimiClaw-OrangePi v${PROJECT_VERSION}")

# C99 标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# 默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 查找依赖
find_package(SQLite3 REQUIRED)
message(STATUS "SQLite3 version: ${SQLite3_VERSION}")

# 构建选项
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build example applications" ON)

# ============================================================================
# 库
# ============================================================================

message(STATUS "Building libraries...")

add_subdirectory(libs/libmimi-core)
add_subdirectory(libs/libmimi-memory)
add_subdirectory(libs/libmimi-config)
add_subdirectory(libs/libmimi-tools)

# ============================================================================
# 示例应用
# ============================================================================

if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    
    # 示例：核心库测试
    add_executable(example-core examples/example_core.c)
    target_link_libraries(example-core PRIVATE mimi-core)
    
    # 示例：内存库测试
    add_executable(example-memory examples/example_memory.c)
    target_link_libraries(example-memory PRIVATE mimi-memory ${SQLite3_LIBRARIES})
    
    # 示例：配置库测试
    add_executable(example-config examples/example_config.c)
    target_link_libraries(example-config PRIVATE mimi-config)
    
    # 示例：工具库测试
    add_executable(example-tools examples/example_tools.c)
    target_link_libraries(example-tools PRIVATE mimi-tools ${CMAKE_DL_LIBS})
endif()

# ============================================================================
# 测试
# ============================================================================

if(BUILD_TESTS)
    message(STATUS "Building tests...")
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# 安装
# ============================================================================

install(DIRECTORY include/
    DESTINATION include/mimi
    FILES_MATCHING PATTERN "*.h"
)

# 创建配置文件
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/mimi-config.pc.in
    ${CMAKE_BINARY_DIR}/mimi.pc
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/mimi.pc
    DESTINATION lib/pkgconfig
)

# ============================================================================
# 总结
# ============================================================================

message(STATUS "")
message(STATUS "=== MimiClaw-OrangePi v${PROJECT_VERSION} ===")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "SQLite3:           ${SQLite3_LIBRARIES}")
message(STATUS "Build tests:       ${BUILD_TESTS}")
message(STATUS "Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "")
