# MimiClaw-OrangePi v2.0
# AI Assistant for OrangePi Zero3

cmake_minimum_required(VERSION 3.16)
project(MimiClaw-OrangePi VERSION 2.0.0 LANGUAGES C)

# ============================================================================
# Project Configuration
# ============================================================================

message(STATUS "")
message(STATUS "=== MimiClaw-OrangePi v${PROJECT_VERSION} ===")
message(STATUS "")

# C Standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Standard: C99")

# ============================================================================
# Dependencies
# ============================================================================

find_package(PkgConfig REQUIRED)

# SQLite3
pkg_check_modules(SQLite3 REQUIRED sqlite3)
message(STATUS "SQLite3: ${SQLite3_VERSION}")

# libcurl
pkg_check_modules(CURL REQUIRED libcurl)
message(STATUS "libcurl: ${CURL_VERSION}")

# OpenSSL
pkg_check_modules(OPENSSL REQUIRED openssl)
message(STATUS "OpenSSL: ${OPENSSL_VERSION}")

# ============================================================================
# Build Options
# ============================================================================

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_APPS "Build applications" ON)
option(BUILD_PLUGINS "Build plugins" ON)

# ============================================================================
# Directories
# ============================================================================

# Source directories
add_subdirectory(libs)
add_subdirectory(tests)

if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    # Examples are included in libs/CMakeLists.txt
endif()

if(BUILD_TESTS)
    message(STATUS "Building tests...")
    enable_testing()
endif()

if(BUILD_APPS)
    message(STATUS "Building applications...")
    add_subdirectory(apps)
endif()

if(BUILD_PLUGINS)
    message(STATUS "Building plugins...")
    add_subdirectory(plugins)
endif()

# ============================================================================
# Installation
# ============================================================================

# Install headers
install(DIRECTORY libs/libmimi-core/include/
    DESTINATION include/mimi/core
    FILES_MATCHING PATTERN "*.h")

install(DIRECTORY libs/libmimi-memory/include/
    DESTINATION include/mimi/memory
    FILES_MATCHING PATTERN "*.h")

install(DIRECTORY libs/libmimi-config/include/
    DESTINATION include/mimi/config
    FILES_MATCHING PATTERN "*.h")

install(DIRECTORY libs/libmimi-tools/include/
    DESTINATION include/mimi/tools
    FILES_MATCHING PATTERN "*.h")

install(DIRECTORY apps/mimi-gateway/include/
    DESTINATION include/mimi/gateway
    FILES_MATCHING PATTERN "*.h")

# pkg-config
configure_file(cmake/mimi.pc.in ${CMAKE_BINARY_DIR}/mimi.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/mimi.pc
    DESTINATION lib/pkgconfig)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "=== Build Summary ===")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build tests:    ${BUILD_TESTS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
message(STATUS "Build apps:     ${BUILD_APPS}")
message(STATUS "Build plugins:  ${BUILD_PLUGINS}")
message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  cmake -B build -DCMAKE_BUILD_TYPE=Release")
message(STATUS "  cmake --build build")
message(STATUS "  cmake --install build")
message(STATUS "")