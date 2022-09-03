# Using cbindgen to generate C headers

In the previous chapters we manually wrote the C declarations
corresponding to our Rust FFI functions. There's at least two
good reasons to automate this process:

* Writing duplicate C declarations is tedious
* An automated system leaves much less room for error

cbindgen to the rescue! Add it as a build-dependcy:

Filename: Cargo.toml

```toml
# --snip--

[build-dependencies]
cbindgen = "0.24"
```

We can customize our Cargo build by adding a build.rs to our project root:

Filename: build.rs
```rust
use cbindgen::Language;
use std::env;

fn main() {
    println!("cargo:rerun-if-changed=src/lib.rs");

    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    cbindgen::Builder::new()
        .with_crate(manifest_dir)
        .with_language(Language::C)
        .generate()
        .expect("Unable to generate C bindings")
        .write_to_file("target/bridge/bindings.h");
}
```

On every change to lib.rs we generate bindings.h. This will crawl through our Rust
code and generate C declarations for all FFI-exported types and functions.

You can have a look at the output by running `cargo build` and checking out
`target/bridge/bindings.h`.

Let's update our CMake build correspondingly:

Filename: CMakeLists.txt
```cmake
# --snip--

set(
        RUST_LIB_SOURCES
        ${CMAKE_SOURCE_DIR}/build.rs
        ${CMAKE_SOURCE_DIR}/src/lib.rs
)

add_custom_command(
        # --snip--

        DEPENDS ${RUST_LIB_SOURCES}

        # --snip--
)

# --snip--

target_include_directories(count PRIVATE ${CMAKE_SOURCE_DIR}/target/bridge)
```

We've added a list of all the Rust sources instead of listing
files directly in the DEPENDS clause. This will scale better as
our application continues to grow.

We also included the directory of our generated C headers, so that
the compiler will know where to look for them.

The last step is to include the header in our C code, and to remove
our manually written declarations.

Filename: src/main.c
```c
#include "modules/file/file.h"
#include "bindings.h"

// --snip--

// Remove: void print_version();
// Remove: uint64_t count_characters(const char* text);

// --snip--
```

That's all. If you reconfigure & rebuild the CMake project, the
application should still work exactly like it used to.

This might seem like a lot of work to get rid of two lines of
code, but as our application grows, this investment will pay off.

Let's see how we can leverage this new setup to pass custom
structs across the language boundary.

> **NOTE:** In a later chapter we will see how we can use bindgen (as opposed
> to cbindgen) to generate declarations in the opposite direction -
> allowing us to call C code from Rust.

