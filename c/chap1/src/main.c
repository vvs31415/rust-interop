#include "modules/file/file.h"

#include <stdio.h>
#include <string.h>

void run_command_for_file(const char* command, const char* filename);
size_t do_calculation(const char* command, const char* data);
size_t count_bytes(const char* data);
void print_result(size_t result);

int main(const int argc, const char *argv[]) {
    const char* command = argv[1];
    const char* filename = argv[2] ;
    run_command_for_file(command, filename);
}

void run_command_for_file(const char* command, const char* filename) {
    File file = file_read(filename);
    char* str = file_to_string(file);

    const size_t result = do_calculation(command, str);
    print_result(result);

    free(str);
    file_free(file);
}

size_t do_calculation(const char* command, const char* data) {
    if (strcmp(command, "bytes") == 0) {
        return count_bytes(data);
    } else {
        fprintf(stderr, "Unrecognized command: %s\n", command);
        exit(1);
    }
}

void print_result(const size_t result) {
    printf("%li\n", result);
}

size_t count_bytes(const char* data) {
    return strlen(data);
}
