cmake_minimum_required(VERSION 3.22)
project(rust-interop-c)
set(CMAKE_C_STANDARD 17)

set(RUST_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}count${CMAKE_STATIC_LIBRARY_SUFFIX})
set(RUST_LIB_PATH ${CMAKE_SOURCE_DIR}/target/debug/${RUST_LIB_NAME})
set(
        RUST_LIB_SOURCES
        ${CMAKE_SOURCE_DIR}/build.rs
        ${CMAKE_SOURCE_DIR}/src/lib.rs
)

add_custom_command(
        OUTPUT ${RUST_LIB_PATH}
        COMMAND cargo build --manifest-path ${CMAKE_SOURCE_DIR}/Cargo.toml
        DEPENDS ${RUST_LIB_SOURCES}
        USES_TERMINAL
)

add_executable(count src/main.c src/modules/file/file.c ${RUST_LIB_PATH})
target_include_directories(count PRIVATE ${CMAKE_SOURCE_DIR}/target/bridge)
target_link_libraries(count ${RUST_LIB_PATH})
