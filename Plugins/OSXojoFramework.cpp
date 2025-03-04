// OSXojoFramework.cpp
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
// Build (macOS/Linux): g++ -shared -fPIC -o OSXojoFramework.dylib/so OSXojoFramework.cpp
// Build (Windows):    g++ -shared -o OSXojoFramework.dll OSXojoFramework.cpp

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#include <unistd.h>
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

//------------------------------------------------------------------------------
// Memory Management (Thread-safe Cleanup)
//------------------------------------------------------------------------------
std::mutex mem_mutex;                    // Ensures thread-safe memory cleanup
std::vector<char*> allocatedMemory;      // Tracks allocated memory for cleanup

void registerAllocation(char* ptr) {
    std::lock_guard<std::mutex> lock(mem_mutex);
    allocatedMemory.push_back(ptr);
}

extern "C" XPLUGIN_API void CleanupMemory() {
    std::lock_guard<std::mutex> lock(mem_mutex);
    for (char* ptr : allocatedMemory) {
        delete[] ptr;
    }
    allocatedMemory.clear();
}

//------------------------------------------------------------------------------
// Utility: Split a string by a delimiter
//------------------------------------------------------------------------------
std::vector<std::string> splitString(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

//------------------------------------------------------------------------------
// Advanced Format Implementation
//
// This implementation fully parses the format string and applies the rules:
//
//   '#'   -> Placeholder for a digit if present; nothing is shown if absent.
//   '0'   -> Placeholder for a digit if present; shows '0' if missing.
//   '.'   -> Denotes the position of the decimal point.
//   ','   -> Indicates that the integer portion should be formatted with thousands separators.
//   '%'   -> Multiplies the number by 100 and appends a '%' sign.
//   '(' and ')' -> If the numeric format is enclosed in these, negative numbers appear in parentheses.
//   '+'   -> Forces a plus sign for positive numbers.
//   '-'   -> Displays a minus sign for negatives (no effect on positives).
//   'E'/'e' -> Displays the number in scientific notation.
//   '\\character' -> Outputs the literal character that follows the backslash.
//
// The format string may contain up to three sections (separated by semicolons):
//   section1 for positive numbers, section2 for negatives, section3 for zero.
//------------------------------------------------------------------------------
std::string applyFormatFull(double number, const std::string &fmt) {
    std::string prefix;
    std::string integerSpec;
    std::string fractionSpec;
    std::string exponentSpec;
    std::string suffix;
    bool useScientific = false;
    bool usePercent = false;
    bool forceSign = false;
    bool grouping = false;          // Set if a comma is present in the integer section.
    bool negativeParentheses = false;
    
    // State machine:
    // 0: prefix literal, 1: integer part, 2: fraction part, 3: exponent part, 4: suffix literal.
    int state = 0;
    for (size_t i = 0; i < fmt.size(); i++) {
        char ch = fmt[i];
        // Process escape: backslash makes the next character literal.
        if (ch == '\\' && i + 1 < fmt.size()) {
            char literalChar = fmt[i+1];
            if (state == 0)
                prefix.push_back(literalChar);
            else if (state == 1)
                integerSpec.push_back(literalChar);
            else if (state == 2)
                fractionSpec.push_back(literalChar);
            else if (state == 3)
                exponentSpec.push_back(literalChar);
            else if (state == 4)
                suffix.push_back(literalChar);
            i++;
            continue;
        }
        if (state == 0) {
            if (ch == '0' || ch == '#' || ch == '+' || ch == '-' || ch == '(') {
                state = 1;
                i--; // Re-read this character in state 1.
                continue;
            } else if (ch == '%') {
                usePercent = true;
                continue;
            } else {
                prefix.push_back(ch);
            }
        }
        else if (state == 1) {
            if (ch == '.') {
                state = 2;
                continue;
            } else if (ch == 'e' || ch == 'E') {
                useScientific = true;
                exponentSpec.push_back(ch);
                state = 3;
                continue;
            } else if (ch == '%') {
                usePercent = true;
            } else if (ch == ',') {
                grouping = true;
                integerSpec.push_back(ch);
            } else if (ch == '0' || ch == '#' || ch == '+' || ch == '-' || ch == '(' || ch == ')') {
                integerSpec.push_back(ch);
            } else {
                state = 4;
                suffix.push_back(ch);
            }
        }
        else if (state == 2) {
            if (ch == 'e' || ch == 'E') {
                useScientific = true;
                exponentSpec.push_back(ch);
                state = 3;
                continue;
            } else if (ch == '%') {
                usePercent = true;
            } else if (ch == '0' || ch == '#') {
                fractionSpec.push_back(ch);
            } else {
                state = 4;
                suffix.push_back(ch);
            }
        }
        else if (state == 3) {
            if (ch == '%') {
                usePercent = true;
            } else if (ch == '0' || ch == '#' || ch == '+' || ch == '-') {
                exponentSpec.push_back(ch);
            } else {
                state = 4;
                suffix.push_back(ch);
            }
        }
        else if (state == 4) {
            if (ch == '%') {
                usePercent = true;
            } else {
                suffix.push_back(ch);
            }
        }
    }
    
    // Check for forced sign.
    if (!integerSpec.empty() && integerSpec[0] == '+') {
        forceSign = true;
        integerSpec.erase(0, 1);
    }
    // Check for negative parentheses.
    if (!integerSpec.empty() && integerSpec.front() == '(' && integerSpec.back() == ')') {
        negativeParentheses = true;
        integerSpec.erase(0, 1);
        if (!integerSpec.empty() && integerSpec.back() == ')')
            integerSpec.pop_back();
    }
    if (usePercent)
        number *= 100;
    
    bool isNegative = (number < 0);
    double absNumber = std::fabs(number);
    std::string formatted;
    
    if (useScientific) {
        int precision = 0;
        for (char c : fractionSpec) {
            if (c == '0' || c == '#')
                precision++;
        }
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(precision) << absNumber;
        formatted = oss.str();
    } else {
        int reqIntDigits = 0;
        for (char c : integerSpec) {
            if (c == '0')
                reqIntDigits++;
        }
        int reqFracDigits = 0;
        int totalFracDigits = 0;
        for (char c : fractionSpec) {
            if (c == '0') reqFracDigits++;
            if (c == '0' || c == '#') totalFracDigits++;
        }
        double roundingFactor = std::pow(10.0, totalFracDigits);
        double rounded = std::round(absNumber * roundingFactor) / roundingFactor;
        
        long long intPart = static_cast<long long>(rounded);
        double fracPart = rounded - intPart;
        
        std::ostringstream intStream;
        intStream << intPart;
        std::string intStr = intStream.str();
        if ((int)intStr.size() < reqIntDigits) {
            int pad = reqIntDigits - intStr.size();
            intStr = std::string(pad, '0') + intStr;
        }
        if (grouping) {
            int pos = intStr.size() - 3;
            while (pos > 0) {
                intStr.insert(pos, ",");
                pos -= 3;
            }
        }
        std::string fracStr;
        if (totalFracDigits > 0) {
            int fracValue = static_cast<int>(std::round(fracPart * std::pow(10, totalFracDigits)));
            std::ostringstream fracStream;
            fracStream << fracValue;
            fracStr = fracStream.str();
            if ((int)fracStr.size() < totalFracDigits) {
                fracStr = std::string(totalFracDigits - fracStr.size(), '0') + fracStr;
            }
            if (totalFracDigits > reqFracDigits) {
                int removeCount = 0;
                for (int i = fracStr.size() - 1; i >= reqFracDigits; i--) {
                    if (fracStr[i] == '0')
                        removeCount++;
                    else
                        break;
                }
                if (removeCount > 0)
                    fracStr.erase(fracStr.size() - removeCount, removeCount);
            }
        }
        formatted = intStr;
        if (!fracStr.empty()) {
            formatted.push_back('.');
            formatted.append(fracStr);
        }
    }
    
    if (isNegative) {
        if (negativeParentheses)
            formatted = "(" + formatted + ")";
        else
            formatted = "-" + formatted;
    } else if (forceSign) {
        formatted = "+" + formatted;
    }
    formatted.append(suffix);
    return formatted;
}

extern "C" XPLUGIN_API char* Format(double number, const char* formatSpec) {
    if (!formatSpec) return nullptr;
    std::string fmtStr(formatSpec);
    std::vector<std::string> sections = splitString(fmtStr, ';');
    std::string chosenFormat;
    if (sections.size() == 1) {
        chosenFormat = sections[0];
    } else if (sections.size() == 2) {
        chosenFormat = (number < 0) ? sections[1] : sections[0];
    } else if (sections.size() >= 3) {
        if (number > 0)
            chosenFormat = sections[0];
        else if (number < 0)
            chosenFormat = sections[1];
        else
            chosenFormat = sections[2];
    } else {
        chosenFormat = fmtStr;
    }
    std::string result = applyFormatFull(number, chosenFormat);
    char* formatted = new char[result.size() + 1];
    std::strcpy(formatted, result.c_str());
    registerAllocation(formatted);
    return formatted;
}

//------------------------------------------------------------------------------
// URL Encoding/Decoding Functions
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API char* EncodeURLComponent(const char* str) {
    if (!str) return nullptr;
    size_t len = std::strlen(str);
    size_t new_len = len * 3 + 1;
    char* encoded = new char[new_len];
    char* ptr = encoded;
    const char* hexDigits = "0123456789ABCDEF";
    for (size_t i = 0; i < len; i++) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *ptr++ = c;
        } else {
            *ptr++ = '%';
            *ptr++ = hexDigits[c >> 4];
            *ptr++ = hexDigits[c & 0x0F];
        }
    }
    *ptr = '\0';
    registerAllocation(encoded);
    return encoded;
}

