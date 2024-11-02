# Introducing our C application

Our project will start out with a simple C application
called `count`, which we'll gradually extend with Rust
code.

The application relies on an internal C module (`file.h` / `file.c`) to read
a file from disk and convert it to a string.

Based on a user-supplied command, the application then runs
a calculation on the file text. Initially,
`bytes` is the only supported command, but we will soon add
other options.

## Building and running the code

The source code for each chapter is available in our GitHub
repository, and you'll find the initial code here.

Start by checking out the code and preparing a build directory:

```shell
$ git checkout git://rust-interop
$ cd rust-interop/c/chap1
$ mkdir build && cd build
```

Configure & run the build, create some test data, and run
the program:

```shell
$ cmake .. && make
$ echo -n "Rust interop" > test.txt
$ ./count bytes test.txt
12
```

## Source code

As we go along, we will try to add new functionality to this
application, implementing most of the new logic in Rust.

The initial code is included in full below
if you want to familiarize yourself with it:

`src/main.c` - the entry point

```c
#include "modules/file/file.h"

#include <stdio.h>
#include <string.h>

void run_command_for_file(const char* command, const char* filename);
uint64_t do_calculation(const char* command, const char* data);
uint64_t count_bytes(const char* data);
void print_result(const uint64_t result);

int main(const int argc, const char *argv[]) {
    const char* command = argv[1];
    const char* filename = argv[2] ;
    run_command_for_file(command, filename);
    return 0;
}

void run_command_for_file(const char* command, const char* filename) {
    File file = file_read(filename);
    char* str = file_to_string(file);

    const uint64_t result = do_calculation(command, str);
    print_result(result);

    file_free_string(str);
    file_free(file);
}

uint64_t do_calculation(const char* command, const char* data) {
    if (strcmp(command, "bytes") == 0) {
        return count_bytes(data);
    } else {
        fprintf(stderr, "Unrecognized command: %s\n", command);
        exit(1);
    }
}

uint64_t count_bytes(const char* data) {
    return strlen(data);
}

void print_result(const uint64_t result) {
    printf("%llu\n", result);
}
```

`src/modules/file.h ` - The file module interface

```c
#pragma once

#include <stdlib.h>
#include <stdint.h>

typedef struct File {
    const char* filename;
    uint8_t* data;
    size_t length;
} File;

File file_read(const char* filename);
char* file_to_string(File file);
void file_free(File file);
void file_free_string(char* file_string);
```

`src/modules/file.c ` - The file module implementation

```c
#include "file.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

File file_read(const char* filename) {
    FILE* file_handle = fopen (filename, "rb");
    if (!file_handle) {
        printf("Could not open file: '%s'\n", filename);
        exit(1);
    }
    fseek(file_handle, 0, SEEK_END);
    long length = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);
    uint8_t* data = (uint8_t*)malloc(length);
    fread(data, 1, length, file_handle);
    fclose(file_handle);
    return (File) {
            filename,
            data,
            length
    };
}

char* file_to_string(const File file) {
    char* str = (char*)malloc(file.length + 1);
    memcpy(str, file.data, file.length);
    str[file.length] = '\0';
    return str;
}

void file_free(const File file) {
    free(file.data);
}

void file_free_string(char* file_string) {
    free(file_string);
}
```

`CMakeLists.txt` - build configuration

```cmake
cmake_minimum_required(VERSION 3.22)
project(rust-interop-c-chap1)
set(CMAKE_C_STANDARD 17)

add_executable(count src/main.c src/modules/file/file.c)
```