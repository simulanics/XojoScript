// ============================================================================  
// XojoScript Bytecode Compiler and Virtual Machine  
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
// Build: g++ -o xojoscript.exe xojoscript.cpp -lffi -O3 -march=native -mtune=native -flto

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <cstdlib>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <functional>
#include <cstdio>
#include <cmath>
#include <random>
#include <iomanip>
#include <cstring>
#include <typeinfo>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <dirent.h>
#endif

#include <ffi.h>

// ============================================================================  
// Debugging and Time globals  
// ============================================================================
bool DEBUG_MODE = false; // set to true for debug logging
void debugLog(const std::string& msg) {
    if (DEBUG_MODE)
        std::cout << "[DEBUG] " << msg << std::endl;
}
std::chrono::steady_clock::time_point startTime;

// ---------------------------------------------------------------------------  
// Global random engine used by built-in rnd (and random class)
//TODO: Make RNG thread-safe when multithreaded support is added - currently bound to main App thread as in Xojo's implementation.
std::mt19937 global_rng(std::chrono::steady_clock::now().time_since_epoch().count());

// ============================================================================  
// Forward declarations for object types
// ============================================================================
struct ObjFunction;
struct ObjClass;
struct ObjInstance;
struct ObjArray;
struct ObjBoundMethod;
struct ObjModule;

// ============================================================================  
// Color type  
// In Xojo a color literal is written as &cRRGGBB (hexadecimal).
// ============================================================================
struct Color {
    unsigned int value;
};

// ============================================================================  
// NEW: Enum object definition
// ============================================================================
struct ObjEnum {
    std::string name;
    std::unordered_map<std::string, int> members;
};

// ============================================================================  
// Built-in function type
// ============================================================================
using BuiltinFn = std::function<struct Value(const std::vector<struct Value>&)>;

// ----------------------------------------------------------------------------  
// For class property defaults we use a property map
// ----------------------------------------------------------------------------
using PropertiesType = std::vector<std::pair<std::string, struct Value>>;

// ============================================================================  
// Dynamic Value type – a wrapper around std::variant
// Now includes pointer type (void*) for pointer variables.
// ============================================================================
// By Design - Not a duplication mistake: The duplicate "using" declaration 
// inherits std::variant's constructors so Value can be directly constructed 
// from any of its alternatives.
struct Value : public std::variant<
    std::monostate,
    int,
    double,
    bool,
    std::string,
    Color,
    std::shared_ptr<ObjFunction>,
    std::shared_ptr<ObjClass>,
    std::shared_ptr<ObjInstance>,
    std::shared_ptr<ObjArray>,
    std::shared_ptr<ObjBoundMethod>,
    BuiltinFn,
    PropertiesType,
    std::vector<std::shared_ptr<ObjFunction>>,
    std::shared_ptr<ObjModule>,
    std::shared_ptr<ObjEnum>, // Enum type
    void* // Pointer type
> {
    using std::variant<
        std::monostate,
        int,
        double,
        bool,
        std::string,
        Color,
        std::shared_ptr<ObjFunction>,
        std::shared_ptr<ObjClass>,
        std::shared_ptr<ObjInstance>,
        std::shared_ptr<ObjArray>,
        std::shared_ptr<ObjBoundMethod>,
        BuiltinFn,
        PropertiesType,
        std::vector<std::shared_ptr<ObjFunction>>,
        std::shared_ptr<ObjModule>,
        std::shared_ptr<ObjEnum>,
        void*
    >::variant;
};

// ----------------------------------------------------------------------------  
// Helper templates for type queries and access.
// ----------------------------------------------------------------------------
template<typename T>
bool holds(const Value& v) {
    return std::holds_alternative<T>(v); 
}
template<typename T>
T getVal(const Value& v) {
    return std::get<T>(v);
}
template <typename T>
std::string getTypeName(const T& var) {
    return typeid(var).name();
}
// ----------------------------------------------------------------------------  
// Helper: Convert a string to lowercase
// ----------------------------------------------------------------------------
std::string toLower(const std::string& s) {
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

// ----------------------------------------------------------------------------  
// Helper: Return a string naming the underlying type of a Value.
// ----------------------------------------------------------------------------
std::string getTypeName(const Value& v) {
    struct TypeVisitor {
        std::string operator()(std::monostate) const { return "nil"; }
        std::string operator()(int) const { return "int"; }
        std::string operator()(double) const { return "double"; }
        std::string operator()(bool) const { return "bool"; }
        std::string operator()(const std::string&) const { return "string"; }
        std::string operator()(const Color&) const { return "Color"; }
        std::string operator()(const std::shared_ptr<ObjFunction>&) const { return "ObjFunction"; }
        std::string operator()(const std::shared_ptr<ObjClass>&) const { return "ObjClass"; }
        std::string operator()(const std::shared_ptr<ObjInstance>&) const { return "ObjInstance"; }
        std::string operator()(const std::shared_ptr<ObjArray>&) const { return "ObjArray"; }
        std::string operator()(const std::shared_ptr<ObjBoundMethod>&) const { return "ObjBoundMethod"; }
        std::string operator()(const BuiltinFn&) const { return "BuiltinFn"; }
        std::string operator()(const PropertiesType&) const { return "PropertiesType"; }
        std::string operator()(const std::vector<std::shared_ptr<ObjFunction>>&) const { return "OverloadedFunctions"; }
        std::string operator()(const std::shared_ptr<ObjModule>&) const { return "ObjModule"; }
        std::string operator()(const std::shared_ptr<ObjEnum>&) const { return "ObjEnum"; }
        std::string operator()(void* ptr) const { return "pointer"; }
    } visitor;
    return std::visit(visitor, v);
}

// ============================================================================  
// Parameter structure for functions/methods
// ============================================================================
struct Param {
    std::string name;
    std::string type;
    bool optional;
    Value defaultValue;
};

// ============================================================================  
// Enum for Access Modifiers (for module members)
// ============================================================================
enum class AccessModifier { PUBLIC, PRIVATE };

// ============================================================================  
// Object definitions
// ============================================================================
struct ObjFunction {
    std::string name;
    int arity = 0; // Parameter initialization.
    std::vector<Param> params; // Full parameter list
    struct CodeChunk {
        std::vector<int> code;
        std::vector<Value> constants;
    } chunk;
};

struct ObjClass {
    std::string name;
    std::unordered_map<std::string, Value> methods;
    PropertiesType properties;
    bool isPlugin = false;
    BuiltinFn pluginConstructor;
    std::unordered_map<std::string, std::pair<BuiltinFn, BuiltinFn>> pluginProperties;
};

struct ObjInstance {
    std::shared_ptr<ObjClass> klass;
    std::unordered_map<std::string, Value> fields;
    void* pluginInstance = nullptr;
};

struct ObjArray {
    std::vector<Value> elements;
};

struct ObjBoundMethod {
    Value receiver;
    std::string name;
};

struct ObjModule {
    std::string name;
    std::unordered_map<std::string, Value> publicMembers;
};

// ============================================================================  
// valueToString – visitor for Value conversion (with trailing zero trimming to mirror Xojo)
// ============================================================================
// valueToString converts a Value to a string
std::string valueToString(const Value& val) {
    struct Visitor {
        std::string operator()(std::monostate) const { return "nil"; }
        std::string operator()(int i) const { return std::to_string(i); }
        std::string operator()(double d) const {
            std::string s = std::to_string(d);
            size_t pos = s.find('.');
            if (pos != std::string::npos) {
                while (!s.empty() && s.back() == '0')
                    s.pop_back();
                if (!s.empty() && s.back() == '.')
                    s.pop_back();
            }
            return s;
        }
        std::string operator()(bool b) const { return b ? "true" : "false"; }
        std::string operator()(const std::string& s) const { return s; }
        std::string operator()(const Color& col) const {
            char buf[10];
            std::snprintf(buf, sizeof(buf), "&h%06X", col.value & 0xFFFFFF);
            return std::string(buf);
        }
        std::string operator()(const std::shared_ptr<ObjFunction>& fn) const { return "<function " + fn->name + ">"; }
        std::string operator()(const std::shared_ptr<ObjClass>& cls) const { return "<class " + cls->name + ">"; }
        std::string operator()(const std::shared_ptr<ObjInstance>& inst) const { return "<instance of " + inst->klass->name + ">"; }
        std::string operator()(const std::shared_ptr<ObjArray>& arr) const { return "Array(" + std::to_string(arr->elements.size()) + ")"; }
        std::string operator()(const std::shared_ptr<ObjBoundMethod>& bm) const { return "<bound method " + bm->name + ">"; }
        std::string operator()(const BuiltinFn&) const { return "<builtin fn>"; }
        std::string operator()(const PropertiesType&) const { return "<properties>"; }
        std::string operator()(const std::vector<std::shared_ptr<ObjFunction>>&) const { return "<overloaded functions>"; }
        std::string operator()(const std::shared_ptr<ObjModule>& mod) const { return "<module " + mod->name + ">"; }
        std::string operator()(const std::shared_ptr<ObjEnum>& e) const { return "<enum " + e->name + ">"; }
        std::string operator()(void* ptr) const {
            if(ptr == nullptr) return "nil";
            char buf[20];
            std::snprintf(buf, sizeof(buf), "ptr(%p)", ptr);
            return std::string(buf);
        } // Pointer type
    } visitor;
    return std::visit(visitor, val);
}

// ============================================================================  
// Token and XTokenType Definitions
// ============================================================================
enum class XTokenType {
    LEFT_PAREN, RIGHT_PAREN, COMMA, PLUS, MINUS, STAR, SLASH, EQUAL,
    DOT, LEFT_BRACKET, RIGHT_BRACKET,
    IDENTIFIER, STRING, NUMBER, COLOR, BOOLEAN_TRUE, BOOLEAN_FALSE,
    FUNCTION, SUB, END, RETURN, CLASS, NEW, DIM, AS, XOPTIONAL, PUBLIC, PRIVATE,
    XCONST, PRINT,
    IF, THEN, ELSE, ELSEIF,
    FOR, TO, STEP, NEXT,
    WHILE, WEND,
    NOT, AND, OR,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, NOT_EQUAL,
    EOF_TOKEN,
    CARET,  // '^'
    MOD,   // "mod" keyword/operator
    MODULE, // Module declaration
    DECLARE, // Module declaration
    SELECT,  // Select keyword for Select Case statement
    CASE,   // Case keyword for Select Case statement
    ENUM    // Enum keyword
};

struct Token {
    XTokenType type;
    std::string lexeme;
    int line;
};

// ============================================================================  
// Lexer
// ============================================================================
class Lexer {
public:
    Lexer(const std::string& source) : source(source) {}
    std::vector<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }
        tokens.push_back({ XTokenType::EOF_TOKEN, "", line });
        return tokens;
    }
private:
    const std::string source;
    std::vector<Token> tokens;
    int start = 0, current = 0, line = 1;

    bool isAtEnd() { return current >= source.size(); }
    char advance() { return source[current++]; }
    void addToken(XTokenType type) {
        tokens.push_back({ type, source.substr(start, current - start), line });
    }
    bool match(char expected) {
        if (isAtEnd() || source[current] != expected) return false;
        current++;
        return true;
    }
    char peek() { return isAtEnd() ? '\0' : source[current]; }
    char peekNext() { return (current + 1 >= source.size()) ? '\0' : source[current + 1]; }
    void scanToken() {
        char c = advance();
        switch (c) {
        case '(': addToken(XTokenType::LEFT_PAREN); break;
        case ')': addToken(XTokenType::RIGHT_PAREN); break;
        case ',': addToken(XTokenType::COMMA); break;
        case '+': addToken(XTokenType::PLUS); break;
        case '-': addToken(XTokenType::MINUS); break;
        case '*': addToken(XTokenType::STAR); break;
        case '/':
            if (peek() == '/' || peek() == '\'') {
                while (peek() != '\n' && !isAtEnd()) advance();
            }
            else {
                addToken(XTokenType::SLASH);
            }
            break;
        case '^': addToken(XTokenType::CARET); break;
        case '=': addToken(XTokenType::EQUAL); break;
        case '<':
            if (match('=')) addToken(XTokenType::LESS_EQUAL);
            else if (match('>')) addToken(XTokenType::NOT_EQUAL);
            else addToken(XTokenType::LESS);
            break;
        case '>':
            if (match('=')) addToken(XTokenType::GREATER_EQUAL);
            else addToken(XTokenType::GREATER);
            break;
        case '.': addToken(XTokenType::DOT); break;
        case '[': addToken(XTokenType::LEFT_BRACKET); break;
        case ']': addToken(XTokenType::RIGHT_BRACKET); break;
        case '&': {
            if (peek() == 'c' || peek() == 'C') {
                advance();
                std::string hex;
                while (isxdigit(peek())) { hex.push_back(advance()); }
                tokens.push_back({ XTokenType::COLOR, "&c" + hex, line });
            }
            else {
                std::cerr << "Unexpected '&' token at line " << line << std::endl;
                exit(1);
            }
            break;
        }
        case ' ':
        case '\r':
        case '\t': break;
        case '\n': line++; break;
        case '"': string(); break;
        case '\'':
            while (peek() != '\n' && !isAtEnd()) advance();
            break;
        default:
            if (isdigit(c)) { number(); }
            else if (isalpha(c)) { identifier(); }
            break;
        }
    }
    void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }
        if (isAtEnd()) return;
        advance(); // closing "
        addToken(XTokenType::STRING);
    }
    void number() {
        while (isdigit(peek())) advance();
        if (peek() == '.' && isdigit(peekNext())) {
            advance();
            while (isdigit(peek())) advance();
        }
        addToken(XTokenType::NUMBER);
    }
    void identifier() {
        while (isalnum(peek()) || peek() == '_') advance();
        std::string text = source.substr(start, current - start);
        std::string lowerText = toLower(text);
        XTokenType type = XTokenType::IDENTIFIER;
        if (lowerText == "function") type = XTokenType::FUNCTION;
        else if (lowerText == "sub")      type = XTokenType::SUB;
        else if (lowerText == "end")      type = XTokenType::END;
        else if (lowerText == "return")   type = XTokenType::RETURN;
        else if (lowerText == "class")    type = XTokenType::CLASS;
        else if (lowerText == "new")      type = XTokenType::NEW;
        else if (lowerText == "dim" || lowerText == "var") type = XTokenType::DIM;
        else if (lowerText == "const")    type = XTokenType::XCONST;
        else if (lowerText == "as")       type = XTokenType::AS;
        else if (lowerText == "optional") type = XTokenType::XOPTIONAL;
        else if (lowerText == "public")   type = XTokenType::PUBLIC;
        else if (lowerText == "private")  type = XTokenType::PRIVATE;
        else if (lowerText == "print")    type = XTokenType::PRINT;
        else if (lowerText == "if")       type = XTokenType::IF;
        else if (lowerText == "then")     type = XTokenType::THEN;
        else if (lowerText == "else")     type = XTokenType::ELSE;
        else if (lowerText == "elseif")   type = XTokenType::ELSEIF;
        else if (lowerText == "for")      type = XTokenType::FOR;
        else if (lowerText == "to")       type = XTokenType::TO;
        else if (lowerText == "step")     type = XTokenType::STEP;
        else if (lowerText == "next")     type = XTokenType::NEXT;
        else if (lowerText == "while")    type = XTokenType::WHILE;
        else if (lowerText == "wend")     type = XTokenType::WEND;
        else if (lowerText == "not")      type = XTokenType::NOT;
        else if (lowerText == "and")      type = XTokenType::AND;
        else if (lowerText == "or")       type = XTokenType::OR;
        else if (lowerText == "mod")      type = XTokenType::MOD;
        else if (lowerText == "true")     type = XTokenType::BOOLEAN_TRUE;
        else if (lowerText == "false")    type = XTokenType::BOOLEAN_FALSE;
        else if (lowerText == "module")   type = XTokenType::MODULE;
        else if (lowerText == "declare")  type = XTokenType::DECLARE;
        else if (lowerText == "select")   type = XTokenType::SELECT;
        else if (lowerText == "case")     type = XTokenType::CASE;
        else if (lowerText == "enum")     type = XTokenType::ENUM;
        addToken(type);
    }
};

// ----------------------------------------------------------------------------  
// Helper function to trim trailing whitespace
// ----------------------------------------------------------------------------
std::string rtrim(const std::string& s) {
    size_t end = s.find_last_not_of(" \t\r\n");
    if (end == std::string::npos) return "";
    return s.substr(0, end + 1);
}

