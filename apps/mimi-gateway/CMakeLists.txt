cmake_minimum_required(VERSION 3.16)

# ============================================================================
# MimiClaw Gateway Application
# ============================================================================

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(OPENSSL REQUIRED openssl)

# Gateway executable
add_executable(mimi-gateway
    src/main.c
    src/gateway.c
    src/telegram.c
    src/websocket.c
)

# Link libraries
target_link_libraries(mimi-gateway PRIVATE
    mimi-core
    mimi-memory
    mimi-config
    mimi-tools
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    pthread
)

# Include directories
target_include_directories(mimi-gateway PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/libmimi-core/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-memory/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-config/include
    ${CMAKE_SOURCE_DIR}/libs/libmimi-tools/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

# Compile definitions
target_compile_definitions(mimi-gateway PRIVATE
    -D_XOPEN_SOURCE=600
)

# Installation
install(TARGETS mimi-gateway DESTINATION bin)

# Configuration file template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/gateway.json.in
    ${CMAKE_BINARY_DIR}/apps/mimi-gateway/gateway.json
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/apps/mimi-gateway/gateway.json
    DESTINATION etc/mimi
    RENAME gateway.json
)