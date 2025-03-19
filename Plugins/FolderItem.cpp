// XojoFolderItemPlugin.cpp
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
// Build: g++ -shared -fPIC -o FolderItem.so FolderItem.cpp (Linux/macOS)
//        g++ -shared -o FolderItem.dll FolderItem.cpp (Windows)

#include <string>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define XPLUGIN_API __declspec(dllexport)
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#include <sys/types.h>
#define XPLUGIN_API __attribute__((visibility("default")))
#define PATH_SEPARATOR "/"
#endif

extern "C" {

// Checks if a file or directory exists
XPLUGIN_API bool FolderItem_Exists(const char* path) {
    struct stat info;
    return (stat(path, &info) == 0);
}

// Deletes a file or directory
XPLUGIN_API bool FolderItem_Delete(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) return false;

    if (S_ISDIR(info.st_mode)) {
#ifdef _WIN32
        return (_rmdir(path) == 0);
#else
        return (rmdir(path) == 0);
#endif
    } else {
        return (remove(path) == 0);
    }
}

// Creates a directory
XPLUGIN_API bool FolderItem_CreateDirectory(const char* path) {
#ifdef _WIN32
    return (_mkdir(path) == 0);
#else
    return (mkdir(path, 0777) == 0); // 0777 for full permissions
#endif
}

// Checks if the given path is a directory
XPLUGIN_API bool FolderItem_IsDirectory(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) return false;
    return S_ISDIR(info.st_mode);
}

// Returns the file size in bytes (0 for directories or errors)
XPLUGIN_API int FolderItem_Size(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0 || S_ISDIR(info.st_mode)) return 0;
    return static_cast<int>(info.st_size);
}

// Returns the full path as a string
XPLUGIN_API const char* FolderItem_GetPath(const char* path) {
    static std::string fullPath;
#ifdef _WIN32
    char absolutePath[MAX_PATH];
    if (_fullpath(absolutePath, path, MAX_PATH)) {
        fullPath = absolutePath;
    } else {
        fullPath = path;
    }
#else
    char absolutePath[PATH_MAX];
    if (realpath(path, absolutePath)) {
        fullPath = absolutePath;
    } else {
        fullPath = path;
    }
#endif
    return fullPath.c_str();
}

// Gets the Unix-style file permissions (e.g., 755)
XPLUGIN_API int FolderItem_GetPermission(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) return -1; // Error case
    return (info.st_mode & 0777); // Extract permission bits
}

// Sets the Unix-style file permissions (e.g., 755)
XPLUGIN_API bool FolderItem_SetPermission(const char* path, int permission) {
#ifdef _WIN32
    return false; // Not supported on Windows
#else
    return (chmod(path, permission) == 0);
#endif
}

// Converts a file path to a URL path (file://...)
XPLUGIN_API const char* FolderItem_URLPath(const char* path) {
    static std::string urlPath = "file://";
#ifdef _WIN32
    urlPath += "/";
#endif
    urlPath += path;
    for (size_t i = 0; i < urlPath.size(); ++i) {
        if (urlPath[i] == '\\') urlPath[i] = '/'; // Convert Windows slashes to URL format
    }
    return urlPath.c_str();
}

// Returns a shell-safe file path (escapes spaces)
XPLUGIN_API const char* FolderItem_ShellPath(const char* path) {
    static std::string shellPath = path;
    std::string result;
    for (char c : shellPath) {
        if (c == ' ') result += "\\ "; // Escape spaces
        else result += c;
    }
    shellPath = result;
    return shellPath.c_str();
}

// Plugin function entries
typedef struct PluginEntry {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} PluginEntry;

static PluginEntry pluginEntries[] = {
    {"FolderItem_Exists", (void*)FolderItem_Exists, 1, {"string"}, "boolean"},
    {"FolderItem_Delete", (void*)FolderItem_Delete, 1, {"string"}, "boolean"},
    {"FolderItem_CreateDirectory", (void*)FolderItem_CreateDirectory, 1, {"string"}, "boolean"},
    {"FolderItem_IsDirectory", (void*)FolderItem_IsDirectory, 1, {"string"}, "boolean"},
    {"FolderItem_Size", (void*)FolderItem_Size, 1, {"string"}, "integer"},
    {"FolderItem_GetPath", (void*)FolderItem_GetPath, 1, {"string"}, "string"},
    {"FolderItem_GetPermission", (void*)FolderItem_GetPermission, 1, {"string"}, "integer"},
    {"FolderItem_SetPermission", (void*)FolderItem_SetPermission, 2, {"string", "integer"}, "boolean"},
    {"FolderItem_URLPath", (void*)FolderItem_URLPath, 1, {"string"}, "string"},
    {"FolderItem_ShellPath", (void*)FolderItem_ShellPath, 1, {"string"}, "string"},
};

// Exported function to retrieve plugin entries
XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

#ifdef _WIN32
// Optional DllMain for Windows-specific initialization
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
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

} // extern "C"