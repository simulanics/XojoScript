# /simulanics/XojoScript

An open-source XojoScript compiler and virtual machine for the Xojo language. Compile and Run XojoScript at native C++ speed, on any system supporting C++ (99.9% of devices - even the web! 🙏) 

# XojoScript Bytecode Compiler and Virtual Machine 🚀

Welcome to the **XojoScript Bytecode Compiler and Virtual Machine**! This project is a lightweight bytecode compiler and virtual machine written in C++ that handles XojoScript execution—including functions, classes, and more - entirely cross- platform. It compiles XojoScript into bytecode and executes it on a custom virtual machine. 🤯

## Features ✨

- **Function Support:** Compile and execute user-defined functions and built-in ones. Overloading of functions is permitted.
- **Module Support:** Create XojoScript Modules. (● "extends" is in the works).
- **Class & Instance Support:** Create classes, define methods, and instantiate objects.
- **Intrinsic Types:** Handles types like Color, Integer, Double, Boolean, Variant and String.
- **Bytecode Execution:** Runs compiled bytecode on a custom VM.
- **Debug Logging:** Step-by-step debug logs to trace lexing, parsing, compiling, and execution.
- **100%:** Matches Xojo language syntax
- **Open Source:** Released under the MIT License.

## Getting Started 🏁

### Prerequisites

- A C++ compiler with C++17 support (e.g. g++, clang++)
- Git

### Installation

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/yourusername/XojoScript
   cd XojoScript
   ```
2. Compile the Project:

For example, using g++:

```
g++ -std=c++17 -o xojoscript xojoscript.cpp -lffi
```

With speed optimizations:

```
g++ -o xojoscript xojoscript.cpp -lffi -O3 -march=native -mtune=native -flto
```

3. Prepare Your Script:

Create a file named test.txt with your XojoScript code. For example:

```
Function addtwonumbers(num1 As Integer, num2 As Integer) As Integer
    Return num1 + num2
End Function

Public Sub Main()
    Dim result As Integer = addtwonumbers(2, 3)
    Print(str(result))
End Sub
```

4. Run the Compiler on a script:

```
./xojoscript --s filename
(*currently compiles test.txt file and immediately runs it in the VM if no source file is specified.)
```

Debugging 🔍

Debug logging is enabled via the DEBUG_MODE flag. Set it to true or false at the top of the code:

```
bool DEBUG_MODE = true;
```

This will output detailed logs for lexing, parsing, compiling, and execution.

Contributing 🤝

Contributions are welcome! Please feel free to open issues or submit pull requests. Your help is appreciated! 🎉

License 📄

This project is licensed under the MIT License. See the LICENSE file for details.

**This is currently a working proof-of-concept to be expanded upon.
---

Happy coding! 😄



