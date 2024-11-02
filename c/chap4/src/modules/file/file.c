#include "file.h"

#include <stdio.h>
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
