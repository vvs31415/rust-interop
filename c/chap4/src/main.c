#include "modules/file/file.h"
#include "bindings.h"

#include <stdio.h>
#include <string.h>

void run_command_for_file(const char* command, const char* filename);
uint64_t do_calculation(const char* command, const char* data);
uint64_t count_bytes(const char* data);
void print_result(uint64_t result);

int main(const int argc, const char *argv[]) {
    const char* command = argv[1];

    if (strcmp(command, "version") == 0) {
        print_version();
        return 0;
    }

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
    } else if (strcmp(command, "characters") == 0) {
        return count_characters(data);
    } else {
        fprintf(stderr, "Unrecognized command: %s\n", command);
        exit(1);
    }
}

void print_result(const uint64_t result) {
    printf("%llu\n", result);
}

uint64_t count_bytes(const char* data) {
    return strlen(data);
}