// ============================================================================  
// Environment (case–insensitive for variable names)
// ============================================================================
struct Environment {
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;
    Environment(std::shared_ptr<Environment> enclosing = nullptr)
        : enclosing(enclosing) { }
    void define(const std::string& name, const Value& value) {
        values[toLower(name)] = value;
    }
    Value get(const std::string& name) {
        std::string key = toLower(name);
        if (values.find(key) != values.end())
            return values[key];
        if (values.find("self") != values.end()) {
            Value selfVal = values["self"];
            if (holds<std::shared_ptr<ObjInstance>>(selfVal)) {
                auto instance = getVal<std::shared_ptr<ObjInstance>>(selfVal);
                if (instance->fields.find(key) != instance->fields.end())
                    return instance->fields[key];
            }
        }
        if (enclosing) return enclosing->get(name);
        std::cerr << "Undefined variable: " << name << std::endl;
        exit(1);
        return Value(std::monostate{});
    }
    void assign(const std::string& name, const Value& value) {
        std::string key = toLower(name);
        if (values.find(key) != values.end()) {
            values[key] = value;
            return;
        }
        if (values.find("self") != values.end()) {
            Value selfVal = values["self"];
            if (holds<std::shared_ptr<ObjInstance>>(selfVal)) {
                auto instance = getVal<std::shared_ptr<ObjInstance>>(selfVal);
                if (instance->fields.find(key) != instance->fields.end()) {
                    instance->fields[key] = value;
                    return;
                }
            }
        }
        if (enclosing) {
            enclosing->assign(name, value);
            return;
        }
        std::cerr << "Undefined variable: " << name << std::endl;
        exit(1);
    }
};

// ============================================================================  
// Runtime error helper
// ============================================================================
[[noreturn]] void runtimeError(const std::string& msg) {
    std::cerr << "Runtime Error: " << msg << std::endl;
    exit(1);
}

// ============================================================================  
// Preprocessing: Remove line continuations and comments
// ============================================================================
std::string preprocessSource(const std::string& source) {
    std::istringstream iss(source);
    std::string line, result;
    while (std::getline(iss, line)) {
        bool inString = false;
        std::string newline;
        for (size_t i = 0; i < line.size(); i++) {
            char c = line[i];
            // Toggle inString flag on unescaped double quotes.
            if (c == '"' && (i == 0 || line[i - 1] != '\\')) {
                inString = !inString;
            }
           // If not inside a string literal, check for comment markers.
            if (!inString) {
                if (c == '/' && i + 1 < line.size() && line[i + 1] == '/')
                    break; // strip from here to end of line
                if (c == '\'')
                    break; // strip from here to end of line
            }
            newline.push_back(c);
        }
        
        // Process line continuations (underscore at end)
        std::string trimmed = rtrim(newline);
        if (!trimmed.empty() && trimmed.back() == '_') {
            size_t endPos = trimmed.find_last_not_of(" \t_");
            result += trimmed.substr(0, endPos + 1);
        }
        else {
            result += newline + "\n";
        }
    }
    return result;
}

// ============================================================================  
// Bytecode Instructions
// ============================================================================
enum OpCode {
    OP_CONSTANT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEGATE,
    OP_POW,
    OP_MOD,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_NE,
    OP_EQ,
    OP_AND,
    OP_OR,
    OP_PRINT,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_NEW,
    OP_CALL,
    OP_OPTIONAL_CALL,
    OP_RETURN,
    OP_NIL,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_CLASS,
    OP_METHOD,
    OP_ARRAY,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_PROPERTIES,
    OP_DUP,
    OP_CONSTRUCTOR_END
};

std::string opcodeToString(int opcode) {
    switch (opcode) {
    case OP_CONSTANT:      return "OP_CONSTANT";
    case OP_ADD:           return "OP_ADD";
    case OP_SUB:           return "OP_SUB";
    case OP_MUL:           return "OP_MUL";
    case OP_DIV:           return "OP_DIV";
    case OP_NEGATE:        return "OP_NEGATE";
    case OP_POW:           return "OP_POW";
    case OP_MOD:           return "OP_MOD";
    case OP_LT:            return "OP_LT";
    case OP_LE:            return "OP_LE";
    case OP_GT:            return "OP_GT";
    case OP_GE:            return "OP_GE";
    case OP_NE:            return "OP_NE";
    case OP_EQ:            return "OP_EQ";
    case OP_AND:           return "OP_AND";
    case OP_OR:            return "OP_OR";
    case OP_PRINT:         return "OP_PRINT";
    case OP_POP:           return "OP_POP";
    case OP_DEFINE_GLOBAL: return "OP_DEFINE_GLOBAL";
    case OP_GET_GLOBAL:    return "OP_GET_GLOBAL";
    case OP_SET_GLOBAL:    return "OP_SET_GLOBAL";
    case OP_NEW:           return "OP_NEW";
    case OP_CALL:          return "OP_CALL";
    case OP_OPTIONAL_CALL: return "OP_OPTIONAL_CALL";
    case OP_RETURN:        return "OP_RETURN";
    case OP_NIL:           return "OP_NIL";
    case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
    case OP_JUMP:          return "OP_JUMP";
    case OP_CLASS:         return "OP_CLASS";
    case OP_METHOD:        return "OP_METHOD";
    case OP_ARRAY:         return "OP_ARRAY";
    case OP_GET_PROPERTY:  return "OP_GET_PROPERTY";
    case OP_SET_PROPERTY:  return "OP_SET_PROPERTY";
    case OP_PROPERTIES:    return "OP_PROPERTIES";
    case OP_DUP:           return "OP_DUP";
    case OP_CONSTRUCTOR_END: return "OP_CONSTRUCTOR_END";
    default:               return "UNKNOWN";
    }
}

// ============================================================================  
// Virtual Machine
// ============================================================================
struct VM {
    std::vector<Value> stack;
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    ObjFunction::CodeChunk mainChunk;
};

// ----------------------------------------------------------------------------  
// Helper: pop from VM stack (with logging)
// ----------------------------------------------------------------------------
Value pop(VM& vm) {
    if (vm.stack.empty()) {
        debugLog("OP_POP: Attempted to pop from an empty stack.");
        runtimeError("VM: Stack underflow on POP.");
    }
    Value v = vm.stack.back();
    vm.stack.pop_back();
    return v;
}

// ============================================================================  
// AST Definitions: Expressions
// ============================================================================
enum class BinaryOp { ADD, SUB, MUL, DIV, LT, LE, GT, GE, NE, EQ, AND, OR, POW, MOD };

struct Expr { virtual ~Expr() = default; };

struct LiteralExpr : Expr { 
    Value value; 
    LiteralExpr(const Value& value) : value(value) { }
};

struct VariableExpr : Expr {
    std::string name;
    VariableExpr(const std::string& name) : name(name) { }
};

struct UnaryExpr : Expr {
    std::string op;
    std::shared_ptr<Expr> right;
    UnaryExpr(const std::string& op, std::shared_ptr<Expr> right)
        : op(op), right(right) { }
};

struct AssignmentExpr : Expr {
    std::string name;
    std::shared_ptr<Expr> value;
    AssignmentExpr(const std::string& name, std::shared_ptr<Expr> value)
        : name(name), value(value) { }
};

struct BinaryExpr : Expr {
    std::shared_ptr<Expr> left;
    BinaryOp op;
    std::shared_ptr<Expr> right;
    BinaryExpr(std::shared_ptr<Expr> left, BinaryOp op, std::shared_ptr<Expr> right)
        : left(left), op(op), right(right) { }
};

struct GroupingExpr : Expr {
    std::shared_ptr<Expr> expression;
    GroupingExpr(std::shared_ptr<Expr> expression)
        : expression(expression) { }
};

struct CallExpr : Expr {
    std::shared_ptr<Expr> callee;
    std::vector<std::shared_ptr<Expr>> arguments;
    CallExpr(std::shared_ptr<Expr> callee, const std::vector<std::shared_ptr<Expr>>& arguments)
        : callee(callee), arguments(arguments) { }
};

struct ArrayLiteralExpr : Expr {
    std::vector<std::shared_ptr<Expr>> elements;
    ArrayLiteralExpr(const std::vector<std::shared_ptr<Expr>>& elements)
        : elements(elements) { }
};

struct GetPropExpr : Expr {
    std::shared_ptr<Expr> object;
    std::string name;
    GetPropExpr(std::shared_ptr<Expr> object, const std::string& name)
        : object(object), name(toLower(name)) { }
};

struct SetPropExpr : Expr {
    std::shared_ptr<Expr> object;
    std::string name;
    std::shared_ptr<Expr> value;
    SetPropExpr(std::shared_ptr<Expr> object, const std::string& name, std::shared_ptr<Expr> value)
        : object(object), name(toLower(name)), value(value) { }
};

struct NewExpr : Expr {
    std::string className;
    std::vector<std::shared_ptr<Expr>> arguments;
    NewExpr(const std::string& className, const std::vector<std::shared_ptr<Expr>>& arguments)
        : className(toLower(className)), arguments(arguments) { }
};

// ============================================================================  
// AST Definitions: Statements
// ============================================================================
enum class StmtType { EXPRESSION, FUNCTION, RETURN, CLASS, VAR, IF, WHILE, BLOCK, FOR, MODULE, DECLARE };

struct Stmt { virtual ~Stmt() = default; };

struct ExpressionStmt : Stmt {
    std::shared_ptr<Expr> expression;
    ExpressionStmt(std::shared_ptr<Expr> expression) : expression(expression) { }
};

struct ReturnStmt : Stmt {
    std::shared_ptr<Expr> value;
    ReturnStmt(std::shared_ptr<Expr> value) : value(value) { }
};

struct FunctionStmt : Stmt {
    std::string name;
    std::vector<Param> params;
    std::vector<std::shared_ptr<Stmt>> body;
    AccessModifier access;
    FunctionStmt(const std::string& name, const std::vector<Param>& params,
        const std::vector<std::shared_ptr<Stmt>>& body, AccessModifier access = AccessModifier::PUBLIC)
        : name(name), params(params), body(body), access(access) { }
};

struct VarStmt : Stmt {
    std::string name; 
    std::shared_ptr<Expr> initializer;
    std::string varType;
    bool isConstant; // for Const declarations
    AccessModifier access;
    VarStmt(const std::string& name, std::shared_ptr<Expr> initializer, const std::string& varType = "",
        bool isConstant = false, AccessModifier access = AccessModifier::PUBLIC) 
        : name(name), initializer(initializer), varType(toLower(varType)), isConstant(isConstant), access(access) { }
};

struct PropertyAssignmentStmt : Stmt {
    std::shared_ptr<Expr> object;
    std::string property;
    std::shared_ptr<Expr> value;
    PropertyAssignmentStmt(std::shared_ptr<Expr> object, const std::string& property, std::shared_ptr<Expr> value)
        : object(object), property(property), value(value) { }
};

struct ClassStmt : Stmt {
    std::string name;
    std::vector<std::shared_ptr<FunctionStmt>> methods;
    PropertiesType properties;
    ClassStmt(const std::string& name,
        const std::vector<std::shared_ptr<FunctionStmt>>& methods,
        const PropertiesType& properties)
        : name(name), methods(methods), properties(properties) { }
};

struct IfStmt : Stmt {
    std::shared_ptr<Expr> condition;
    std::vector<std::shared_ptr<Stmt>> thenBranch;
    std::vector<std::shared_ptr<Stmt>> elseBranch;
    IfStmt(std::shared_ptr<Expr> condition,
        const std::vector<std::shared_ptr<Stmt>>& thenBranch,
        const std::vector<std::shared_ptr<Stmt>>& elseBranch)
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) { }
};

struct WhileStmt : Stmt {
    std::shared_ptr<Expr> condition;
    std::vector<std::shared_ptr<Stmt>> body;
    WhileStmt(std::shared_ptr<Expr> condition, const std::vector<std::shared_ptr<Stmt>>& body)
        : condition(condition), body(body) { } 
};

struct AssignmentStmt : Stmt {
    std::string name;
    std::shared_ptr<Expr> value;
    AssignmentStmt(const std::string& name, std::shared_ptr<Expr> value)
        : name(name), value(value) { }
};

struct BlockStmt : Stmt {
    std::vector<std::shared_ptr<Stmt>> statements;
    BlockStmt(const std::vector<std::shared_ptr<Stmt>>& statements)
        : statements(statements) { }
};

struct ForStmt : Stmt {
    std::string varName;
    std::shared_ptr<Expr> start;
    std::shared_ptr<Expr> end;
    std::shared_ptr<Expr> step;
    std::vector<std::shared_ptr<Stmt>> body;
    ForStmt(const std::string& varName,
        std::shared_ptr<Expr> start,
        std::shared_ptr<Expr> end,
        std::shared_ptr<Expr> step,
        const std::vector<std::shared_ptr<Stmt>>& body)
        : varName(varName), start(start), end(end), step(step), body(body) { }
};

// Module AST node
struct ModuleStmt : Stmt {
     std::string name;
     std::vector<std::shared_ptr<Stmt>> body;
     ModuleStmt(const std::string& name, const std::vector<std::shared_ptr<Stmt>>& body)
        : name(name), body(body) { }
};

// Declare API statement AST node
struct DeclareStmt : Stmt {
    bool isFunction; // true if Function, false if Sub
    std::string apiName;
    std::string libraryName;
    std::string aliasName;    // optional; empty if not provided
    std::string selector;     // optional; empty if not provided
    std::vector<Param> params;
    std::string returnType;   // if function; empty for Sub
    DeclareStmt(bool isFunc, const std::string& name, const std::string& lib,
        const std::string& alias, const std::string& sel,
        const std::vector<Param>& params, const std::string& retType)
        : isFunction(isFunc), apiName(name), libraryName(lib), aliasName(alias), selector(sel), params(params), returnType(retType) { } 
};

// Enum AST node
struct EnumStmt : Stmt {
    std::string name;
    std::unordered_map<std::string, int> members;
    EnumStmt(const std::string& name, const std::unordered_map<std::string, int>& members)
    : name(name), members(members) { }
};

// ---------------------------------------------------------------------------  
// Forward declaration for plugin function wrapper for Declare statements.
BuiltinFn wrapPluginFunctionForDeclare(const std::vector<Param>& params, const std::string& retType, const std::string& apiName, const std::string& libName);

// ============================================================================  
// Parser
// ============================================================================
class Parser {
public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens), inModule(false) {}
    std::vector<std::shared_ptr<Stmt>> parse() {
       debugLog("Parser: Starting parse. Total tokens: " + std::to_string(tokens.size()));
        std::vector<std::shared_ptr<Stmt>> statements;
        while (!isAtEnd()) {
            statements.push_back(declaration());
        }
        debugLog("Parser: Finished parse.");
        return statements;
    }
