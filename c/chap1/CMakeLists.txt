cmake_minimum_required(VERSION 3.22)
project(rust-interop-c-chap1)
set(CMAKE_C_STANDARD 17)

add_executable(count src/main.c src/modules/file/file.c)
