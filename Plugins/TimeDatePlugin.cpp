// TimeDatePlugin.cpp
// Build: g++ -shared -fPIC -o TimeDatePlugin.so TimeDatePlugin.cpp (Linux/macOS)
//        g++ -shared -o TimeDatePlugin.dll TimeDatePlugin.cpp (Windows)

#include <ctime>
#include <string>
#include <locale>
#include <sstream>
#include <iomanip>
#include <mutex>

#ifdef _WIN32
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

std::mutex timeMutex; // Ensures thread safety for shared resources

// Function to get the current date formatted for the user's locale
extern "C" XPLUGIN_API const char* GetCurrentDate() {
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
extern "C" XPLUGIN_API const char* GetCurrentTime() {
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
