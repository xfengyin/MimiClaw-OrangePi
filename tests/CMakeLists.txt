# MimiClaw-OrangePi v2.0 测试配置
cmake_minimum_required(VERSION 3.20)
project(MimiClaw-Tests VERSION 2.0.0)

# 测试选项
option(BUILD_TESTS "Build tests" ON)
option(CODE_COVERAGE "Enable code coverage" OFF)
option(BUILD_PERF_TESTS "Build performance tests" ON)
option(BUILD_INTEGRATION_TESTS "Build integration tests" ON)

if(NOT BUILD_TESTS)
    return()
endif()

# 查找依赖
find_package(cmocka REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML REQUIRED yaml-0.1)

# 编译选项
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 覆盖率标志
if(CODE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")
    message(STATUS "Code coverage enabled")
endif()

# 测试源文件
set(TEST_SOURCES
    # libmimi-core 测试
    tests/core/test_agent_loop.c
    tests/core/test_context_manager.c
    tests/core/test_tool_invocation.c
    
    # libmimi-memory 测试
    tests/memory/test_sqlite_crud.c
    tests/memory/test_connection_pool.c
    tests/memory/test_persistence.c
    
    # libmimi-config 测试
    tests/config/test_parser.c
    tests/config/test_hot_reload.c
    tests/config/test_validation.c
    
    # libmimi-tools 测试
    tests/tools/test_plugin_lifecycle.c
    tests/tools/test_plugin_sandbox.c
    tests/tools/test_registry.c
)

# Mock 文件
set(MOCK_SOURCES
    tests/mocks/mock_http_client.c
    tests/mocks/mock_database.c
    tests/mocks/mock_file_system.c
    tests/mocks/mock_external_api.c
)

# 创建测试可执行文件
add_executable(mimiclaw-tests
    ${TEST_SOURCES}
    ${MOCK_SOURCES}
)

# 链接库
target_link_libraries(mimiclaw-tests
    PRIVATE
        cmocka
        cmocka-pthread
        SQLite3::SQLite3
        CURL::libcurl
        ${YAML_LIBRARIES}
        mimimi-core
        mimimi-memory
        mimimi-config
        mimimi-tools
)

# 包含目录
target_include_directories(mimiclaw-tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/mocks
        ${YAML_INCLUDE_DIRS}
)

# 编译定义
target_compile_definitions(mimiclaw-tests
    PRIVATE
        TEST_MODE=1
        $<$<BOOL:${CODE_COVERAGE}>:CODE_COVERAGE=1>
)

# 注册测试
enable_testing()

# Core 测试
add_test(NAME test_agent_loop
    COMMAND mimiclaw-tests --group=core --test=agent_loop
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_context_manager
    COMMAND mimiclaw-tests --group=core --test=context_manager
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_tool_invocation
    COMMAND mimiclaw-tests --group=core --test=tool_invocation
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Memory 测试
add_test(NAME test_sqlite_crud
    COMMAND mimiclaw-tests --group=memory --test=sqlite_crud
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_connection_pool
    COMMAND mimiclaw-tests --group=memory --test=connection_pool
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_persistence
    COMMAND mimiclaw-tests --group=memory --test=persistence
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Config 测试
add_test(NAME test_parser
    COMMAND mimiclaw-tests --group=config --test=parser
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_hot_reload
    COMMAND mimiclaw-tests --group=config --test=hot_reload
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_validation
    COMMAND mimiclaw-tests --group=config --test=validation
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Tools 测试
add_test(NAME test_plugin_lifecycle
    COMMAND mimiclaw-tests --group=tools --test=plugin_lifecycle
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_plugin_sandbox
    COMMAND mimiclaw-tests --group=tools --test=plugin_sandbox
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME test_registry
    COMMAND mimiclaw-tests --group=tools --test=registry
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 性能测试 (可选)
if(BUILD_PERF_TESTS)
    add_executable(mimiclaw-perf-tests
        tests/perf/test_memory.c
        tests/perf/test_latency.c
    )
    
    target_link_libraries(mimiclaw-perf-tests
        PRIVATE
            cmocka
            mimimi-core
            mimimi-memory
    )
    
    add_test(NAME perf_memory
        COMMAND mimiclaw-perf-tests --test=memory
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    
    add_test(NAME perf_latency
        COMMAND mimiclaw-perf-tests --test=latency
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# 自定义测试目标
add_custom_target(run-tests
    COMMAND ctest --output-on-failure
    DEPENDS mimiclaw-tests
    COMMENT "Running all tests"
)

add_custom_target(coverage
    COMMAND lcov --capture --directory . --output-file coverage.info
                --exclude '*/tests/*'
                --exclude '*/third_party/*'
                --exclude '*/mocks/*'
    COMMAND genhtml coverage.info --output-directory coverage-report
    COMMENT "Generating coverage report"
)

add_custom_target(perf-tests
    COMMAND ./scripts/run-perf-tests.sh
    DEPENDS mimiclaw-perf-tests
    COMMENT "Running performance tests"
)

add_custom_target(integration-tests
    COMMAND ./scripts/run-integration-tests.sh
    COMMENT "Running integration tests"
)

# 安装测试
install(TARGETS mimiclaw-tests DESTINATION bin)
install(DIRECTORY tests/ DESTINATION share/mimiclaw/tests)
