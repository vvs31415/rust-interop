use std::ffi::CStr;
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn print_version() {
    println!("count version 2.0.0");
}

#[no_mangle]
pub extern "C" fn count_characters(text: *const c_char) -> u64 {
    let text = unsafe { CStr::from_ptr(text) };
    let text = text.to_str().expect("Unicode conversion failed.");
    text.chars().count().try_into().unwrap()
}
