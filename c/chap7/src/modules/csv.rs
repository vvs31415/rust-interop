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

    #[no_mangle]
    pub extern "C" fn csv_merge_files(
        csv: *mut c_char,
        free_csv: unsafe extern "C" fn(*mut c_char),
    ) -> *mut c_char {
        let csv_str = unsafe { CStr::from_ptr(csv) }.to_str().unwrap();
        let merged = super::merge_files(&csv_str);
        unsafe { free_csv(csv); }
        CString::new(merged).unwrap().into_raw()
    }

    #[no_mangle]
    pub extern "C" fn csv_free_merged_file(merged: *mut c_char) {
        unsafe { CString::from_raw(merged) };
    }
}

use crate::modules::file;

fn for_each_value(csv: &str, callback: impl Fn(&str)) {
    for value in csv.split(",") {
        callback(value.trim());
    }
}

fn merge_files(csv: &str) -> String {
    let mut merged = String::new();
    for value in csv.split(",") {
        let file = file::read_file(value.trim());
        merged.push_str(file.to_str());
    }
    merged
}
