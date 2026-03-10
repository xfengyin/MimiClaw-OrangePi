cmake_minimum_required(VERSION 3.16)
project(plugin-time VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-tools/include
)

# 源文件
set(PLUGIN_SOURCES
    src/time_plugin.c
)

# 创建共享库
add_library(plugin-time SHARED ${PLUGIN_SOURCES})

# 设置输出文件名
set_target_properties(plugin-time PROPERTIES
    OUTPUT_NAME "mimi-plugin-time"
    PREFIX "lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 安装规则
install(TARGETS plugin-time
    LIBRARY DESTINATION lib/mimiclaw/plugins
)
