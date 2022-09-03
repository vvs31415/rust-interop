# Interop with C

This section will take you through a practical example 
of how to integrate Rust code into an existing C codebase.

What you learn here is fundamental to all interop with Rust
since the C ABI (Application Binary Interface) is the only
way to communicate with Rust functions from foreign languages. 
All tools and frameworks that offer integration with other 
programming languages build on the same techniques we use here.

## The tools we'll use

On the Rust side of the fence, we have the FFI (Foreign 
Function Interface). We are provided a few keywords and a
handful of library utilities to expose functions to the 
outside world (through the C ABI).

We will start by making a minimal Rust library, exposing a
function over the FFI, and manually writing the matching
C declarations to be able to call our function.

Further on, we will explore how we can use the 
cbindgen tool to automatically generate function declarations
and types for our C code.

Finally, we'll show how we can make function calls in the
other direction, from C to Rust, using Rust bindgen to
keep the Rust declarations in sync.

## Other helpful resources

* The [FFI chapter](https://doc.rust-lang.org/nomicon/ffi.html)
  of the Rustonomicon offers some technical details on FFI
  and a few practical examples (focusing mainly on calling C from Rust).
* [The Rust FFI Omnibus](http://jakegoulding.com/rust-ffi-omnibus/)
  has a handful of snippets demonstrating how to pass data 
  across the language boundary.
* cbindgen has a 
  [User Guide](https://github.com/eqrion/cbindgen/blob/master/docs.md),
  but it doesn't really show how to use the tool, focusing
  mainly on configuration options.