private:
    std::vector<Token> tokens;
    int current = 0;
    bool inModule; // Flag to indicate module context

    bool isAtEnd() { return peek().type == XTokenType::EOF_TOKEN; }
    Token peek() { return tokens[current]; }
    Token previous() { return tokens[current - 1]; }
    Token advance() { if (!isAtEnd()) current++; return previous(); }
    bool check(XTokenType type) { return !isAtEnd() && peek().type == type; }
    bool match(const std::vector<XTokenType>& types) {
        for (auto type : types)
            if (check(type)) { advance(); return true; }
        return false;
    }
    Token consume(XTokenType type, const std::string& msg) {
        if (check(type)) return advance();
        std::cerr << "Parse error at line " << peek().line << ": " << msg << std::endl;
        exit(1);
    }
    std::vector<std::shared_ptr<Stmt>> block(const std::vector<XTokenType>& terminators) {
        std::vector<std::shared_ptr<Stmt>> statements;
        while (!isAtEnd() && std::find(terminators.begin(), terminators.end(), peek().type) == terminators.end()) {
            statements.push_back(declaration());
        }
        return statements;
    }
    bool isPotentialCallArgument(const Token& token) {
        if (token.type == XTokenType::RIGHT_PAREN || token.type == XTokenType::EOF_TOKEN)
            return false;
        switch (token.type) {
        case XTokenType::NUMBER:
        case XTokenType::STRING:
        case XTokenType::COLOR:
        case XTokenType::BOOLEAN_TRUE:
        case XTokenType::BOOLEAN_FALSE:
        case XTokenType::IDENTIFIER:
        case XTokenType::LEFT_PAREN:
            return true;
        default:
            return false;
        }
    }
    // Parse enum declaration
    std::shared_ptr<Stmt> enumDeclaration() {
        advance(); // consume ENUM
        Token name = consume(XTokenType::IDENTIFIER, "Expect enum name.");
        std::unordered_map<std::string, int> members;
        while (!check(XTokenType::END)) {
            Token memberName = consume(XTokenType::IDENTIFIER, "Expect enum member name.");
            consume(XTokenType::EQUAL, "Expect '=' after enum member name.");
            Token numberToken = consume(XTokenType::NUMBER, "Expect number for enum member value.");
            int value = std::stoi(numberToken.lexeme);
            members[toLower(memberName.lexeme)] = value;
        }
        consume(XTokenType::END, "Expect 'End' after enum definition.");
        if (check(XTokenType::ENUM)) { advance(); }
        return std::make_shared<EnumStmt>(name.lexeme, members);
    }
    // Parse module declaration
    std::shared_ptr<Stmt> moduleDeclaration() {
        Token name = consume(XTokenType::IDENTIFIER, "Expect module name.");
        bool oldInModule = inModule;
        inModule = true;
        std::vector<std::shared_ptr<Stmt>> body = block({ XTokenType::END });
        consume(XTokenType::END, "Expect 'End' after module body.");
        consume(XTokenType::MODULE, "Expect 'Module' after End in module declaration.");
        inModule = oldInModule;
        return std::make_shared<ModuleStmt>(name.lexeme, body);
    }
    // Parse Declare statement
    std::shared_ptr<Stmt> declareStatement() {
        bool isFunc;
        if (match({ XTokenType::SUB }))
            isFunc = false;
        else if (match({ XTokenType::FUNCTION }))
            isFunc = true;
        else {
            std::cerr << "Parse error at line " << peek().line << ": Expected Sub or Function after Declare." << std::endl;
            exit(1);
        }
        Token nameTok = consume(XTokenType::IDENTIFIER, "Expect API name in Declare statement.");
        std::string apiName = nameTok.lexeme;
        Token libTok = consume(XTokenType::IDENTIFIER, "Expect 'Lib' keyword in Declare statement.");
        if (toLower(libTok.lexeme) != "lib") {
            std::cerr << "Parse error at line " << libTok.line << ": Expected 'Lib' keyword in Declare statement." << std::endl;
            exit(1);
        }
        Token libNameTok = consume(XTokenType::STRING, "Expect library name (a string literal) in Declare statement.");
        std::string libraryName = libNameTok.lexeme.substr(1, libNameTok.lexeme.size() - 2);
        std::string aliasName = "";
        if (check(XTokenType::IDENTIFIER) && toLower(peek().lexeme) == "alias") {
            advance();
            Token aliasTok = consume(XTokenType::STRING, "Expect alias name (a string literal) in Declare statement.");
            aliasName = aliasTok.lexeme.substr(1, aliasTok.lexeme.size() - 2);
        }
        std::string selector = "";
        if (check(XTokenType::IDENTIFIER) && toLower(peek().lexeme) == "selector") {
            advance();
            Token selTok = consume(XTokenType::STRING, "Expect selector (a string literal) in Declare statement.");
            selector = selTok.lexeme.substr(1, selTok.lexeme.size() - 2);
        }
        std::vector<Param> params;
        consume(XTokenType::LEFT_PAREN, "Expect '(' for parameter list in Declare statement.");
        if (!check(XTokenType::RIGHT_PAREN)) {
            do {
                bool isOptional = false;
                if (match({ XTokenType::XOPTIONAL })) { isOptional = true; }
                Token paramNameTok = consume(XTokenType::IDENTIFIER, "Expect parameter name in Declare statement.");
                std::string paramName = paramNameTok.lexeme;
                std::string paramType = "";
                if (match({ XTokenType::AS })) {
                    Token typeTok = consume(XTokenType::IDENTIFIER, "Expect type after 'As' in parameter list.");
                    paramType = toLower(typeTok.lexeme);
                }
                Value defaultValue = Value(std::monostate{});
                if (isOptional && match({ XTokenType::EQUAL })) {
                    std::shared_ptr<Expr> defaultExpr = expression();
                    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(defaultExpr))
                        defaultValue = lit->value;
                    else
                        runtimeError("Optional parameter default value must be a literal.");
                }
                params.push_back({ paramName, paramType, isOptional, defaultValue });
            } while (match({ XTokenType::COMMA }));
        }
        consume(XTokenType::RIGHT_PAREN, "Expect ')' after parameter list in Declare statement.");
        std::string retType = "";
        if (isFunc && check(XTokenType::AS)) {
            advance();
            Token retTok = consume(XTokenType::IDENTIFIER, "Expect return type after 'As' in Declare statement.");
            retType = toLower(retTok.lexeme);
        }
        return std::make_shared<DeclareStmt>(isFunc, apiName, libraryName, aliasName, selector, params, retType);
    }
    // Modified declaration to capture access modifiers in module context.
    std::shared_ptr<Stmt> declaration() {
        AccessModifier access = AccessModifier::PUBLIC;
        if (inModule && (check(XTokenType::PUBLIC) || check(XTokenType::PRIVATE))) {
            if (match({ XTokenType::PUBLIC })) access = AccessModifier::PUBLIC;
            else if (match({ XTokenType::PRIVATE })) access = AccessModifier::PRIVATE;
        }
        if (check(XTokenType::MODULE))
            return (advance(), moduleDeclaration());
        if (check(XTokenType::ENUM))
            return enumDeclaration();
        if (check(XTokenType::DECLARE))
            return (advance(), declareStatement());
        if (check(XTokenType::SELECT))
            return (advance(), selectCaseStatement());
        if (check(XTokenType::IDENTIFIER) && (current + 3 < tokens.size() &&
            tokens[current + 1].type == XTokenType::DOT &&
            tokens[current + 2].type == XTokenType::IDENTIFIER &&
            tokens[current + 3].type == XTokenType::EQUAL)) {
            Token obj = advance();
            consume(XTokenType::DOT, "Expect '.' in property assignment.");
            Token prop = consume(XTokenType::IDENTIFIER, "Expect property name in property assignment.");
            consume(XTokenType::EQUAL, "Expect '=' in property assignment.");
            std::shared_ptr<Expr> valueExpr = expression();
            return std::make_shared<PropertyAssignmentStmt>(std::make_shared<VariableExpr>(obj.lexeme), prop.lexeme, valueExpr);
        }
        if (match({ XTokenType::FUNCTION, XTokenType::SUB }))
            return functionDeclaration(access);
        if (match({ XTokenType::CLASS }))
            return classDeclaration();
        if (match({ XTokenType::XCONST }))
            return varDeclaration(access, true);
        if (match({ XTokenType::DIM }))
            return varDeclaration(access, false);
        if (match({ XTokenType::IF }))
            return ifStatement();
        if (match({ XTokenType::FOR }))
            return forStatement();
        if (match({ XTokenType::WHILE }))
            return whileStatement();
        if (check(XTokenType::IDENTIFIER) && (current + 1 < tokens.size() && tokens[current + 1].type == XTokenType::EQUAL)) {
            Token id = advance();
            advance();
            std::shared_ptr<Expr> value = expression();
            return std::make_shared<AssignmentStmt>(id.lexeme, value);
        }
        return statement();
    }
    std::shared_ptr<Stmt> functionDeclaration(AccessModifier access) {
        Token name = consume(XTokenType::IDENTIFIER, "Expect function name.");
        consume(XTokenType::LEFT_PAREN, "Expect '(' after function name.");
        std::vector<Param> parameters;
        if (!check(XTokenType::RIGHT_PAREN)) {
            do {
                bool isOptional = false;
                if (match({ XTokenType::XOPTIONAL })) { isOptional = true; }
                Token paramName = consume(XTokenType::IDENTIFIER, "Expect parameter name.");
                std::string paramType = "";
                if (match({ XTokenType::AS })) {
                    Token typeToken = consume(XTokenType::IDENTIFIER, "Expect type after 'As'.");
                    paramType = toLower(typeToken.lexeme);
                }
                Value defaultValue = Value(std::monostate{});
                if (isOptional && match({ XTokenType::EQUAL })) {
                    std::shared_ptr<Expr> defaultExpr = expression();
                    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(defaultExpr))
                        defaultValue = lit->value;
                    else
                        runtimeError("Optional parameter default value must be a literal.");
                }
                parameters.push_back({ paramName.lexeme, paramType, isOptional, defaultValue });
            } while (match({ XTokenType::COMMA }));
        }
        consume(XTokenType::RIGHT_PAREN, "Expect ')' after parameters.");
        if (match({ XTokenType::AS }))
            consume(XTokenType::IDENTIFIER, "Expect return type after 'As'.");
        std::vector<std::shared_ptr<Stmt>> body = block({ XTokenType::END });
        consume(XTokenType::END, "Expect 'End' after function body.");
        match({ XTokenType::FUNCTION, XTokenType::SUB });
        int req = 0;
        for (auto& p : parameters)
            if (!p.optional) req++;
        return std::make_shared<FunctionStmt>(name.lexeme, parameters, body, access);
    }
    std::shared_ptr<Stmt> classDeclaration() {
        Token name = consume(XTokenType::IDENTIFIER, "Expect class name.");
        std::vector<std::shared_ptr<FunctionStmt>> methods;
        PropertiesType properties;
        while (!check(XTokenType::END) && !isAtEnd()) {
            if (match({ XTokenType::DIM })) {
                Token propName = consume(XTokenType::IDENTIFIER, "Expect property name.");
                std::string typeStr = "";
                if (match({ XTokenType::AS })) {
                    Token typeToken = consume(XTokenType::IDENTIFIER, "Expect type after 'As'.");
                    typeStr = toLower(typeToken.lexeme);
                }
                Value defaultVal;
                if (typeStr == "integer" || typeStr == "double")
                    defaultVal = 0;
                else if (typeStr == "boolean")
                    defaultVal = false;
                else if (typeStr == "string")
                    defaultVal = std::string("");
                else if (typeStr == "color")
                    defaultVal = Color{ 0 };
                else if (typeStr == "array")
                    defaultVal = Value(std::make_shared<ObjArray>());
                else
                    defaultVal = std::monostate{};
                properties.push_back({ toLower(propName.lexeme), defaultVal });
            }
            else if (match({ XTokenType::FUNCTION, XTokenType::SUB })) {
                Token methodName = consume(XTokenType::IDENTIFIER, "Expect method name.");
                consume(XTokenType::LEFT_PAREN, "Expect '(' after method name.");
                std::vector<Param> parameters;
                if (!check(XTokenType::RIGHT_PAREN)) {
                    do {
                        bool isOptional = false;
                        if (match({ XTokenType::XOPTIONAL })) { isOptional = true; }
                        Token param = consume(XTokenType::IDENTIFIER, "Expect parameter name.");
                        std::string paramType = "";
                        if (match({ XTokenType::AS })) {
                            Token typeToken = consume(XTokenType::IDENTIFIER, "Expect type after 'As'.");
                            paramType = toLower(typeToken.lexeme);
                        }
                        Value defaultValue = Value(std::monostate{});
                        if (isOptional && match({ XTokenType::EQUAL })) {
                            std::shared_ptr<Expr> defaultExpr = expression();
                            if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(defaultExpr))
                                defaultValue = lit->value;
                            else
                                runtimeError("Optional parameter default value must be a literal.");
                        }
                        parameters.push_back({ param.lexeme, paramType, isOptional, defaultValue });
                    } while (match({ XTokenType::COMMA }));
                }
                consume(XTokenType::RIGHT_PAREN, "Expect ')' after parameters.");
                if (match({ XTokenType::AS }))
                    consume(XTokenType::IDENTIFIER, "Expect return type after 'As'.");
                std::vector<std::shared_ptr<Stmt>> body = block({ XTokenType::END });
                consume(XTokenType::END, "Expect 'End' after method body.");
                match({ XTokenType::FUNCTION, XTokenType::SUB });
                methods.push_back(std::make_shared<FunctionStmt>(methodName.lexeme, parameters, body));
            }
            else {
                advance();
            }
        }
        consume(XTokenType::END, "Expect 'End' after class.");
        consume(XTokenType::CLASS, "Expect 'Class' after End.");
        return std::make_shared<ClassStmt>(name.lexeme, methods, properties);
    }
    std::shared_ptr<Stmt> varDeclaration(AccessModifier access, bool isConstant) {
        Token name = consume(XTokenType::IDENTIFIER, "Expect variable name.");
        bool isArray = false;
        if (match({ XTokenType::LEFT_PAREN })) {
            consume(XTokenType::RIGHT_PAREN, "Expect ')' in array declaration.");
            isArray = true;
        }
        std::string typeStr = "";
        std::shared_ptr<Expr> initializer = nullptr;
        if (match({ XTokenType::AS })) {
            if (check(XTokenType::NEW)) {
                advance(); // consume NEW
                Token typeToken = consume(XTokenType::IDENTIFIER, "Expect class name after 'New' in variable declaration.");
                typeStr = typeToken.lexeme;
                initializer = std::make_shared<NewExpr>(typeToken.lexeme, std::vector<std::shared_ptr<Expr>>{});
                if (match({ XTokenType::LEFT_PAREN })) {
                    std::vector<std::shared_ptr<Expr>> args;
                    if (!check(XTokenType::RIGHT_PAREN)) {
                        do {
                            args.push_back(expression());
                        } while (match({ XTokenType::COMMA }));
                    }
                    consume(XTokenType::RIGHT_PAREN, "Expect ')' after constructor arguments.");
                    initializer = std::make_shared<NewExpr>(typeToken.lexeme, args);
                }
            }
            else {
                Token typeToken = consume(XTokenType::IDENTIFIER, "Expect type after 'As' in variable declaration.");
                typeStr = toLower(typeToken.lexeme);
                if (match({ XTokenType::NEW })) {
                    Token classToken = consume(XTokenType::IDENTIFIER, "Expect class name after 'New'.");
                    initializer = std::make_shared<NewExpr>(classToken.lexeme, std::vector<std::shared_ptr<Expr>>{});
                    if (match({ XTokenType::LEFT_PAREN })) {
                        std::vector<std::shared_ptr<Expr>> args;
                        if (!check(XTokenType::RIGHT_PAREN)) {
                            do {
                                args.push_back(expression());
                            } while (match({ XTokenType::COMMA }));
                        }
                        consume(XTokenType::RIGHT_PAREN, "Expect ')' after constructor arguments.");
                        initializer = std::make_shared<NewExpr>(classToken.lexeme, args);
                    }
                }
            }
        }
        if (!initializer && match({ XTokenType::EQUAL }))
            initializer = expression();
        else if (isArray)
            initializer = std::make_shared<ArrayLiteralExpr>(std::vector<std::shared_ptr<Expr>>{});
        else if (typeStr == "pointer" || typeStr == "ptr")
            initializer = std::make_shared<LiteralExpr>(static_cast<void*>(nullptr)); // Initialize pointer to nullptr
        return std::make_shared<VarStmt>(name.lexeme, initializer, typeStr, isConstant, access);
    }
    std::shared_ptr<Stmt> ifStatement() {
        std::shared_ptr<Expr> condition = expression();
        consume(XTokenType::THEN, "Expect 'Then' after if condition.");
        std::vector<std::shared_ptr<Stmt>> thenBranch = block({ XTokenType::ELSEIF, XTokenType::ELSE, XTokenType::END });
        std::shared_ptr<Stmt> result = std::make_shared<IfStmt>(condition, thenBranch, std::vector<std::shared_ptr<Stmt>>{});
        while (match({ XTokenType::ELSEIF })) {
            std::shared_ptr<Expr> elseifCondition = expression();
            consume(XTokenType::THEN, "Expect 'Then' after ElseIf condition.");
            std::vector<std::shared_ptr<Stmt>> elseifBranch = block({ XTokenType::ELSEIF, XTokenType::ELSE, XTokenType::END });
            std::shared_ptr<Stmt> elseifStmt = std::make_shared<IfStmt>(elseifCondition, elseifBranch, std::vector<std::shared_ptr<Stmt>>{});
            std::shared_ptr<IfStmt> lastIf = std::dynamic_pointer_cast<IfStmt>(result);
            while (lastIf->elseBranch.size() == 1 && std::dynamic_pointer_cast<IfStmt>(lastIf->elseBranch[0])) {
                lastIf = std::dynamic_pointer_cast<IfStmt>(lastIf->elseBranch[0]);
            }
            lastIf->elseBranch = { elseifStmt };
        }
        if (match({ XTokenType::ELSE })) {
            std::vector<std::shared_ptr<Stmt>> elseBranch = block({ XTokenType::END });
            std::shared_ptr<IfStmt> lastIf = std::dynamic_pointer_cast<IfStmt>(result);
            while (lastIf->elseBranch.size() == 1 && std::dynamic_pointer_cast<IfStmt>(lastIf->elseBranch[0])) {
                lastIf = std::dynamic_pointer_cast<IfStmt>(lastIf->elseBranch[0]);
            }
            lastIf->elseBranch = elseBranch;
        }
        consume(XTokenType::END, "Expect 'End' after if statement.");
        consume(XTokenType::IF, "Expect 'If' after End in if statement.");
        return result;
    }
    // ---------------------------------------------------------
    std::shared_ptr<Stmt> forStatement() {
        Token varName = consume(XTokenType::IDENTIFIER, "Expect loop variable name.");
        if (match({ XTokenType::AS })) { consume(XTokenType::IDENTIFIER, "Expect type after 'As'."); }
        consume(XTokenType::EQUAL, "Expect '=' after loop variable.");
        std::shared_ptr<Expr> startExpr = expression();
        consume(XTokenType::TO, "Expect 'To' after initializer.");
        std::shared_ptr<Expr> endExpr = expression();
        std::shared_ptr<Expr> stepExpr = std::make_shared<LiteralExpr>(1);
        if (match({ XTokenType::STEP })) {
            stepExpr = expression();
        }
        std::vector<std::shared_ptr<Stmt>> body = block({ XTokenType::NEXT });
        consume(XTokenType::NEXT, "Expect 'Next' after For loop body.");
        if (check(XTokenType::IDENTIFIER)) advance();
        std::shared_ptr<Stmt> initializer = std::make_shared<VarStmt>(varName.lexeme, startExpr);
        std::shared_ptr<Expr> loopVar = std::make_shared<VariableExpr>(varName.lexeme);
        std::shared_ptr<Expr> condition = std::make_shared<BinaryExpr>(loopVar, BinaryOp::LE, endExpr);
        std::shared_ptr<Expr> increment = std::make_shared<AssignmentExpr>(varName.lexeme,
            std::make_shared<BinaryExpr>(loopVar, BinaryOp::ADD, stepExpr));
        body.push_back(std::make_shared<ExpressionStmt>(increment));
        std::vector<std::shared_ptr<Stmt>> forBlock = { initializer, std::make_shared<WhileStmt>(condition, body) };
        return std::make_shared<BlockStmt>(forBlock);
    }
    std::shared_ptr<Stmt> whileStatement() {
        std::shared_ptr<Expr> condition = expression();
        std::vector<std::shared_ptr<Stmt>> body = block({ XTokenType::WEND });
        consume(XTokenType::WEND, "Expect 'Wend' after while loop.");
        return std::make_shared<WhileStmt>(condition, body);
    }
    std::shared_ptr<Stmt> statement() {
        if (match({ XTokenType::RETURN }))
            return returnStatement();
        if (match({ XTokenType::PRINT }))
            return printStatement();
        return expressionStatement();
    }
    std::shared_ptr<Stmt> printStatement() {
        std::shared_ptr<Expr> value = expression();
        return std::make_shared<ExpressionStmt>(
            std::make_shared<CallExpr>(
                std::make_shared<LiteralExpr>(std::string("print")),
                std::vector<std::shared_ptr<Expr>>{value}
            )
        );
    }
    std::shared_ptr<Stmt> returnStatement() {
        std::shared_ptr<Expr> value = expression();
        return std::make_shared<ReturnStmt>(value);
    }
    std::shared_ptr<Stmt> expressionStatement() {
        std::shared_ptr<Expr> expr = expression();
        return std::make_shared<ExpressionStmt>(expr);
    }
    std::shared_ptr<Expr> assignment() {
        std::shared_ptr<Expr> expr = equality();
        if (match({ XTokenType::EQUAL })) {
            Token equals = previous();
            std::shared_ptr<Expr> value = assignment();
            if (auto varExpr = std::dynamic_pointer_cast<VariableExpr>(expr)) {
                return std::make_shared<AssignmentExpr>(varExpr->name, value);
            }
            runtimeError("Invalid assignment target.");
        }
        return expr;
    }
    std::shared_ptr<Expr> expression() { return assignment(); }
    std::shared_ptr<Expr> equality() {
        std::shared_ptr<Expr> expr = comparison();
        while (match({ XTokenType::EQUAL, XTokenType::NOT_EQUAL })) {
            Token op = previous();
            BinaryOp binOp = (op.type == XTokenType::EQUAL) ? BinaryOp::EQ : BinaryOp::NE;
            std::shared_ptr<Expr> right = comparison();
            expr = std::make_shared<BinaryExpr>(expr, binOp, right);
        }
        return expr;
    }
    std::shared_ptr<Expr> comparison() {
        std::shared_ptr<Expr> expr = addition();
        while (match({ XTokenType::LESS, XTokenType::LESS_EQUAL, XTokenType::GREATER, XTokenType::GREATER_EQUAL })) {
            Token op = previous();
            BinaryOp binOp;
            switch (op.type) {
            case XTokenType::LESS: binOp = BinaryOp::LT; break;
            case XTokenType::LESS_EQUAL: binOp = BinaryOp::LE; break;
            case XTokenType::GREATER: binOp = BinaryOp::GT; break;
            case XTokenType::GREATER_EQUAL: binOp = BinaryOp::GE; break;
            default: binOp = BinaryOp::EQ; break;
            }
            std::shared_ptr<Expr> right = addition();
            expr = std::make_shared<BinaryExpr>(expr, binOp, right);
        }
        return expr;
    }
    std::shared_ptr<Expr> addition() {
        std::shared_ptr<Expr> expr = multiplication();
        while (match({ XTokenType::PLUS, XTokenType::MINUS })) {
            Token op = previous();
            BinaryOp binOp = (op.type == XTokenType::PLUS) ? BinaryOp::ADD : BinaryOp::SUB;
            std::shared_ptr<Expr> right = multiplication();
            expr = std::make_shared<BinaryExpr>(expr, binOp, right);
        }
        return expr;
    }
    std::shared_ptr<Expr> multiplication() {
        std::shared_ptr<Expr> expr = exponentiation();
        while (match({ XTokenType::STAR, XTokenType::SLASH, XTokenType::MOD })) {
            Token op = previous();
            BinaryOp binOp;
            if (op.type == XTokenType::STAR) binOp = BinaryOp::MUL;
            else if (op.type == XTokenType::SLASH) binOp = BinaryOp::DIV;
            else if (op.type == XTokenType::MOD) binOp = BinaryOp::MOD;
            std::shared_ptr<Expr> right = exponentiation();
            expr = std::make_shared<BinaryExpr>(expr, binOp, right);
        }
        return expr;
    }
    std::shared_ptr<Expr> exponentiation() {
        std::shared_ptr<Expr> expr = unary();
        if (match({ XTokenType::CARET })) {
            std::shared_ptr<Expr> right = exponentiation();
            expr = std::make_shared<BinaryExpr>(expr, BinaryOp::POW, right);
        }
        return expr;
    }
    std::shared_ptr<Expr> unary() {
        if (match({ XTokenType::MINUS, XTokenType::NOT })) {
            Token op = previous();
            std::shared_ptr<Expr> right = unary();
            return std::make_shared<UnaryExpr>(op.lexeme, right);
        }
        return call();
    }
    std::shared_ptr<Expr> call() {
        std::shared_ptr<Expr> expr = primary();
        bool explicitCallUsed = false;
        while (true) {
            if (match({ XTokenType::LEFT_PAREN })) {
                explicitCallUsed = true;
                expr = finishCall(expr);
            }
            else if (match({ XTokenType::DOT })) {
                Token prop = consume(XTokenType::IDENTIFIER, "Expect property name after '.'");
                expr = std::make_shared<GetPropExpr>(expr, prop.lexeme);
            }
            else {
                break;
            }
        }
        return expr;
    }
    std::shared_ptr<Expr> finishCall(std::shared_ptr<Expr> callee) {
        std::vector<std::shared_ptr<Expr>> arguments;
        if (!check(XTokenType::RIGHT_PAREN)) {
            do {
                arguments.push_back(expression());
            } while (match({ XTokenType::COMMA }));
        }
        consume(XTokenType::RIGHT_PAREN, "Expect ')' after arguments.");
        return std::make_shared<CallExpr>(callee, arguments);
    }
    std::shared_ptr<Expr> primary() {
        if (match({ XTokenType::NUMBER })) {
            std::string lex = previous().lexeme;
            if (lex.find('.') != std::string::npos)
                return std::make_shared<LiteralExpr>(std::stod(lex));
            else
                return std::make_shared<LiteralExpr>(std::stoi(lex));
        }
        if (match({ XTokenType::STRING })) {
            std::string s = previous().lexeme;
            s = s.substr(1, s.size() - 2);
            return std::make_shared<LiteralExpr>(s);
        }
        if (match({ XTokenType::COLOR })) {
            std::string s = previous().lexeme;
            std::string hex = s.substr(2);
            unsigned int col = std::stoul(hex, nullptr, 16);
            return std::make_shared<LiteralExpr>(Color{ col });
        }
        if (match({ XTokenType::BOOLEAN_TRUE }))
            return std::make_shared<LiteralExpr>(true);
        if (match({ XTokenType::BOOLEAN_FALSE }))
            return std::make_shared<LiteralExpr>(false);
        if (match({ XTokenType::IDENTIFIER })) {
            Token id = previous();
            if (toLower(id.lexeme) == "array" && match({ XTokenType::LEFT_BRACKET })) {
                std::vector<std::shared_ptr<Expr>> elements;
                if (!check(XTokenType::RIGHT_BRACKET)) {
                    do {
                        elements.push_back(expression());
                    } while (match({ XTokenType::COMMA }));
                }
                consume(XTokenType::RIGHT_BRACKET, "Expect ']' after array literal.");
                return std::make_shared<ArrayLiteralExpr>(elements);
            }
            return std::make_shared<VariableExpr>(id.lexeme);
        }
        if (match({ XTokenType::LEFT_PAREN })) {
            std::shared_ptr<Expr> expr = expression();
            consume(XTokenType::RIGHT_PAREN, "Expect ')' after expression.");
            return std::make_shared<GroupingExpr>(expr);
        }
        std::cerr << "Parse error at line " << peek().line << ": Expected expression." << std::endl;
        exit(1);
        return nullptr;
    }
    
    // ***** selectCaseStatement() to support "Select Case" constructs *****
    std::shared_ptr<Stmt> selectCaseStatement() {
        consume(XTokenType::CASE, "Expect 'Case' after 'Select' in Select Case statement.");
        std::shared_ptr<Expr> switchExpr = expression();
        struct CaseClause {
            bool isDefault = false;
            std::shared_ptr<Expr> expr;
            std::vector<std::shared_ptr<Stmt>> statements;
        };
        std::vector<CaseClause> clauses;
        while (!check(XTokenType::END)) {
            consume(XTokenType::CASE, "Expect 'Case' at start of case clause.");
            CaseClause clause;
            if (match({ XTokenType::ELSE })) {
                clause.isDefault = true;
            }
            else {
                clause.isDefault = false;
                clause.expr = expression();
            }
            clause.statements = block({ XTokenType::CASE, XTokenType::END });
            clauses.push_back(clause);
        }
        consume(XTokenType::END, "Expect 'End' after Select Case statement.");
        consume(XTokenType::SELECT, "Expect 'Select' after 'End' in Select Case statement.");
        std::vector<std::shared_ptr<Stmt>> currentElse;
        for (int i = clauses.size() - 1; i >= 0; i--) {
            if (clauses[i].isDefault) {
                currentElse = clauses[i].statements;
            }
            else {
                auto condition = std::make_shared<BinaryExpr>(switchExpr, BinaryOp::EQ, clauses[i].expr);
                auto ifStmt = std::make_shared<IfStmt>(condition, clauses[i].statements, currentElse);
                currentElse.clear();
                currentElse.push_back(ifStmt);
            }
        }
        if (currentElse.empty()) {
            return std::make_shared<BlockStmt>(std::vector<std::shared_ptr<Stmt>>{});
        }
        return currentElse[0];
    }
    // ***** End of Select Case support *****
};

