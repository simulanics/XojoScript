// PluginTemplate.cpp 
    
#include <cstdio>
#include <string>
       
#ifdef _WIN32   
#include <windows.h>   
#define XPLUGIN_API __declspec(dllexport)   
#else   
#define XPLUGIN_API __attribute__((visibility("default")))   
#endif
       
extern "C" {  
    // Type definition for a plugin function that takes two integers and returns an integer.
    
    typedef int (*PluginFuncInt)(int, int);
      
    // Structure representing a plugin function entry.
    
    typedef struct PluginEntry {
        const char* name;    // Name to be used by the bytecode interpreter.
    
        void* funcPtr;       // Pointer to the exported function.
    
        int arity;           // Number of parameters the function accepts.
    
        const char* paramTypes[10];  // Pass the parameter datatypes to the compiler.
    
        const char* retType;     // Expose method return datatype to the compiler.
    
    } PluginEntry;
 
    // Sample exported function: adds two integers.
    
    XPLUGIN_API int addtwonumbers(int a, int b) {
        return a + b;
    }
  
    
    // Sample exported function: Says Hello to the user.
   
    XPLUGIN_API const char* sayhello(const char* name) {
        static std::string greeting;
        greeting = "Hello, " + std::string(name);
        return greeting.c_str();
    
    }
    
  
    // Static array of plugin method entries; add more entries as needed.
    
    static PluginEntry pluginEntries[] = {
        { "addtwonumbers", (void*)addtwonumbers, 2, {"double", "double"}, "double" },
    
        { "sayhello", (void*)sayhello, 1, {"string"}, "string" }
    
    };
    
    
    // Exported function to retrieve the plugin entries.    
    // 'count' will be set to the number of entries.
    XPLUGIN_API PluginEntry* GetPluginEntries(int* count) { 
        if (count) { 
            *count = sizeof(pluginEntries) / sizeof(PluginEntry);  
        } 
        return pluginEntries;  
    }
    
    
} // extern "C"
    
  
#ifdef _WIN32   
// Optional: DllMain for Windows-specific initialization.
    
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