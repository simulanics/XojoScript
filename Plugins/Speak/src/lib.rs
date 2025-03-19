// Created by Matthew A Combatti  
// Simulanics Technologies and Xojo Developers Studio  
// https://www.simulanics.com  
// https://www.xojostudio.org  
// DISCLAIMER: Simulanics Technologies and Xojo Developers Studio are not affiliated with Xojo, Inc.
// -----------------------------------------------------------------------------  
// Copyright (c) 2025 Simulanics Technologies and Xojo Developers Studio  
//  
// Permission is hereby granted, free of charge, to any person obtaining a copy  
// of this software and associated documentation files (the "Software"), to deal  
// in the Software without restriction, including without limitation the rights  
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
// copies of the Software, and to permit persons to whom the Software is  
// furnished to do so, subject to the following conditions:  
//  
// The above copyright notice and this permission notice shall be included in all  
// copies or substantial portions of the Software.  
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  
// SOFTWARE.
// ----------------------------------------------------------------------------- 

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;
use std::process::Command;

#[no_mangle]
pub extern "C" fn speak(text: *const c_char) -> bool {
    if text.is_null() {
        return false;
    }
    let c_str = unsafe { CStr::from_ptr(text) };
    let text_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return false,
    };

    #[cfg(target_os = "windows")]
    {
        // Escape single quotes by doubling them.
        let escaped_text = text_str.replace("'", "''");
        // Build the PowerShell command.
        let command_str = format!(
            "Add-Type -AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('{}')",
            escaped_text
        );
        let status = Command::new("powershell")
            .args(&["-Command", &command_str])
            .status();
        return status.map_or(false, |s| s.success());
    }

    #[cfg(target_os = "macos")]
    {
        // Use the macOS "say" command.
        let status = Command::new("say").arg(text_str).status();
        return status.map_or(false, |s| s.success());
    }

    #[cfg(target_os = "linux")]
    {
        // Attempt to use "spd-say"; if that fails, fallback to "espeak".
        let status = Command::new("spd-say").arg(text_str).status();
        if let Ok(s) = status {
            if s.success() {
                return true;
            }
        }
        let status = Command::new("espeak").arg(text_str).status();
        return status.map_or(false, |s| s.success());
    }

    #[cfg(not(any(target_os = "windows", target_os = "macos", target_os = "linux")))]
    {
        false
    }
}

#[repr(C)]
pub struct PluginEntry {
    name: *const c_char,            // Plugin name.
    func_ptr: *const c_void,        // Pointer to the exported function.
    arity: c_int,                   // Number of parameters.
    param_types: [*const c_char; 10], // Parameter datatypes.
    ret_type: *const c_char,        // Return datatype.
}

#[no_mangle]
pub static mut PLUGIN_ENTRIES: [PluginEntry; 1] = [
    PluginEntry {
        name: b"Speak\0".as_ptr() as *const c_char,
        func_ptr: speak as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"boolean\0".as_ptr() as *const c_char,
    },
];

#[no_mangle]
pub extern "C" fn GetPluginEntries(count: *mut c_int) -> *mut PluginEntry {
    unsafe {
        if !count.is_null() {
            *count = PLUGIN_ENTRIES.len() as c_int;
        }
        PLUGIN_ENTRIES.as_mut_ptr()
    }
}
