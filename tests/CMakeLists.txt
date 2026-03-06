# Tests CMakeLists.txt

enable_testing()

# Test executables
add_executable(test_config test_config.c)
target_link_libraries(test_config ${CMAKE_SOURCE_DIR}/src/core/config.c json-c)

add_executable(test_memory test_memory.c)
target_link_libraries(test_memory ${CMAKE_SOURCE_DIR}/src/memory/memory_store.c sqlite3)

# Add tests
add_test(NAME config_test COMMAND test_config)
add_test(NAME memory_test COMMAND test_memory)
