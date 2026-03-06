cmake_minimum_required(VERSION 3.16)

# Project configuration
project(mimiclaw-orangepi
    VERSION 1.0.0
    DESCRIPTION "MimiClaw AI Assistant for OrangePi Zero3"
    LANGUAGES C CXX
)

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror=return-type")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find libraries
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(JSON-C REQUIRED json-c)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(OPENSSL REQUIRED openssl)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${JSON-C_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

# Source files
set(CORE_SOURCES
    src/core/main.c
    src/core/config.c
    src/core/logger.c
)

set(AGENT_SOURCES
    src/agent/agent_loop.c
)

set(MEMORY_SOURCES
    src/memory/memory_store.c
)

set(GATEWAY_SOURCES
    src/gateway/websocket_server.c
    src/gateway/telegram_bot.c
)

set(CLI_SOURCES
)

set(TOOLS_SOURCES
    src/tools/tool_registry.c
    src/tools/tool_web_search.c
    src/tools/tool_get_time.c
    src/tools/tool_files.c
)

# All sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${AGENT_SOURCES}
    ${MEMORY_SOURCES}
    ${GATEWAY_SOURCES}
    ${CLI_SOURCES}
    ${TOOLS_SOURCES}
)

# Executable
add_executable(mimiclaw ${ALL_SOURCES})

# Link libraries
target_link_libraries(mimiclaw
    ${CMAKE_THREAD_LIBS_INIT}
    ${CURL_LIBRARIES}
    ${JSON-C_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    m
)

# Link directories
target_link_directories(mimiclaw PRIVATE
    ${CURL_LIBRARY_DIRS}
    ${JSON-C_LIBRARY_DIRS}
    ${SQLITE3_LIBRARY_DIRS}
)

# Compile options
target_compile_options(mimiclaw PRIVATE
    ${CURL_CFLAGS_OTHER}
    ${JSON-C_CFLAGS_OTHER}
)

# Install targets
install(TARGETS mimiclaw
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION /etc/mimiclaw
    FILES_MATCHING PATTERN "*.json" PATTERN "*.conf"
)

install(DIRECTORY scripts/
    DESTINATION /usr/share/mimiclaw
    FILES_MATCHING PATTERN "*.sh" PATTERN "*.py"
    PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)

# CPack configuration
set(CPACK_PACKAGE_NAME "mimiclaw-orangepi")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_CONTACT "xfengyin@gmail.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4-openssl-dev, libjson-c-dev, libsqlite3-dev, libssl-dev")
set(CPACK_GENERATOR "DEB;TGZ")

include(CPack)

# Testing
enable_testing()
add_subdirectory(tests)
