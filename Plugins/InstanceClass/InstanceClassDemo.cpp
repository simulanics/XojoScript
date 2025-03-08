#include <mutex>
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#include <unistd.h>
#endif

//------------------------------------------------------------------------------
// Instance class that holds a single integer value with thread‐safe access
//------------------------------------------------------------------------------
class Instance {
private:
    int value;
    std::mutex instanceMutex;
public:
    Instance() : value(0) { }
    void setValue(int newValue) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        value = newValue;
    }
    int getValue() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        return value;
    }
};

//------------------------------------------------------------------------------
// Global tracking of allocated instances to avoid memory leaks
//------------------------------------------------------------------------------
static std::mutex globalSetMutex;
static std::unordered_set<Instance*> instanceSet;

//------------------------------------------------------------------------------
// Extern "C" export block so symbols are exported with unmangled names
//------------------------------------------------------------------------------
extern "C" {

// Constructor: returns a new Instance pointer
XPLUGIN_API void* Constructor() {
    Instance* newInstance = new Instance();
    {
        std::lock_guard<std::mutex> lock(globalSetMutex);
        instanceSet.insert(newInstance);
    }
    return newInstance;
}

// SetValue: sets the instance's value
XPLUGIN_API void SetValue(void* instance, int value) {
    if (!instance) return;
    static_cast<Instance*>(instance)->setValue(value);
}

// GetValue: returns the instance's value
XPLUGIN_API int GetValue(void* instance) {
    if (!instance) return 0;
    return static_cast<Instance*>(instance)->getValue();
}

// Destructor: deletes the instance and removes it from tracking
XPLUGIN_API void Destructor(void* instance) {
    if (!instance) return;
    {
        std::lock_guard<std::mutex> lock(globalSetMutex);
        instanceSet.erase(static_cast<Instance*>(instance));
    }
    delete static_cast<Instance*>(instance);
}

// Getter for the property "Value"
XPLUGIN_API int ValueGetter(void* instance) {
    return GetValue(instance);
}

// Setter for the property "Value"
XPLUGIN_API void ValueSetter(void* instance, int newValue) {
    SetValue(instance, newValue);
}

//------------------------------------------------------------------------------
// Definition of the class property structure
//------------------------------------------------------------------------------
typedef struct {
    const char* name;
    const char* type;
    void* getter;
    void* setter;
} ClassProperty;

//------------------------------------------------------------------------------
// Definition of the class method (entry) structure
//------------------------------------------------------------------------------
typedef struct {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} ClassEntry;

//------------------------------------------------------------------------------
// Definition of the class constant structure
//------------------------------------------------------------------------------
typedef struct {
    const char* declaration;
} ClassConstant;

//------------------------------------------------------------------------------
// Definition of the complete class definition structure
//------------------------------------------------------------------------------
typedef struct {
    const char* className;
    size_t classSize;
    void* constructor;
    ClassProperty* properties;
    size_t propertiesCount;
    ClassEntry* methods;
    size_t methodsCount;
    ClassConstant* constants;
    size_t constantsCount;
} ClassDefinition;

//------------------------------------------------------------------------------
// Static array of property definitions (only one property: "Value")
//------------------------------------------------------------------------------
static ClassProperty ClassProperties[] = {
    { "Value", "integer", (void*)ValueGetter, (void*)ValueSetter }
};

//------------------------------------------------------------------------------
// Static array of method definitions (two methods: SetValue and GetValue)
//------------------------------------------------------------------------------
static ClassEntry ClassEntries[] = {
    { "SetValue", (void*)SetValue, 2, {"ptr", "integer"}, "variant" },
    { "GetValue", (void*)GetValue, 1, {"ptr"}, "integer" }
};

//------------------------------------------------------------------------------
// Static array of constant definitions
//------------------------------------------------------------------------------
static ClassConstant ClassConstants[] = {
    { "kMaxValue as Integer = 100" }
};

//------------------------------------------------------------------------------
// Complete class definition for the Instance class plugin
//------------------------------------------------------------------------------
static ClassDefinition InstanceClass = {
    "Instance",                      // className
    sizeof(Instance),                // classSize
    (void*)Constructor,              // constructor
    ClassProperties,                 // properties
    sizeof(ClassProperties) / sizeof(ClassProperty),  // propertiesCount
    ClassEntries,                    // methods
    sizeof(ClassEntries) / sizeof(ClassEntry),        // methodsCount
    ClassConstants,                  // constants
    sizeof(ClassConstants) / sizeof(ClassConstant)      // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition
//------------------------------------------------------------------------------
XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &InstanceClass;
}

} // end extern "C"

#ifdef _WIN32
// Optional: DllMain for Windows initialization
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