// ============================================================================  
// Helpers for constant pool management
// ============================================================================
int addConstant(ObjFunction::CodeChunk& chunk, const Value& v) {
    chunk.constants.push_back(v);
    return chunk.constants.size() - 1;
}

int addConstantString(ObjFunction::CodeChunk& chunk, const std::string& s) {
    for (int i = 0; i < chunk.constants.size(); i++) {
        if (holds<std::string>(chunk.constants[i])) {
            if (getVal<std::string>(chunk.constants[i]) == s)
                return i;
        }
    }
    chunk.constants.push_back(s);
    return chunk.constants.size() - 1;
}

// ============================================================================  
// Built-in Array Methods
// ============================================================================

Value callArrayMethod(std::shared_ptr<ObjArray> array, const std::string& method, const std::vector<Value>& args) {
    std::string m = toLower(method);
    if (m == "add") {
        if (args.size() != 1) runtimeError("Array.add expects 1 argument.");
        array->elements.push_back(args[0]);
        return Value(std::monostate{});
    }
    else if (m == "indexof") {
        if (args.size() != 1) runtimeError("Array.indexof expects 1 argument.");
        for (size_t i = 0; i < array->elements.size(); i++) {
            if (valueToString(array->elements[i]) == valueToString(args[0]))
                return (int)i;
        }
        return -1;
    }
    else if (m == "lastindex") {
        return array->elements.empty() ? -1 : (int)(array->elements.size() - 1);
    }
    else if (m == "count") {
        return (int)array->elements.size();
    }
    else if (m == "pop") {
        if (array->elements.empty()) runtimeError("Array.pop called on empty array.");
        Value last = array->elements.back();
        array->elements.pop_back();
        return last;
    }
    else if (m == "removeat") {
        if (args.size() != 1) runtimeError("Array.removeat expects 1 argument.");
        int index = 0;
        if (holds<int>(args[0]))
            index = getVal<int>(args[0]);
        else runtimeError("Array.removeat expects an integer index.");
        if (index < 0 || index >= (int)array->elements.size())
            runtimeError("Array.removeat index out of bounds.");
        array->elements.erase(array->elements.begin() + index);
        return Value(std::monostate{});
    }
    else if (m == "removeall") {
        array->elements.clear();
        return Value(std::monostate{});
    }
    else {
        runtimeError("Unknown array method: " + method);
    }
    return Value(std::monostate{});
}

// ============================================================================  
// Plugin Loader and libffi wrappers
// ============================================================================
struct PluginEntry {
    const char* name;
    void* funcPtr;
    int arity = 0;
    const char* paramTypes[10]; // Supports up to 10 parameters
    const char* returnType;     // Return type string
};

typedef PluginEntry* (*GetPluginEntriesFunc)(int*);

struct ClassProperty {
    const char* name;
    const char* type;
    void* getter;
    void* setter;
};

struct ClassEntry {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
};

struct ClassConstant {
    const char* declaration;
};

struct ClassDefinition {
    const char* className;
    size_t classSize;
    void* constructor;
    ClassProperty* properties;
    size_t propertiesCount;
    ClassEntry* methods;
    size_t methodsCount;
    ClassConstant* constants;
    size_t constantsCount;
};
typedef ClassDefinition* (*GetClassDefinitionFunc)();

// Updated mapType to support "array"
ffi_type* mapType(const std::string& type) {
    if (type == "string") return &ffi_type_pointer;
    else if (type == "double") return &ffi_type_double;
    else if (type == "number") return &ffi_type_double;
    else if (type == "void") return &ffi_type_pointer;
    else if (type == "integer") return &ffi_type_sint;
    else if (type == "int") return &ffi_type_sint;
    else if (type == "boolean") return &ffi_type_uint8;
    else if (type == "bool") return &ffi_type_uint8;
    else if (type == "color") return &ffi_type_uint32;
    else if (type == "variant") return &ffi_type_pointer;
    else if (type == "pointer") return &ffi_type_pointer;
    else if (type == "ptr") return &ffi_type_pointer;
    else if (type == "array") return &ffi_type_pointer;
    else return nullptr;
}

