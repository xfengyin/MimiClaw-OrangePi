cmake_minimum_required(VERSION 3.16)
project(libmimi-tools VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件 - 合并为单一文件
set(TOOLS_SOURCES
    src/mimi_tools.c
)

# 创建静态库
add_library(mimi-tools STATIC ${TOOLS_SOURCES})

# 链接 dl 库用于动态加载
target_link_libraries(mimi-tools PRIVATE dl)

# 设置库属性
set_target_properties(mimi-tools PROPERTIES
    OUTPUT_NAME "mimi-tools"
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
)

# 安装规则
install(TARGETS mimi-tools
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
