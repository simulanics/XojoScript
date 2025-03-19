// PluginHTMLtoMarkdown.rs
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
// Build (Windows): rustc --crate-type=cdylib PluginHTMLtoMarkdown.rs -o libHTMLtoMarkdown.dll
// Build (macOS/Linux): rustc --crate-type=cdylib PluginHTMLtoMarkdown.rs -o libHTMLtoMarkdown.dylib/so
//
// Requires: regex = "1", curl = "0.5"

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;
use std::collections::HashMap;
use regex::Regex;
use curl::easy::Easy;

/// Converts HTML to Markdown via basic tag replacements.
fn html_to_markdown_impl(html: &str) -> String {
    let mut md = html.to_string();

    // Remove <script> tags.
    let re_script = Regex::new(r"(?is)<script.*?</script>").unwrap();
    md = re_script.replace_all(&md, "").to_string();

    // Remove <style> tags.
    let re_style = Regex::new(r"(?is)<style.*?</style>").unwrap();
    md = re_style.replace_all(&md, "").to_string();

    // Replace <br> tags with newline.
    let re_br = Regex::new(r"(?i)<br\s*/?>").unwrap();
    md = re_br.replace_all(&md, "\n").to_string();

    // Replace header tags.
    let re_h1 = Regex::new(r"(?i)<h1>(.*?)</h1>").unwrap();
    md = re_h1.replace_all(&md, "# $1\n\n").to_string();
    let re_h2 = Regex::new(r"(?i)<h2>(.*?)</h2>").unwrap();
    md = re_h2.replace_all(&md, "## $1\n\n").to_string();
    let re_h3 = Regex::new(r"(?i)<h3>(.*?)</h3>").unwrap();
    md = re_h3.replace_all(&md, "### $1\n\n").to_string();
    let re_h4 = Regex::new(r"(?i)<h4>(.*?)</h4>").unwrap();
    md = re_h4.replace_all(&md, "#### $1\n\n").to_string();
    let re_h5 = Regex::new(r"(?i)<h5>(.*?)</h5>").unwrap();
    md = re_h5.replace_all(&md, "##### $1\n\n").to_string();
    let re_h6 = Regex::new(r"(?i)<h6>(.*?)</h6>").unwrap();
    md = re_h6.replace_all(&md, "###### $1\n\n").to_string();

    // Replace paragraph tags.
    let re_p = Regex::new(r"(?i)<p>(.*?)</p>").unwrap();
    md = re_p.replace_all(&md, "$1\n\n").to_string();

    // Replace bold tags (<strong> or <b>) using non-capturing groups.
    let re_bold = Regex::new(r"(?i)<(?:strong|b)>(.*?)</(?:strong|b)>").unwrap();
    md = re_bold.replace_all(&md, "**$1**").to_string();

    // Replace italic tags (<em> or <i>) using non-capturing groups.
    let re_italic = Regex::new(r"(?i)<(?:em|i)>(.*?)</(?:em|i)>").unwrap();
    md = re_italic.replace_all(&md, "*$1*").to_string();

    // Replace list items.
    let re_li = Regex::new(r"(?i)<li>(.*?)</li>").unwrap();
    md = re_li.replace_all(&md, "- $1\n").to_string();

    // Remove unordered and ordered list tags.
    let re_list = Regex::new(r"(?i)</?(ul|ol)>").unwrap();
    md = re_list.replace_all(&md, "").to_string();

    // Remove any remaining HTML tags.
    let re_tags = Regex::new(r"(?i)<[^>]*>").unwrap();
    md = re_tags.replace_all(&md, "").to_string();

    md
}

/// Collapses multiple consecutive newlines into double newlines.
fn collapse_newlines(text: &str) -> String {
    let re_newlines = Regex::new(r"(\n\s*){2,}").unwrap();
    re_newlines.replace_all(text, "\n\n").to_string()
}