BuiltinFn wrapPluginFunction(void* funcPtr, int arity, const char** paramTypes, const char* returnTypeStr) {
    ffi_cif* cif = new ffi_cif;
    ffi_type** argTypes = new ffi_type*[arity];
    for (int i = 0; i < arity; i++) {
        std::string t = toLower(std::string(paramTypes[i] ? paramTypes[i] : ""));
        ffi_type* ft = mapType(t);
        if (!ft) {
            std::cerr << "Unknown plugin parameter type: " << t << std::endl;
            exit(1);
        }
        argTypes[i] = ft;
    }
    // IMPORTANT: Plugin functions that return objects must be declared with "pointer" (or "ptr").
    std::string retTypeString = toLower(std::string(returnTypeStr ? returnTypeStr : "variant"));
    ffi_type* retType = mapType(retTypeString);
    if (!retType) {
        std::cerr << "Unknown plugin return type: " << retTypeString << std::endl;
        exit(1);
    }
    if (ffi_prep_cif(cif, FFI_DEFAULT_ABI, arity, retType, argTypes) != FFI_OK) {
        std::cerr << "ffi_prep_cif failed for plugin function." << std::endl;
        exit(1);
    }
    return [funcPtr, cif, arity, argTypes, paramTypes, retType, retTypeString](const std::vector<Value>& args) -> Value {
        debugLog("PluginFunction: Calling plugin function with " + std::to_string(args.size()) + " arguments.");
        for (size_t i = 0; i < args.size(); i++) {
            debugLog("Arg[" + std::to_string(i) + "] type: " + getTypeName(args[i]) +
                     " value: " + valueToString(args[i]));
        }
        if ((int)args.size() != arity)
            runtimeError("Plugin function expects " + std::to_string(arity) + " arguments.");
        void** argValues = new void*[arity];
        int intStorage[10] = {0};
        double doubleStorage[10] = {0.0};
        bool boolStorage[10] = {false};
        const char* stringStorage[10] = {nullptr};
        unsigned int uintStorage[10] = {0};
        Value* variantStorage[10] = {nullptr};
        void* pointerStorage[10] = {nullptr};
        for (int i = 0; i < arity; i++) {
            std::string pType = toLower(std::string(paramTypes[i] ? paramTypes[i] : ""));
            if (pType == "string") {
                if (!holds<std::string>(args[i])) runtimeError("Plugin expects a string argument.");
                std::string s = getVal<std::string>(args[i]);
                stringStorage[i] = strdup(s.c_str());
                argValues[i] = &stringStorage[i];
            }
            else if (pType == "double") {
                if (holds<double>(args[i]))
                    doubleStorage[i] = getVal<double>(args[i]);
                else if (holds<int>(args[i]))
                    doubleStorage[i] = static_cast<double>(getVal<int>(args[i]));
                else
                    runtimeError("Plugin expects a double argument.");
                argValues[i] = &doubleStorage[i];
            }
            else if (pType == "integer") {
                if (!holds<int>(args[i])) runtimeError("Plugin expects an integer argument.");
                intStorage[i] = getVal<int>(args[i]);
                argValues[i] = &intStorage[i];
            }
            else if (pType == "boolean") {
                if (!holds<bool>(args[i])) runtimeError("Plugin expects a boolean argument.");
                boolStorage[i] = getVal<bool>(args[i]);
                argValues[i] = &boolStorage[i];
            }
            else if (pType == "color") {
                if (!holds<Color>(args[i])) runtimeError("Plugin expects a color argument.");
                uintStorage[i] = getVal<Color>(args[i]).value;
                argValues[i] = &uintStorage[i];
            }
            else if (pType == "variant") {
                // For parameters declared as variant we copy the Value.
                variantStorage[i] = new Value(args[i]);
                argValues[i] = &variantStorage[i];
            }
            else if (pType == "array") {
                if (!holds<std::shared_ptr<ObjArray>>(args[i])) runtimeError("Plugin expects an array argument.");
                auto arr = getVal<std::shared_ptr<ObjArray>>(args[i]);
                pointerStorage[i] = static_cast<void*>(arr.get());
                argValues[i] = &pointerStorage[i];
            }
            else if (pType == "pointer" || pType == "ptr") {
                if (!holds<void*>(args[i])) runtimeError("Plugin expects a pointer argument.");
                pointerStorage[i] = getVal<void*>(args[i]);
                argValues[i] = &pointerStorage[i];
            }
            else {
                runtimeError("Unsupported plugin parameter type: " + pType);
            }
        }

        debugLog("PluginFunction: About to call ffi_call.");
        union {
            int i;
            double d;
            bool b;
            const char* s;
            unsigned int ui;
            Value* variant;
            void* p;// Pointer return storage
        } resultStorage;

        std::ostringstream oss;
        oss << "Result Storage Function Pointer: " << reinterpret_cast<void*>(&resultStorage);
        debugLog(oss.str());

        ffi_call(cif, FFI_FN(funcPtr), &resultStorage, argValues);

        debugLog("PluginFunction: ffi_call returned; result type: " + retTypeString);
        for (int i = 0; i < arity; i++) {
            std::string pType = toLower(std::string(paramTypes[i] ? paramTypes[i] : ""));
            if (pType == "string") {
                free((void*)stringStorage[i]);
            }
            else if (pType == "variant") {
                delete variantStorage[i];
            }
        }
        delete[] argValues;
        if (retTypeString == "string") {
            debugLog("PluginFunction: Returning value: " + valueToString(Value(std::string(resultStorage.s ? resultStorage.s : ""))));
            return Value(std::string(resultStorage.s ? resultStorage.s : ""));
        }
        else if (retTypeString == "double") {
            debugLog("PluginFunction: Returning value: " + valueToString(Value(resultStorage.d)));
            return Value(resultStorage.d);
        }
        else if (retTypeString == "integer") {
            debugLog("PluginFunction: Returning value: " + valueToString(Value(resultStorage.i)));
            return Value(resultStorage.i);
        }
        else if (retTypeString == "boolean") {
            debugLog("PluginFunction: Returning value: " + valueToString(Value(resultStorage.b)));
            return Value(resultStorage.b);
        }
        else if (retTypeString == "color") {
            debugLog("PluginFunction: Returning value: " + valueToString(Value(Color{ resultStorage.ui })));
            return Value(Color{ resultStorage.ui });
        }
        else if (retTypeString == "variant") {
            debugLog("Setting Variant pointer...");
            if (resultStorage.variant) {
                Value retVal = *(resultStorage.variant);
                delete resultStorage.variant;
                return retVal;
            }
            else {
                debugLog("PluginFunction: Returning nil variant.");
                return Value(std::monostate{});
            }
        }
        else if (retTypeString == "array") {
            ObjArray* arrPtr = static_cast<ObjArray*>(resultStorage.p);
            if (arrPtr) {
                auto sharedArr = std::shared_ptr<ObjArray>(arrPtr, [](ObjArray*){});
                return Value(sharedArr);
            } else {
                return Value(std::monostate{});
            }
        }
        else if (retTypeString == "pointer" || retTypeString == "ptr") {
            debugLog("PluginFunction: Returning pointer value: " + valueToString(Value(resultStorage.p)));
            return Value(resultStorage.p);
        }
        else {
            runtimeError("Unsupported plugin return type: " + getTypeName(resultStorage));
        }
        return Value(std::monostate{});
    };
}

// In our system, we create a helper for Declare statements that loads the plugin library
// and wraps the exported function using libffi.
BuiltinFn wrapPluginFunctionForDeclare(const std::vector<Param>& params, const std::string& retType,
    const std::string& apiName, const std::string& libName) {
    void* libHandle = nullptr;
#ifdef _WIN32
    libHandle = LoadLibraryA(libName.c_str());
    if (!libHandle) {
        debugLog("Error loading library: " + libName);
        exit(1);
    }
    void* funcPtr = reinterpret_cast<void*>(GetProcAddress((HMODULE)libHandle, apiName.c_str()));
#else
    libHandle = dlopen(libName.c_str(), RTLD_LAZY);
    if (!libHandle) {
        std::cerr << "Error loading library: " << libName << std::endl;
        exit(1);
    }
    void* funcPtr = dlsym(libHandle, apiName.c_str());
#endif
    if (!funcPtr) {
        std::cerr << "Error finding symbol: " << apiName << " in library: " << libName << std::endl;
        exit(1);
    }
    const char* pTypes[10] = { nullptr };
    int arity = params.size();
    for (int i = 0; i < arity; i++) {
        pTypes[i] = params[i].type.c_str();
    }
    return wrapPluginFunction(funcPtr, arity, pTypes, retType.c_str());
}

// loadPlugins: Loads plugin libraries from the "libs" folder (located beside the executable)
// using cross-platform directory listing and libffi for function wrapping.
void loadPlugins(VM& vm) {
    std::string libsDir;
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir = exePath;
    size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos) {
        exeDir = exeDir.substr(0, pos);
    }
    libsDir = exeDir + "\\libs\\";
#else
    libsDir = "libs/";
#endif
#ifdef _WIN32
    std::string pattern = libsDir + "*.dll";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string dllPath = libsDir + findData.cFileName;
            HMODULE hModule = LoadLibraryA(dllPath.c_str());
            if (hModule) {
                GetPluginEntriesFunc getEntries = (GetPluginEntriesFunc)GetProcAddress(hModule, "GetPluginEntries");
                if (getEntries) {
                    int count = 0;
                    PluginEntry* entries = getEntries(&count);
                    for (int i = 0; i < count; i++) {
                        PluginEntry& entry = entries[i];
                        BuiltinFn fn = wrapPluginFunction(entry.funcPtr, entry.arity, entry.paramTypes, entry.returnType);
                        std::string funcName = toLower(std::string(entry.name));
                        vm.environment->define(funcName, fn);
                        debugLog("Loaded plugin function: " + std::string(entry.name) + " with arity " + std::to_string(entry.arity) + " from " + dllPath);
                    }
                }
                else {
                    GetClassDefinitionFunc getClassDef = (GetClassDefinitionFunc)GetProcAddress(hModule, "GetClassDefinition");
                    if (getClassDef) {
                        ClassDefinition* classDef = getClassDef();
                        auto pluginClass = std::make_shared<ObjClass>();
                        pluginClass->name = toLower(classDef->className);
                        pluginClass->isPlugin = true;
                        pluginClass->pluginConstructor = wrapPluginFunction(classDef->constructor, 0, nullptr, "pointer");
                        for (size_t i = 0; i < classDef->propertiesCount; i++) {
                            ClassProperty& prop = classDef->properties[i];
                            const char* getterParams[1] = { "pointer" };
                            BuiltinFn getterFn = wrapPluginFunction(prop.getter, 1, getterParams, prop.type);
                            const char* setterParams[2] = { "pointer", prop.type };
                            BuiltinFn setterFn = wrapPluginFunction(prop.setter, 2, setterParams, "void");
                            pluginClass->pluginProperties[toLower(prop.name)] = std::make_pair(getterFn, setterFn);
                        }
                        for (size_t i = 0; i < classDef->methodsCount; i++) {
                            ClassEntry& entry = classDef->methods[i];
                            BuiltinFn methodFn = wrapPluginFunction(entry.funcPtr, entry.arity, entry.paramTypes, entry.retType);
                            std::string methodName = toLower(entry.name);
                            pluginClass->methods[methodName] = methodFn;
                        }
                        vm.environment->define(toLower(pluginClass->name), Value(pluginClass));
                        debugLog("Loaded plugin class: " + pluginClass->name + " from " + dllPath);
                    }
                    else {
                        debugLog("DLL " + dllPath + " does not export GetPluginEntries or GetClassDefinition.");
                    }
                }
            }
            else {
                debugLog("Failed to load DLL: " + dllPath);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(libsDir.c_str());
    if (!dir) {
        debugLog("Failed to open libs directory: " + libsDir);
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
#ifdef __APPLE__
        if (filename.size() >= 6 && filename.substr(filename.size() - 6) == ".dylib")
#else
        if (filename.size() >= 3 && filename.substr(filename.size() - 3) == ".so")
#endif
        {
            std::string fullPath = libsDir + filename;
            void* libHandle = dlopen(fullPath.c_str(), RTLD_LAZY);
            if (libHandle) {
                GetPluginEntriesFunc getEntries = (GetPluginEntriesFunc)dlsym(libHandle, "GetPluginEntries");
                if (getEntries) {
                    int count = 0;
                    PluginEntry* entries = getEntries(&count);
                    for (int i = 0; i < count; i++) {
                        PluginEntry& entry = entries[i];
                        BuiltinFn fn = wrapPluginFunction(entry.funcPtr, entry.arity, entry.paramTypes, entry.returnType);
                        std::string funcName = toLower(std::string(entry.name));
                        vm.environment->define(funcName, fn);
                        debugLog("Loaded plugin function: " + std::string(entry.name) + " with arity " + std::to_string(entry.arity) + " from " + fullPath);
                    }
                }
                else {
                    GetClassDefinitionFunc getClassDef = (GetClassDefinitionFunc)dlsym(libHandle, "GetClassDefinition");
                    if (getClassDef) {
                        ClassDefinition* classDef = getClassDef();
                        auto pluginClass = std::make_shared<ObjClass>();
                        pluginClass->name = toLower(classDef->className);
                        pluginClass->isPlugin = true;
                        pluginClass->pluginConstructor = wrapPluginFunction(classDef->constructor, 0, nullptr, "pointer");
                        for (size_t i = 0; i < classDef->propertiesCount; i++) {
                            ClassProperty& prop = classDef->properties[i];
                            const char* getterParams[1] = { "pointer" };
                            BuiltinFn getterFn = wrapPluginFunction(prop.getter, 1, getterParams, prop.type);
                            const char* setterParams[2] = { "pointer", prop.type };
                            BuiltinFn setterFn = wrapPluginFunction(prop.setter, 2, setterParams, "void");
                            pluginClass->pluginProperties[toLower(prop.name)] = std::make_pair(getterFn, setterFn);
                        }
                        for (size_t i = 0; i < classDef->methodsCount; i++) {
                            ClassEntry& entry = classDef->methods[i];
                            BuiltinFn methodFn = wrapPluginFunction(entry.funcPtr, entry.arity, entry.paramTypes, entry.retType);
                            std::string methodName = toLower(entry.name);
                            pluginClass->methods[methodName] = methodFn;
                        }
                        vm.environment->define(toLower(pluginClass->name), Value(pluginClass));
                        debugLog("Loaded plugin class: " + pluginClass->name + " from " + fullPath);
                    }
                    else {
                        debugLog("Library " + fullPath + " does not export GetPluginEntries or GetClassDefinition.");
                    }
                }
            }
            else {
                debugLog("Failed to load library: " + fullPath);
            }
        }
    }
    closedir(dir);
#endif
}

// ============================================================================  
// Compiler
// ============================================================================
class Compiler {
public:
    Compiler(VM& virtualMachine) : vm(virtualMachine), compilingModule(false) {}
    void compile(const std::vector<std::shared_ptr<Stmt>>& stmts) {
        for (auto stmt : stmts) {
            compileStmt(stmt, vm.mainChunk);
            debugLog("Compiler: Compiled a statement. Main chunk now has " +
                std::to_string(vm.mainChunk.code.size()) + " instructions.");
        }
    }
private:
    VM& vm;
    bool compilingModule; // Flag indicating if compiling a module
    std::string currentModuleName; // Current module name
    std::unordered_map<std::string, Value> currentModulePublicMembers;  // Public members of current module

