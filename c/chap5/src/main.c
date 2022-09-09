#include "modules/file/file.h"
#include "bindings.h"

#include <stdio.h>
#include <string.h>

void run_command_for_file(Command command, const char* filename);
uint64_t do_calculation(Command command, const char* data);
uint64_t count_bytes(const char* data);
void print_result(uint64_t result);

int main(const int argc, const char *argv[]) {
    const Arguments args = parse_args(argc, argv);

    if (args.command == Command_Version) {
        print_version();
        return 0;
    }

    run_command_for_file(args.command, args.filename);
    return 0;
}

void run_command_for_file(const Command command, const char* filename) {
    File file = file_read(filename);
    char* str = file_to_string(file);

    const uint64_t result = do_calculation(command, str);
    print_result(result);

    free(str);
    file_free(file);
}

uint64_t do_calculation(const Command command, const char* data) {
    switch (command) {
        case Command_Bytes:
            return count_bytes(data);
        case Command_Characters:
            return count_characters(data);
        default:
            fprintf(stderr, "Unrecognized command: %i\n", command);
            exit(1);
    }
}

void print_result(const uint64_t result) {
    printf("%llu\n", result);
}

uint64_t count_bytes(const char* data) {
    return strlen(data);
}
