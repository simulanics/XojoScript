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
#include <unordered_map>
#include <vector>
#include <cstring>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

std::unordered_map<int, std::vector<char>> memoryBlocks;
std::mutex memoryMutex;
int memoryHandleCounter = 1;

// Creates a new memory block
extern "C" XPLUGIN_API int MemoryBlock_Create(int size) {
    if (size <= 0) return -1;
    std::lock_guard<std::mutex> lock(memoryMutex);
    int handle = memoryHandleCounter++;
    memoryBlocks[handle] = std::vector<char>(size);
    return handle;
}

// Returns the size of the memory block
extern "C" XPLUGIN_API int MemoryBlock_Size(int handle) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    return (it != memoryBlocks.end()) ? it->second.size() : -1;
}

// Resizes the memory block
extern "C" XPLUGIN_API void MemoryBlock_Resize(int handle, int newSize) {
    if (newSize <= 0) return;
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it != memoryBlocks.end()) {
        it->second.resize(newSize);
    }
}

// Frees a memory block
extern "C" XPLUGIN_API void MemoryBlock_Destroy(int handle) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    memoryBlocks.erase(handle);
}

// Reads a single byte
extern "C" XPLUGIN_API int MemoryBlock_ReadByte(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    return (it != memoryBlocks.end() && offset >= 0 && offset < it->second.size()) ? static_cast<unsigned char>(it->second[offset]) : -1;
}

// Reads a 2-byte short integer
extern "C" XPLUGIN_API int MemoryBlock_ReadShort(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it == memoryBlocks.end() || offset < 0 || offset + 1 >= it->second.size()) return -1;
    short value;
    std::memcpy(&value, &it->second[offset], sizeof(value));
    return value;
}

// Reads a 4-byte long integer
extern "C" XPLUGIN_API int MemoryBlock_ReadLong(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it == memoryBlocks.end() || offset < 0 || offset + 3 >= it->second.size()) return -1;
    int value;
    std::memcpy(&value, &it->second[offset], sizeof(value));
    return value;
}

// Reads an 8-byte floating-point number
extern "C" XPLUGIN_API double MemoryBlock_ReadDouble(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it == memoryBlocks.end() || offset < 0 || offset + 7 >= it->second.size()) return -1.0;
    double value;
    std::memcpy(&value, &it->second[offset], sizeof(value));
    return value;
}

// Reads a specified number of bytes as a string
extern "C" XPLUGIN_API const char* MemoryBlock_ReadString(int handle, int offset, int length) {
    static thread_local std::string buffer;
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it == memoryBlocks.end() || offset < 0 || offset + length > it->second.size()) return "";
    buffer.assign(it->second.begin() + offset, it->second.begin() + offset + length);
    return buffer.c_str();
}

// Writes a single byte
extern "C" XPLUGIN_API void MemoryBlock_WriteByte(int handle, int offset, int value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it != memoryBlocks.end() && offset >= 0 && offset < it->second.size()) {
        it->second[offset] = static_cast<char>(value);
    }
}

// Writes a 2-byte short integer
extern "C" XPLUGIN_API void MemoryBlock_WriteShort(int handle, int offset, int value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it != memoryBlocks.end() && offset >= 0 && offset + 1 < it->second.size()) {
        std::memcpy(&it->second[offset], &value, sizeof(short));
    }
}

// Writes a 4-byte long integer
extern "C" XPLUGIN_API void MemoryBlock_WriteLong(int handle, int offset, int value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it != memoryBlocks.end() && offset >= 0 && offset + 3 < it->second.size()) {
        std::memcpy(&it->second[offset], &value, sizeof(int));
    }
}

// Writes an 8-byte floating-point number
extern "C" XPLUGIN_API void MemoryBlock_WriteDouble(int handle, int offset, double value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it != memoryBlocks.end() && offset >= 0 && offset + 7 < it->second.size()) {
        std::memcpy(&it->second[offset], &value, sizeof(double));
    }
}

// Writes a string at the given offset
extern "C" XPLUGIN_API void MemoryBlock_WriteString(int handle, int offset, const char* value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlocks.find(handle);
    if (it != memoryBlocks.end() && offset >= 0 && offset + std::strlen(value) <= it->second.size()) {
        std::memcpy(&it->second[offset], value, std::strlen(value));
    }
}

// Copies memory between memory blocks
extern "C" XPLUGIN_API void MemoryBlock_CopyMemory(int destHandle, int destOffset, int srcHandle, int srcOffset, int length) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto srcIt = memoryBlocks.find(srcHandle);
    auto destIt = memoryBlocks.find(destHandle);
    if (srcIt != memoryBlocks.end() && destIt != memoryBlocks.end() &&
        srcOffset >= 0 && destOffset >= 0 &&
        srcOffset + length <= srcIt->second.size() &&
        destOffset + length <= destIt->second.size()) {
        std::memcpy(&destIt->second[destOffset], &srcIt->second[srcOffset], length);
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
        { "MemoryBlock_Create", (void*)MemoryBlock_Create, 1, {"integer"}, "integer" },
        { "MemoryBlock_Size", (void*)MemoryBlock_Size, 1, {"integer"}, "integer" },
        { "MemoryBlock_Resize", (void*)MemoryBlock_Resize, 2, {"integer", "integer"}, "boolean" },
        { "MemoryBlock_Destroy", (void*)MemoryBlock_Destroy, 1, {"integer"}, "boolean" },
        { "MemoryBlock_ReadByte", (void*)MemoryBlock_ReadByte, 2, {"integer", "integer"}, "integer" },
        { "MemoryBlock_ReadShort", (void*)MemoryBlock_ReadShort, 2, {"integer", "integer"}, "integer" },
        { "MemoryBlock_ReadLong", (void*)MemoryBlock_ReadLong, 2, {"integer", "integer"}, "integer" },
        { "MemoryBlock_ReadDouble", (void*)MemoryBlock_ReadDouble, 2, {"integer", "integer"}, "double" },
        { "MemoryBlock_ReadString", (void*)MemoryBlock_ReadString, 3, {"integer", "integer", "integer"}, "string" },
        { "MemoryBlock_WriteByte", (void*)MemoryBlock_WriteByte, 3, {"integer", "integer", "integer"}, "boolean" },
        { "MemoryBlock_WriteShort", (void*)MemoryBlock_WriteShort, 3, {"integer", "integer", "integer"}, "boolean" },
        { "MemoryBlock_WriteLong", (void*)MemoryBlock_WriteLong, 3, {"integer", "integer", "integer"}, "boolean" },
        { "MemoryBlock_WriteDouble", (void*)MemoryBlock_WriteDouble, 3, {"integer", "integer", "double"}, "boolean" },
        { "MemoryBlock_WriteString", (void*)MemoryBlock_WriteString, 3, {"integer", "integer", "string"}, "boolean" },
        { "MemoryBlock_CopyMemory", (void*)MemoryBlock_CopyMemory, 5, {"integer", "integer", "integer", "integer", "integer"}, "boolean" }
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