    void emit(ObjFunction::CodeChunk& chunk, int byte) {
        chunk.code.push_back(byte);
    }
    void emitWithOperand(ObjFunction::CodeChunk& chunk, int opcode, int operand) {
        emit(chunk, opcode);
        emit(chunk, operand);
    }
    void compileStmt(std::shared_ptr<Stmt> stmt, ObjFunction::CodeChunk& chunk) {
        if (auto modStmt = std::dynamic_pointer_cast<ModuleStmt>(stmt)) {
            auto previousEnv = vm.environment;
            auto moduleEnv = std::make_shared<Environment>(previousEnv);
            vm.environment = moduleEnv;
            bool oldCompilingModule = compilingModule;
            compilingModule = true;
            currentModuleName = toLower(modStmt->name);
            currentModulePublicMembers.clear();
            for (auto s : modStmt->body)
                compileStmt(s, chunk);
            auto moduleObj = std::make_shared<ObjModule>();
            moduleObj->name = currentModuleName;
            moduleObj->publicMembers = currentModulePublicMembers;
            vm.environment = previousEnv;
            compilingModule = oldCompilingModule;
            vm.environment->define(toLower(currentModuleName), Value(moduleObj));
            for (auto& entry : currentModulePublicMembers) {
                vm.environment->define(entry.first, entry.second);
            }
            return;
        }
        else if (auto declStmt = std::dynamic_pointer_cast<DeclareStmt>(stmt)) {
            compileDeclare(declStmt, chunk);
        }
        else if (auto enumStmt = std::dynamic_pointer_cast<EnumStmt>(stmt)) {
            auto enumObj = std::make_shared<ObjEnum>();
            enumObj->name = toLower(enumStmt->name);
            enumObj->members = enumStmt->members;
            if (!compilingModule) {
                int nameConst = addConstantString(chunk, toLower(enumStmt->name));
                int enumConstant = addConstant(chunk, Value(enumObj));
                emitWithOperand(chunk, OP_CONSTANT, enumConstant);
                emitWithOperand(chunk, OP_DEFINE_GLOBAL, nameConst);
            }
            else {
                currentModulePublicMembers[toLower(enumStmt->name)] = Value(enumObj);
                vm.environment->define(toLower(enumStmt->name), Value(enumObj));
            }
        }
        else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStmt>(stmt)) {
            compileExpr(exprStmt->expression, chunk);
            emit(chunk, OP_POP);
        }
        else if (auto retStmt = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
            if (retStmt->value)
                compileExpr(retStmt->value, chunk);
            else
                emit(chunk, OP_NIL);
            emit(chunk, OP_RETURN);
        }
        else if (auto funcStmt = std::dynamic_pointer_cast<FunctionStmt>(stmt)) {
            std::shared_ptr<ObjFunction> placeholder = std::make_shared<ObjFunction>();
            placeholder->name = funcStmt->name;
            int req = 0;
            for (auto& p : funcStmt->params)
                if (!p.optional) req++;
            placeholder->arity = req;
            placeholder->params = funcStmt->params;
            vm.environment->define(toLower(funcStmt->name), Value(placeholder));
            compileFunction(funcStmt);
            vm.environment->assign(toLower(funcStmt->name), Value(lastFunction));
            if (!compilingModule) {
                int fnConst = addConstant(chunk, vm.environment->get(toLower(funcStmt->name)));
                emitWithOperand(chunk, OP_CONSTANT, fnConst);
                int nameConst = addConstantString(chunk, toLower(funcStmt->name));
                emitWithOperand(chunk, OP_DEFINE_GLOBAL, nameConst);
            }
            else {
                if (funcStmt->access == AccessModifier::PUBLIC) {
                    currentModulePublicMembers[toLower(funcStmt->name)] = vm.environment->get(toLower(funcStmt->name));
                }
            }
        }
        else if (auto varStmt = std::dynamic_pointer_cast<VarStmt>(stmt)) {
            if (varStmt->initializer)
                compileExpr(varStmt->initializer, chunk);
            else {
                if (varStmt->varType == "integer" || varStmt->varType == "double")
                    compileExpr(std::make_shared<LiteralExpr>(0), chunk);
                else if (varStmt->varType == "boolean")
                    compileExpr(std::make_shared<LiteralExpr>(false), chunk);
                else if (varStmt->varType == "string")
                    compileExpr(std::make_shared<LiteralExpr>(std::string("")), chunk);
                else if (varStmt->varType == "color")
                    compileExpr(std::make_shared<LiteralExpr>(Color{ 0 }), chunk);
                else if (varStmt->varType == "array")
                    compileExpr(std::make_shared<LiteralExpr>(Value(std::make_shared<ObjArray>())), chunk);
                else if (varStmt->varType == "pointer" || varStmt->varType == "ptr")
                    compileExpr(std::make_shared<LiteralExpr>(static_cast<void*>(nullptr)), chunk);
                else
                    compileExpr(std::make_shared<LiteralExpr>(std::monostate{}), chunk);
            }
            if (!compilingModule) {
                int nameConst = addConstantString(chunk, toLower(varStmt->name));
                emitWithOperand(chunk, OP_DEFINE_GLOBAL, nameConst);
            }
            else {
                if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(varStmt->initializer)) {
                    if (varStmt->access == AccessModifier::PUBLIC) {
                        currentModulePublicMembers[toLower(varStmt->name)] = lit->value;
                    }
                    vm.environment->define(toLower(varStmt->name), lit->value);
                }
            }
        }
        else if (auto classStmt = std::dynamic_pointer_cast<ClassStmt>(stmt)) {
            int nameConst = addConstantString(chunk, toLower(classStmt->name));
            emitWithOperand(chunk, OP_CLASS, nameConst);
            for (auto method : classStmt->methods) {
                compileFunction(method);
                int fnConst = addConstant(chunk, Value(lastFunction));
                emitWithOperand(chunk, OP_CONSTANT, fnConst);
                int methodNameConst = addConstantString(chunk, toLower(method->name));
                emitWithOperand(chunk, OP_METHOD, methodNameConst);
            }
            if (!classStmt->properties.empty()) {
                int propConst = addConstant(chunk, Value(classStmt->properties));
                emitWithOperand(chunk, OP_PROPERTIES, propConst);
            }
            int classNameConst = addConstantString(chunk, toLower(classStmt->name));
            emitWithOperand(chunk, OP_DEFINE_GLOBAL, classNameConst);
        }
        else if (auto propAssign = std::dynamic_pointer_cast<PropertyAssignmentStmt>(stmt)) {
            compileExpr(propAssign->object, chunk);
            compileExpr(propAssign->value, chunk);
            int propConst = addConstantString(chunk, toLower(propAssign->property));
            emitWithOperand(chunk, OP_SET_PROPERTY, propConst);
        }
        else if (auto assignStmt = std::dynamic_pointer_cast<AssignmentStmt>(stmt)) {
            compileExpr(std::make_shared<VariableExpr>(assignStmt->name), chunk);
            compileExpr(assignStmt->value, chunk);
            int nameConst = addConstantString(chunk, toLower(assignStmt->name));
            emitWithOperand(chunk, OP_SET_GLOBAL, nameConst);
        }
        else if (auto setProp = std::dynamic_pointer_cast<SetPropExpr>(stmt)) {
            compileExpr(setProp->object, chunk);
            compileExpr(setProp->value, chunk);
            int propConst = addConstantString(chunk, toLower(setProp->name));
            emitWithOperand(chunk, OP_SET_PROPERTY, propConst);
        }
        else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
            compileExpr(ifStmt->condition, chunk);
            int jumpIfFalsePos = chunk.code.size();
            emitWithOperand(chunk, OP_JUMP_IF_FALSE, 0);
            for (auto thenStmt : ifStmt->thenBranch)
                compileStmt(thenStmt, chunk);
            int jumpPos = chunk.code.size();
            emitWithOperand(chunk, OP_JUMP, 0);
            int elseStart = chunk.code.size();
            chunk.code[jumpIfFalsePos + 1] = elseStart;
            for (auto elseStmt : ifStmt->elseBranch)
                compileStmt(elseStmt, chunk);
            int endIf = chunk.code.size();
            chunk.code[jumpPos + 1] = endIf;
        }
        else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
            int loopStart = chunk.code.size();
            compileExpr(whileStmt->condition, chunk);
            int exitJumpPos = chunk.code.size();
            emitWithOperand(chunk, OP_JUMP_IF_FALSE, 0);
            for (auto bodyStmt : whileStmt->body)
                compileStmt(bodyStmt, chunk);
            emitWithOperand(chunk, OP_JUMP, loopStart);
            int loopEnd = chunk.code.size();
            chunk.code[exitJumpPos + 1] = loopEnd;
        }
        else if (auto blockStmt = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
            for (auto s : blockStmt->statements)
                compileStmt(s, chunk);
        }
    }
    // compileDeclare for API declarations using libffi
    void compileDeclare(std::shared_ptr<DeclareStmt> declStmt, ObjFunction::CodeChunk& chunk) {
        BuiltinFn apiFunc = wrapPluginFunctionForDeclare(
            declStmt->params,
            declStmt->returnType,
            declStmt->apiName,
            declStmt->libraryName
        );
        vm.environment->define(toLower(declStmt->apiName), Value(apiFunc));
        if (!compilingModule) {
            int fnConst = addConstant(chunk, vm.environment->get(toLower(declStmt->apiName)));
            emitWithOperand(chunk, OP_CONSTANT, fnConst);
            int nameConst = addConstantString(chunk, toLower(declStmt->apiName));
            emitWithOperand(chunk, OP_DEFINE_GLOBAL, nameConst);
        }
        else {
            currentModulePublicMembers[toLower(declStmt->apiName)] = vm.environment->get(toLower(declStmt->apiName));
        }
    }
    void compileExpr(std::shared_ptr<Expr> expr, ObjFunction::CodeChunk& chunk) {
        if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
            int constIndex = addConstant(chunk, lit->value);
            emitWithOperand(chunk, OP_CONSTANT, constIndex);
        }
        else if (auto var = std::dynamic_pointer_cast<VariableExpr>(expr)) {
            int nameConst = addConstantString(chunk, toLower(var->name));
            emitWithOperand(chunk, OP_GET_GLOBAL, nameConst);
        }
        else if (auto un = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
            compileExpr(un->right, chunk);
            if (un->op == "-")
                emit(chunk, OP_NEGATE);
        }
        else if (auto assignExpr = std::dynamic_pointer_cast<AssignmentExpr>(expr)) {
            compileExpr(std::make_shared<VariableExpr>(assignExpr->name), chunk);
            compileExpr(assignExpr->value, chunk);
            int nameConst = addConstantString(chunk, toLower(assignExpr->name));
            emitWithOperand(chunk, OP_SET_GLOBAL, nameConst);
        }
        else if (auto setProp = std::dynamic_pointer_cast<SetPropExpr>(expr)) {
            compileExpr(setProp->object, chunk);
            compileExpr(setProp->value, chunk);
            int propConst = addConstantString(chunk, toLower(setProp->name));
            emitWithOperand(chunk, OP_SET_PROPERTY, propConst);
        }
        else if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
            compileExpr(bin->left, chunk);
            compileExpr(bin->right, chunk);
            switch (bin->op) {
            case BinaryOp::ADD: emit(chunk, OP_ADD); break;
            case BinaryOp::SUB: emit(chunk, OP_SUB); break;
            case BinaryOp::MUL: emit(chunk, OP_MUL); break;
            case BinaryOp::DIV: emit(chunk, OP_DIV); break;
            case BinaryOp::LT:  emit(chunk, OP_LT); break;
            case BinaryOp::LE:  emit(chunk, OP_LE); break;
            case BinaryOp::GT:  emit(chunk, OP_GT); break;
            case BinaryOp::GE:  emit(chunk, OP_GE); break;
            case BinaryOp::NE:  emit(chunk, OP_NE); break;
            case BinaryOp::EQ:  emit(chunk, OP_EQ); break;
            case BinaryOp::AND: emit(chunk, OP_AND); break;
            case BinaryOp::OR:  emit(chunk, OP_OR); break;
            case BinaryOp::POW: emit(chunk, OP_POW); break;
            case BinaryOp::MOD: emit(chunk, OP_MOD); break;
            default: break;
            }
        }
        else if (auto group = std::dynamic_pointer_cast<GroupingExpr>(expr)) {
            compileExpr(group->expression, chunk);
        }
        else if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
            compileExpr(call->callee, chunk);
            for (auto arg : call->arguments)
                compileExpr(arg, chunk);
            emitWithOperand(chunk, OP_CALL, call->arguments.size());
        }
        else if (auto arrLit = std::dynamic_pointer_cast<ArrayLiteralExpr>(expr)) {
            for (auto& elem : arrLit->elements)
                compileExpr(elem, chunk);
            emitWithOperand(chunk, OP_ARRAY, arrLit->elements.size());
        }
        else if (auto getProp = std::dynamic_pointer_cast<GetPropExpr>(expr)) {
            compileExpr(getProp->object, chunk);
            int propConst = addConstantString(chunk, toLower(getProp->name));
            emitWithOperand(chunk, OP_GET_PROPERTY, propConst);
        }
        else if (auto newExpr = std::dynamic_pointer_cast<NewExpr>(expr)) {
            int classConst = addConstantString(chunk, toLower(newExpr->className));
            emitWithOperand(chunk, OP_GET_GLOBAL, classConst);
            emit(chunk, OP_NEW);
            if (!newExpr->arguments.empty()) {
                emit(chunk, OP_DUP);
                int consName = addConstantString(chunk, "constructor");
                emitWithOperand(chunk, OP_GET_PROPERTY, consName);
                emitWithOperand(chunk, OP_OPTIONAL_CALL, newExpr->arguments.size());
                emit(chunk, OP_CONSTRUCTOR_END);
            }
        }
    }
    std::shared_ptr<ObjFunction> lastFunction;
    void compileFunction(std::shared_ptr<FunctionStmt> funcStmt) {
        auto function = std::make_shared<ObjFunction>();
        function->name = funcStmt->name;
        int req = 0;
        for (auto& p : funcStmt->params)
            if (!p.optional) req++;
        function->arity = req;
        function->params = funcStmt->params;
        ObjFunction::CodeChunk fnChunk;
        for (auto stmt : funcStmt->body)
            compileStmt(stmt, fnChunk);
        emit(fnChunk, OP_NIL);
        emit(fnChunk, OP_RETURN);
        function->chunk = fnChunk;
        lastFunction = function;
        debugLog("Compiler: Compiled function: " + function->name + " with required arity " + std::to_string(function->arity));
    }
};

