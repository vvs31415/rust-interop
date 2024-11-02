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
