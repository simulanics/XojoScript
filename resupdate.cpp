//g++ resmod.cpp -o resmod.exe -static -static-libgcc -static-libstdc++ -O2 -s

#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>

// Helper: Convert std::wstring to vector<BYTE> (including terminating null)
std::vector<BYTE> WStringToBytes(const std::wstring &s) {
    std::vector<BYTE> v((s.size() + 1) * sizeof(wchar_t));
    memcpy(v.data(), s.c_str(), (s.size() + 1) * sizeof(wchar_t));
    return v;
}

// Helper: pad a buffer so its size is a multiple of 4 bytes
void AlignToDword(std::vector<BYTE>& buf) {
    while (buf.size() % 4 != 0) {
        buf.push_back(0);
    }
}

// Build a version-resource “block” (a header plus its value and children)
// Each block begins with three WORDs (total length, value length, type),
// followed by a null-terminated Unicode key (padded to DWORD boundary),
// then the binary value (if any), and then any child blocks.
std::vector<BYTE> BuildBlock(const std::wstring &key, WORD wType, const std::vector<BYTE>& value, const std::vector<std::vector<BYTE>>& children) {
    std::vector<BYTE> block;
    // Reserve space for header: 3 WORDs = 6 bytes
    size_t headerPos = block.size();
    block.resize(block.size() + 3 * sizeof(WORD));

    // Append key as Unicode (including null terminator)
    std::vector<BYTE> keyBytes = WStringToBytes(key);
    block.insert(block.end(), keyBytes.begin(), keyBytes.end());
    // Align to DWORD boundary
    AlignToDword(block);

    // Append value data (for text blocks, value should be in wide characters with null terminator)
    block.insert(block.end(), value.begin(), value.end());

    // Append any child blocks
    for (const auto &child : children) {
        block.insert(block.end(), child.begin(), child.end());
    }

    // Total length of this block in bytes
    WORD wLength = (WORD)block.size();
    // For text strings, the value length is the number of characters (excluding the null)
    WORD wValueLength = (WORD)value.size();
    if (wType == 1 && !value.empty()) {
        wValueLength = (WORD)((value.size() / sizeof(wchar_t)) - 1);
    }

    // Write the header (first 6 bytes) into the block
    memcpy(&block[0], &wLength, sizeof(WORD));
    memcpy(&block[sizeof(WORD)], &wValueLength, sizeof(WORD));
    memcpy(&block[2 * sizeof(WORD)], &wType, sizeof(WORD));

    return block;
}

#pragma pack(push, 1)
struct MyVS_FIXEDFILEINFO {
    DWORD dwSignature;
    DWORD dwStrucVersion;
    DWORD dwFileVersionMS;
    DWORD dwFileVersionLS;
    DWORD dwProductVersionMS;
    DWORD dwProductVersionLS;
    DWORD dwFileFlagsMask;
    DWORD dwFileFlags;
    DWORD dwFileOS;
    DWORD dwFileType;
    DWORD dwFileSubtype;
    DWORD dwFileDateMS;
    DWORD dwFileDateLS;
};
#pragma pack(pop)

// Build the binary fixed file info structure from version numbers
std::vector<BYTE> BuildFixedFileInfo(const DWORD fileVersion[4], const DWORD productVersion[4]) {
    MyVS_FIXEDFILEINFO ffi = {0};
    ffi.dwSignature = 0xFEEF04BD;
    ffi.dwStrucVersion = 0x00010000;
    ffi.dwFileVersionMS = (fileVersion[0] << 16) | fileVersion[1];
    ffi.dwFileVersionLS = (fileVersion[2] << 16) | fileVersion[3];
    ffi.dwProductVersionMS = (productVersion[0] << 16) | productVersion[1];
    ffi.dwProductVersionLS = (productVersion[2] << 16) | productVersion[3];
    ffi.dwFileFlagsMask = 0x3F;
    ffi.dwFileFlags = 0;
    ffi.dwFileOS = 0x40004; // VOS_NT_WINDOWS32
    ffi.dwFileType = 0x1;  // VFT_APP
    ffi.dwFileSubtype = 0;
    ffi.dwFileDateMS = 0;
    ffi.dwFileDateLS = 0;

    std::vector<BYTE> data(sizeof(ffi));
    memcpy(data.data(), &ffi, sizeof(ffi));
    return data;
}

