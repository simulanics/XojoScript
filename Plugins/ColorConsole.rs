// colorconsole.rs
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
// Plugin that provides three functions:
// - PrintColor(text: string, color: string) as boolean
// - RGBtoHex(r: integer, g: integer, b: integer) as string
// - PrintColorMarkdown(markdown: string) as boolean
// Required: RUST
// Build (Windows): rustc --crate-type=cdylib colorconsole.rs -o ColorConsole.dll
// Build (macOS/Linux): rustc --crate-type=cdylib colorconsole.rs -o colorconsole.so

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;
use std::io::Write;

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
            if color_str.len() != 7 || !color_str.starts_with('#') {
                eprintln!("Invalid color format. Expected format: #RRGGBB");
                return false;
            }
            let r = u8::from_str_radix(&color_str[1..3], 16).unwrap_or(255);
            let g = u8::from_str_radix(&color_str[3..5], 16).unwrap_or(255);
            let b = u8::from_str_radix(&color_str[5..7], 16).unwrap_or(255);
            let escape = format!("\x1B[38;2;{};{};{}m", r, g, b);
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
    let hex = format!("#{:02X}{:02X}{:02X}", r, g, b);
    CString::new(hex).unwrap().into_raw()
}

/// Helper function to process inline bold formatting.
/// It prints parts of the line using the provided default color,
/// and bold text (delimited by "**") in green.
fn process_inline_line(line: &str, default_color: &str) {
    let mut remaining = line;
    while let Some(start) = remaining.find("**") {
        let (before, rest) = remaining.split_at(start);
        if !before.is_empty() {
            let part = CString::new(before).unwrap();
            let def = CString::new(default_color).unwrap();
            printcolor(part.as_ptr(), def.as_ptr());
        }
        let after_start = &rest[2..]; // skip the starting "**"
        if let Some(end) = after_start.find("**") {
            let (bold_text, after) = after_start.split_at(end);
            if !bold_text.is_empty() {
                let bold = CString::new(bold_text).unwrap();
                let bold_color = CString::new("#32b80d").unwrap(); // Green for bold text
                printcolor(bold.as_ptr(), bold_color.as_ptr());
            }
            remaining = &after[2..]; // skip the ending "**"
        } else {
            // No closing marker found; print the rest as default.
            let part = CString::new("**").unwrap();
            let def = CString::new(default_color).unwrap();
            printcolor(part.as_ptr(), def.as_ptr());
            remaining = after_start;
            break;
        }
    }
    if !remaining.is_empty() {
        let part = CString::new(remaining).unwrap();
        let def = CString::new(default_color).unwrap();
        printcolor(part.as_ptr(), def.as_ptr());
    }
}

/// Prints the provided markdown text with colored syntax.
/// - Headers are colored based on level.
/// - List items have a default cyan color except for inline bold text (green).
/// - Bold text (surrounded by "**") appears in green.
/// - Horizontal rules (lines containing only dashes) appear in yellow.
/// - Code blocks (delimited by "```") are printed with magenta delimiters and gray content.
#[no_mangle]
pub extern "C" fn printcolormarkdown(markdown: *const c_char) -> bool {
    unsafe {
        if markdown.is_null() {
            return false;
        }
        let md_cstr = CStr::from_ptr(markdown);
        if let Ok(md_str) = md_cstr.to_str() {
            let mut in_code_block = false;
            for line in md_str.lines() {
                let trimmed = line.trim_start();
                // Toggle code block on delimiter lines.
                if trimmed.starts_with("```") {
                    in_code_block = !in_code_block;
                    let delim = CString::new(trimmed).unwrap();
                    let color = CString::new("#FF00FF").unwrap(); // Magenta for code block delimiters
                    printcolor(delim.as_ptr(), color.as_ptr());
                    println!();
                    continue;
                }
                // Inside a code block, print in gray.
                if in_code_block {
                    let content = CString::new(line).unwrap();
                    let color = CString::new("#AAAAAA").unwrap(); // Gray for code block content
                    printcolor(content.as_ptr(), color.as_ptr());
                    println!();
                    continue;
                }
                // Horizontal rule: line containing only dashes.
                if !trimmed.is_empty() && trimmed.chars().all(|c| c == '-') {
                    let hr = CString::new(line).unwrap();
                    let hr_color = CString::new("#FFFF00").unwrap(); // Yellow for horizontal rule
                    printcolor(hr.as_ptr(), hr_color.as_ptr());
                    println!();
                    continue;
                }
                // Header processing.
                if trimmed.starts_with('#') {
                    let header_level = trimmed.chars().take_while(|&c| c == '#').count();
                    let header_text = trimmed[header_level..].trim();
                    let color_str = match header_level {
                        1 => "#FF0000", // Red
                        2 => "#e0690d", // Orange
                        3 => "#FFFF00", // Yellow
                        4 => "#32b80d", // Green
                        5 => "#adabba", // Blue/gray
                        _ => "#8B00FF", // Violet
                    };
                    let text_to_print = format!("{} {}", "#".repeat(header_level), header_text);
                    let header_cstring = CString::new(text_to_print).unwrap();
                    let header_color = CString::new(color_str).unwrap();
                    printcolor(header_cstring.as_ptr(), header_color.as_ptr());
                    println!();
                    continue;
                }
                // List item detection.
                if trimmed.starts_with('-') ||
                   (trimmed.starts_with('*') && !trimmed.starts_with("**")) ||
                   (trimmed.len() > 1 && trimmed.chars().next().unwrap().is_digit(10) && trimmed.chars().nth(1) == Some('.')) {
                    process_inline_line(line, "#adabba"); // Cyan for list items
                    println!();
                    continue;
                }
                // Default processing for normal lines.
                process_inline_line(line, "#e4e1f2"); // White for normal text
                println!();
            }
            true
        } else {
            false
        }
    }
}

#[no_mangle]
pub static mut PLUGIN_ENTRIES: [PluginEntry; 3] = [
    PluginEntry {
        name: b"PrintColor\0" as *const u8 as *const c_char,
        func_ptr: printcolor as *const c_void,
        arity: 2,
        param_types: [
            b"string\0" as *const u8 as *const c_char,
            b"string\0" as *const u8 as *const c_char,
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
            b"integer\0" as *const u8 as *const c_char,
            b"integer\0" as *const u8 as *const c_char,
            b"integer\0" as *const u8 as *const c_char,
            ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"string\0" as *const u8 as *const c_char,
    },
    PluginEntry {
        name: b"PrintColorMarkdown\0" as *const u8 as *const c_char,
        func_ptr: printcolormarkdown as *const c_void,
        arity: 1,
        param_types: [
            b"string\0" as *const u8 as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(), ptr::null(),
        ],
        ret_type: b"boolean\0" as *const u8 as *const c_char,
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
