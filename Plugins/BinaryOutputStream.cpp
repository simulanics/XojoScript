// BinaryOutputStreamPlugin.cpp
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

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <cstring>  // Added to use std::strlen

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

std::unordered_map<int, std::ofstream*> binaryFiles;
std::mutex fileMutex;
int fileHandleCounter = 1;

// Opens a file for binary writing; appends if append is true, otherwise overwrites.
extern "C" XPLUGIN_API int BinaryOutputStream_Create(const char* path, bool append) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ofstream* file = new std::ofstream(path, append ? std::ios::app | std::ios::binary : std::ios::trunc | std::ios::binary);
    if (!file->is_open()) {
        delete file;
        return -1; // Error: Cannot open file
    }
    int handle = fileHandleCounter++;
    binaryFiles[handle] = file;
    return handle;
}

// Writes a single byte (0-255)
extern "C" XPLUGIN_API void BinaryOutputStream_WriteByte(int handle, int value) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open()) {
        char byte = static_cast<char>(value);
        it->second->write(&byte, sizeof(byte));
    }
}

// Writes a 2-byte short integer
extern "C" XPLUGIN_API void BinaryOutputStream_WriteShort(int handle, int value) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open()) {
        short shortValue = static_cast<short>(value);
        it->second->write(reinterpret_cast<char*>(&shortValue), sizeof(shortValue));
    }
}

// Writes a 4-byte long integer
extern "C" XPLUGIN_API void BinaryOutputStream_WriteLong(int handle, int value) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open()) {
        it->second->write(reinterpret_cast<char*>(&value), sizeof(value));
    }
}

// Writes an 8-byte floating-point number
extern "C" XPLUGIN_API void BinaryOutputStream_WriteDouble(int handle, double value) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open()) {
        it->second->write(reinterpret_cast<char*>(&value), sizeof(value));
    }
}

// Writes a string as binary data
extern "C" XPLUGIN_API void BinaryOutputStream_WriteString(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open()) {
        size_t length = std::strlen(text);
        it->second->write(text, length);
    }
}

// Returns the current position in the file
extern "C" XPLUGIN_API int BinaryOutputStream_Position(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open()) {
        return -1;
    }
    return static_cast<int>(it->second->tellp());
}

// Moves to a specific position in the file
extern "C" XPLUGIN_API void BinaryOutputStream_Seek(int handle, int position) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open() && position >= 0) {
        it->second->seekp(position, std::ios::beg);
    }
}

// Flushes the output buffer to the file
extern "C" XPLUGIN_API void BinaryOutputStream_Flush(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open()) {
        it->second->flush();
    }
}

// Closes an opened binary file
extern "C" XPLUGIN_API void BinaryOutputStream_Close(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end()) {
        it->second->close();
        delete it->second;
        binaryFiles.erase(it);
    }
}

// Plugin function entries
extern "C" {
    typedef struct {
        const char* name;
        void* funcPtr;
        int arity;
        const char* paramTypes[10];
        const char* retType;
    } PluginEntry;

    static PluginEntry pluginEntries[] = {
        { "BinaryOutputStream_Create", (void*)BinaryOutputStream_Create, 2, {"string", "boolean"}, "integer" },
        { "BinaryOutputStream_WriteByte", (void*)BinaryOutputStream_WriteByte, 2, {"integer", "integer"}, "boolean" },
        { "BinaryOutputStream_WriteShort", (void*)BinaryOutputStream_WriteShort, 2, {"integer", "integer"}, "boolean" },
        { "BinaryOutputStream_WriteLong", (void*)BinaryOutputStream_WriteLong, 2, {"integer", "integer"}, "boolean" },
        { "BinaryOutputStream_WriteDouble", (void*)BinaryOutputStream_WriteDouble, 2, {"integer", "double"}, "boolean" },
        { "BinaryOutputStream_WriteString", (void*)BinaryOutputStream_WriteString, 2, {"integer", "string"}, "boolean" },
        { "BinaryOutputStream_Position", (void*)BinaryOutputStream_Position, 1, {"integer"}, "integer" },
        { "BinaryOutputStream_Seek", (void*)BinaryOutputStream_Seek, 2, {"integer", "integer"}, "boolean" },
        { "BinaryOutputStream_Flush", (void*)BinaryOutputStream_Flush, 1, {"integer"}, "boolean" },
        { "BinaryOutputStream_Close", (void*)BinaryOutputStream_Close, 1, {"integer"}, "boolean" }
    };

    extern "C" XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
        if (count) {
            *count = sizeof(pluginEntries) / sizeof(PluginEntry);
        }
        return pluginEntries;
    }
}

#ifdef _WIN32
// Optional DllMain for Windows-specific initialization
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif
