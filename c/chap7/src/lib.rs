mod modules {
    mod csv;
    mod file;
}

use std::ffi::CStr;
use std::os::raw::c_char;
use std::{slice, ptr};

#[no_mangle]
pub extern "C" fn print_version() {
    println!("count version 1.0.0");
}

#[no_mangle]
pub extern "C" fn count_characters(text: *const c_char) -> u64 {
    let text = unsafe { CStr::from_ptr(text) };
    let text = text.to_str().expect("Unicode conversion failed.");
    text.chars().count().try_into().unwrap()
}

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
    CsvMerged
}

/// cbindgen:prefix-with-name
#[repr(C)]
#[derive(PartialEq)]
pub enum Command {
    Version,
    Bytes,
    Characters,
}

#[no_mangle]
pub extern "C" fn parse_args(argc: usize, argv: *const *const c_char) -> Arguments {
    let arguments = unsafe { slice::from_raw_parts(argv, argc) };

    let command = arguments.get(1).copied().expect("Missing command.");
    let command = unsafe { CStr::from_ptr(command) }.to_str().unwrap();
    let command = match command {
        "version" => Command::Version,
        "bytes" => Command::Bytes,
        "characters" => Command::Characters,
        _ => panic!("Command not recognized: {command}")
    };

    let filename = arguments.get(2).copied();
    if command != Command::Version && filename.is_none() {
        panic!("Missing filename.");
    }
    let filename = filename.unwrap_or(ptr::null());

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

    Arguments { command, filename, file_mode }
}