// ============================================================================  
// Virtual Machine Execution
// ============================================================================
Value runVM(VM& vm, const ObjFunction::CodeChunk& chunk) {
    int ip = 0;
    while (ip < chunk.code.size()) {
        int currentIp = ip;
        int instruction = chunk.code[ip++];
        debugLog("VM: IP " + std::to_string(currentIp) + ": Executing " + opcodeToString(instruction));
        switch (instruction) {
        case OP_CONSTANT: {
            int index = chunk.code[ip++];
            Value constant = chunk.constants[index];
            vm.stack.push_back(constant);
            debugLog("VM: Loaded constant: " + valueToString(constant));
            break;
        }
        case OP_ADD: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) + getVal<int>(b));
            else if (holds<double>(a) || holds<double>(b)) {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad + bd);
            }
            else if (holds<std::string>(a) && holds<std::string>(b))
                vm.stack.push_back(getVal<std::string>(a) + getVal<std::string>(b));
            else runtimeError("VM: Operands must be numbers or strings for addition.");
            break;
        }
        case OP_SUB: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) - getVal<int>(b));
            else {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad - bd);
            }
            break;
        }
        case OP_MUL: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) * getVal<int>(b));
            else {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad * bd);
            }
            break;
        }
        case OP_DIV: {
            Value b = pop(vm), a = pop(vm);
            double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
            double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
            vm.stack.push_back(ad / bd);
            break;
        }
        case OP_NEGATE: {
            Value v = pop(vm);
            if (holds<int>(v))
                vm.stack.push_back(-getVal<int>(v));
            else if (holds<double>(v))
                vm.stack.push_back(-getVal<double>(v));
            else runtimeError("VM: Operand must be a number for negation.");
            break;
        }
        case OP_POW: {
            Value b = pop(vm), a = pop(vm);
            double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
            double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
            vm.stack.push_back(std::pow(ad, bd));
            break;
        }
        case OP_MOD: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) % getVal<int>(b));
            else {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(std::fmod(ad, bd));
            }
            break;
        }
        case OP_LT: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) < getVal<int>(b));
            else {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad < bd);
            }
            break;
        }
        case OP_LE: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) <= getVal<int>(b));
            else {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad <= bd);
            }
            break;
        }
        case OP_GT: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) > getVal<int>(b));
            else {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad > bd);
            }
            break;
        }
        case OP_GE: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) >= getVal<int>(b));
            else {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad >= bd);
            }
            break;
        }
        case OP_NE: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) != getVal<int>(b));
            else if (holds<double>(a) || holds<double>(b)) {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad != bd);
            }
            else if (holds<bool>(a) && holds<bool>(b))
                vm.stack.push_back(getVal<bool>(a) != getVal<bool>(b));
            else if (holds<std::string>(a) && holds<std::string>(b))
                vm.stack.push_back(getVal<std::string>(a) != getVal<std::string>(b));
            else runtimeError("VM: Operands are not comparable for '<>'.");
            break;
        }
        case OP_EQ: {
            Value b = pop(vm), a = pop(vm);
            if (holds<int>(a) && holds<int>(b))
                vm.stack.push_back(getVal<int>(a) == getVal<int>(b));
            else if (holds<double>(a) || holds<double>(b)) {
                double ad = holds<double>(a) ? getVal<double>(a) : static_cast<double>(getVal<int>(a));
                double bd = holds<double>(b) ? getVal<double>(b) : static_cast<double>(getVal<int>(b));
                vm.stack.push_back(ad == bd);
            }
            else if (holds<bool>(a) && holds<bool>(b))
                vm.stack.push_back(getVal<bool>(a) == getVal<bool>(b));
            else if (holds<std::string>(a) && holds<std::string>(b))
                vm.stack.push_back(getVal<std::string>(a) == getVal<std::string>(b));
            else
                vm.stack.push_back(false);
            break;
        }
        case OP_AND: {
            Value b = pop(vm), a = pop(vm);
            bool ab = (holds<bool>(a)) ? getVal<bool>(a) : (holds<int>(a) ? (getVal<int>(a) != 0) : false);
            bool bb = (holds<bool>(b)) ? getVal<bool>(b) : (holds<int>(b) ? (getVal<int>(b) != 0) : false);
            vm.stack.push_back(ab && bb);
            break;
        }
        case OP_OR: {
            Value b = pop(vm), a = pop(vm);
            bool ab = (holds<bool>(a)) ? getVal<bool>(a) : (holds<int>(a) ? (getVal<int>(a) != 0) : false);
            bool bb = (holds<bool>(b)) ? getVal<bool>(b) : (holds<int>(b) ? (getVal<int>(b) != 0) : false);
            vm.stack.push_back(ab || bb);
            break;
        }
        case OP_PRINT: {
            Value v = pop(vm);
            std::cout << valueToString(v) << std::endl;
            break;
        }
        case OP_POP: {
            debugLog("OP_POP: Attempting to pop a value.");
            if (vm.stack.empty())
                runtimeError("VM: Stack underflow on POP.");
            vm.stack.pop_back();
            break;
        }
        case OP_DEFINE_GLOBAL: {
            int nameIndex = chunk.code[ip++];
            if (nameIndex < 0 || nameIndex >= (int)chunk.constants.size())
                runtimeError("VM: Invalid constant index for global name.");
            Value nameVal = chunk.constants[nameIndex];
            if (!holds<std::string>(nameVal))
                runtimeError("VM: Global name must be a string.");
            std::string name = getVal<std::string>(nameVal);
            if (vm.stack.empty())
                runtimeError("VM: Stack underflow on global definition for " + name);
            Value val = pop(vm);
            vm.environment->define(name, val);
            debugLog("VM: Defined global variable: " + name + " = " + valueToString(val));
            break;
        }
        case OP_GET_GLOBAL: {
            int nameIndex = chunk.code[ip++];
            if (nameIndex < 0 || nameIndex >= (int)chunk.constants.size())
                runtimeError("VM: Invalid constant index for global name.");
            Value nameVal = chunk.constants[nameIndex];
            if (!holds<std::string>(nameVal))
                runtimeError("VM: Global name must be a string.");
            std::string name = getVal<std::string>(nameVal);
            if (toLower(name) == "microseconds") {
                auto now = std::chrono::steady_clock::now();
                double us = std::chrono::duration<double, std::micro>(now - startTime).count();
                vm.stack.push_back(us);
                debugLog("VM: Loaded built-in microseconds: " + std::to_string(us));
            }
            else if (toLower(name) == "ticks") {
                auto now = std::chrono::steady_clock::now();
                double seconds = std::chrono::duration<double>(now - startTime).count();
                int ticks = static_cast<int>(seconds * 60);
                vm.stack.push_back(ticks);
                debugLog("VM: Loaded built-in ticks: " + std::to_string(ticks));
            }
            else {
                Value val = vm.environment->get(name);
                vm.stack.push_back(val);
                debugLog("VM: Loaded global variable: " + name + " = " + valueToString(val));
            }
            break;
        }
        case OP_SET_GLOBAL: {
            int nameIndex = chunk.code[ip++];
            if (nameIndex < 0 || nameIndex >= (int)chunk.constants.size())
                runtimeError("VM: Invalid constant index for global name.");
            Value nameVal = chunk.constants[nameIndex];
            if (!holds<std::string>(nameVal))
                runtimeError("VM: Global name must be a string.");
            std::string name = getVal<std::string>(nameVal);
            Value newVal = pop(vm);
            vm.environment->assign(name, newVal);
            debugLog("VM: Set global variable: " + name + " = " + valueToString(newVal));
            break;
        }
        case OP_NEW: {
            Value classVal = pop(vm);
            if (!holds<std::shared_ptr<ObjClass>>(classVal))
                runtimeError("VM: 'new' applied to non-class.");
            auto cls = getVal<std::shared_ptr<ObjClass>>(classVal);
            if (cls->isPlugin) {
                Value result = cls->pluginConstructor({});
                auto instance = std::make_shared<ObjInstance>();
                instance->klass = cls;
                instance->pluginInstance = getVal<void*>(result);
                vm.stack.push_back(Value(instance));
            }
            else {
                auto instance = std::make_shared<ObjInstance>();
                instance->klass = cls;
                for (auto& p : cls->properties) {
                    instance->fields[p.first] = p.second;
                }
                vm.stack.push_back(Value(instance));
            }
            break;
        }
        case OP_DUP: {
            if (vm.stack.empty())
                runtimeError("VM: Stack underflow on DUP.");
            vm.stack.push_back(vm.stack.back());
            break;
        }
        case OP_CALL: {
            int argCount = chunk.code[ip++];
            std::vector<Value> args;
            for (int i = 0; i < argCount; i++) {
                args.push_back(pop(vm));
            }
            std::reverse(args.begin(), args.end());
            Value callee = pop(vm);
            debugLog("VM: Calling function with " + std::to_string(argCount) + " arguments.");
            if (holds<BuiltinFn>(callee)) {
                auto fn = getVal<BuiltinFn>(callee);
                Value result = fn(args);
                vm.stack.push_back(result);
            }
            else if (holds<std::shared_ptr<ObjFunction>>(callee)) {
                auto function = getVal<std::shared_ptr<ObjFunction>>(callee);
                int total = function->params.size();
                int required = function->arity;
                if ((int)args.size() < required || (int)args.size() > total)
                    runtimeError("VM: Expected between " + std::to_string(required) + " and " + std::to_string(total) + " arguments for function " + function->name);
                for (int i = args.size(); i < total; i++) {
                    args.push_back(function->params[i].defaultValue);
                }
                auto previousEnv = vm.environment;
                vm.environment = std::make_shared<Environment>(previousEnv);
                for (size_t i = 0; i < function->params.size(); i++) {
                    vm.environment->define(function->params[i].name, args[i]);
                }
                Value result = runVM(vm, function->chunk);
                vm.environment = previousEnv;
                vm.stack.push_back(result);
                debugLog("VM: Function " + function->name + " returned " + valueToString(result));
            }
            else if (holds<std::vector<std::shared_ptr<ObjFunction>>>(callee)) {
                auto overloads = getVal<std::vector<std::shared_ptr<ObjFunction>>>(callee);
                std::shared_ptr<ObjFunction> chosen = nullptr;
                for (auto f : overloads) {
                    int total = f->params.size();
                    int required = f->arity;
                    if ((int)args.size() >= required && (int)args.size() <= total) {
                        chosen = f;
                        break;
                    }
                }
                if (!chosen)
                    runtimeError("VM: No matching overload found for function call with " + std::to_string(args.size()) + " arguments.");
                for (int i = args.size(); i < chosen->params.size(); i++) {
                    args.push_back(chosen->params[i].defaultValue);
                }
                auto previousEnv = vm.environment;
                vm.environment = std::make_shared<Environment>(previousEnv);
                for (size_t i = 0; i < chosen->params.size(); i++) {
                    vm.environment->define(chosen->params[i].name, args[i]);
                }
                Value result = runVM(vm, chosen->chunk);
                vm.environment = previousEnv;
                vm.stack.push_back(result);
                debugLog("VM: Function " + chosen->name + " returned " + valueToString(result));
            }
            else if (holds<std::shared_ptr<ObjBoundMethod>>(callee)) {
                auto bound = getVal<std::shared_ptr<ObjBoundMethod>>(callee);
                if (holds<std::shared_ptr<ObjInstance>>(bound->receiver)) {
                    auto instance = getVal<std::shared_ptr<ObjInstance>>(bound->receiver);
                    std::string key = toLower(bound->name);
                    Value methodVal = instance->klass->methods[key];
                    if (holds<BuiltinFn>(methodVal)) {
                        auto fn = getVal<BuiltinFn>(methodVal);
                        Value result = fn(args);
                        vm.stack.push_back(result);
                    }
                    else {
                        std::shared_ptr<ObjFunction> methodFn = nullptr;
                        if (holds<std::shared_ptr<ObjFunction>>(methodVal)) {
                            methodFn = getVal<std::shared_ptr<ObjFunction>>(methodVal);
                        }
                        else if (holds<std::vector<std::shared_ptr<ObjFunction>>>(methodVal)) {
                            auto overloads = getVal<std::vector<std::shared_ptr<ObjFunction>>>(methodVal);
                            for (auto f : overloads) {
                                int total = f->params.size();
                                int required = f->arity;
                                if ((int)args.size() >= required && (int)args.size() <= total) {
                                    methodFn = f;
                                    break;
                                }
                            }
                        }
                        if (!methodFn)
                            runtimeError("VM: No matching method found for " + bound->name);
                        auto previousEnv = vm.environment;
                        vm.environment = std::make_shared<Environment>(previousEnv);
                        vm.environment->define("self", bound->receiver);
                        for (size_t i = 0; i < methodFn->params.size(); i++) {
                            if (i < args.size())
                                vm.environment->define(methodFn->params[i].name, args[i]);
                            else
                                vm.environment->define(methodFn->params[i].name, methodFn->params[i].defaultValue);
                        }
                        Value result = runVM(vm, methodFn->chunk);
                        vm.environment = previousEnv;
                        vm.stack.push_back(result);
                        debugLog("VM: Function " + methodFn->name + " returned " + valueToString(result));
                    }
                }
                else if (holds<std::shared_ptr<ObjArray>>(bound->receiver)) {
                    auto array = getVal<std::shared_ptr<ObjArray>>(bound->receiver);
                    Value result = callArrayMethod(array, bound->name, args);
                    vm.stack.push_back(result);
                }
                else {
                    runtimeError("VM: Bound method receiver is of unsupported type.");
                }
            }
            else if (holds<std::shared_ptr<ObjArray>>(callee)) {
                auto array = getVal<std::shared_ptr<ObjArray>>(callee);
                if (argCount != 1)
                    runtimeError("VM: Array call expects exactly 1 argument for indexing.");
                Value indexVal = args[0];
                int index;
                if (holds<int>(indexVal))
                    index = getVal<int>(indexVal);
                else
                    runtimeError("VM: Array index must be an integer.");
                if (index < 0 || index >= (int)array->elements.size())
                    runtimeError("VM: Array index out of bounds.");
                vm.stack.push_back(array->elements[index]);
            }
            else if (holds<std::string>(callee)) {
                std::string funcName = toLower(getVal<std::string>(callee));
                if (funcName == "print") {
                    if (args.size() < 1) runtimeError("VM: print expects an argument.");
                    std::cout << valueToString(args[0]) << std::endl;
                    vm.stack.push_back(args[0]);
                }
                else if (funcName == "str") {
                    if (args.size() < 1) runtimeError("VM: str expects an argument.");
                    vm.stack.push_back(Value(valueToString(args[0])));
                }
                else if (funcName == "ticks") {
                    auto now = std::chrono::steady_clock::now();
                    double seconds = std::chrono::duration<double>(now - startTime).count();
                    int ticks = static_cast<int>(seconds * 60);
                    vm.stack.push_back(ticks);
                }
                else if (funcName == "microseconds") {
                    auto now = std::chrono::steady_clock::now();
                    double us = std::chrono::duration<double, std::micro>(now - startTime).count();
                    vm.stack.push_back(us);
                }
                else if (funcName == "val") {
                    if (args.size() != 1)
                        runtimeError("VM: val expects exactly one argument.");
                    if (!holds<std::string>(args[0]))
                        runtimeError("VM: val expects a string argument.");
                    double d = std::stod(getVal<std::string>(args[0]));
                    vm.stack.push_back(d);
                }
                else {
                    runtimeError("VM: Unknown built-in function: " + funcName);
                }
            }
            else {
                runtimeError("VM: Can only call functions, methods, arrays, or built-in functions.");
            }
            break;
        }
        case OP_OPTIONAL_CALL: {
            int argCount = chunk.code[ip++];
            std::vector<Value> args;
            for (int i = 0; i < argCount; i++) {
                args.push_back(pop(vm));
            }
            std::reverse(args.begin(), args.end());
            Value callee = pop(vm);
            debugLog("OP_OPTIONAL_CALL: callee type: " + getTypeName(callee));
            if (holds<std::monostate>(callee)) {
                debugLog("OP_OPTIONAL_CALL: No constructor found; skipping call.");
            }
            else if (holds<std::shared_ptr<ObjFunction>>(callee)) {
                auto function = getVal<std::shared_ptr<ObjFunction>>(callee);
                int total = function->params.size();
                int required = function->arity;
                if ((int)args.size() < required || (int)args.size() > total)
                    runtimeError("VM: Expected between " + std::to_string(required) + " and " + std::to_string(total) + " arguments for constructor " + function->name);
                for (int i = args.size(); i < total; i++) {
                    args.push_back(function->params[i].defaultValue);
                }
                auto previousEnv = vm.environment;
                vm.environment = std::make_shared<Environment>(previousEnv);
                for (size_t i = 0; i < function->params.size(); i++) {
                    vm.environment->define(function->params[i].name, args[i]);
                }
                Value result = runVM(vm, function->chunk);
                vm.environment = previousEnv;
                debugLog("OP_OPTIONAL_CALL: Constructor function " + function->name + " returned " + valueToString(result));
                if (!holds<std::monostate>(result))
                    vm.stack.push_back(result);
            }
            else {
                runtimeError("OP_OPTIONAL_CALL: Can only call functions or nil.");
            }
            break;
        }
        case OP_RETURN: {
            Value ret = vm.stack.empty() ? Value(std::monostate{}) : pop(vm);
            return ret;
        }
        case OP_NIL: {
            vm.stack.push_back(Value(std::monostate{}));
            break;
        }
        case OP_JUMP_IF_FALSE: {
            int offset = chunk.code[ip++];
            Value condition = pop(vm);
            bool condTruth = false;
            if (holds<bool>(condition))
                condTruth = getVal<bool>(condition);
            else if (holds<int>(condition))
                condTruth = (getVal<int>(condition) != 0);
            else if (holds<std::string>(condition))
                condTruth = !getVal<std::string>(condition).empty();
            else if (std::holds_alternative<std::monostate>(condition))
                condTruth = false;
            if (!condTruth) {
                ip = offset;
            }
            break;
        }
        case OP_JUMP: {
            int offset = chunk.code[ip++];
            ip = offset;
            break;
        }
        case OP_CLASS: {
            int nameIndex = chunk.code[ip++];
            Value nameVal = chunk.constants[nameIndex];
            if (!holds<std::string>(nameVal))
                runtimeError("VM: Class name must be a string.");
            auto klass = std::make_shared<ObjClass>();
            klass->name = getVal<std::string>(nameVal);
            vm.stack.push_back(Value(klass));
            break;
        }
        case OP_METHOD: {
            int methodNameIndex = chunk.code[ip++];
            Value methodNameVal = chunk.constants[methodNameIndex];
            if (!holds<std::string>(methodNameVal))
                runtimeError("VM: Method name must be a string.");
            Value methodVal = pop(vm);
            if (!holds<std::shared_ptr<ObjFunction>>(methodVal))
                runtimeError("VM: Method must be a function.");
            Value classVal = pop(vm);
            if (!holds<std::shared_ptr<ObjClass>>(classVal))
                runtimeError("VM: No class found for method.");
            auto klass = getVal<std::shared_ptr<ObjClass>>(classVal);
            std::string methodName = toLower(getVal<std::string>(methodNameVal));
            if (klass->methods.find(methodName) != klass->methods.end()) {
                // Overload handling omitted.
            }
            else {
                klass->methods[methodName] = methodVal;
            }
            vm.stack.push_back(Value(klass));
            break;
        }
        case OP_PROPERTIES: {
            int propIndex = chunk.code[ip++];
            Value propVal = chunk.constants[propIndex];
            if (!holds<PropertiesType>(propVal))
                runtimeError("VM: Properties must be a property map.");
            auto props = getVal<PropertiesType>(propVal);
            Value classVal = pop(vm);
            if (!holds<std::shared_ptr<ObjClass>>(classVal))
                runtimeError("VM: Properties can only be set on a class object.");
            auto klass = getVal<std::shared_ptr<ObjClass>>(classVal);
            klass->properties = props;
            vm.stack.push_back(Value(klass));
            break;
        }
        case OP_ARRAY: {
            int count = chunk.code[ip++];
            std::vector<Value> elems;
            for (int i = 0; i < count; i++) {
                elems.push_back(pop(vm));
            }
            std::reverse(elems.begin(), elems.end());
            auto array = std::make_shared<ObjArray>();
            array->elements = elems;
            vm.stack.push_back(Value(array));
            debugLog("VM: Created array with " + std::to_string(count) + " elements.");
            break;
        }
        case OP_GET_PROPERTY: {
            int nameIndex = chunk.code[ip++];
            Value propNameVal = chunk.constants[nameIndex];
            if (!holds<std::string>(propNameVal))
                runtimeError("VM: Property name must be a string.");
            std::string propName = toLower(getVal<std::string>(propNameVal));
            Value object = pop(vm);
            if (holds<std::shared_ptr<ObjInstance>>(object)) {
                auto instance = getVal<std::shared_ptr<ObjInstance>>(object);
                std::string key = toLower(propName);
                if (instance->klass->isPlugin) {
                    auto it = instance->klass->pluginProperties.find(key);
                    if (it != instance->klass->pluginProperties.end()) {
                        BuiltinFn getter = it->second.first;
                        debugLog("OP_GET_PROPERTY: Calling plugin getter for property '" + key + "'");
                        Value result = getter({ Value(instance->pluginInstance) });
                        debugLog("OP_GET_PROPERTY: Plugin getter returned type: " + getTypeName(result) +
                                 " value: " + valueToString(result));
                        vm.stack.push_back(result);
                    }
                    else {
                        if (instance->fields.find(key) != instance->fields.end())
                            vm.stack.push_back(instance->fields[key]);
                        else if (instance->klass && instance->klass->methods.find(key) != instance->klass->methods.end()) {
                            auto bound = std::make_shared<ObjBoundMethod>();
                            bound->receiver = object;
                            bound->name = key;
                            vm.stack.push_back(Value(bound));
                        }
                        else {
                            runtimeError("VM: Undefined property: " + propName);
                        }
                    }
                }
                else {
                    if (instance->fields.find(key) != instance->fields.end()) {
                        vm.stack.push_back(instance->fields[key]);
                    }
                    else if (instance->klass && instance->klass->methods.find(key) != instance->klass->methods.end()) {
                        auto bound = std::make_shared<ObjBoundMethod>();
                        bound->receiver = object;
                        bound->name = key;
                        vm.stack.push_back(Value(bound));
                    }
                    else if (key == "tostring") {
                        vm.stack.push_back(valueToString(object));
                    }
                    else {
                        if (key == "constructor") {
                            vm.stack.push_back(Value(std::monostate{}));
                        }
                        else {
                            runtimeError("VM: Undefined property: " + propName);
                        }
                    }
                }
            }
            else if (holds<std::shared_ptr<ObjArray>>(object)) {
                auto array = getVal<std::shared_ptr<ObjArray>>(object);
                auto bound = std::make_shared<ObjBoundMethod>();
                bound->receiver = object;
                bound->name = propName;
                vm.stack.push_back(Value(bound));
            }
            else if (holds<int>(object)) {
                if (propName == "tostring") {
                    vm.stack.push_back(valueToString(object));
                }
                else {
                    runtimeError("VM: Unknown property for integer: " + propName);
                }
            }
            else if (holds<double>(object)) {
                if (propName == "tostring") {
                    vm.stack.push_back(valueToString(object));
                }
                else {
                    runtimeError("VM: Unknown property for double: " + propName);
                }
            }
            else if (holds<std::string>(object)) {
                std::string s = getVal<std::string>(object);
                if (propName == "tostring") {
                    vm.stack.push_back(s);
                }
                else {
                    runtimeError("VM: Unknown property for string: " + propName);
                }
            }
            else if (holds<std::shared_ptr<ObjModule>>(object)) {
                auto module = getVal<std::shared_ptr<ObjModule>>(object);
                std::string key = toLower(propName);
                if (module->publicMembers.find(key) != module->publicMembers.end())
                    vm.stack.push_back(module->publicMembers[key]);
                else
                    runtimeError("VM: Undefined module property: " + propName);
            }
            else if (holds<std::shared_ptr<ObjEnum>>(object)) {
                auto en = getVal<std::shared_ptr<ObjEnum>>(object);
                std::string key = toLower(propName);
                if (en->members.find(key) != en->members.end()) {
                    vm.stack.push_back(en->members[key]);
                }
                else {
                    runtimeError("VM: Undefined enum member: " + propName);
                }
            }
            else {
                runtimeError("VM: Property access on unsupported type.");
            }
            break;
        }
        case OP_SET_PROPERTY: {
            int propNameIndex = chunk.code[ip++];
            Value propNameVal = chunk.constants[propNameIndex];
            if (!holds<std::string>(propNameVal))
                runtimeError("VM: Property name must be a string.");
            std::string propName = toLower(getVal<std::string>(propNameVal));
            Value value = pop(vm);
            Value object = pop(vm);
            debugLog("OP_SET_PROPERTY: About to set property '" + propName + "'.");
            debugLog("OP_SET_PROPERTY: Value = " + valueToString(value));
            debugLog("OP_SET_PROPERTY: Object type = " + getTypeName(object) + " (" + valueToString(object) + ")");
            if (holds<std::shared_ptr<ObjInstance>>(object)) {
                auto instance = getVal<std::shared_ptr<ObjInstance>>(object);
                if (instance->klass->isPlugin) {
                    auto it = instance->klass->pluginProperties.find(propName);
                    if (it != instance->klass->pluginProperties.end()) {
                        BuiltinFn setter = it->second.second;
                        setter({ Value(instance->pluginInstance), value });
                        vm.stack.push_back(object);
                    }
                    else {
                        instance->fields[propName] = value;
                        vm.stack.push_back(object);
                    }
                }
                else {
                    instance->fields[propName] = value;
                    vm.stack.push_back(object);
                }
            }
            else {
                runtimeError("VM: Can only set properties on instances. Instead got type: " + getTypeName(object));
            }
            break;
        }
        case OP_CONSTRUCTOR_END: {
            if (vm.stack.size() < 2)
                runtimeError("VM: Not enough values for constructor end.");
            Value constructorResult = pop(vm);
            Value instance = pop(vm);
            if (holds<std::monostate>(constructorResult))
                vm.stack.push_back(instance);
            else
                vm.stack.push_back(constructorResult);
            break;
        }
        default:
            break;
        }
        {
            std::string s = "[";
            for (auto& v : vm.stack)
                s += valueToString(v) + ", ";
            s += "]";
            debugLog("VM: Stack after execution: " + s);
        }
    }
    return Value(std::monostate{});
}

