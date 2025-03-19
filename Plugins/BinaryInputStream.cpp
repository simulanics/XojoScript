// BinaryInputStreamPlugin.cpp
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

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

std::unordered_map<int, std::ifstream*> binaryFiles;
std::mutex fileMutex;
int fileHandleCounter = 1;

// Opens a file for binary reading
extern "C" XPLUGIN_API int BinaryInputStream_Open(const char* path) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ifstream* file = new std::ifstream(path, std::ios::binary);
    if (!file->is_open()) {
        delete file;
        return -1; // Error: Cannot open file
    }
    int handle = fileHandleCounter++;
    binaryFiles[handle] = file;
    return handle;
}

// Reads a single byte (returns 0-255)
extern "C" XPLUGIN_API int BinaryInputStream_ReadByte(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open()) {
        return -1; // Error: Invalid handle
    }
    char byte;
    it->second->read(&byte, 1);
    return it->second->gcount() == 1 ? static_cast<unsigned char>(byte) : -1;
}

// Reads a 2-byte short integer
extern "C" XPLUGIN_API int BinaryInputStream_ReadShort(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open()) {
        return -1;
    }
    short value;
    it->second->read(reinterpret_cast<char*>(&value), sizeof(value));
    return it->second->gcount() == sizeof(value) ? value : -1;
}

// Reads a 4-byte long integer
extern "C" XPLUGIN_API int BinaryInputStream_ReadLong(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open()) {
        return -1;
    }
    int value;
    it->second->read(reinterpret_cast<char*>(&value), sizeof(value));
    return it->second->gcount() == sizeof(value) ? value : -1;
}

// Reads an 8-byte floating-point number
extern "C" XPLUGIN_API double BinaryInputStream_ReadDouble(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open()) {
        return -1.0;
    }
    double value;
    it->second->read(reinterpret_cast<char*>(&value), sizeof(value));
    return it->second->gcount() == sizeof(value) ? value : -1.0;
}

// Reads a specified number of bytes as a string
extern "C" XPLUGIN_API const char* BinaryInputStream_ReadString(int handle, int length) {
    static thread_local std::string buffer;
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open() || length <= 0) {
        return "";
    }
    buffer.resize(length);
    it->second->read(&buffer[0], length);
    buffer.resize(it->second->gcount());
    return buffer.c_str();
}

// Returns the current position in the file
extern "C" XPLUGIN_API int BinaryInputStream_Position(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open()) {
        return -1;
    }
    return static_cast<int>(it->second->tellg());
}

// Moves to a specific position in the file
extern "C" XPLUGIN_API void BinaryInputStream_Seek(int handle, int position) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it != binaryFiles.end() && it->second->is_open() && position >= 0) {
        it->second->seekg(position, std::ios::beg);
    }
}

// Checks if EOF is reached
extern "C" XPLUGIN_API int BinaryInputStream_EOF(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = binaryFiles.find(handle);
    if (it == binaryFiles.end() || !it->second->is_open()) {
        return -1;
    }
    return it->second->eof() ? 1 : 0;
}

// Closes an opened binary file
extern "C" XPLUGIN_API void BinaryInputStream_Close(int handle) {
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
        { "BinaryInputStream_Open", (void*)BinaryInputStream_Open, 1, {"string"}, "integer" },
        { "BinaryInputStream_ReadByte", (void*)BinaryInputStream_ReadByte, 1, {"integer"}, "integer" },
        { "BinaryInputStream_ReadShort", (void*)BinaryInputStream_ReadShort, 1, {"integer"}, "integer" },
        { "BinaryInputStream_ReadLong", (void*)BinaryInputStream_ReadLong, 1, {"integer"}, "integer" },
        { "BinaryInputStream_ReadDouble", (void*)BinaryInputStream_ReadDouble, 1, {"integer"}, "double" },
        { "BinaryInputStream_ReadString", (void*)BinaryInputStream_ReadString, 2, {"integer", "integer"}, "string" },
        { "BinaryInputStream_Position", (void*)BinaryInputStream_Position, 1, {"integer"}, "integer" },
        { "BinaryInputStream_Seek", (void*)BinaryInputStream_Seek, 2, {"integer", "integer"}, "boolean" },
        { "BinaryInputStream_EOF", (void*)BinaryInputStream_EOF, 1, {"integer"}, "integer" },
        { "BinaryInputStream_Close", (void*)BinaryInputStream_Close, 1, {"integer"}, "boolean" }
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