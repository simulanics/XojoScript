// PluginReadPDF.rs
// A cross-platform Rust plugin that extracts text from a PDF file.
// If the provided path is a URL (begins with "http://" or "https://"), the plugin downloads the PDF
// to a temporary file, extracts its text, and then deletes the temporary file.
// Build (Windows): rustc --crate-type=cdylib PluginReadPDF.rs -o PluginReadPDF.dll
// Build (macOS/Linux): rustc --crate-type=cdylib PluginReadPDF.rs -o PluginReadPDF.so
//
// This plugin requires the following dependencies. In your Cargo.toml add:
// [dependencies]
// pdf_extract = "0.6"
// reqwest = { version = "0.11", features = ["blocking", "rustls-tls"] }
// tempfile = "3"

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;
use std::io::Write;
use std::path::PathBuf;

use pdf_extract::extract_text;
use reqwest::blocking::get;
use tempfile::NamedTempFile;

/// Reads the PDF at the given filepath (or URL) and returns its text as a UTF-8 string.
/// If an error occurs, an empty string is returned.
/// This function is thread-safe.
#[no_mangle]
pub extern "C" fn readpdf(filepath: *const c_char) -> *const c_char {
    if filepath.is_null() {
        return CString::new("").unwrap().into_raw();
    }
    // Convert the C string to a Rust string.
    let c_str = unsafe { CStr::from_ptr(filepath) };
    let path_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return CString::new("").unwrap().into_raw(),
    };

    // Determine whether the path is a URL or a local file.
    let text_result = if path_str.starts_with("http://") || path_str.starts_with("https://") {
        // Download the PDF from the URL.
        match get(path_str) {
            Ok(response) => {
                if !response.status().is_success() {
                    Err("HTTP request failed")
                } else {
                    match response.bytes() {
                        Ok(bytes) => {
                            // Create a temporary file.
                            match NamedTempFile::new() {
                                Ok(mut tmp_file) => {
                                    if let Err(_) = tmp_file.write_all(&bytes) {
                                        Err("Failed to write to temporary file")
                                    } else {
                                        // Extract text from the temporary file.
                                        extract_text(tmp_file.path()).map_err(|_| "PDF extraction failed")
                                    }
                                },
                                Err(_) => Err("Failed to create temporary file"),
                            }
                        },
                        Err(_) => Err("Failed to get response bytes"),
                    }
                }
            },
            Err(_) => Err("HTTP request error"),
        }
    } else {
        // Treat as a local file path.
        let file_path = PathBuf::from(path_str);
        extract_text(&file_path).map_err(|_| "PDF extraction failed")
    };

    // Convert the extracted text (or empty string on error) into a C string.
    let result_str = text_result.unwrap_or_else(|_| String::new());
    CString::new(result_str).unwrap().into_raw()
}

/// PluginEntry structure for exposing plugin functions.
#[repr(C)]
pub struct PluginEntry {
    name: *const c_char,            // Plugin function name.
    func_ptr: *const c_void,        // Pointer to the exported function.
    arity: c_int,                   // Number of parameters.
    param_types: [*const c_char; 10], // Parameter datatypes.
    ret_type: *const c_char,        // Return datatype.
}

/// Expose the ReadPDF function.
#[no_mangle]
pub static mut PLUGIN_ENTRIES: [PluginEntry; 1] = [
    PluginEntry {
        name: b"ReadPDF\0".as_ptr() as *const c_char,
        func_ptr: readpdf as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(),
        ],
        ret_type: b"string\0".as_ptr() as *const c_char,
    },
];

/// Returns a pointer to the plugin entries array and sets the count.
#[no_mangle]
pub extern "C" fn GetPluginEntries(count: *mut c_int) -> *mut PluginEntry {
    unsafe {
        if !count.is_null() {
            *count = PLUGIN_ENTRIES.len() as c_int;
        }
        PLUGIN_ENTRIES.as_mut_ptr()
    }
}
