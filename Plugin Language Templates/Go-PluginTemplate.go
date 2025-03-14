// PluginTemplate.go
// Created by Matthew A Combatti
// Simulanics Technologies and Xojo Developers Studio
// -----------------------------------------------------------------------------
// Build commands:
// Windows: go build -buildmode=c-shared -o PluginTemplate.dll PluginTemplate.go
// Linux:   go build -buildmode=c-shared -o PluginTemplate.so PluginTemplate.go
// macOS:   go build -buildmode=c-shared -o PluginTemplate.dylib PluginTemplate.go

package main

/*
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

// Define the PluginEntry structure.
typedef struct PluginEntry {
    const char* name;                 // Name used by the bytecode interpreter.
    void* funcPtr;                    // Pointer to the exported function.
    int arity;                        // Number of parameters the function accepts.
    const char* paramTypes[10];       // Parameter datatypes.
    const char* retType;              // Return datatype.
} PluginEntry;

// Extern declarations for the functions implemented in Go.
extern int addtwonumbers(int, int);
extern const char* sayhello(const char*);
extern int factorial(int);
extern int Fibonacci(int);
extern bool XBeep(int, int);
extern bool PluginSleep(int);
extern void DoEvents();

// Static array of plugin entries.
static PluginEntry pluginEntries[] = {
    { "AddTwoNumbers", (void*)addtwonumbers, 2, {"double", "double"}, "double" },
    { "SayHello",      (void*)sayhello,      1, {"string"}, "string" },
    { "Factorial",     (void*)factorial,     1, {"integer"}, "integer" },
    { "Fibonacci",     (void*)Fibonacci,     1, {"integer"}, "integer" },
    { "Beep",          (void*)XBeep,         2, {"integer", "integer"}, "boolean" },
    { "Sleep",         (void*)PluginSleep,   1, {"integer"}, "boolean" },
    { "DoEvents",      (void*)DoEvents,      0, {NULL}, "boolean" }
};

static int pluginEntryCount = sizeof(pluginEntries) / sizeof(PluginEntry);

// Exported function to retrieve the plugin entries.
PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = pluginEntryCount;
    }
    return pluginEntries;
}
*/
import "C"
import (
	"fmt"
	"runtime"
	"syscall"
	"time"
)

//export addtwonumbers
func addtwonumbers(a C.int, b C.int) C.int {
	return a + b
}

//export sayhello
func sayhello(name *C.char) *C.char {
	goName := C.GoString(name)
	greeting := "Hello, " + goName
	return C.CString(greeting)
}

//export factorial
func factorial(n C.int) C.int {
	if n <= 1 {
		return 1
	}
	return n * factorial(n-1)
}

//export Fibonacci
func Fibonacci(n C.int) C.int {
	if n <= 0 {
		return 0
	} else if n == 1 {
		return 1
	}
	return Fibonacci(n-1) + Fibonacci(n-2)
}

//export XBeep
func XBeep(frequency C.int, duration C.int) C.bool {
	if runtime.GOOS == "windows" {
		// Call Windows' Beep function from kernel32.dll.
		kernel32 := syscall.NewLazyDLL("kernel32.dll")
		beepProc := kernel32.NewProc("Beep")
		ret, _, _ := beepProc.Call(uintptr(frequency), uintptr(duration))
		if ret != 0 {
			return C.bool(true)
		}
		return C.bool(false)
	} else {
		// On macOS/Linux, print the bell character and sleep.
		fmt.Print("\a")
		time.Sleep(time.Duration(duration) * time.Millisecond)
		return C.bool(true)
	}
}

//export PluginSleep
func PluginSleep(ms C.int) C.bool {
	time.Sleep(time.Duration(ms) * time.Millisecond)
	return C.bool(true)
}

//export DoEvents
func DoEvents() {
	// Yield processor to allow other tasks to run.
	runtime.Gosched()
}

func main() {}