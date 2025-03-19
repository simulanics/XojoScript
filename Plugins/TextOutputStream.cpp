// TextOutputStreamPlugin.cpp
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

std::unordered_map<int, std::ofstream*> outputFiles;
std::mutex fileMutex;
int fileHandleCounter = 1;

// Opens a file for writing; appends if append is true, otherwise overwrites.
extern "C" XPLUGIN_API int TextOutputStream_Create(const char* path, bool append) {
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

// Writes text to the file without adding a newline.
extern "C" XPLUGIN_API void TextOutputStream_Write(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = outputFiles.find(handle);
    if (it != outputFiles.end() && it->second->is_open()) {
        *(it->second) << text;
    }
}

// Writes a line to the file with a newline character.
extern "C" XPLUGIN_API void TextOutputStream_WriteLine(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = outputFiles.find(handle);
    if (it != outputFiles.end() && it->second->is_open()) {
        *(it->second) << text << std::endl;
    }
}

// Flushes the file output, ensuring data is written immediately.
extern "C" XPLUGIN_API void TextOutputStream_Flush(int handle) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = outputFiles.find(handle);
    if (it != outputFiles.end() && it->second->is_open()) {
        it->second->flush();
    }
}

// Closes an opened output file.
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
        { "TextOutputStream_Create", (void*)TextOutputStream_Create, 2, {"string", "boolean"}, "integer" },
        { "TextOutputStream_Write", (void*)TextOutputStream_Write, 2, {"integer", "string"}, "boolean" },
        { "TextOutputStream_WriteLine", (void*)TextOutputStream_WriteLine, 2, {"integer", "string"}, "boolean" },
        { "TextOutputStream_Flush", (void*)TextOutputStream_Flush, 1, {"integer"}, "boolean" },
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