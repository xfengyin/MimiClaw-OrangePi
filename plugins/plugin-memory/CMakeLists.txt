cmake_minimum_required(VERSION 3.16)
project(plugin-memory VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-tools/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-memory/include
)

# 源文件
set(PLUGIN_SOURCES
    src/memory_plugin.c
)

# 创建共享库
add_library(plugin-memory SHARED ${PLUGIN_SOURCES})

# 链接 libmimi-memory 共享库 (修复符号可见性问题)
target_link_libraries(plugin-memory 
    ${CMAKE_SOURCE_DIR}/../libs/build/lib/libmimi-memory.so
    ${CMAKE_SOURCE_DIR}/../libs/build/lib/libmimi-core.a
)

# 设置 RPATH 以便运行时找到共享库
set_target_properties(plugin-memory PROPERTIES
    BUILD_RPATH "${CMAKE_SOURCE_DIR}/../libs/build/lib"
    INSTALL_RPATH "$ORIGIN/../../../libs/build/lib"
)

# 设置输出文件名
set_target_properties(plugin-memory PROPERTIES
    OUTPUT_NAME "mimi-plugin-memory"
    PREFIX "lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 安装规则
install(TARGETS plugin-memory
    LIBRARY DESTINATION lib/mimiclaw/plugins
)
