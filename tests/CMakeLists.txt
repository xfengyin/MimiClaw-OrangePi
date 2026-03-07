# Tests CMakeLists.txt

enable_testing()

# Test executables - link with required source files
add_executable(test_config test_config.c ${CMAKE_SOURCE_DIR}/src/core/config.c)
target_link_libraries(test_config json-c)

add_executable(test_memory test_memory.c ${CMAKE_SOURCE_DIR}/src/memory/memory_store.c ${CMAKE_SOURCE_DIR}/src/core/logger.c)
target_link_libraries(test_memory sqlite3 pthread)

# Add tests
add_test(NAME config_test COMMAND test_config)
add_test(NAME memory_test COMMAND test_memory)
