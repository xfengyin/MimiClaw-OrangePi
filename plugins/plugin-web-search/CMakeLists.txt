cmake_minimum_required(VERSION 3.16)
project(plugin-web-search VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找 curl 库
find_package(CURL REQUIRED)

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-tools/include
    ${CURL_INCLUDE_DIRS}
)

# 源文件
set(PLUGIN_SOURCES
    src/web_search_plugin.c
)

# 创建共享库
add_library(plugin-web-search SHARED ${PLUGIN_SOURCES})

# 链接 curl 库
target_link_libraries(plugin-web-search ${CURL_LIBRARIES})

# 设置输出文件名
set_target_properties(plugin-web-search PROPERTIES
    OUTPUT_NAME "mimi-plugin-web-search"
    PREFIX "lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 安装规则
install(TARGETS plugin-web-search
    LIBRARY DESTINATION lib/mimiclaw/plugins
)
