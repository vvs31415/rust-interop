#include "modules/file/file.h"
#include "bindings.h"

#include <stdio.h>
#include <string.h>

typedef struct CommandContext {
    Command command;
    bool print_filename;
} CommandContext;

void run_command_for_file(const char* filename, const void* ctx_ptr);
uint64_t do_calculation(Command command, const char* data);
uint64_t count_bytes(const char* data);
void print_result(uint64_t result);
void print_result_with_filename(uint64_t result, const char* filename);

int main(const int argc, const char *argv[]) {
    const Arguments args = parse_args(argc, argv);

    if (args.command == Command_Version) {
        print_version();
        return 0;
    }

    switch (args.file_mode) {
        case FileMode_Normal: {
            CommandContext ctx = { .command = args.command, .print_filename = false };
            run_command_for_file(args.filename, &ctx);
            break;
        }
        case FileMode_CsvList: {
            char* csv = file_to_string(file_read(args.filename));
            CommandContext ctx = { .command = args.command, .print_filename = true };
            csv_for_each_value(csv, run_command_for_file, &ctx);
            file_free_string(csv);
            break;
        }
        case FileMode_CsvMerged: {
            char* csv = file_to_string(file_read(args.filename));
            char* content = csv_merge_files(csv, file_free_string);
            const size_t result = do_calculation(args.command, content);
            csv_free_merged_file(content);
            print_result(result);
            break;
        }
    }

    return 0;
}

void run_command_for_file(const char* filename, const void* ctx_ptr) {
    const CommandContext* ctx = (CommandContext*) ctx_ptr;
    File file = file_read(filename);
    char* str = file_to_string(file);

    const uint64_t result = do_calculation(ctx->command, str);
    if (ctx->print_filename) {
        print_result_with_filename(result, filename);
    } else {
        print_result(result);
    }

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

void print_result_with_filename(const uint64_t result, const char* filename) {
    printf("%lli %s\n", result, filename);
}

uint64_t count_bytes(const char* data) {
    return strlen(data);
}
