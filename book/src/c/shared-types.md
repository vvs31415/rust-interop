# Shared structs and enums

The current program works if you pass the correct command
line arguments, but if a user tries to
call it without any arguments it will fail with a segfault:

```console
$ ./count
zsh: segmentation fault  ./count
```

To improve upon this, we'll add error handling by validating the command line arguments passed in by the user.

Let's start by adding a struct to represent the command line
arguments, and an enum to hold the chosen command.

Filename: src/lib.rs
```rust
// --snip--

pub struct Arguments {
    command: Command,
    filename: *const c_char,
}

#[derive(PartialEq)]
pub enum Command {
    Version,
    Bytes,
    Characters,
}
```

We use `*const c_char` instead of the built-in `String` type because
we intend to read the filename property from our C code.

Let's define the interface of the function that will parse the arguments:

Filename: src/lib.rs
```rust
// --snip--

#[no_mangle]
pub extern "C" fn parse_args(argc: usize, argv: *const *const c_char) -> Arguments {
    let arguments = unsafe { slice::from_raw_parts(argv, argc) };

    // --snip--

    Arguments { command, filename }
}
```

We define `argc` and `argv` to mimic the parameters our entry point takes in `main.c`.
There's a subtle difference in that `usize` translates to `uintptr_t`, not `size_t` as one
might have thought. Although not guaranteed by the C standard, `size_t` is smaller or
equal to `unitptr_t` in all known implementations, so we should be fine.

The two parameters are then upgraded to a slice (`&[*const c_char]`) for easier access. This is
unsafe because we have to promise the compiler that `argv` contains exactly `argc` elements. Once the slice
is created, its elements can be accessed safely.

Let's see how we can convert the second element to a `Command`:

Filename: src/lib.rs
```rust
// --snip--

#[no_mangle]
pub extern "C" fn parse_args(argc: usize, argv: *const *const c_char) -> Arguments {
    // --snip--

    let command = arguments.get(1).copied().expect("Missing command.");
    let command = unsafe { CStr::from_ptr(command) }.to_str().unwrap();
    let command = match command {
        "version" => Command::Version,
        "bytes" => Command::Bytes,
        "characters" => Command::Characters,
        _ => panic!("Command not recognized: {command}")
    };

    // --snip--
}
```

We use three separate steps to gradually turn `command` into the type we want it to be:

1. We `.get()` the second element of the `arguments` slice, make a copy of the referenced
   pointer, and trigger a `panic!` if it's missing. If everything worked as expected,
   we now have a `*const c_char`.
2. The pointer is then fed in to `Cstr::from_ptr`, which is unsafe because we have to promise
   that our string is zero-terminated (ends with a `\0` character). We then convert it
   to an `&str`.
3. Lastly, we use a match expression to map it to the correct `Command`.

The filename also needs to be handled:

Filename: src/lib.rs
```rust
// --snip--

#[no_mangle]
pub extern "C" fn parse_args(argc: usize, argv: *const *const c_char) -> Arguments {
    // --snip--

    let filename = arguments.get(2).copied();
    if filename.is_none() && command != Command::Version {
        panic!("Missing filename.");
    }
    let filename = filename.unwrap_or(ptr::null());

    // --snip--
}
```

We only require the filename to be present if the command is something else than "version".
In the other case, we create a null pointer by calling `ptr::null()`.

## Inspecting the generated bindings

Let's have a look at the generated C bindings by running a `cargo build`:

Filename: target/bridge/bindings.h
```c
// --snip--

typedef struct Arguments Arguments;

void print_version(void);

uint64_t count_characters(const char *text);

struct Arguments parse_args(uintptr_t argc, const char *const *argv);
```

We have a problem! The `Arguments` is defined as an opaque struct. So while we
can get a handle on it, we won't be able to access any of its fields in C.

The reason that cbindgen went for the opaque type is that Rust's default data layout is
incompatible with C. Luckily, Rust has an attribute called `repr`, that will fix that
problem for us:

Filename: src/lib.rs
```rust
// --snip --

#[repr(C)]
pub struct Arguments {
    command: Command,
    filename: *const c_char,
}

#[repr(C)]
#[derive(PartialEq)]
pub enum Command {
    Version,
    Bytes,
    Characters,
}

// --snip --
```

With this fix in hand, we can do another `cargo build` and look at cbindgen's output.

Filename: target/bridge/bindings.h
```c
// --snip--

typedef enum Command {
  Version,
  Bytes,
  Characters,
} Command;

typedef struct Arguments {
  enum Command command;
  const char *filename;
} Arguments;

// --snip--
```

This looks much better! But let's ask cbindgen to prefix our enum values to reduce the
risk of a name colission:

Filename: src/lib.rs
```rust
// --snip --

/// cbindgen:prefix-with-name
#[repr(C)]
#[derive(PartialEq)]
pub enum Command {
    Version,
    Bytes,
    Characters,
}

// --snip --
```

A rebuild should change the Command enum to this:

Filename: target/bridge/bindings.h
```c
// --snip--

typedef enum Command {
  Command_Version,
  Command_Bytes,
  Command_Characters,
} Command;

// --snip--
```

That looks much better! Let's take it for a spin.

## Using our argument parser

Using our new argument parsing function is fairly straightforward:

Filename: src/main.c
```c
// --snip--

int main(const int argc, const char *argv[]) {
    const Arguments args = parse_args(argc, argv);

    if (args.command == Command_Version) {
        print_version();
        return 0;
    }

    run_command_for_file(args.command, args.filename);
    return 0;
}

// --snip--
```

This won't work quite yet -
we also have to update the command type in `run_command_for_file` and `do_calculation`:

Filename: src/main.c
```c
// --snip--

void run_command_for_file(Command command, const char* filename);
uint64_t do_calculation(Command command, const char* data);

// --snip--

void run_command_for_file(const Command command, const char* filename) {
    // --snip--
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

// --snip--
```

Now that `command` is an enum, we can get rid of all the calls to `strcmp`,
and we can also opt to use a `switch` statement.

Build and run the application to see the input validation in action:

```console
$ cmake --build .
$ ./count
thread '<unnamed>' panicked at 'Missing command.', src/lib.rs:36:45
note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace
fatal runtime error: failed to initiate panic, error 5
zsh: abort      ./count
```

Panicking is a very crude form of error handling, but at least we give the user
a hint that there's a `'Missing command.'`.

Sharing structs across the FFI boundary is a useful technique when your function
is dealing with more than simple values. In the next section we will show how you
can send function callbacks from C to Rust.
