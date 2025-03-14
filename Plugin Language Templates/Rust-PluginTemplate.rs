// lib.rs
//
// PluginTemplate converted from C++ to Rust
// Created by Matthew A Combatti
// Simulanics Technologies and Xojo Developers Studio
//
// Build commands:
// Windows: cargo build --release --target x86_64-pc-windows-msvc
// Linux:   cargo build --release --target x86_64-unknown-linux-gnu
// macOS:   cargo build --release --target x86_64-apple-darwin

#![crate_type = "cdylib"]

extern crate libc;
use libc::{c_char, c_int, c_void};
use std::ffi::{CStr, CString};
use std::io::{stdout, Write};
use std::ptr;
use std::thread;
use std::time::Duration;

#[cfg(windows)]
extern "system" {
    fn Beep(dwFreq: u32, dwDuration: u32) -> i32;
}

#[no_mangle]
pub extern "C" fn addtwonumbers(a: c_int, b: c_int) -> c_int {
    a + b
}

#[no_mangle]
pub extern "C" fn sayhello(name: *const c_char) -> *const c_char {
    unsafe {
        if name.is_null() {
            return ptr::null();
        }
        let c_str = CStr::from_ptr(name);
        let greeting = format!("Hello, {}", c_str.to_str().unwrap_or(""));
        // Leak the CString so that the returned pointer remains valid.
        // (Caller is responsible for freeing it if needed.)
        CString::new(greeting).unwrap().into_raw()
    }
}

#[no_mangle]
pub extern "C" fn factorial(n: c_int) -> c_int {
    if n <= 1 { 1 } else { n * factorial(n - 1) }
}

#[no_mangle]
pub extern "C" fn Fibonacci(n: c_int) -> c_int {
    if n <= 0 {
        0
    } else if n == 1 {
        1
    } else {
        Fibonacci(n - 1) + Fibonacci(n - 2)
    }
}

#[no_mangle]
pub extern "C" fn XBeep(frequency: c_int, duration: c_int) -> bool {
    #[cfg(windows)]
    {
        unsafe { Beep(frequency as u32, duration as u32) != 0 }
    }
    #[cfg(not(windows))]
    {
        print!("\x07");
        stdout().flush().unwrap();
        thread::sleep(Duration::from_millis(duration as u64));
        true
    }
}

#[no_mangle]
pub extern "C" fn PluginSleep(ms: c_int) -> bool {
    thread::sleep(Duration::from_millis(ms as u64));
    true
}

#[no_mangle]
pub extern "C" fn DoEvents() {
    // Yield the thread so that other tasks can run.
    thread::yield_now();
}

/// The PluginEntry structure mirrors the C++ definition.
#[repr(C)]
pub struct PluginEntry {
    pub name: *const c_char,                // Function name.
    pub funcPtr: *const c_void,             // Pointer to function.
    pub arity: c_int,                       // Number of parameters.
    pub paramTypes: [*const c_char; 10],      // Array of parameter type strings.
    pub retType: *const c_char,             // Return type string.
}

// Helper: convert a function pointer to *const c_void.
fn to_void_ptr<T>(f: T) -> *const c_void {
    f as *const () as *const c_void
}

// Macro to create a null-terminated static C string.
macro_rules! cstr {
    ($s:expr) => {
        concat!($s, "\0").as_ptr() as *const c_char
    };
}

// Predefined C strings for types.
static DOUBLE: *const c_char = cstr!("double");
static STRING: *const c_char = cstr!("string");
static INTEGER: *const c_char = cstr!("integer");
static BOOLEAN: *const c_char = cstr!("boolean");

// Define constant parameter arrays (10 entries each; unused slots are null).
const PARAMS2_DOUBLE: [*const c_char; 10] = [DOUBLE, DOUBLE, ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null()];
const PARAMS1_STRING: [*const c_char; 10] = [STRING, ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null()];
const PARAMS1_INTEGER: [*const c_char; 10] = [INTEGER, ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null()];
const PARAMS2_INTEGER: [*const c_char; 10] = [INTEGER, INTEGER, ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null()];
const PARAMS0: [*const c_char; 10] = [ptr::null(); 10];

// Define a mutable static array of plugin entries. Function pointers will be filled in the init.
#[no_mangle]
pub static mut PLUGIN_ENTRIES: [PluginEntry; 7] = [
    PluginEntry {
        name: cstr!("AddTwoNumbers"),
        funcPtr: ptr::null(),
        arity: 2,
        paramTypes: PARAMS2_DOUBLE,
        retType: DOUBLE,
    },
    PluginEntry {
        name: cstr!("SayHello"),
        funcPtr: ptr::null(),
        arity: 1,
        paramTypes: PARAMS1_STRING,
        retType: STRING,
    },
    PluginEntry {
        name: cstr!("Factorial"),
        funcPtr: ptr::null(),
        arity: 1,
        paramTypes: PARAMS1_INTEGER,
        retType: INTEGER,
    },
    PluginEntry {
        name: cstr!("Fibonacci"),
        funcPtr: ptr::null(),
        arity: 1,
        paramTypes: PARAMS1_INTEGER,
        retType: INTEGER,
    },
    PluginEntry {
        name: cstr!("Beep"),
        funcPtr: ptr::null(),
        arity: 2,
        paramTypes: PARAMS2_INTEGER,
        retType: BOOLEAN,
    },
    PluginEntry {
        name: cstr!("Sleep"),
        funcPtr: ptr::null(),
        arity: 1,
        paramTypes: PARAMS1_INTEGER,
        retType: BOOLEAN,
    },
    PluginEntry {
        name: cstr!("DoEvents"),
        funcPtr: ptr::null(),
        arity: 0,
        paramTypes: PARAMS0,
        retType: BOOLEAN,
    },
];

/// Call this function once (for example, in DllMain on Windows or library initializer)
/// to fill in the function pointers in PLUGIN_ENTRIES.
#[no_mangle]
pub extern "C" fn init_plugin_entries() {
    unsafe {
        PLUGIN_ENTRIES[0].funcPtr = to_void_ptr(addtwonumbers as extern "C" fn(c_int, c_int) -> c_int);
        PLUGIN_ENTRIES[1].funcPtr = to_void_ptr(sayhello as extern "C" fn(*const c_char) -> *const c_char);
        PLUGIN_ENTRIES[2].funcPtr = to_void_ptr(factorial as extern "C" fn(c_int) -> c_int);
        PLUGIN_ENTRIES[3].funcPtr = to_void_ptr(Fibonacci as extern "C" fn(c_int) -> c_int);
        PLUGIN_ENTRIES[4].funcPtr = to_void_ptr(XBeep as extern "C" fn(c_int, c_int) -> bool);
        PLUGIN_ENTRIES[5].funcPtr = to_void_ptr(PluginSleep as extern "C" fn(c_int) -> bool);
        PLUGIN_ENTRIES[6].funcPtr = to_void_ptr(DoEvents as extern "C" fn());
    }
}

/// Exported function to retrieve the plugin entries.
/// The caller passes a pointer to an integer that will be set to the number of entries,
/// and receives a pointer to the first PluginEntry.
#[no_mangle]
pub extern "C" fn GetPluginEntries(count: *mut c_int) -> *mut PluginEntry {
    unsafe {
        if !count.is_null() {
            *count = PLUGIN_ENTRIES.len() as c_int;
        }
        PLUGIN_ENTRIES.as_mut_ptr()
    }
}