/// Decodes HTML entities into their corresponding characters.
fn decode_html_entities_impl(text: &str) -> String {
    let mut result = String::new();
    let mut last_pos = 0;
    let re_entity = Regex::new(r"(&#(\d+);)|(&([a-zA-Z]+);)").unwrap();

    let entity_map: HashMap<&str, &str> = [
        ("bull", "•"), ("amp", "&"), ("lt", "<"), ("gt", ">"), ("quot", "\""),
        ("apos", "'"), ("copy", "©"), ("nbsp", " "), ("ndash", "–"), ("mdash", "—"),
        ("lsquo", "‘"), ("rsquo", "’"), ("ldquo", "“"), ("rdquo", "”"), ("hellip", "…")
    ].iter().cloned().collect();

    for cap in re_entity.captures_iter(text) {
        let m = cap.get(0).unwrap();
        result.push_str(&text[last_pos..m.start()]);
        if let Some(num_match) = cap.get(2) {
            let code: i32 = num_match.as_str().parse().unwrap_or(0);
            if code == 8217 {
                result.push('\'');
            } else if code == 8220 || code == 8221 {
                result.push('"');
            } else if code == 8211 {
                result.push('-');
            } else if code == 8230 {
                result.push('…');
            } else if code < 128 {
                if let Some(ch) = std::char::from_u32(code as u32) {
                    result.push(ch);
                } else {
                    result.push_str(m.as_str());
                }
            } else {
                if let Some(ch) = std::char::from_u32(code as u32) {
                    result.push(ch);
                } else {
                    result.push_str(m.as_str());
                }
            }
        } else if let Some(name_match) = cap.get(4) {
            let name = name_match.as_str();
            if let Some(&replacement) = entity_map.get(name) {
                result.push_str(replacement);
            } else {
                result.push_str(m.as_str());
            }
        }
        last_pos = m.end();
    }
    result.push_str(&text[last_pos..]);
    result
}

/// Encodes special characters in the text into their corresponding HTML entities.
fn encode_html_entities_impl(text: &str) -> String {
    let mut result = String::new();
    for c in text.chars() {
        match c {
            '&' => result.push_str("&amp;"),
            '<' => result.push_str("&lt;"),
            '>' => result.push_str("&gt;"),
            '"' => result.push_str("&quot;"),
            '\'' => result.push_str("&#39;"),
            _ => result.push(c),
        }
    }
    result
}

/// Converts Markdown text to basic HTML.
/// This is a simple implementation and may not cover all Markdown features.
fn markdown_to_html_impl(markdown: &str) -> String {
    let mut html = markdown.to_string();

    // Convert headers.
    let re_h6 = Regex::new(r"(?m)^###### (.*)$").unwrap();
    html = re_h6.replace_all(&html, "<h6>$1</h6>").to_string();
    let re_h5 = Regex::new(r"(?m)^##### (.*)$").unwrap();
    html = re_h5.replace_all(&html, "<h5>$1</h5>").to_string();
    let re_h4 = Regex::new(r"(?m)^#### (.*)$").unwrap();
    html = re_h4.replace_all(&html, "<h4>$1</h4>").to_string();
    let re_h3 = Regex::new(r"(?m)^### (.*)$").unwrap();
    html = re_h3.replace_all(&html, "<h3>$1</h3>").to_string();
    let re_h2 = Regex::new(r"(?m)^## (.*)$").unwrap();
    html = re_h2.replace_all(&html, "<h2>$1</h2>").to_string();
    let re_h1 = Regex::new(r"(?m)^# (.*)$").unwrap();
    html = re_h1.replace_all(&html, "<h1>$1</h1>").to_string();

    // Convert bold text.
    let re_bold = Regex::new(r"\*\*(.*?)\*\*").unwrap();
    html = re_bold.replace_all(&html, "<strong>$1</strong>").to_string();

    // Convert italic text.
    let re_italic = Regex::new(r"\*(.*?)\*").unwrap();
    html = re_italic.replace_all(&html, "<em>$1</em>").to_string();

    // Convert list items.
    let re_li = Regex::new(r"(?m)^- (.*)$").unwrap();
    html = re_li.replace_all(&html, "<li>$1</li>").to_string();

    html
}

/// Exported function: converts HTML to Markdown.
#[no_mangle]
pub extern "C" fn htmltomarkdown(html: *const c_char) -> *const c_char {
    if html.is_null() {
        return CString::new("").unwrap().into_raw();
    }
    unsafe {
        let c_str = CStr::from_ptr(html);
        if let Ok(input) = c_str.to_str() {
            let mut result = html_to_markdown_impl(input);
            result = collapse_newlines(&result);
            result = decode_html_entities_impl(&result);
            // Return as UTF-8 encoded CString.
            CString::new(result).unwrap().into_raw()
        } else {
            CString::new("").unwrap().into_raw()
        }
    }
}

