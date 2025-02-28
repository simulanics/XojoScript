// TimeDatePlugin.cpp
// Build: g++ -shared -o TimeDatePlugin.dll TimeDatePlugin.cpp (Windows)
//        g++ -shared -fPIC -o TimeDatePlugin.so TimeDatePlugin.cpp (Linux/macOS)

#include <ctime>
#include <string>
#include <locale>
#include <sstream>
#include <iomanip>
#include <mutex>

#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#include <unistd.h> // for usleep on Unix
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

std::mutex timeMutex; // Ensures thread safety for shared resources

// Function to get the current date formatted for the user's locale
extern "C" XPLUGIN_API const char* XGetCurrentDate() {  // Renamed to XGetCurrentDate
    static std::string formattedDate;
    std::lock_guard<std::mutex> lock(timeMutex); // Ensures thread safety

    try {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
    #ifdef _WIN32
        localtime_s(&localTime, &now);
    #else
        localtime_r(&now, &localTime);
    #endif

        std::ostringstream oss;
        std::locale userLocale(""); // Uses the system's locale
        oss.imbue(userLocale);
        oss << std::put_time(&localTime, "%x"); // Locale-specific date format
        formattedDate = oss.str();
    } catch (...) {
        formattedDate = "Error retrieving date";
    }

    return formattedDate.c_str();
}

// Function to get the current time formatted for the user's locale
extern "C" XPLUGIN_API const char* XGetCurrentTime() {  // Renamed to XGetCurrentTime
    static std::string formattedTime;
    std::lock_guard<std::mutex> lock(timeMutex); // Ensures thread safety

    try {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
    #ifdef _WIN32
        localtime_s(&localTime, &now);
    #else
        localtime_r(&now, &localTime);
    #endif

        std::ostringstream oss;
        std::locale userLocale(""); // Uses the system's locale
        oss.imbue(userLocale);
        oss << std::put_time(&localTime, "%X"); // Locale-specific time format
        formattedTime = oss.str();
    } catch (...) {
        formattedTime = "Error retrieving time";
    }

    return formattedTime.c_str();
}

// Structure representing a plugin function entry.
typedef struct PluginEntry {
    const char* name;           // Function name in the interpreter
    void* funcPtr;              // Pointer to the function
    int arity;                  // Number of parameters
    const char* paramTypes[10]; // Parameter types (max 10)
    const char* retType;        // Return type
} PluginEntry;

// Define the available plugin functions
static PluginEntry pluginEntries[] = {
    { "GetCurrentDate", (void*)XGetCurrentDate, 0, {}, "string" }, // Updated name
    { "GetCurrentTime", (void*)XGetCurrentTime, 0, {}, "string" }  // Updated name
};

// Function to retrieve available plugin functions
extern "C" XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

#ifdef _WIN32
// Windows DLL entry point (Optional, can be omitted if not needed)
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
