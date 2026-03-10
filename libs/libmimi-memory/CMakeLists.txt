cmake_minimum_required(VERSION 3.16)
project(libmimi-memory VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找 SQLite3
find_package(SQLite3 REQUIRED)

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${SQLite3_INCLUDE_DIRS})

# 源文件
set(MEMORY_SOURCES
    src/memory.c
)

# 创建共享库 (修复 dlopen 符号可见性问题)
add_library(mimi-memory SHARED ${MEMORY_SOURCES})

# 设置符号可见性：默认隐藏，仅导出标记的符号
set_target_properties(mimi-memory PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# 定义导出宏
target_compile_definitions(mimi-memory PRIVATE MIMI_MEMORY_BUILDING_DLL)

# 链接依赖
target_link_libraries(mimi-memory PRIVATE ${SQLite3_LIBRARIES})

# 设置库属性
set_target_properties(mimi-memory PROPERTIES
    OUTPUT_NAME "mimi-memory"
    VERSION ${PROJECT_VERSION}
    SOVERSION 2
)

# 安装规则
install(TARGETS mimi-memory
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
