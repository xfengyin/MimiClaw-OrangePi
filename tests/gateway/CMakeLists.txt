cmake_minimum_required(VERSION 3.16)

# Gateway Tests
# ============================================================================

add_executable(test_gateway
    test_gateway.c
)

target_include_directories(test_gateway PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/mimi-gateway/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-core/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-memory/include
)

target_link_libraries(test_gateway PRIVATE
    mimi-core
    mimi-memory
    mimi-config
    mimi-tools
    pthread
)

# Install
install(TARGETS test_gateway DESTINATION bin/test)

# Add to ctest
add_test(NAME gateway COMMAND test_gateway)