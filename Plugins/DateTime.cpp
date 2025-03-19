// XojoDateTimePlugin.cpp
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
// Build: g++ -shared -fPIC -o DateTimePlugin.so XojoDateTimePlugin.cpp (Linux/macOS)
//        g++ -shared -o DateTimePlugin.dll XojoDateTimePlugin.cpp (Windows)

#include <ctime>
#include <map>
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#include <unistd.h>
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

// Structure to store DateTime instances
struct DateTime {
    std::tm timeData;
};

// Store instances with unique handles
std::map<int, DateTime*> dateTimeInstances;
int currentHandle = 1; // Unique handle counter

extern "C" {

// Creates a new DateTime instance
XPLUGIN_API int DateTime_Create(int year, int month, int day, int hour, int minute, int second) {
    DateTime* dt = new DateTime();
    dt->timeData = {0};
    dt->timeData.tm_year = year - 1900;  // tm_year is years since 1900
    dt->timeData.tm_mon = month - 1;     // tm_mon is 0-based
    dt->timeData.tm_mday = day;
    dt->timeData.tm_hour = hour;
    dt->timeData.tm_min = minute;
    dt->timeData.tm_sec = second;
    dt->timeData.tm_isdst = -1; // Let the system determine DST

    dateTimeInstances[currentHandle] = dt;
    return currentHandle++;
}

// Creates a DateTime instance for the current date/time
XPLUGIN_API int DateTime_Now() {
    time_t now = time(nullptr);
    struct tm* localTime = localtime(&now);
    
    if (!localTime) return 0; // Return 0 on failure

    DateTime* dt = new DateTime();
    dt->timeData = *localTime;
    dateTimeInstances[currentHandle] = dt;
    return currentHandle++;
}

// Retrieves the year
XPLUGIN_API int DateTime_GetYear(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->timeData.tm_year + 1900;
}

// Retrieves the month (1-12)
XPLUGIN_API int DateTime_GetMonth(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->timeData.tm_mon + 1;
}

// Retrieves the day (1-31)
XPLUGIN_API int DateTime_GetDay(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->timeData.tm_mday;
}

// Retrieves the hour (0-23)
XPLUGIN_API int DateTime_GetHour(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->timeData.tm_hour;
}

// Retrieves the minute (0-59)
XPLUGIN_API int DateTime_GetMinute(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->timeData.tm_min;
}

// Retrieves the second (0-59)
XPLUGIN_API int DateTime_GetSecond(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->timeData.tm_sec;
}

// Returns a string representation of the DateTime
XPLUGIN_API const char* DateTime_ToString(int handle) {
    static std::string formattedTime;
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return "Invalid Handle";

    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &dateTimeInstances[handle]->timeData);
    formattedTime = buffer;
    return formattedTime.c_str();
}

// Destroys a DateTime instance
XPLUGIN_API bool DateTime_Destroy(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return false;
    
    delete dateTimeInstances[handle];
    dateTimeInstances.erase(handle);
    return true;
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
    {"DateTime_Create", (void*)DateTime_Create, 6, {"integer", "integer", "integer", "integer", "integer", "integer"}, "integer"},
    {"DateTime_Now", (void*)DateTime_Now, 0, {}, "integer"},
    {"DateTime_GetYear", (void*)DateTime_GetYear, 1, {"integer"}, "integer"},
    {"DateTime_GetMonth", (void*)DateTime_GetMonth, 1, {"integer"}, "integer"},
    {"DateTime_GetDay", (void*)DateTime_GetDay, 1, {"integer"}, "integer"},
    {"DateTime_GetHour", (void*)DateTime_GetHour, 1, {"integer"}, "integer"},
    {"DateTime_GetMinute", (void*)DateTime_GetMinute, 1, {"integer"}, "integer"},
    {"DateTime_GetSecond", (void*)DateTime_GetSecond, 1, {"integer"}, "integer"},
    {"DateTime_ToString", (void*)DateTime_ToString, 1, {"integer"}, "string"},
    {"DateTime_Destroy", (void*)DateTime_Destroy, 1, {"integer"}, "boolean"},
};

// Exported function to retrieve plugin entries
XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

#ifdef _WIN32
// Windows-specific initialization
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
