# Calling a Rust function from C

> **_NOTE:_** 
>
> If you're going to type the code as you follow along,
> you should start by making a copy of the initial C application in 
> the first chapter.
> ```shell
> $ cp -R rust-interop/c/chap1 count
> $ cd count
> ```
> The final result of this chapter is also available 
> [here](https://github.com/paandahl/rust-interop/tree/main/c/chap2).

Let's dive straight in and use cargo to initialize a Rust library
directly in our project folder:

```shell
$ cp -R rust-interop/c/chap1 count
$ cd count
$ cargo init --lib --name count
```

> Another common approach is to keep the Rust library in a separate folder.
> If it is a self-contained library, that is a good way to handle it, but
> in our project, we want to demonstrate a mixed-language unit of code with
> dependencies in both directions. The upside to this approach, is that
> you can gradually introduce Rust to your codebase.

We also have to tell cargo that we intend to produce a static system library
(to be linked into our C binary):

Filename: Cargo.toml

```toml
# --snip--

[lib]
crate-type = ["staticlib"]

[dependencies]
```

## Making the function

Let's add a `version`-command that works like this:

```shell
$ ./count version
1.0.0
```

We start by replacing the contents of lib.rs with our new function:

Filename: src/lib.rs

```rust
#[no_mangle]
pub extern "C" fn print_version() {
    println!("count version 1.0.0");
}
```
There's a couple of noteworthy things to point out here:

### Name mangling

Normally the Rust compiler rewrites the function names behind the scenes, to include 
details such as the crate name and the containing module.

Our function name will turn into something like this:

```
__ZN5count11get_version17h0b87bf00f9702f77E
```

C has no concept of crates and modules, so we need to add `#[no_mangle]`, to be able
to resolve the function simply as `print_version()`.

With mangling disabled, all exported function names need to be unique.

### ABI (Application Binary Interface)

We also add `extern "C"` to the function, to allow it to be called with your platforms
C ABI. This specifies how data is laid out in memory, and how functions are called.

The C ABI is the lingua franca of application binaries, and our only line of 
communication to the non-Rust world. All interop between Rust 
and other languages is based on calling conventions from C.

> **_NOTE:_** We have also added the `pub` keyword to our function. Although strictly
> not necessary (C has no concept of private functions), it's good to be explicit about
> the fact that our function is part of the library's public interface.

### Building the Rust library

We can test that our Rust library builds with cargo:

```shell
$ cargo build
Compiling count v0.1.0 (/path/to/count)
 Finished dev [unoptimized + debuginfo] target(s) in 0.56s
```

The static library should now be ready at `target/debug/libcount.a` (Unix-like) or
`target/debug/count.lib` (Windows). 

## Calling our function from C

We tell our C application that the function `get_version()` exists, by manually
writing a function declaration. Then we call that function if `command` is equal
to `"version"`. We make sure to do this before the file name is parsed, since there
is no file involved.

Filename: src/main.c

```c
// --snip--

void print_version();

int main(const int argc, const char *argv[]) {
    const char* command = argv[1];

    if (strcmp(command, "version") == 0) {
        print_version();
        return 0;
    }

    // --snip--
```

We also need to amend our CMake configuration to link to the Rust library. Add the
following lines to the bottom of the file:

Filename: CMakeLists.txt

```cmake
# --snip--

set(RUST_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}count${CMAKE_STATIC_LIBRARY_SUFFIX})
set(RUST_LIB_PATH ${CMAKE_SOURCE_DIR}/target/debug/${RUST_LIB_NAME})
target_link_libraries(count ${RUST_LIB_PATH})
```

We construct the library name in a platform independent way (libcount.a or
count.lib), and link it to our executable.

Let's build and run the program:

```shell
$ mkdir -p build
$ cd build
$ cmake ..
$ cmake --build .
$ ./count version
count version 1.0.0
```

Our C application has been extended with Rust code!

## Harmonizing CMake & Cargo

The current setup works, but if we make changes on the Rust
side, we have to manually trigger the cargo build before
the CMake build.

Let's rewrite our CMake configuration so that it automatically
rebuilds the Rust code upon changes:

Filename: CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.22)
project(rust-interop-c)
set(CMAKE_C_STANDARD 17)

set(RUST_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}count${CMAKE_STATIC_LIBRARY_SUFFIX})
set(RUST_LIB_PATH ${CMAKE_SOURCE_DIR}/target/debug/${RUST_LIB_NAME})

add_custom_command(
        OUTPUT ${RUST_LIB_PATH}
        COMMAND cargo build --manifest-path ${CMAKE_SOURCE_DIR}/Cargo.toml
        DEPENDS ${CMAKE_SOURCE_DIR}/src/lib.rs
        USES_TERMINAL
)

add_executable(count src/main.c src/modules/file/file.c ${RUST_LIB_PATH})
target_link_libraries(count ${RUST_LIB_PATH})
```

We use `add_custom_command()` to define our library as an 
`OUTPUT` of the cargo build, that `DEPENDS` on changes
to the content of `lib.rs`. 

Adding the library to the 
source list in `add_executable()` will evaluate our custom
command before linking happens.

From the `build`-folder, we can now re-configure CMake:

```shell
$ cmake ..
```

And subsequent builds should recompile the Rust library 
if there are new changes:

```shell
$ cmake --build .
```

### What's next?

The facilities we use to bind Rust to other languages are often
referred to as the Rust FFI (Foreign Function Interface). Now
that we have  a working configuration, we will see how we can
send and receive data across the FFI boundary.
