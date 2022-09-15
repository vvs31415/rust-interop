# Taking references and returning primitives from Rust

Let's add a third command to our application:


<pre>
./count version
./count bytes file.txt
<i><b>./count characters file.txt</b></i>
</pre>

The number of bytes and
characters in a file will often be the same. But a byte can
only represent 256 different values, and to support all the
alphabets out there, and special characters like emojis,
UTF-8 encoded text allows for multiple bytes representing
single characters.

Example: while the string `Kaimū` has five characters, it is six bytes
long because the `ū` is a two-byte character.

C has no built-in support for multibyte characters, so let's
implement the counting function in Rust, where Unicode characters
are first-class citizens.

## Dispatching our new command

This  time we start from the other side by adding a function
declaration for our new function: `count_characters()`. We
also add an `else if` clause to dispatch the new command
in `do_calculation()`:

Filename: src/main.c

```c
// --snip --

void print_version();
uint64_t count_characters(const char* text);

// --snip --

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
```


## Adding a function with parameters and a return type

Filename: src/lib.rs

```rust
use std::ffi::CStr;
use std::os::raw::c_char;

// --snip--

#[no_mangle]
pub extern "C" fn count_characters(text: *const c_char) -> u64 {
    let text = unsafe { CStr::from_ptr(text) };
    let text = text.to_str().expect("Unicode conversion failed.");
    text.chars().count().try_into().unwrap()
}
```

Our new function takes a pointer to C characters, represented
by the `c_char` type from `std::os::raw`. We then turn it into
a CStr with `CStr::from_ptr()`. `CStr` is a handy
utility class that deals with
C string references (i.e. it doesn't try to take ownership
of or free the data).

By converting the `CStr` to a `&str`, we gain access to Rust's
regular string utilities and can proceed with
`text.chars().count()` to get the number of Unicode characters.

The function returns a plain `u64`, since that type matches the
definition of `uint64_t` on the C side.

Let's try it:

```shell
$ cd build
$ cmake --build .
$ echo "Kaimū" > kaimu.txt
$ ./count bytes kaimu.txt
6
$ ./count characters kaimu.txt
5
```

## Which types match up?

Here's a quick reference of the most common Rust primitives
you can pass directly across the FFI boundary.

|   Rust    |             C             |
|:---------:|:-------------------------:|
|   bool    |           bool            |
|  u8 / i8  |     uint8_t / int8_t      |
| u16 / i16 |    uint16_t / int16_t     |
| u32 / i32 |    uint32_t / int32_t     |
| u64 / i64 |    uint64_t / int64_t     |
|    f32    |           float           |
|    f64    |          double           |
|    usize  |          uintptr_t        |

There are also some compatibility types in
[std::os::raw](https://doc.rust-lang.org/std/os/raw/index.html)
for the platform-specific C types. Here's an excerpt:

|        Rust        |       C       |
|:------------------:|:-------------:|
|       c_char       |     char      |
|       c_int        |  signed int   |
|       c_uint       | unsigned int  |
|       c_long       |  signed long  |
|      c_ulong       | unsigned long |
|       c_void       |     void      |

> __NOTE:__ The C standard does not strictly define the length of float and
> double, but in practice, this mapping will work
> on all major platforms. For the paranoid, there's also a
> `c_float` and a `c_double` in `std::os::raw`.
>
> You can read more detailed documentation about the memory layout of scalar types
> [here](https://rust-lang.github.io/unsafe-code-guidelines/layout/scalars.html).

In the next chapter, we will discover how the function
declarations that our C code needs from Rust can be generated
automatically instead of the error-prone and tedious task
of writing and maintaining them by hand.