// Parse a version string (e.g. "1.0.0.2") into four DWORD numbers.
bool ParseVersionString(const std::string &versionStr, DWORD version[4]) {
    std::istringstream iss(versionStr);
    std::string token;
    int index = 0;
    while (std::getline(iss, token, '.') && index < 4) {
        version[index] = std::stoul(token);
        index++;
    }
    while (index < 4) {
        version[index++] = 0;
    }
    return true;
}

// Structure to hold all version info parameters.
struct VersionParams {
    std::wstring company;
    std::wstring description;
    std::wstring fileVersionStr;
    std::wstring internalName;
    std::wstring legalCopyright;
    std::wstring originalFilename;
    std::wstring productName;
    std::wstring productVersionStr;
};

// Build the complete version resource binary data
std::vector<BYTE> BuildVersionResource(const VersionParams &params) {
    // Parse version strings (convert from wstring to string for parsing)
    DWORD fileVer[4], prodVer[4];
    {
        std::string fver(params.fileVersionStr.begin(), params.fileVersionStr.end());
        std::string pver(params.productVersionStr.begin(), params.productVersionStr.end());
        ParseVersionString(fver, fileVer);
        ParseVersionString(pver, prodVer);
    }

    // Build the fixed file info block (the "Value" of the top-level block)
    std::vector<BYTE> ffiData = BuildFixedFileInfo(fileVer, prodVer);

    // Build individual String blocks for each field.
    std::vector<std::vector<BYTE>> stringBlocks;
    stringBlocks.push_back(BuildBlock(L"CompanyName", 1, WStringToBytes(params.company), {}));
    stringBlocks.push_back(BuildBlock(L"FileDescription", 1, WStringToBytes(params.description), {}));
    stringBlocks.push_back(BuildBlock(L"FileVersion", 1, WStringToBytes(params.fileVersionStr), {}));
    stringBlocks.push_back(BuildBlock(L"InternalName", 1, WStringToBytes(params.internalName), {}));
    stringBlocks.push_back(BuildBlock(L"LegalCopyright", 1, WStringToBytes(params.legalCopyright), {}));
    stringBlocks.push_back(BuildBlock(L"OriginalFilename", 1, WStringToBytes(params.originalFilename), {}));
    stringBlocks.push_back(BuildBlock(L"ProductName", 1, WStringToBytes(params.productName), {}));
    stringBlocks.push_back(BuildBlock(L"ProductVersion", 1, WStringToBytes(params.productVersionStr), {}));

    // Build the StringTable block (using language/codepage "040904B0")
    std::vector<std::vector<BYTE>> stChildren = stringBlocks;
    std::vector<BYTE> stringTable = BuildBlock(L"040904B0", 1, std::vector<BYTE>(), stChildren);

    // Build the StringFileInfo block, which contains the StringTable block.
    std::vector<std::vector<BYTE>> sfiChildren;
    sfiChildren.push_back(stringTable);
    std::vector<BYTE> stringFileInfo = BuildBlock(L"StringFileInfo", 1, std::vector<BYTE>(), sfiChildren);

    // Build the Var block for Translation (language and codepage).
    std::vector<BYTE> translationValue;
    {
        DWORD lang = 0x0409;
        DWORD codepage = 0x04B0; // 0x04B0 == 1200 decimal
        translationValue.resize(sizeof(DWORD) * 2);
        memcpy(translationValue.data(), &lang, sizeof(DWORD));
        memcpy(translationValue.data() + sizeof(DWORD), &codepage, sizeof(DWORD));
    }
    std::vector<BYTE> varBlock = BuildBlock(L"Translation", 0, translationValue, {});

    // Build the VarFileInfo block, which contains the Var block.
    std::vector<std::vector<BYTE>> vfiChildren;
    vfiChildren.push_back(varBlock);
    std::vector<BYTE> varFileInfo = BuildBlock(L"VarFileInfo", 0, std::vector<BYTE>(), vfiChildren);

    // Finally, build the top-level VS_VERSION_INFO block with key "VS_VERSION_INFO"
    std::vector<std::vector<BYTE>> children;
    children.push_back(stringFileInfo);
    children.push_back(varFileInfo);
    std::vector<BYTE> versionInfo = BuildBlock(L"VS_VERSION_INFO", 0, ffiData, children);

    return versionInfo;
}

