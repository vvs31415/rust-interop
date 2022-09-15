# Sending callbacks from C

Our application is counting bytes and characters like there's no tomorrow. But imagine
you're writing a book, have one file per chapter, and want to count characters
across them regularly.

Given a CSV file (Comma Separated Values), we want our application to run the
calculation on each file in the list.

An example:

Filename: list.csv
```
chapter1.md,chapter2.md
```

Filename: chapter1.md
```
# Getting started
```

Filename: chapter2.md
```
# Wrapping up
```

Our programs command line interface gets a new flag:

<pre>
./count version
./count bytes list.csv <i><b>[--csv-list]</i></b>
./count characters list.csv <i><b>[--csv-list]</i></b>
</pre>

For the given files we want to be able to run commands like this:

```console
$ ./count bytes list.csv --csv-list
18 chapter1.md
14 chapter2.md
```

## Adding a CSV module

This time around we'll start by making the logic in Rust, and then we'll make a FFI wrapper
separately, with C types and attributes. The core of our module is a function that
takes some string data,
splits it on commas, and calls a callback one time for each of the separated values:

Filename: src/modules/csv.rs
```rust
fn for_each_value(csv: &str, callback: impl Fn(&str)) {
    for value in csv.split(",") {
        callback(value.trim());
    }
}
```

We proceed to add the C interface in a separate module at the beginning of the file:

Filename: src/modules/csv.rs
```rust
mod ffi {
    use std::ffi::{c_void, CStr, CString};
    use std::os::raw::c_char;

    #[no_mangle]
    pub extern "C" fn csv_for_each_value(
        csv: *const c_char,
        c_callback: unsafe extern "C" fn(*const c_char, *const c_void),
        context: *const c_void,
    ) {
        let csv = unsafe { CStr::from_ptr(csv) }.to_str().unwrap();
        super::for_each_value(csv, |value| {
            let value = CString::new(value).unwrap();
            unsafe { c_callback(value.as_ptr(), context) };
        });
    }
}

// --snip--
```

We have separated out the FFI-related type conversions from our logic. Notice that
our exported wrapper function has the same name, but with the module name prefixed:
`csv_for_each_value()`.

The wrapper takes three parameters:

1\. `csv: *const c_char`

The contents of a CSV file, as a char pointer. Just like earlier, we process it from `*const c_char` to `CStr` to `&str`.

2\. `c_callback: unsafe extern "C" fn(*const c_char, *const c_void)`

An external function callback that takes two arguments. The first is a char pointer taking values from our CSV, and the second is a `c_void` pointer. Since C doesn't have closures, a void pointer is a common way
to allow the callee to pass along arbitrary data / state to the callback function.

3\. `context: *const c_void`

The last paramter is a void pointer to the data we want to pass along to the callback.

---

Upon receiving the string data in our wrapper, we pass it along to `CString::new(value).unwrap()`.

`CString` is to `CStr` what `String` is to `&str` - an owned version of a
C string. But why are we creating an owned string when we want to pass
along a reference?

Ideally, we would have liked to do the inverse of what we do on
the receiving end, going from `&str` to `CStr` to `*const c_char`. But
to convert to a C string Rust needs to zero-terminate it by adding a `\0`
at the end of the buffer, thereby requiring ownership of the data.

We then call `value.as_ptr()` to get a `*const c_char` reference to our
temporary zero-terminated string.

## Parsing the new command line argument

In our library's entry point, we need to pull in our new module at the
beginning of the file, and extend the argument parsing to look for and
validate our new `--csv-list` flag:

Filename: src/lib.rs
```rust
mod modules {
    mod csv;
}

// --snip--

#[repr(C)]
pub struct Arguments {
    command: Command,
    filename: *const c_char,
    file_mode: FileMode,
}

/// cbindgen:prefix-with-name
#[repr(C)]
pub enum FileMode {
    Normal,
    CsvList,
}

// --snip--

pub extern "C" fn parse_args(argc: usize, argv: *const *const c_char) -> Arguments {
    // --snip--

    let file_mode = if let Some(csv_flag) = arguments.get(3).copied() {
        let csv_flag = unsafe { CStr::from_ptr(csv_flag) }.to_str().unwrap();
        match csv_flag {
            "--csv-list" => FileMode::CsvList,
            _ => panic!("CSV flag not recognized: {csv_flag}")
        }
    } else {
        FileMode::Normal
    };

    Arguments { command, filename, file_mode }
}
```