extern "C" XPLUGIN_API char* DecodeURLComponent(const char* str) {
    if (!str) return nullptr;
    size_t len = std::strlen(str);
    char* decoded = new char[len + 1];
    char* ptr = decoded;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '%' && i + 2 < len &&
            std::isxdigit(str[i+1]) && std::isxdigit(str[i+2])) {
            char hexVal[3] = { str[i+1], str[i+2], '\0' };
            *ptr++ = static_cast<char>(std::strtol(hexVal, nullptr, 16));
            i += 2;
        } else {
            *ptr++ = str[i];
        }
    }
    *ptr = '\0';
    registerAllocation(decoded);
    return decoded;
}

//------------------------------------------------------------------------------
// Hex Encoding/Decoding Functions
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API char* Hex(int value) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << value;
    std::string result = oss.str();
    char* hexStr = new char[result.size() + 1];
    std::strcpy(hexStr, result.c_str());
    registerAllocation(hexStr);
    return hexStr;
}

extern "C" XPLUGIN_API char* EncodeHexEx(const char* str, int insertSpaces) {
    bool spaces = (insertSpaces != 0);
    if (!str) return nullptr;
    std::ostringstream oss;
    size_t len = std::strlen(str);
    for (size_t i = 0; i < len; i++) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(static_cast<unsigned char>(str[i]));
        if (spaces && i < len - 1)
            oss << ' ';
    }
    std::string result = oss.str();
    char* encodedHex = new char[result.size() + 1];
    std::strcpy(encodedHex, result.c_str());
    registerAllocation(encodedHex);
    return encodedHex;
}