int main(int argc, char* argv[]) {
    // Check that all required parameters are provided.
    if (argc < 17) {
        std::cout << "Usage: " << argv[0] << " --exe <exefile> --ico <iconfile> --company <company> --description <description> --version <file version> "
                  << "--name <internal name> --copyright <copyright> --filename <original filename> --product <product name> --productversion <product version>\n";
        return 1;
    }

    std::string exePath, icoPath, company, description, fileVersion, internalName,
                copyright,
                originalFilename, productName, productVersion;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--exe" && i + 1 < argc) {
            exePath = argv[++i];
        } else if (arg == "--ico" && i + 1 < argc) {
            icoPath = argv[++i];
        } else if (arg == "--company" && i + 1 < argc) {
            company = argv[++i];
        } else if (arg == "--description" && i + 1 < argc) {
            description = argv[++i];
        } else if (arg == "--version" && i + 1 < argc) {
            fileVersion = argv[++i];
        } else if (arg == "--name" && i + 1 < argc) {
            internalName = argv[++i];
        } else if (arg == "--copyright" && i + 1 < argc) {
            copyright = argv[++i];
        } else if (arg == "--filename" && i + 1 < argc) {
            originalFilename = argv[++i];
        } else if (arg == "--product" && i + 1 < argc) {
            productName = argv[++i];
        } else if (arg == "--productversion" && i + 1 < argc) {
            productVersion = argv[++i];
        }
    }

    if (exePath.empty() || icoPath.empty()) {
        std::cout << "Missing required parameters.\n";
        return 1;
    }

    // Open the target executable for resource updating.
    HANDLE hUpdate = BeginUpdateResourceA(exePath.c_str(), FALSE);
    if (!hUpdate) {
        std::cout << "BeginUpdateResource failed. Error: " << GetLastError() << "\n";
        return 1;
    }

    // Read the icon file from disk.
    std::ifstream icoFile(icoPath, std::ios::binary);
    if (!icoFile) {
        std::cout << "Failed to open icon file.\n";
        return 1;
    }
    std::vector<BYTE> icoData((std::istreambuf_iterator<char>(icoFile)),
                              std::istreambuf_iterator<char>());
    icoFile.close();

    // Update the ICON resource (ID 101, type RT_ICON)
    if (!UpdateResourceA(hUpdate, RT_ICON, MAKEINTRESOURCEA(101),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         icoData.data(), (DWORD)icoData.size()))
    {
        std::cout << "UpdateResource for ICON failed. Error: " << GetLastError() << "\n";
        EndUpdateResource(hUpdate, TRUE);
        return 1;
    }

    // Build the VERSIONINFO resource from the parameters.
    VersionParams verParams;
    verParams.company          = std::wstring(company.begin(), company.end());
    verParams.description      = std::wstring(description.begin(), description.end());
    verParams.fileVersionStr   = std::wstring(fileVersion.begin(), fileVersion.end());
    verParams.internalName     = std::wstring(internalName.begin(), internalName.end());
    verParams.legalCopyright   = std::wstring(copyright.begin(), copyright.end());
    verParams.originalFilename = std::wstring(originalFilename.begin(), originalFilename.end());
    verParams.productName      = std::wstring(productName.begin(), productName.end());
    verParams.productVersionStr= std::wstring(productVersion.begin(), productVersion.end());

    std::vector<BYTE> versionResource = BuildVersionResource(verParams);

    // Update the VERSION resource (ID 1, type RT_VERSION)
    if (!UpdateResourceA(hUpdate, RT_VERSION, MAKEINTRESOURCEA(1),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         versionResource.data(), (DWORD)versionResource.size()))
    {
        std::cout << "UpdateResource for VERSION failed. Error: " << GetLastError() << "\n";
        EndUpdateResource(hUpdate, TRUE);
        return 1;
    }

    // Commit the resource changes.
    if (!EndUpdateResource(hUpdate, FALSE)) {
        std::cout << "EndUpdateResource failed. Error: " << GetLastError() << "\n";
        return 1;
    }

    std::cout << "Resource update successful.\n";
    return 0;
}
