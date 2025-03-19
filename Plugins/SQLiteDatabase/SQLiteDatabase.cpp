// SQLitePlugin.cpp
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
// Compile with: g++ -shared -fPIC -o SQLitePlugin.so SQLitePlugin.cpp -lsqlite3 (Linux/macOS)
//               g++ -shared -o SQLitePlugin.dll SQLitePlugin.cpp -lsqlite3 (Windows)

#include <sqlite3.h>
#include <map>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {

// Maps to store database and prepared statement handles
static std::map<int, sqlite3*> dbHandles;
static std::map<int, sqlite3_stmt*> stmtHandles;
static int nextDbHandle = 1;
static int nextStmtHandle = 1;

// Open an SQLite database (Supports file-based and in-memory databases)
XPLUGIN_API int OpenDatabase(const char* path) {
    sqlite3* db;
    if (sqlite3_open(path, &db) != SQLITE_OK) {
        return 0;
    }
    int handle = nextDbHandle++;
    dbHandles[handle] = db;
    return handle;
}

// Close an SQLite database
XPLUGIN_API bool CloseDatabase(int dbHandle) {
    auto it = dbHandles.find(dbHandle);
    if (it == dbHandles.end()) return false;
    sqlite3_close(it->second);
    dbHandles.erase(it);
    return true;
}

// Execute a non-query SQL command (INSERT, UPDATE, DELETE)
XPLUGIN_API bool ExecuteSQL(int dbHandle, const char* sql) {
    auto it = dbHandles.find(dbHandle);
    if (it == dbHandles.end()) return false;

    char* errMsg = nullptr;
    int result = sqlite3_exec(it->second, sql, nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    return result == SQLITE_OK;
}

// Prepare a SQL statement (Returns a statement handle)
XPLUGIN_API int PrepareStatement(int dbHandle, const char* sql) {
    auto it = dbHandles.find(dbHandle);
    if (it == dbHandles.end()) return 0;

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(it->second, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    int handle = nextStmtHandle++;
    stmtHandles[handle] = stmt;
    return handle;
}

// Bind integer to prepared statement
XPLUGIN_API bool BindInteger(int stmtHandle, int index, int value) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return false;
    return sqlite3_bind_int(it->second, index, value) == SQLITE_OK;
}

// Bind double to prepared statement
XPLUGIN_API bool BindDouble(int stmtHandle, int index, double value) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return false;
    return sqlite3_bind_double(it->second, index, value) == SQLITE_OK;
}

// Bind string to prepared statement
XPLUGIN_API bool BindString(int stmtHandle, int index, const char* value) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return false;
    return sqlite3_bind_text(it->second, index, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

// Execute a prepared statement (INSERT, UPDATE, DELETE)
XPLUGIN_API bool ExecutePrepared(int stmtHandle) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return false;
    return sqlite3_step(it->second) == SQLITE_DONE;
}

// Get number of columns in a result set
XPLUGIN_API int GetColumnCount(int stmtHandle) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return 0;
    return sqlite3_column_count(it->second);
}

// Get column name by index
XPLUGIN_API const char* GetColumnName(int stmtHandle, int index) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return "";
    return sqlite3_column_name(it->second, index);
}

// Move cursor to the first row in a result set
XPLUGIN_API bool MoveToFirstRow(int stmtHandle) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return false;
    sqlite3_reset(it->second);
    return sqlite3_step(it->second) == SQLITE_ROW;
}

// Move cursor to the next row in a result set
XPLUGIN_API bool MoveToNextRow(int stmtHandle) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return false;
    return sqlite3_step(it->second) == SQLITE_ROW;
}

// Retrieve integer column value
XPLUGIN_API int ColumnInteger(int stmtHandle, int index) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return 0;
    return sqlite3_column_int(it->second, index);
}

// Retrieve double column value
XPLUGIN_API double ColumnDouble(int stmtHandle, int index) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return 0.0;
    return sqlite3_column_double(it->second, index);
}

// Retrieve string column value
XPLUGIN_API const char* ColumnString(int stmtHandle, int index) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return "";
    return reinterpret_cast<const char*>(sqlite3_column_text(it->second, index));
}

// Finalize a prepared statement (free resources)
XPLUGIN_API bool FinalizeStatement(int stmtHandle) {
    auto it = stmtHandles.find(stmtHandle);
    if (it == stmtHandles.end()) return false;
    sqlite3_finalize(it->second);
    stmtHandles.erase(it);
    return true;
}

// Get the last SQLite error message (Renamed to avoid conflict with Windows GetLastError)
XPLUGIN_API const char* SQLite_GetLastError(int dbHandle) {
    auto it = dbHandles.find(dbHandle);
    if (it == dbHandles.end()) return "Invalid database handle";
    return sqlite3_errmsg(it->second);
}

// Plugin function entries
static struct PluginEntry {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} pluginEntries[] = {
    {"OpenDatabase", (void*)OpenDatabase, 1, {"string"}, "integer"},
    {"CloseDatabase", (void*)CloseDatabase, 1, {"integer"}, "boolean"},
    {"ExecuteSQL", (void*)ExecuteSQL, 2, {"integer", "string"}, "boolean"},
    {"PrepareStatement", (void*)PrepareStatement, 2, {"integer", "string"}, "integer"},
    {"BindInteger", (void*)BindInteger, 3, {"integer", "integer", "integer"}, "boolean"},
    {"BindDouble", (void*)BindDouble, 3, {"integer", "integer", "double"}, "boolean"},
    {"BindString", (void*)BindString, 3, {"integer", "integer", "string"}, "boolean"},
    {"ExecutePrepared", (void*)ExecutePrepared, 1, {"integer"}, "boolean"},
    {"GetColumnCount", (void*)GetColumnCount, 1, {"integer"}, "integer"},
    {"GetColumnName", (void*)GetColumnName, 2, {"integer", "integer"}, "string"},
    {"MoveToFirstRow", (void*)MoveToFirstRow, 1, {"integer"}, "boolean"},
    {"MoveToNextRow", (void*)MoveToNextRow, 1, {"integer"}, "boolean"},
    {"ColumnInteger", (void*)ColumnInteger, 2, {"integer", "integer"}, "integer"},
    {"ColumnDouble", (void*)ColumnDouble, 2, {"integer", "integer"}, "double"},
    {"ColumnString", (void*)ColumnString, 2, {"integer", "integer"}, "string"},
    {"FinalizeStatement", (void*)FinalizeStatement, 1, {"integer"}, "boolean"},
    {"GetLastError", (void*)SQLite_GetLastError, 1, {"integer"}, "string"}
};

// Retrieve plugin entries
XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

} // extern "C"

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
#endif