extern "C" XPLUGIN_API char* EncodeHex(const char* str) {
    return EncodeHexEx(str, 0);
}

extern "C" XPLUGIN_API char* DecodeHex(const char* str) {
    if (!str) return nullptr;
    std::ostringstream oss;
    size_t len = std::strlen(str);
    for (size_t i = 0; i < len;) {
        if (std::isspace(str[i])) { i++; continue; }
        if (i + 1 < len && std::isxdigit(str[i]) && std::isxdigit(str[i+1])) {
            char hexVal[3] = { str[i], str[i+1], '\0' };
            oss << static_cast<char>(std::strtol(hexVal, nullptr, 16));
            i += 2;
        } else {
            i++;
        }
    }
    std::string result = oss.str();
    char* decodedHex = new char[result.size() + 1];
    std::strcpy(decodedHex, result.c_str());
    registerAllocation(decodedHex);
    return decodedHex;
}

//------------------------------------------------------------------------------
// Base64 Encoding/Decoding Functions
//------------------------------------------------------------------------------
static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

extern "C" XPLUGIN_API char* EncodeBase64(const char* str, int lineWrap) {
    if (!str) return nullptr;
    size_t len = std::strlen(str);
    std::ostringstream oss;
    int lineLength = 0;
    for (size_t i = 0; i < len; i += 3) {
        int val = (static_cast<unsigned char>(str[i]) << 16) |
                  ((i+1 < len ? static_cast<unsigned char>(str[i+1]) : 0) << 8) |
                  (i+2 < len ? static_cast<unsigned char>(str[i+2]) : 0);
        oss << base64_chars[(val >> 18) & 0x3F]
            << base64_chars[(val >> 12) & 0x3F]
            << (i+1 < len ? base64_chars[(val >> 6) & 0x3F] : '=')
            << (i+2 < len ? base64_chars[val & 0x3F] : '=');
        if (lineWrap > 0) {
            lineLength += 4;
            if (lineLength >= lineWrap) {
                oss << '\n';
                lineLength = 0;
            }
        }
    }
    std::string result = oss.str();
    char* encodedBase64 = new char[result.size() + 1];
    std::strcpy(encodedBase64, result.c_str());
    registerAllocation(encodedBase64);
    return encodedBase64;
}

