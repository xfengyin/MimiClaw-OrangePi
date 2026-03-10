cmake_minimum_required(VERSION 3.16)
project(libmimi-config VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件
set(CONFIG_SOURCES
    src/config.c
)

# 创建静态库
add_library(mimi-config STATIC ${CONFIG_SOURCES})

# 设置库属性
set_target_properties(mimi-config PROPERTIES
    OUTPUT_NAME "mimi-config"
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
)

# 安装规则
install(TARGETS mimi-config
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
