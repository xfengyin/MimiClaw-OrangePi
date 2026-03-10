cmake_minimum_required(VERSION 3.16)
project(mimiclaw-plugins VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 设置输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 添加所有插件子目录
add_subdirectory(plugin-time)
add_subdirectory(plugin-echo)
add_subdirectory(plugin-web-search)
add_subdirectory(plugin-file-ops)
add_subdirectory(plugin-memory)

# 可选：添加测试
option(BUILD_TESTS "Build plugin tests" ON)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# 安装规则
install(DIRECTORY ${CMAKE_BINARY_DIR}/
    DESTINATION lib/mimiclaw/plugins
    FILES_MATCHING PATTERN "*.so*"
)

# 打印构建信息
message(STATUS "")
message(STATUS "=== MimiClaw Plugins Build Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Plugins output: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "")