Let's not forget to add the new file to our CMake config:

Filename: CMakeLists.txt
```cmake
# --snip--

set(
        RUST_LIB_SOURCES
        ${CMAKE_SOURCE_DIR}/build.rs
        ${CMAKE_SOURCE_DIR}/src/lib.rs
        ${CMAKE_SOURCE_DIR}/src/modules/csv.rs
)

# --snip--
```

## Re-wiring main.c

We also have to adapt our entry point to the new realities. First, we
change `run_command_for_file` so that we'll be able to use it as a
callback. We flip around the two parameters it takes, and substitute
`Command` for a `CommandContext`, which is the state we soon will pass around
as a void pointer:

Filename: src/main.c
```c
// --snip--

typedef struct CommandContext {
    Command command;
} CommandContext;

void run_command_for_file(const char* filename, const void* ctx_ptr);

// --snip--

void run_command_for_file(const char* filename, const void* ctx_ptr) {
    const CommandContext* ctx = (CommandContext*) ctx_ptr;
    File file = file_read(filename);
    char* str = file_to_string(file);

    const uint64_t result = do_calculation(ctx->command, str);
    print_result(result);

    free(str);
    file_free(file);
}

// --snip--
```

We also have to rewrite the `main()`-function to adhere to our new
`file_mode` property. If we have `FileMode_Normal`, we just wrap the
command in a `CommandContext`, and call `run_command_for_file` the same
way we always did.

If we have `FileMode_CsvList`, we read the contents of the CSV-file
to a string, and pass it on to the Rust-defined `csv_for_each_value()`.

```c
// --snip--

int main(const int argc, const char *argv[]) {
    const Arguments args = parse_args(argc, argv);

    if (args.command == Command_Version) {
        print_version();
        return 0;
    }

    switch (args.file_mode) {
        case FileMode_Normal: {
            CommandContext ctx = { .command = args.command };
            run_command_for_file(args.filename, &ctx);
            break;
        }
        case FileMode_CsvList: {
            char* csv = file_to_string(file_read(args.filename));
            CommandContext ctx = { .command = args.command };
            csv_for_each_value(csv, run_command_for_file, &ctx);
            free(csv);
            break;
        }
    }

    return 0;
}

// --snip--

```

Let's test what we've got so far:

```console
$ cmake ..
$ cmake --build .
$ echo "# Getting started" > chapter1.md
$ echo "# Wrapping up" > chapter2.md
$ echo "chapter1.md,chapter2.md" > list.csv
$ ./count bytes list.csv --csv-list
18
14
```

While we do get the count for each of the files, it's not very easy to see
which count is for which file.

As a finishing touch, we'll add the filename for each count, if we are
in CSV-mode:

```c
// --snip--

typedef struct CommandContext {
    Command command;
    bool print_filename;
} CommandContext;

// --snip--

void print_result_with_filename(uint64_t result, const char* filename);

// --snip--

int main(const int argc, const char *argv[]) {

    // --snip--

    switch (args.file_mode) {
        case FileMode_Normal: {
            CommandContext ctx = { .command = args.command, .print_filename = false };
            // --snip--
        }
        case FileMode_CsvList: {
            char* csv = file_to_string(file_read(args.filename));
            // --snip--
        }
    }

    // --snip--
}

// --snip--

void run_command_for_file(const char* filename, const void* ctx_ptr) {
    // --snip--

    const uint64_t result = do_calculation(ctx->command, str);
    if (ctx->print_filename) {
        print_result_with_filename(result, filename);
    } else {
        print_result(result);
    }

    // --snip--
}

// --snip--

void print_result_with_filename(const uint64_t result, const char* filename) {
    printf("%s: %lli\n", filename, result);
}

// --snip--
```

Our void pointer lets us add a new property to the `CommandContext` without
touching any code on the Rust side.

We also have to add a `print_result_with_filename()` function, and
selecteviley execute it in `run_command_for_file()`.

A final test is in order:

```console
$ cmake --build .
$ ./count bytes list.csv --csv-list
18 chapter1.md
14 chapter2.md
```

This output is much easier to digest.

In this section we have showed how you can pass control from C to
Rust and back again. This is a very powerful technique, and in the next
section we will give you even more control, by enabling you to call
any C function directly from Rust, not just callbacks.
