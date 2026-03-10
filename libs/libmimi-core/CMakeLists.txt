cmake_minimum_required(VERSION 3.16)
project(libmimi-core VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件
set(CORE_SOURCES
    src/core.c
    src/mempool.c
    src/async_io.c
)

# 创建静态库
add_library(mimi-core STATIC ${CORE_SOURCES})

# 设置库属性
set_target_properties(mimi-core PROPERTIES
    OUTPUT_NAME "mimi-core"
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
)

# 安装规则
install(TARGETS mimi-core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
