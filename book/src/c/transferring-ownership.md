# Transferring ownership of data

In the previous section we added the ability to run calculations on a
comma separated list of filenames, counting characters in e.g. a list
of chapters in a book.

But what if you want to add all those counts together, counting the
number of characters in the entire book? We'll add a new `--csv-merged` flag
that merges all the files, and run a count on the result:

<pre>
./count version
./count bytes list.csv [--csv-list | <i><b>--csv-merged</i></b>]
./count characters list.csv [--csv-list | <i><b>--csv-merged</i></b>]
</pre>

Supplied with the same files as the previous chapter, it should work like this:

```console
$ ./count characters list.csv --csv-merged
32
```

To make things interesting we will implement this with a Rust function that
takes ownership of the incoming CSV data, and that returns an owned string
of the merged file content. We want something like this:

```rust
pub extern "C" fn csv_merge_files(csv: *mut c_char) -> *mut c_char;
```

Transferring ownership means that `csv` should be free'd by `csv_merge_files`,
and preferably not be used anymore by the caller. The caller also has to
make sure the returned string is dropped. We'll get back to how we deal with
this a little bit later.

The new function will also have to read the files before it can merge them
together. Our application already has a `file`-module written in C, and we're going to reuse that logic. Calling C
functions from Rust in the topic of the next section, so we'll make a mock module to help us in the meantime:

Filename: src/modules/file/mod.rs
```rust
pub struct File(String);

impl File {
    pub fn to_str(&self) -> &str {
        if self.0 == "chapter1.md" {
            "# Getting started\n"
        } else if self.0 == "chapter2.md" {
            "# Wrapping up\n"
        } else {
            panic!("No content defined for file: {}", self.0);
        }
    }
}

pub fn read_file(filename: &str) -> File {
    File(filename.to_owned())
}
```

Add the new module to our library:

Filename: src/lib.rs
```rust
mod modules {
    mod csv;
    mod file;
}

// --snip--
```

Before we forget, let's add the new file to the `RUST_LIB_SOURCES` list in CMakeLists.txt: `${CMAKE_SOURCE_DIR}/src/modules/file/mod.rs`

> __NOTE__
>
> It would of course have been easier to rewrite the `file_read()`
> function completely in Rust, but with this project we want to show how you can reuse pre-existing
> code. When gradually porting a real world application, this is a useful skill.

## Merging the files

The file merging logic is rather simple. We read each file, and push
the text content to a `merged` string:

Filename: src/modules/csv.rs
```rust
use crate::modules::file;

// --snip--

fn merge_files(csv: &str) -> String {
    let mut merged = String::new();
    for value in csv.split(",") {
        let file = file::read_file(value.trim());
        merged.push_str(file.to_str());
    }
    merged
}
```

We also need an FFI wrapper to be able to call it from C:

Filename: src/modules/csv.rs
```rust
// --snip--

mod ffi {
    // --snip--

    #[no_mangle]
    pub extern "C" fn csv_merge_files(csv: *mut c_char) -> *mut c_char {
        let csv_str = unsafe { CStr::from_ptr(csv) }.to_str().unwrap();
        let merged = super::merge_files(&csv_str);
        CString::new(merged).unwrap().into_raw()
    }
}
```

`CString::into_raw` is the only thing new here. It gives us a `*mut char` and
consumes the original `CString`. The last part is important - to stop the `CString`
destructor from immediately clearing the string upon returning. We transfer
ownership of the data to C.

## Putting the pieces together

To be able to pass along the new flag, we need to update our command line parser:

Filename: src/lib.rs
```rust
// --snip--

/// cbindgen:prefix-with-name
#[repr(C)]
pub enum FileMode {
    Normal,
    CsvList,
    CsvMerged
}

#[no_mangle]
pub extern "C" fn parse_args(argc: usize, argv: *const *const c_char) -> Arguments {
    // --snip--

    let file_mode = if let Some(csv_flag) = arguments.get(3).copied() {
        let csv_flag = unsafe { CStr::from_ptr(csv_flag) }.to_str().unwrap();
        match csv_flag {
            "--csv-list" => FileMode::CsvList,
            "--csv-merged" => FileMode::CsvMerged,
            _ => panic!("CSV flag not recognized: {csv_flag}")
        }
    } else {
        FileMode::Normal
    };

    // --snip--
}
```

And we also need to update our `main()`-function to use the new function:

Filename: src/main.c
```c
// --snip--

int main(const int argc, const char *argv[]) {
    // --snip--

    switch (args.file_mode) {
        // --snip--
        case FileMode_CsvMerged: {
            char* csv = file_to_string(file_read(args.filename));
            char* content = csv_merge_files(csv);
            const size_t result = do_calculation(args.command, content);
            print_result(result);
            break;
        }
    }

    // --snip--
}
```

We read the CSV file, and pass it onto `csv_merge_files()`. Since all the
file reading is already done once we have the merged content, we skip
right ahead to `do_calculation()` and `print_result()`.

We can go ahead and test the new functionality:

```console
$ cmake ..
$ cmake --build .
$ echo "chapter1.md,chapter2.md" > list.csv
$ ./count characters list.csv --csv-merged
32
```

