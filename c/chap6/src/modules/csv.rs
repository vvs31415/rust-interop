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

fn for_each_value(csv: &str, callback: impl Fn(&str)) {
    for value in csv.split(",") {
        callback(value.trim());
    }
}