/// Exported function: fetches HTML from a URL and converts it to Markdown.
#[no_mangle]
pub extern "C" fn urltomarkdown(url: *const c_char) -> *const c_char {
    if url.is_null() {
        return CString::new("Failed to initialize curl").unwrap().into_raw();
    }
    let c_str = unsafe { CStr::from_ptr(url) };
    let url_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return CString::new("Invalid URL").unwrap().into_raw(),
    };

    let mut html_content = Vec::new();
    let mut easy = Easy::new();
    if easy.url(url_str).is_err() {
        return CString::new("Failed to set URL").unwrap().into_raw();
    }
    easy.useragent("Mozilla/5.0 (compatible; MyApp/1.0; +http://example.com/)")
        .unwrap();
    {
        let mut transfer = easy.transfer();
        if transfer.write_function(|data| {
            html_content.extend_from_slice(data);
            Ok(data.len())
        }).is_err() {
            return CString::new("Error fetching URL").unwrap().into_raw();
        }
        if transfer.perform().is_err() {
            return CString::new("Error fetching URL").unwrap().into_raw();
        }
    }
    let html_str = match String::from_utf8(html_content) {
        Ok(s) => s,
        Err(_) => return CString::new("Error decoding content").unwrap().into_raw(),
    };

    let mut result = html_to_markdown_impl(&html_str);
    result = collapse_newlines(&result);
    result = decode_html_entities_impl(&result);
    CString::new(result).unwrap().into_raw()
}

/// Exported function: collapses multiple newlines in the input text.
#[no_mangle]
pub extern "C" fn collapse_newline(text: *const c_char) -> *const c_char {
    if text.is_null() {
        return CString::new("").unwrap().into_raw();
    }
    unsafe {
        let c_str = CStr::from_ptr(text);
        if let Ok(input) = c_str.to_str() {
            let result = collapse_newlines(input);
            CString::new(result).unwrap().into_raw()
        } else {
            CString::new("").unwrap().into_raw()
        }
    }
}

/// Exported function: decodes HTML entities in the input text.
#[no_mangle]
pub extern "C" fn decode_html_entities(text: *const c_char) -> *const c_char {
    if text.is_null() {
        return CString::new("").unwrap().into_raw();
    }
    unsafe {
        let c_str = CStr::from_ptr(text);
        if let Ok(input) = c_str.to_str() {
            let result = decode_html_entities_impl(input);
            CString::new(result).unwrap().into_raw()
        } else {
            CString::new("").unwrap().into_raw()
        }
    }
}

/// Exported function: encodes special characters in the input text into HTML entities.
#[no_mangle]
pub extern "C" fn encode_html_entities(text: *const c_char) -> *const c_char {
    if text.is_null() {
        return CString::new("").unwrap().into_raw();
    }
    unsafe {
        let c_str = CStr::from_ptr(text);
        if let Ok(input) = c_str.to_str() {
            let result = encode_html_entities_impl(input);
            CString::new(result).unwrap().into_raw()
        } else {
            CString::new("").unwrap().into_raw()
        }
    }
}

/// Exported function: converts Markdown text to HTML.
#[no_mangle]
pub extern "C" fn markdowntohtml(markdown: *const c_char) -> *const c_char {
    if markdown.is_null() {
        return CString::new("").unwrap().into_raw();
    }
    unsafe {
        let c_str = CStr::from_ptr(markdown);
        if let Ok(input) = c_str.to_str() {
            let result = markdown_to_html_impl(input);
            CString::new(result).unwrap().into_raw()
        } else {
            CString::new("").unwrap().into_raw()
        }
    }
}

/// PluginEntry structure.
#[repr(C)]
pub struct PluginEntry {
    name: *const c_char,            // Plugin name.
    func_ptr: *const c_void,        // Pointer to the exported function.
    arity: c_int,                   // Number of parameters.
    param_types: [*const c_char; 10], // Parameter datatypes.
    ret_type: *const c_char,        // Return datatype.
}

#[no_mangle]
pub static mut PLUGIN_ENTRIES: [PluginEntry; 6] = [
    PluginEntry {
        name: b"HTMLtoMarkdown\0".as_ptr() as *const c_char,
        func_ptr: htmltomarkdown as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"string\0".as_ptr() as *const c_char,
    },
    PluginEntry {
        name: b"URLtoMarkdown\0".as_ptr() as *const c_char,
        func_ptr: urltomarkdown as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"string\0".as_ptr() as *const c_char,
    },
    PluginEntry {
        name: b"CollapseNewlines\0".as_ptr() as *const c_char,
        func_ptr: collapse_newline as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"string\0".as_ptr() as *const c_char,
    },
    PluginEntry {
        name: b"DecodeHTMLEntities\0".as_ptr() as *const c_char,
        func_ptr: decode_html_entities as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"string\0".as_ptr() as *const c_char,
    },
    PluginEntry {
        name: b"EncodeHTMLEntities\0".as_ptr() as *const c_char,
        func_ptr: encode_html_entities as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"string\0".as_ptr() as *const c_char,
    },
    PluginEntry {
        name: b"MarkdowntoHTML\0".as_ptr() as *const c_char,
        func_ptr: markdowntohtml as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
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