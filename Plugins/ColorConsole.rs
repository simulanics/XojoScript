// colorconsole.rs
// Plugin that provides two functions:
// - PrintColor(text: string, color: string) as boolean
// - RGBtoHex(r: integer, g: integer, b: integer) as string
// Required: RUST
// Build (Windows): rustc --crate-type=cdylib colorconsole.rs -o ColorConsole.dll
// Build (macOS/Linux): rustc --crate-type=cdylib colorconsole.rs -o colorconsole.so

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;
use std::io::Write;
use std::thread;
use std::time::Duration;

#[repr(C)]
pub struct PluginEntry {
    // Name to be used by the bytecode interpreter.
    name: *const c_char,
    // Pointer to the exported function.
    func_ptr: *const c_void,
    // Number of parameters the function accepts.
    arity: c_int,
    // Parameter datatypes.
    param_types: [*const c_char; 10],
    // Return datatype.
    ret_type: *const c_char,
}

/// Prints the provided text using the color specified as "#RRGGBB".
#[no_mangle]
pub extern "C" fn printcolor(text: *const c_char, color: *const c_char) -> bool {
    unsafe {
        if text.is_null() || color.is_null() {
            return false;
        }
        let text_cstr = CStr::from_ptr(text);
        let color_cstr = CStr::from_ptr(color);
        if let (Ok(text_str), Ok(color_str)) = (text_cstr.to_str(), color_cstr.to_str()) {
            // Validate that color_str is in the format "#RRGGBB".
            if color_str.len() != 7 || !color_str.starts_with('#') {
                eprintln!("Invalid color format. Expected format: #RRGGBB");
                return false;
            }
            // Parse hexadecimal values for red, green, and blue.
            let r = u8::from_str_radix(&color_str[1..3], 16).unwrap_or(255);
            let g = u8::from_str_radix(&color_str[3..5], 16).unwrap_or(255);
            let b = u8::from_str_radix(&color_str[5..7], 16).unwrap_or(255);
            // Construct ANSI escape code for 24-bit color.
            let escape = format!("\x1B[38;2;{};{};{}m", r, g, b);
            // Print colored text then reset terminal formatting.
            print!("{}{}{}", escape, text_str, "\x1B[0m");
            std::io::stdout().flush().unwrap();
            true
        } else {
            false
        }
    }
}

/// Converts three integer RGB values to a hex string formatted as "#RRGGBB".
#[no_mangle]
pub extern "C" fn RGBtoHex(r: c_int, g: c_int, b: c_int) -> *const c_char {
    // Format the values as a hexadecimal string.
    let hex = format!("#{:02X}{:02X}{:02X}", r, g, b);
    // Leak the CString so the pointer remains valid.
    CString::new(hex).unwrap().into_raw()
}

// Static plugin entries using constant C string literals for metadata.
#[no_mangle]
pub static mut PLUGIN_ENTRIES: [PluginEntry; 2] = [
    PluginEntry {
        name: b"PrintColor\0" as *const u8 as *const c_char,
        func_ptr: printcolor as *const c_void,
        arity: 2,
        param_types: [
            b"string\0" as *const u8 as *const c_char, // text parameter
            b"string\0" as *const u8 as *const c_char, // color parameter
            ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(),
        ],
        ret_type: b"boolean\0" as *const u8 as *const c_char,
    },
    PluginEntry {
        name: b"RGBtoHex\0" as *const u8 as *const c_char,
        func_ptr: RGBtoHex as *const c_void,
        arity: 3,
        param_types: [
            b"integer\0" as *const u8 as *const c_char, // red parameter
            b"integer\0" as *const u8 as *const c_char, // green parameter
            b"integer\0" as *const u8 as *const c_char, // blue parameter
            ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"string\0" as *const u8 as *const c_char,
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
