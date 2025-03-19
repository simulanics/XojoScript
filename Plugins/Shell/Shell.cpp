// XojoShellPlugin.cpp
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
// Compile (Linux/macOS):
//    g++ -shared -fPIC -o Shell.so Shell.cpp -pthread
//
// Compile (Windows):
//    g++ -shared -o Shell.dll Shell.cpp -pthread

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

#define XPLUGIN_API __declspec(dllexport)

class ShellInstance {
public:
    ShellInstance() : exitCode(-1), running(false), timeoutSeconds(5) {}

    bool Execute(const std::string& command);
    std::string GetOutput();
    int GetExitCode();
    void SetTimeout(int seconds);
    bool Kill();

private:
    std::string output;
    int exitCode;
    bool running;
    int timeoutSeconds;
#ifdef _WIN32
    PROCESS_INFORMATION processInfo;
#else
    pid_t pid;
#endif
    std::mutex shellMutex;
};

// Stores all shell instances
std::map<int, std::shared_ptr<ShellInstance>> shellInstances;
std::mutex globalMutex;
int nextInstanceId = 1;

// Implementation of ShellInstance Methods
bool ShellInstance::Execute(const std::string& command) {
    std::lock_guard<std::mutex> lock(shellMutex);
    output.clear();
    exitCode = -1;
    running = true;

#ifdef _WIN32
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        running = false;
        return false;
    }

    STARTUPINFO si = { sizeof(STARTUPINFO) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.hStdInput = NULL;

    processInfo = { 0 };
    if (!CreateProcess(NULL, const_cast<char*>(command.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &processInfo)) {
        running = false;
        return false;
    }

    CloseHandle(hWritePipe);

    char buffer[128];
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    CloseHandle(hReadPipe);

    WaitForSingleObject(processInfo.hProcess, timeoutSeconds * 1000);
    DWORD exitCodeWin;
    GetExitCodeProcess(processInfo.hProcess, &exitCodeWin);
    exitCode = static_cast<int>(exitCodeWin);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
#else
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        running = false;
        return false;
    }

    pid = fork();
    if (pid == -1) {
        running = false;
        return false;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execl("/bin/sh", "sh", "-c", command.c_str(), (char*)NULL);
        _exit(127);
    }

    close(pipefd[1]);
    char buffer[128];
    ssize_t bytesRead;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    } else {
        exitCode = -1;
    }
#endif

    running = false;
    return true;
}

void ShellInstance::SetTimeout(int seconds) {
    std::lock_guard<std::mutex> lock(shellMutex);
    timeoutSeconds = (seconds > 0) ? seconds : 5;
}

std::string ShellInstance::GetOutput() {
    std::lock_guard<std::mutex> lock(shellMutex);
    return output;
}

int ShellInstance::GetExitCode() {
    std::lock_guard<std::mutex> lock(shellMutex);
    return exitCode;
}

bool ShellInstance::Kill() {
    std::lock_guard<std::mutex> lock(shellMutex);
    if (!running) return false;

#ifdef _WIN32
    TerminateProcess(processInfo.hProcess, 1);
#else
    kill(pid, SIGKILL);
#endif

    running = false;
    return true;
}

extern "C" {

// Creates a new shell instance
XPLUGIN_API int Shell_Create() {
    std::lock_guard<std::mutex> lock(globalMutex);
    int id = nextInstanceId++;
    shellInstances[id] = std::make_shared<ShellInstance>();
    return id;
}

// Executes a command in the shell instance
XPLUGIN_API bool Shell_Execute(int id, const char* command) {
    std::lock_guard<std::mutex> lock(globalMutex);
    auto it = shellInstances.find(id);
    if (it == shellInstances.end()) return false;
    return it->second->Execute(command);
}

// Sets timeout for a shell instance
XPLUGIN_API void Shell_SetTimeout(int id, int seconds) {
    std::lock_guard<std::mutex> lock(globalMutex);
    auto it = shellInstances.find(id);
    if (it != shellInstances.end()) {
        it->second->SetTimeout(seconds);
    }
}

// Gets output from a shell instance
XPLUGIN_API const char* Shell_Result(int id) {
    std::lock_guard<std::mutex> lock(globalMutex);
    auto it = shellInstances.find(id);
    if (it == shellInstances.end()) return "";
    static std::string output = it->second->GetOutput();
    return output.c_str();
}

// Gets exit code from a shell instance
XPLUGIN_API int Shell_ExitCode(int id) {
    std::lock_guard<std::mutex> lock(globalMutex);
    auto it = shellInstances.find(id);
    if (it == shellInstances.end()) return -1;
    return it->second->GetExitCode();
}

// Kills the running process in the shell instance
XPLUGIN_API bool Shell_Kill(int id) {
    std::lock_guard<std::mutex> lock(globalMutex);
    auto it = shellInstances.find(id);
    if (it == shellInstances.end()) return false;
    return it->second->Kill();
}

// Destroys a shell instance
XPLUGIN_API bool Shell_Destroy(int id) {
    std::lock_guard<std::mutex> lock(globalMutex);
    return shellInstances.erase(id) > 0;
}

// Plugin function entry structure
typedef struct PluginEntry {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} PluginEntry;

// Plugin function entries
static PluginEntry pluginEntries[] = {
    {"Shell_Create", (void*)Shell_Create, 0, {}, "integer"},
    {"Shell_Execute", (void*)Shell_Execute, 2, {"integer", "string"}, "boolean"},
    {"Shell_SetTimeout", (void*)Shell_SetTimeout, 2, {"integer", "integer"}, "boolean"},
    {"Shell_Result", (void*)Shell_Result, 1, {"integer"}, "string"},
    {"Shell_ExitCode", (void*)Shell_ExitCode, 1, {"integer"}, "integer"},
    {"Shell_Kill", (void*)Shell_Kill, 1, {"integer"}, "boolean"},
    {"Shell_Destroy", (void*)Shell_Destroy, 1, {"integer"}, "boolean"},
};

XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

} // extern "C"
