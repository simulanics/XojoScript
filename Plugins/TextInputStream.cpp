// XojoTextStreamPlugin.cpp
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
#include <string>
#include <unordered_map>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

std::unordered_map<int, std::ifstream*> inputFiles;
std::unordered_map<int, std::ofstream*> outputFiles;
std::mutex fileMutex;
int fileHandleCounter = 1;

// Opens a file for reading
extern "C" XPLUGIN_API int TextInputStream_Open(const char* path) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ifstream* file = new std::ifstream(path);
    if (!file->is_open()) {
        delete file;
        return -1; // Error: Cannot open file
    }
    int handle = fileHandleCounter++;
    inputFiles[handle] = file;
    return handle;
}

// Reads a single line from the file
extern "C" XPLUGIN_API const char* TextInputStream_ReadLine(int handle) {
    static thread_local std::string line;
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = inputFiles.find(handle);
    if (it == inputFiles.end() || !it->second->is_open()) {
        return ""; // Error: Invalid handle
    }
    if (!std::getline(*(it->second), line)) {
        return ""; // EOF or error
    }
    return line.c_str();
}

// Reads the entire file content
extern "C" XPLUGIN_API const char* TextInputStream_ReadAll(int handle) {
    static thread_local std::string content;
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = inputFiles.find(handle);
    if (it == inputFiles.end() || !it->second->is_open()) {
        return ""; // Error: Invalid handle
    }
    content.assign((std::istreambuf_iterator<char>(*(it->second))), std::istreambuf_iterator<char>());
    return content.c_str();
}

// Checks if EOF is reached
extern "C" XPLUGIN_API int TextInputStream_EOF(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = inputFiles.find(handle);
    if (it == inputFiles.end() || !it->second->is_open()) {
        return -1; // Error: Invalid handle
    }
    return it->second->eof() ? 1 : 0;
}

// Closes an opened input file
extern "C" XPLUGIN_API void TextInputStream_Close(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = inputFiles.find(handle);
    if (it != inputFiles.end()) {
        it->second->close();
        delete it->second;
        inputFiles.erase(it);
    }
}

// Opens a file for writing (overwrites or appends)
extern "C" XPLUGIN_API int TextOutputStream_Open(const char* path, bool append) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ofstream* file = new std::ofstream(path, append ? std::ios::app : std::ios::trunc);
    if (!file->is_open()) {
        delete file;
        return -1; // Error: Cannot open file
    }
    int handle = fileHandleCounter++;
    outputFiles[handle] = file;
    return handle;
}

// Writes a line to the file
extern "C" XPLUGIN_API void TextOutputStream_WriteLine(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = outputFiles.find(handle);
    if (it != outputFiles.end() && it->second->is_open()) {
        *(it->second) << text << std::endl;
    }
}

// Writes text without a newline
extern "C" XPLUGIN_API void TextOutputStream_Write(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = outputFiles.find(handle);
    if (it != outputFiles.end() && it->second->is_open()) {
        *(it->second) << text;
    }
}

// Closes an opened output file
extern "C" XPLUGIN_API void TextOutputStream_Close(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = outputFiles.find(handle);
    if (it != outputFiles.end()) {
        it->second->close();
        delete it->second;
        outputFiles.erase(it);
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
        { "TextInputStream_Open", (void*)TextInputStream_Open, 1, {"string"}, "integer" },
        { "TextInputStream_ReadLine", (void*)TextInputStream_ReadLine, 1, {"integer"}, "string" },
        { "TextInputStream_ReadAll", (void*)TextInputStream_ReadAll, 1, {"integer"}, "string" },
        { "TextInputStream_EOF", (void*)TextInputStream_EOF, 1, {"integer"}, "integer" },
        { "TextInputStream_Close", (void*)TextInputStream_Close, 1, {"integer"}, "boolean" },
        { "TextOutputStream_Open", (void*)TextOutputStream_Open, 2, {"string", "boolean"}, "integer" },
        { "TextOutputStream_WriteLine", (void*)TextOutputStream_WriteLine, 2, {"integer", "string"}, "boolean" },
        { "TextOutputStream_Write", (void*)TextOutputStream_Write, 2, {"integer", "string"}, "boolean" },
        { "TextOutputStream_Close", (void*)TextOutputStream_Close, 1, {"integer"}, "boolean" }
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
