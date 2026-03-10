cmake_minimum_required(VERSION 3.16)
project(plugin-file-ops VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-tools/include
)

# 源文件
set(PLUGIN_SOURCES
    src/file_ops_plugin.c
)

# 创建共享库
add_library(plugin-file-ops SHARED ${PLUGIN_SOURCES})

# 设置输出文件名
set_target_properties(plugin-file-ops PROPERTIES
    OUTPUT_NAME "mimi-plugin-file-ops"
    PREFIX "lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 安装规则
install(TARGETS plugin-file-ops
    LIBRARY DESTINATION lib/mimiclaw/plugins
)