This is indeed the output we expected. The files chapter1.md and chapter2.md
have 32 charactes in total.

But while we passed ownership of data in both directions, we never did any
cleanup. For a small program like this it wouldn't really matter, as the
operating system will reclaim the memory when the process finishes. It is
however a good practice to free all the memory you use, and in a long running
or data intensive application it is strictly necessary, lest you'll run
out of memory eventually.

## Freeing up memory from the other side

We cannot reliably free C allocated memory from Rust, and vice versa. It
has to be free'd / dropped where it was created. In practice this means
that if you transfer ownership of heap allocated data across the language border,
you also have to provide a way to deallocate that data.

Let's start with the `csv` argument that we pass to `csv_merge_files()`.
Alongside our data, we require a callback function, `free_csv`, that
will free it. We use this callback to free `csv` as soon as we're done with it:

Filename: src/modules/csv.rs
```rust
mod ffi {
    // --snip--

    #[no_mangle]
    pub extern "C" fn csv_merge_files(
        csv: *mut c_char,
        free_csv: unsafe extern "C" fn(*mut c_char),
    ) -> *mut c_char {
        let csv_str = unsafe { CStr::from_ptr(csv) }.to_str().unwrap();
        unsafe { free_csv(csv); }
        let merged = super::merge_files(&csv_str);
        CString::new(merged).unwrap().into_raw()
    }
}

// --snip--
```

We need to update `main.c` to pass along the callback:

Filename: src/main.c
```c
// --snip--

int main(const int argc, const char *argv[]) {
    // --snip--

    switch (args.file_mode) {
        // --snip--
        case FileMode_CsvMerged: {
            // --snip--
            char* content = csv_merge_files(csv, file_free_string);
            // --snip--
        }
    }
}

// --snip--
```

After these changes, `csv_merge_files()` is truly the responsible owner
of `csv`. But it also returns data, that we should deal with. We start
by adding a function for C to free the data:

Filename: src/lib.rs
```rust
mod ffi {
    //--snip--

    #[no_mangle]
    pub extern "C" fn csv_free_merged_file(merged: *mut c_char) {
        unsafe { CString::from_raw(merged) };
    }
}

//--snip--
```

At first glance it might look like this code is doing nothing. But just
like `CString::into_raw()` transfers ownership away from an object,
`CString::from_raw()` will reclaim it. Since the `CString` immediately
goes out of scope, it's destructor will be called, and the data will
be deallocated. Let's clean up after ourselves in `main.c`:

Filename: src/main.c
```c
// --snip--

int main(const int argc, const char *argv[]) {
    // --snip--

    switch (args.file_mode) {
        // --snip--

        case FileMode_CsvMerged: {
            char* csv = file_to_string(file_read(args.filename));
            char* content = csv_merge_files(csv, file_free_string);
            const size_t result = do_calculation(args.command, content);
            csv_free_merged_file(content);
            print_result(result);
            break;
        }
    }

    // --snip--
}
```

A call to `csv_free_merged_file()` has been added, and all memory should
now explicitly have been taken care of.

## References vs owned data

When you transfer data to or from C, you generally have to read the
documentation to know who's the owner of the data after the function
call has happened.

We've created a mini-reference below for how you can pass common types of heap
data through FFI.

### Passing heap data from C to Rust

| Description                          | Type                                                  |
|:-------------------------------------|:------------------------------------------------------|
| *const c_char -> &str (reference)    | unsafe { CStr::from_ptr(char_ptr) }.to_str().unwrap() |
| *mut c_char -> &str (owned)          | unsafe { CStr::from_ptr(char_ptr) }.to_str().unwrap() | 
| *const u8 + len -> &[u8] (reference) | unsafe { slice::from_raw_parts(u8_ptr, len) }         | 
| *const u8 + len -> &[u8] (owned)     | unsafe { slice::from_raw_parts(u8_ptr, len) }         | 

Remember that if you transfer ownership, you should also supply a callback or a
function to free the memory.

### Passing heap data from Rust to C

| Description                             | Type                                                                                                                         | Deallocation                                     |
|:----------------------------------------|:-----------------------------------------------------------------------------------------------------------------------------|:-------------------------------------------------|
| &str -> *const c_char (reference)       | CString::new(rust_str).unwrap().as_ptr()                                                                                     |                                                  |
| String -> *mut c_char (owned)           | CString::new(rust_str).unwrap().into_raw()                                                                                   | unsafe { CString::from_raw(char_ptr) };          |
| &[u8] -> *const u8 (reference)          | u8_slice.as_ptr()                                                                                                            |                                                  | 
| Vec<u8> -> *mut u8 (owned)              | vec.shrink_to_fit();<br/>let mut vec = mem::ManuallyDrop::new(vec);<br/>let u8_ptr = vec.as_mut_ptr()<br/>let len = v.len(); | unsafe { Vec::from_raw_parts(u8_ptr, len, len) } |
| RustType -> *const RustType (reference) | &rust_obj as *const RustType                                                                                                 |                                                  |
| RustType -> *mut RustType (owned)       | Box::into_raw(Box::new(rust_obj));                                                                                           | unsafe { Box::from_raw(ptr) };                   |


In the next section we will give you even more control, by enabling you to call
any C function directly from Rust, not just callbacks.