extern "C" XPLUGIN_API char* DecodeBase64(const char* str) {
    if (!str) return nullptr;
    size_t len = std::strlen(str);
    std::vector<unsigned char> decodedData;
    int val = 0, valb = -8;
    for (size_t i = 0; i < len; i++) {
        if (std::isspace(str[i])) continue;
        if (str[i] == '=') break;
        const char* pos = std::strchr(base64_chars, str[i]);
        if (!pos) continue;
        val = (val << 6) + (pos - base64_chars);
        valb += 6;
        if (valb >= 0) {
            decodedData.push_back(static_cast<unsigned char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    char* decodedBase64 = new char[decodedData.size() + 1];
    std::memcpy(decodedBase64, decodedData.data(), decodedData.size());
    decodedBase64[decodedData.size()] = '\0';
    registerAllocation(decodedBase64);
    return decodedBase64;
}

//------------------------------------------------------------------------------
// Additional Functions: Bin, Chr, ChrByte, CLong, CStrDouble, CStrLong, Asc, AscByte
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API char* Bin(int value) {
    std::string binStr;
    if (value == 0)
        binStr = "0";
    else {
        while (value > 0) {
            binStr = ((value & 1) ? "1" : "0") + binStr;
            value >>= 1;
        }
    }
    char* result = new char[binStr.size() + 1];
    std::strcpy(result, binStr.c_str());
    registerAllocation(result);
    return result;
}

extern "C" XPLUGIN_API char* Chr(int value) {
    char* result = nullptr;
    if (value < 0) value = 0;
    if (value < 128) {
        result = new char[2];
        result[0] = static_cast<char>(value);
        result[1] = '\0';
    } else if (value < 0x800) {
        result = new char[3];
        result[0] = static_cast<char>(0xC0 | ((value >> 6) & 0x1F));
        result[1] = static_cast<char>(0x80 | (value & 0x3F));
        result[2] = '\0';
    } else {
        result = new char[4];
        result[0] = static_cast<char>(0xE0 | ((value >> 12) & 0x0F));
        result[1] = static_cast<char>(0x80 | ((value >> 6) & 0x3F));
        result[2] = static_cast<char>(0x80 | (value & 0x3F));
        result[3] = '\0';
    }
    registerAllocation(result);
    return result;
}

extern "C" XPLUGIN_API char* ChrByte(int value) {
    char* result = new char[2];
    result[0] = static_cast<char>(value & 0xFF);
    result[1] = '\0';
    registerAllocation(result);
    return result;
}

extern "C" XPLUGIN_API long long CLong(const char* str) {
    if (!str) return 0;
    return std::atoll(str);
}

extern "C" XPLUGIN_API char* CStrDouble(double data) {
    std::ostringstream oss;
    oss << std::defaultfloat << std::setprecision(7) << data;
    std::string result = oss.str();
    char* strData = new char[result.size() + 1];
    std::strcpy(strData, result.c_str());
    registerAllocation(strData);
    return strData;
}

extern "C" XPLUGIN_API char* CStrLong(long long data) {
    std::ostringstream oss;
    oss << data;
    std::string result = oss.str();
    char* strData = new char[result.size() + 1];
    std::strcpy(strData, result.c_str());
    registerAllocation(strData);
    return strData;
}

extern "C" XPLUGIN_API int Asc(const char* s) {
    if (!s || !s[0]) return 0;
    return static_cast<unsigned char>(s[0]);
}

extern "C" XPLUGIN_API int AscByte(const char* s) {
    if (!s || !s[0]) return 0;
    return static_cast<unsigned char>(s[0]);
}

//------------------------------------------------------------------------------
// Plugin Entry Table
//------------------------------------------------------------------------------
struct PluginEntry {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
};

static PluginEntry pluginEntries[] = {
    {"EncodeURLComponent", (void*)EncodeURLComponent, 1, {"string"}, "string"},
    {"DecodeURLComponent", (void*)DecodeURLComponent, 1, {"string"}, "string"},
    {"Hex", (void*)Hex, 1, {"integer"}, "string"},
    {"EncodeHex", (void*)EncodeHex, 1, {"string"}, "string"},
    {"EncodeHexEx", (void*)EncodeHexEx, 2, {"string", "boolean"}, "string"},
    {"DecodeHex", (void*)DecodeHex, 1, {"string"}, "string"},
    {"EncodeBase64", (void*)EncodeBase64, 2, {"string", "integer"}, "string"},
    {"DecodeBase64", (void*)DecodeBase64, 1, {"string"}, "string"},
    {"Format", (void*)Format, 2, {"integer", "string"}, "string"},
    {"Bin", (void*)Bin, 1, {"integer"}, "string"},
    {"Chr", (void*)Chr, 1, {"integer"}, "string"},
    {"ChrByte", (void*)ChrByte, 1, {"integer"}, "string"},
    {"CLong", (void*)CLong, 1, {"string"}, "integer"},
    {"CStrDouble", (void*)CStrDouble, 1, {"string"}, "string"},
    {"CStrLong", (void*)CStrLong, 1, {"integer"}, "string"},
    {"Asc", (void*)Asc, 1, {"string"}, "integer"},
    {"AscByte", (void*)AscByte, 1, {"string"}, "integer"},
    {"CleanupMemory", (void*)CleanupMemory, 0, {}, nullptr}
};

extern "C" XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    return pluginEntries;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
#endif
