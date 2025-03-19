// TimeTickerPlugin.cpp
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
// Revised plugin for dynamic event handling with added debug output.
// This plugin creates a TimeTicker instance that supports multiple events:
// e.g. "Trigger" (called every 00 or 30 seconds), "Error" (to report errors),
// "OnInitialize" (called once on initialization), etc.
//
// Compile (Linux/macOS):
//    g++ -shared -fPIC -o TimeTickerPlugin.so TimeTickerPlugin.cpp -std=c++11 -pthread
//
// Compile (Windows):
//    g++ -shared -o TimeTickerPlugin.dll TimeTickerPlugin.cpp -std=c++11 -pthread

#include <ctime>
#include <thread>
#include <atomic>
#include <map>
#include <string>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <iostream>  // For debug output

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

// Define callback types for various events.
typedef void (*TriggerCallback)(const char* currentTime);
typedef void (*ErrorCallback)(const char* errorMessage);
typedef void (*OnInitCallback)();

// Structure representing a TimeTicker instance with dynamic events.
struct TimeTickerInstance {
    int handle;                       // Unique handle for this instance.
    std::thread tickerThread;         // Thread running the ticker loop.
    std::atomic<bool> running;        // Controls when the thread should stop.
    // Map of event keys to callback pointers.
    // The key format is "plugin:<handle>:<eventName>"
    std::map<std::string, void*> eventCallbacks;
};

// Global container for instances.
static std::map<int, TimeTickerInstance*> timeTickerInstances;
static std::mutex instancesMutex;
static int nextHandle = 1; // Starting handle value.

// Helper to trigger an event callback if registered.
void triggerEvent(TimeTickerInstance* instance, const std::string& eventKey, const char* param) {
    // Debug: Log that an event is about to be triggered.
    std::cout << "DEBUG: triggerEvent called for eventKey: " << eventKey;
    if (param)
        std::cout << " with param: " << param;
    std::cout << std::endl;
    
    auto it = instance->eventCallbacks.find(eventKey);
    if (it != instance->eventCallbacks.end() && it->second != nullptr) {
        // Extract the event name.
        size_t pos = eventKey.rfind(":");
        if (pos != std::string::npos) {
            std::string eventName = eventKey.substr(pos + 1);
            if (eventName == "Trigger") {
                TriggerCallback cb = (TriggerCallback)it->second;
                std::cout << "DEBUG: Firing Trigger callback." << std::endl;
                cb(param);
            } else if (eventName == "Error") {
                ErrorCallback cb = (ErrorCallback)it->second;
                std::cout << "DEBUG: Firing Error callback." << std::endl;
                cb(param);
            } else if (eventName == "OnInitialize") {
                OnInitCallback cb = (OnInitCallback)it->second;
                std::cout << "DEBUG: Firing OnInitialize callback." << std::endl;
                cb();
            }
        }
    } else {
        std::cout << "DEBUG: No callback registered for eventKey: " << eventKey << std::endl;
    }
}

// Thread function that monitors the system time.
void TickerThreadFunction(TimeTickerInstance* instance) {
    // Trigger the OnInitialize event.
    std::string initKey = "plugin:" + std::to_string(instance->handle) + ":OnInitialize";
    std::cout << "DEBUG: TickerThreadFunction: Firing OnInitialize event with key: " << initKey << std::endl;
    triggerEvent(instance, initKey, nullptr);
    
    while (instance->running.load()) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        struct tm localTime;
#if defined(_WIN32)
        localtime_s(&localTime, &t);
#else
        localtime_r(&t, &localTime);
#endif
        // When seconds equal 0 or 30, trigger the "Trigger" event.
        if (localTime.tm_sec == 0 || localTime.tm_sec == 30) {
            char timeStr[9] = {0}; // "HH:MM:SS"
            std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &localTime);
            std::string triggerKey = "plugin:" + std::to_string(instance->handle) + ":Trigger";
            std::cout << "DEBUG: TickerThreadFunction: Firing Trigger event with time: " << timeStr << std::endl;
            triggerEvent(instance, triggerKey, timeStr);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// CreateTimeTicker:
// Creates a new TimeTicker instance, starts its ticker thread, and returns a unique handle.
extern "C" XPLUGIN_API int CreateTimeTicker() {
    std::lock_guard<std::mutex> lock(instancesMutex);
    int handle = nextHandle++;
    TimeTickerInstance* instance = new TimeTickerInstance();
    instance->handle = handle;
    instance->running.store(true);
    std::cout << "DEBUG: CreateTimeTicker: Creating instance with handle: " << handle << std::endl;
    instance->tickerThread = std::thread(TickerThreadFunction, instance);
    timeTickerInstances[handle] = instance;
    return handle;
}

// DestroyTimeTicker:
// Stops the ticker thread for the given handle and cleans up the instance.
extern "C" XPLUGIN_API bool DestroyTimeTicker(int handle) {
    std::lock_guard<std::mutex> lock(instancesMutex);
    auto it = timeTickerInstances.find(handle);
    if (it == timeTickerInstances.end()) return false;
    TimeTickerInstance* instance = it->second;
    instance->running.store(false);
    if (instance->tickerThread.joinable()) {
        instance->tickerThread.join();
    }
    std::cout << "DEBUG: DestroyTimeTicker: Destroying instance with handle: " << handle << std::endl;
    delete instance;
    timeTickerInstances.erase(it);
    return true;
}

// SetEventCallback:
// Sets a callback for a specific event on the TimeTicker instance.
// The target key must be in the format "plugin:<handle>:<eventName>".
// Returns true if the callback is set successfully.
extern "C" XPLUGIN_API bool SetEventCallback(int handle, const char* eventKey, void* callback) {
    if (!eventKey) return false;
    std::lock_guard<std::mutex> lock(instancesMutex);
    auto it = timeTickerInstances.find(handle);
    if (it == timeTickerInstances.end()) return false;
    TimeTickerInstance* instance = it->second;
    std::string key(eventKey);
    std::cout << "DEBUG: SetEventCallback: Registering callback for handle " << handle
              << " eventKey: " << key << " callback pointer: " << callback << std::endl;
    instance->eventCallbacks[key] = callback;
    return true;
}

// Plugin entry structure used by the bytecode interpreter.
struct PluginEntry {
    const char* name;           // Name of the plugin function.
    void* funcPtr;              // Pointer to the function.
    int arity;                  // Expected number of parameters.
    const char* paramTypes[10]; // Parameter type strings.
    const char* retType;        // Return type string.
};

// Plugin entries for the TimeTicker plugin.
static PluginEntry timeTickerPluginEntries[] = {
    {"CreateTimeTicker", (void*)CreateTimeTicker, 0, {nullptr}, "integer"},
    {"DestroyTimeTicker", (void*)DestroyTimeTicker, 1, {"integer"}, "boolean"},
    {"SetEventCallback", (void*)SetEventCallback, 3, {"integer", "string", "pointer"}, "boolean"}
};

// GetPluginEntries:
// Returns the array of plugin entries and sets the count.
extern "C" XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(timeTickerPluginEntries) / sizeof(PluginEntry);
    }
    return timeTickerPluginEntries;
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
