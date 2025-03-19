// LLMPlugin.cpp
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
// Build: g++ -shared -fPIC -o LLMPlugin.so LLMPlugin.cpp -lcurl (Linux/macOS)
//        g++ -shared -o LLMConnection.dll LLMConnection.cpp -static -lcurl -lcurl.dll (Windows)
// Plugin uses files from: https://github.com/olrea/openai-cpp


// Header Dependencies: 
//https://github.com/nlohmann/json
#include <nlohmann/json.hpp>
////https://github.com/olrea/openai-cpp
#include "openai/openai.hpp"
#include <string>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#include <unistd.h> // for usleep on Unix
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

using json = nlohmann::json;

// Global variables for API configuration, protected by mutex
static std::string apiHost = "https://api.openai.com/v1/";
static std::string apiKey = "";
static std::string organization = "";
static std::mutex configMutex;

extern "C" {

// Function to set API host globally
XPLUGIN_API void LLMSetAPIHost(const char* host) {
    if (host) {
        std::lock_guard<std::mutex> lock(configMutex);
        apiHost = std::string(host);
    }
}

// Function to set API key and organization globally (optional for some models)
XPLUGIN_API void LLMSetConfiguration(const char* key, const char* org) {
    std::lock_guard<std::mutex> lock(configMutex);
    if (key) {
        apiKey = std::string(key);
    }
    if (org) {
        organization = std::string(org);
    }
}

// Function to create text completion
XPLUGIN_API const char* LLMCreateCompletion(const char* model, const char* prompt, int max_tokens, double temperature) {
    static std::string responseText;  // Static to ensure persistence after function returns
    if (!model || !prompt || max_tokens <= 0 || temperature < 0.0 || temperature > 1.0) {
        return "Error: Invalid parameters.";
    }

    // Ensure API configuration is set correctly
    {
        std::lock_guard<std::mutex> lock(configMutex);
        openai::instance().setBaseUrl(apiHost);
        if (!apiKey.empty()) {
            openai::start(apiKey.c_str(), organization.empty() ? nullptr : organization.c_str());
        }
    }

    try {
        // Construct the JSON request properly
        json requestPayload = {
            { "model", model },
            { "prompt", prompt },
            { "max_tokens", max_tokens },
            { "temperature", temperature }
        };

        // Send request
        auto completion = openai::completion().create(requestPayload);
        
        // Extract response text
        responseText = completion["choices"][0]["text"].get<std::string>();
        return responseText.c_str();
    } catch (const std::exception& e) {
        return "Error: Exception during API request.";
    }
}

// Function to create an image generation request
XPLUGIN_API const char* LLMCreateImage(const char* model, const char* prompt, int n, const char* size) {
    static std::string imageUrl;
    if (!model || !prompt || n <= 0 || !size) {
        return "Error: Invalid parameters.";
    }

    {
        std::lock_guard<std::mutex> lock(configMutex);
        openai::instance().setBaseUrl(apiHost);
        if (!apiKey.empty()) {
            openai::start(apiKey.c_str(), organization.empty() ? nullptr : organization.c_str());
        }
    }

    try {
        // Construct JSON request for image generation
        json requestPayload = {
            { "model", model },
            { "prompt", prompt },
            { "n", n },
            { "size", size }
        };

        // Send request
        auto image = openai::image().create(requestPayload);

        // Extract image URL
        imageUrl = image["data"][0]["url"].get<std::string>();
        return imageUrl.c_str();
    } catch (const std::exception& e) {
        return "Error: Exception during API request.";
    }
}

// Plugin entry structure
typedef struct PluginEntry {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} PluginEntry;

// List of available plugin functions
static PluginEntry pluginEntries[] = {
    { "LLMSetAPIHost", (void*)LLMSetAPIHost, 1, {"string"}, "string" },
    { "LLMSetConfiguration", (void*)LLMSetConfiguration, 2, {"string", "string"}, "string" },
    { "LLMCreateCompletion", (void*)LLMCreateCompletion, 4, {"string", "string", "integer", "double"}, "string" },
    { "LLMCreateImage", (void*)LLMCreateImage, 4, {"string", "string", "integer", "string"}, "string" }
};

// Function to retrieve plugin entries
XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

} // extern "C"

#ifdef _WIN32
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
