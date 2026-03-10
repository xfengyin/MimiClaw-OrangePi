cmake_minimum_required(VERSION 3.16)
project(mimiclaw-plugin-tests VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs/libmimi-tools/include
)

# 链接目录
link_directories(
    ${CMAKE_SOURCE_DIR}/libs/build
    ${CMAKE_SOURCE_DIR}/plugins/build
)

# 测试可执行文件
add_executable(test_plugin_loader test_plugin_loader.c)
target_link_libraries(test_plugin_loader mimi-tools dl)

add_executable(test_time_plugin test_time_plugin.c)
target_link_libraries(test_time_plugin mimi-tools dl)

add_executable(test_echo_plugin test_echo_plugin.c)
target_link_libraries(test_echo_plugin mimi-tools dl)

# 注册测试
enable_testing()

add_test(NAME PluginLoaderTest COMMAND test_plugin_loader)
add_test(NAME TimePluginTest COMMAND test_time_plugin)
add_test(NAME EchoPluginTest COMMAND test_echo_plugin)

# 设置测试工作目录为 build 目录（插件所在目录）
set_tests_properties(PluginLoaderTest TimePluginTest EchoPluginTest
    PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)