const char MARKER[9] = "XOJOCODE"; // 8 characters + null terminator = 9

std::string retrieveData(const std::string& exePath) {
    std::ifstream exeFile(exePath, std::ios::binary);
    if (!exeFile) {
        debugLog("Error: Cannot load bytecode.\n");
        return "";
    }
    
    // Determine file size
    exeFile.seekg(0, std::ios::end);
    std::streampos fileSize = exeFile.tellg();
    if (fileSize < 12) { // at least marker (8 bytes) + length (4 bytes)
        debugLog("No bytecode data found.\n");
        return "";
    }
    
    // Read the last 12 bytes: marker (8) and text length (4)
    exeFile.seekg(-12, std::ios::end);
    char markerBuffer[9] = {0};
    exeFile.read(markerBuffer, 8);
    uint32_t textLength;
    exeFile.read(reinterpret_cast<char*>(&textLength), sizeof(textLength));
    
    // Verify marker
    if (std::strncmp(markerBuffer, MARKER, 8) != 0) {
        debugLog("Bytecode not found.\n");
        return "";
    }
    
    // Ensure file contains enough data for the embedded text
    if (fileSize < static_cast<std::streamoff>(12 + textLength)) {
        debugLog("Invalid bytecode data length.\n");
        return "";
    }
    
    // Calculate position of text data
    std::streampos textPos = fileSize - static_cast<std::streamoff>(12) - static_cast<std::streamoff>(textLength);
    exeFile.seekg(textPos, std::ios::beg);
    std::vector<char> textData(textLength);
    exeFile.read(textData.data(), textLength);
    
    return std::string(textData.begin(), textData.end());
}
// ============================================================================  
// Main
// ============================================================================
int main(int argc, char* argv[]) {
    #ifdef _WIN32
        SetDllDirectory("libs");
    #endif
    startTime = std::chrono::steady_clock::now();

    std::string filename = "default.xs";

	// Iterate through arguments, skipping argv[0] (program name)
    for (int i = 1; i < argc - 1; i++) {
        std::string arg = argv[i];
        if (arg == "--s" && (i + 1 < argc)) {
            filename = argv[i + 1];
        }
        else if (arg == "--d" && (i + 1 < argc)) {
            std::string debugArg = argv[i + 1];
            
            // Convert to lowercase for case-insensitive comparison
            std::transform(debugArg.begin(), debugArg.end(), debugArg.begin(), ::tolower);
            if (debugArg == "true") {
                DEBUG_MODE = true; 
            } else if (debugArg == "false") {
                DEBUG_MODE = false; 
            } else {
                std::cerr << "Error: Argument for --d must be 'true' or 'false'." << std::endl;
                return 1;
            }
        }
    }

    debugLog(std::string("DEBUG_MODE: ") + (DEBUG_MODE ? "ON" : "OFF"));
////////////////////////Drop-in//////////////////////////////////

    VM vm;
    vm.globals = std::make_shared<Environment>(nullptr);
    vm.environment = vm.globals;

/////////////////INITIALIZE THE VM ENVIRONMENT///////////////////////
    // Define built-in constants.
    vm.environment->define("pi", Value(3.141592653589793));



    // Define built-in functions.
    vm.environment->define("print", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() < 1) runtimeError("print expects an argument.");
        std::cout << valueToString(args[0]) << std::endl;
        return args[0];
    }));
    vm.environment->define("str", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() < 1) runtimeError("str expects an argument.");
        return Value(valueToString(args[0]));
    }));
    vm.environment->define("microseconds", std::string("microseconds"));
    vm.environment->define("ticks", std::string("ticks"));
    vm.environment->define("val", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("val expects exactly one argument.");
        if (!holds<std::string>(args[0]))
            runtimeError("val expects a string argument.");
        double d = std::stod(getVal<std::string>(args[0]));
        return d;
    }));
    vm.environment->define("split", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 2)
            runtimeError("split expects exactly two arguments: text and delimiter.");
        if (!holds<std::string>(args[0]) || !holds<std::string>(args[1]))
            runtimeError("split expects both arguments to be strings.");
        std::string text = getVal<std::string>(args[0]);
        std::string delimiter = getVal<std::string>(args[1]);
        auto arr = std::make_shared<ObjArray>();
        if (delimiter.empty()) {
            for (char c : text) {
                arr->elements.push_back(std::string(1, c));
            }
        } else {
            size_t start = 0;
            size_t pos = text.find(delimiter, start);
            while (pos != std::string::npos) {
                arr->elements.push_back(text.substr(start, pos - start));
                start = pos + delimiter.length();
                pos = text.find(delimiter, start);
            }
            arr->elements.push_back(text.substr(start));
        }
        return Value(arr);
    }));
    vm.environment->define("array", BuiltinFn([](const std::vector<Value>& args) -> Value {
        auto arr = std::make_shared<ObjArray>();
        arr->elements = args;
        return Value(arr);
    }));
    vm.environment->define("abs", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Abs expects exactly one argument.");
        if (holds<int>(args[0]))
            return std::abs(getVal<int>(args[0]));
        else if (holds<double>(args[0]))
            return std::fabs(getVal<double>(args[0]));
        else
            runtimeError("Abs expects a number.");
        return Value(std::monostate{});
    }));
    vm.environment->define("acos", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Acos expects exactly one argument.");
        double x = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Acos expects a number."), 0.0));
        return std::acos(x);
    }));
    vm.environment->define("asc", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Asc expects exactly one argument.");
        if (!holds<std::string>(args[0]))
            runtimeError("Asc expects a string.");
        std::string s = getVal<std::string>(args[0]);
        if (s.empty()) runtimeError("Asc expects a non-empty string.");
        return (int)s[0];
    }));
    vm.environment->define("asin", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Asin expects exactly one argument.");
        double x = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Asin expects a number."), 0.0));
        return std::asin(x);
    }));
    vm.environment->define("atan", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Atan expects exactly one argument.");
        double x = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Atan expects a number."), 0.0));
        return std::atan(x);
    }));
    vm.environment->define("atan2", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) runtimeError("Atan2 expects exactly two arguments.");
        double y = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Atan2 expects numbers."), 0.0));
        double x = holds<int>(args[1]) ? getVal<int>(args[1]) : (holds<double>(args[1]) ? getVal<double>(args[1]) : (runtimeError("Atan2 expects numbers."), 0.0));
        return std::atan2(y, x);
    }));
    vm.environment->define("ceiling", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Ceiling expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Ceiling expects a number."), 0.0));
        return std::ceil(v);
    }));
    vm.environment->define("cos", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Cos expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Cos expects a number."), 0.0));
        return std::cos(v);
    }));
    vm.environment->define("exp", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Exp expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Exp expects a number."), 0.0));
        return std::exp(v);
    }));
    vm.environment->define("floor", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Floor expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Floor expects a number."), 0.0));
        return std::floor(v);
    }));
    vm.environment->define("log", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Log expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : (holds<double>(args[0]) ? getVal<double>(args[0]) : (runtimeError("Log expects a number."), 0.0));
        return std::log(v);
    }));
    vm.environment->define("max", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) runtimeError("Max expects exactly two arguments.");
        if (holds<int>(args[0]) && holds<int>(args[1])) {
            int a = getVal<int>(args[0]), b = getVal<int>(args[1]);
            return a > b ? a : b;
        }
        else {
            double a = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
            double b = holds<int>(args[1]) ? getVal<int>(args[1]) : getVal<double>(args[1]);
            return a > b ? a : b;
        }
    }));
    vm.environment->define("min", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) runtimeError("Min expects exactly two arguments.");
        if (holds<int>(args[0]) && holds<int>(args[1])) {
            int a = getVal<int>(args[0]), b = getVal<int>(args[1]);
            return a < b ? a : b;
        }
        else {
            double a = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
            double b = holds<int>(args[1]) ? getVal<int>(args[1]) : getVal<double>(args[1]);
            return a < b ? a : b;
        }
    }));
    vm.environment->define("oct", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Oct expects exactly one argument.");
        int n = 0;
        if (holds<int>(args[0])) n = getVal<int>(args[0]);
        else if (holds<double>(args[0])) n = static_cast<int>(getVal<double>(args[0]));
        else runtimeError("Oct expects a number.");
        std::stringstream ss;
        ss << std::oct << n;
        return ss.str();
    }));
    vm.environment->define("pow", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) runtimeError("Pow expects exactly two arguments.");
        double a = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
        double b = holds<int>(args[1]) ? getVal<int>(args[1]) : getVal<double>(args[1]);
        return std::pow(a, b);
    }));
    vm.environment->define("round", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Round expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
        return std::round(v);
    }));
    vm.environment->define("sign", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Sign expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
        if (v < 0) return -1;
        else if (v == 0) return 0;
        else return 1;
    }));
    vm.environment->define("sin", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Sin expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
        return std::sin(v);
    }));
    vm.environment->define("sqrt", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Sqrt expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
        return std::sqrt(v);
    }));
    vm.environment->define("tan", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) runtimeError("Tan expects exactly one argument.");
        double v = holds<int>(args[0]) ? getVal<int>(args[0]) : getVal<double>(args[0]);
        return std::tan(v);
    }));
    vm.environment->define("rnd", BuiltinFn([](const std::vector<Value>& args) -> Value {
        if (args.size() != 0) runtimeError("Rnd expects no arguments.");
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(global_rng);
    }));
    {
        auto randomClass = std::make_shared<ObjClass>();
        randomClass->name = "random";
        randomClass->methods["inrange"] = BuiltinFn([](const std::vector<Value>& args) -> Value {
            if (args.size() != 2) runtimeError("Random.InRange expects exactly two arguments.");
            int minVal = 0, maxVal = 0;
            if (holds<int>(args[0]))
                minVal = getVal<int>(args[0]);
            else if (holds<double>(args[0]))
                minVal = static_cast<int>(getVal<double>(args[0]));
            else
                runtimeError("Random.InRange expects a number as first argument.");
            if (holds<int>(args[1]))
                maxVal = getVal<int>(args[1]);
            else if (holds<double>(args[1]))
                maxVal = static_cast<int>(getVal<double>(args[1]));
            else
                runtimeError("Random.InRange expects a number as second argument.");
            if (minVal > maxVal) runtimeError("Random.InRange: min is greater than max.");
            std::uniform_int_distribution<int> dist(minVal, maxVal);
            return dist(global_rng);
        });
        vm.environment->define("random", randomClass);
    }

    //Load Plugin functions, classes, and modules into the VM environment.
    loadPlugins(vm);

	
////////////////////////////////////////////////
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open " << filename << std::endl;
        return EXIT_FAILURE;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    //std::string source = preprocessSource(buffer.str());
    
    std::string exePath = argv[0]; // path to the current executable
    std::string retrieved = retrieveData(exePath);
    std::string source;

    if (!retrieved.empty()) {
        std::cout << "Retrieved Bytecode:\n" << retrieved << "\n";
        source = preprocessSource(retrieved);
    } else {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Unable to open " << filename << std::endl;
            return EXIT_FAILURE;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source = preprocessSource(buffer.str());
}

    debugLog("Starting lexing...");
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    debugLog("Lexing complete. Tokens count: " + std::to_string(tokens.size()));

    debugLog("Starting parsing...");
    Parser parser(tokens);
    std::vector<std::shared_ptr<Stmt>> statements = parser.parse();
    debugLog("Parsing complete. Statements count: " + std::to_string(statements.size()));
///////////////////////////////////////

    //Compile the Xojoscript program.
    debugLog("Starting compilation...");
    Compiler compiler(vm);
    compiler.compile(statements);
    debugLog("Compilation complete. Main chunk instructions count: " + std::to_string(vm.mainChunk.code.size()));

    if (vm.environment->values.find("main") != vm.environment->values.end() &&
        (holds<std::shared_ptr<ObjFunction>>(vm.environment->get("main")) ||
         holds<std::vector<std::shared_ptr<ObjFunction>>>(vm.environment->get("main")))) {
        Value mainVal = vm.environment->get("main");
        if (holds<std::shared_ptr<ObjFunction>>(mainVal)) {
            auto mainFunction = getVal<std::shared_ptr<ObjFunction>>(mainVal);
            debugLog("Calling main function...");
            //Run the compiled bytecode
            runVM(vm, mainFunction->chunk);
        }
        else if (holds<std::vector<std::shared_ptr<ObjFunction>>>(mainVal)) {
            auto overloads = getVal<std::vector<std::shared_ptr<ObjFunction>>>(mainVal);
            std::shared_ptr<ObjFunction> mainFunction = nullptr;
            for (auto f : overloads) {
                if (f->arity == 0) { mainFunction = f; break; }
            }
            if (!mainFunction)
                runtimeError("No main function with 0 parameters found.");
            debugLog("Calling main function...");
            runVM(vm, mainFunction->chunk);
        }
    }
    else {
        debugLog("No main function found. Executing top-level code...");
        runVM(vm, vm.mainChunk);
    }

    debugLog("Program execution finished.");
    return 0;
}



//////////////////////////////////////////
/////////////  Happy Coding!  ////////////
//////////////////////////////////////////
