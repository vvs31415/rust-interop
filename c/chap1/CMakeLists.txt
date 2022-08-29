cmake_minimum_required(VERSION 3.22)
project(rust-interop-c)
set(CMAKE_C_STANDARD 17)

add_executable(count src/main.c src/modules/file/file.c)
