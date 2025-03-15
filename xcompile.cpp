// ============================================================================  
// XojoScript Bytecode Compiler and Virtual Machine Standalone Compiler 
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
// Build: g++ -static -static-libgcc -static-libstdc++ -O3 -o xcompile.exe xcompile.cpp

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdlib>

const char MARKER[9] = "XOJOCODE"; // 8 characters + null terminator

// Copy file from sourcePath to destPath.
bool copyFile(const std::string& sourcePath, const std::string& destPath) {
    std::ifstream src(sourcePath, std::ios::binary);
    if (!src) {
        std::cerr << "Error: Unable to open source file " << sourcePath << "\n";
        return false;
    }
    std::ofstream dst(destPath, std::ios::binary);
    if (!dst) {
        std::cerr << "Error: Unable to create destination file " << destPath << "\n";
        return false;
    }
    dst << src.rdbuf();
    return true;
}

// Checks if the executable already has embedded data.
bool hasEmbeddedData(const std::string& exePath) {
    std::ifstream file(exePath, std::ios::binary);
    if (!file)
        return false; // Could not open; assume false.
    
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (fileSize < 12) // Not enough room for marker and length.
        return false;
    
    file.seekg(-12, std::ios::end);
    char markerBuffer[9] = {0};
    file.read(markerBuffer, 8);
    return (std::strncmp(markerBuffer, MARKER, 8) == 0);
}

// Injects text file contents into the executable by appending:
// [text data][MARKER (8 bytes)][4-byte text length]
void injectData(const std::string& exePath, const std::string& textFilePath) {
    // Read text file into a vector.
    std::ifstream textFile(textFilePath, std::ios::binary);
    if (!textFile) {
        std::cerr << "Error: Cannot open source file " << textFilePath << ".\n";
        return;
    }
    std::vector<char> textData((std::istreambuf_iterator<char>(textFile)),
                               std::istreambuf_iterator<char>());
    textFile.close();

    uint32_t textLength = static_cast<uint32_t>(textData.size());

    // Open the executable for reading and writing in binary mode.
    std::fstream exeFile(exePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!exeFile) {
        std::cerr << "Error: Cannot open executable path " << exePath << " for writing.\n";
        return;
    }
    
    // Append the text data, marker (8 bytes), and text length (4 bytes) at the end.
    exeFile.seekp(0, std::ios::end);
    exeFile.write(textData.data(), textLength);
    exeFile.write(MARKER, 8); // write marker (8 bytes)
    exeFile.write(reinterpret_cast<const char*>(&textLength), sizeof(textLength)); // write 4-byte length
    
    exeFile.close();
    std::cout << "Compilation complete: Wrote " << textLength << " bytes of bytecode to " << exePath << ".\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <target_executable> <text_file>\n";
        return EXIT_FAILURE;
    }
    
    std::string targetExe = argv[1];
    std::string textFilePath = argv[2];
    
    // Prevent the user from supplying the base executable name.
    if (targetExe == "xojoscript" || targetExe == "xojoscript.exe") {
        std::cerr << "Error: Cannot use base executable name as target.\n";
        return EXIT_FAILURE;
    }
    
    // Determine the directory of the current executable.
    std::string currentPath = argv[0];
    size_t pos = currentPath.find_last_of("\\/");
    std::string baseDir = (pos != std::string::npos) ? currentPath.substr(0, pos + 1) : "";
    std::string baseExe = baseDir + "xojoscript.exe"; // Base executable located beside this program.
    
    // Copy the base executable to the user-defined target filename.
    if (!copyFile(baseExe, targetExe)) {
        std::cerr << "Error: Could not copy base executable from " << baseExe << " to " << targetExe << ".\n";
        return EXIT_FAILURE;
    }
    
    // Check if the target executable already has embedded code.
    if (hasEmbeddedData(targetExe)) {
        std::cerr << "Error: The executable already contains embedded code and cannot be overwritten.\n";
        return EXIT_FAILURE;
    }
    
    // Inject the text file's content into the target executable.
    injectData(targetExe, textFilePath);
    
    return 0;
}
