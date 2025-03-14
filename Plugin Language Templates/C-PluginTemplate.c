// PluginTemplate.c
// Created by Matthew A Combatti
// Simulanics Technologies and Xojo Developers Studio
// https://www.simulanics.com
// https://www.xojostudio.org
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
// Build (Windows): cl /LD PluginTemplate.c /Fe:PluginTemplate.dll
// Build (macOS/Linux): gcc -shared -fPIC -o PluginTemplate.dylib(or .so) PluginTemplate.c

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #include <unistd.h>   // for usleep on Unix
  #include <sched.h>    // for sched_yield
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

// Definition of PluginEntry structure.
typedef struct PluginEntry {
    const char* name;               // Name used by the bytecode interpreter.
    void* funcPtr;                  // Pointer to the exported function.
    int arity;                      // Number of parameters the function accepts.
    const char* paramTypes[10];     // Array of parameter type strings.
    const char* retType;            // Return type string.
} PluginEntry;

// Sample exported function: adds two integers.
XPLUGIN_API int addtwonumbers(int a, int b) {
    return a + b;
}

// Sample exported function: says Hello to the user.
// Uses a static buffer to store the greeting.
XPLUGIN_API const char* sayhello(const char* name) {
    static char greeting[256];
    snprintf(greeting, sizeof(greeting), "Hello, %s", name);
    return greeting;
}

// Factorial function.
XPLUGIN_API int factorial(int n) {
    if(n <= 1)
        return 1;
    else
        return n * factorial(n - 1);
}

// Fibonacci function.
XPLUGIN_API int Fibonacci(int n2) {
    if(n2 <= 0)
        return 0;
    else if(n2 == 1)
        return 1;
    else
        return Fibonacci(n2 - 1) + Fibonacci(n2 - 2);
}

// Cross platform Beep function: Beep(Frequency, Duration) as Boolean.
XPLUGIN_API int XBeep(int frequency, int duration) {
#ifdef _WIN32
    return Beep(frequency, duration) != 0;
#else
    printf("\a");
    fflush(stdout);
    usleep(duration * 1000); // usleep takes microseconds.
    return 1; // true
#endif
}

// Cross platform Sleep function (renamed to PluginSleep).
XPLUGIN_API int PluginSleep(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
    return 1; // true
}

// DoEvents function - allows UI events to be processed.
XPLUGIN_API void DoEvents() {
#ifdef _WIN32
    MSG msg;
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
#else
    sched_yield();
#endif
}

// Static array of plugin entries; used by the host to load the plugin.
static PluginEntry pluginEntries[] = {
    { "AddTwoNumbers", (void*)addtwonumbers, 2,
      { "double", "double", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
      "double" },
    { "SayHello", (void*)sayhello, 1,
      { "string", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
      "string" },
    { "Factorial", (void*)factorial, 1,
      { "integer", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
      "integer" },
    { "Fibonacci", (void*)Fibonacci, 1,
      { "integer", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
      "integer" },
    { "Beep", (void*)XBeep, 2,
      { "integer", "integer", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
      "boolean" },
    { "Sleep", (void*)PluginSleep, 1,
      { "integer", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
      "boolean" },
    { "DoEvents", (void*)DoEvents, 0,
      { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
      "boolean" }
};

// Exported function to retrieve the plugin entries.
// The output parameter 'count' is set to the number of entries.
XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if(count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

#ifdef _WIN32
// Optional: DllMain for Windows-specific initialization.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch(ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif