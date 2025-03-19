// icoconvert.cpp
//
// This complete program converts a PNG file into a Windows ICO file containing
// multiple icon sizes (16, 32, 48, and 256). It uses GDI+ to load and resize the PNG.
//
// To compile (MinGW example):
//   g++ icoconvert.cpp -O2 -pipe -static -static-libstdc++ -static-libgcc -lgdiplus -s -o icoconvert.exe
//
// Usage:
//   icoconvert.exe input.png output.ico

#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstdlib>
#include <cstring>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

//---------------------------------------------------------
// Helper functions to write little-endian values into a byte vector.
void write_u16(std::vector<uint8_t>& data, uint16_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
}

void write_u32(std::vector<uint8_t>& data, uint32_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
}

//---------------------------------------------------------
// Create the DIB (device-independent bitmap) data for one icon image.
// The ICO image data consists of a BITMAPINFOHEADER (40 bytes), followed by
// pixel data in BGRA order (written in bottom-up order), and then the AND mask.
std::vector<uint8_t> CreateIconImageFromBitmap(Bitmap* bmp) {
    UINT width = bmp->GetWidth();
    UINT height = bmp->GetHeight();

    // Lock the bitmap bits (we request 32bpp; GDI+ stores it as BGRA)
    Rect rect(0, 0, width, height);
    BitmapData bitmapData;
    if(bmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) != Ok) {
        std::cerr << "Failed to lock bitmap bits.\n";
        return std::vector<uint8_t>();
    }
    // Copy pixel data row by row. Note: bitmapData.Stride may be wider than width*4.
    std::vector<uint8_t> pixels(width * height * 4);
    for (UINT y = 0; y < height; y++) {
        uint8_t* src = (uint8_t*)bitmapData.Scan0 + y * bitmapData.Stride;
        memcpy(&pixels[y * width * 4], src, width * 4);
    }
    bmp->UnlockBits(&bitmapData);

    std::vector<uint8_t> data;
    // Write BITMAPINFOHEADER (40 bytes)
    write_u32(data, 40);               // biSize
    write_u32(data, width);            // biWidth
    write_u32(data, height * 2);       // biHeight (image + mask)
    write_u16(data, 1);                // biPlanes
    write_u16(data, 32);               // biBitCount (32 bits per pixel)
    write_u32(data, 0);                // biCompression (BI_RGB, no compression)
    write_u32(data, width * height * 4); // biSizeImage (size of pixel data)
    write_u32(data, 0);                // biXPelsPerMeter
    write_u32(data, 0);                // biYPelsPerMeter
    write_u32(data, 0);                // biClrUsed
    write_u32(data, 0);                // biClrImportant

    // Write pixel data (XOR mask) in bottom-up order.
    for (int y = height - 1; y >= 0; y--) {
        for (UINT x = 0; x < width; x++) {
            // Each pixel is 4 bytes in the "pixels" vector.
            size_t idx = (y * width + x) * 4;
            // Pixels from GDI+ (PixelFormat32bppARGB) are stored in memory as BGRA.
            data.push_back(pixels[idx + 0]); // Blue
            data.push_back(pixels[idx + 1]); // Green
            data.push_back(pixels[idx + 2]); // Red
            data.push_back(pixels[idx + 3]); // Alpha
        }
    }

    // Create the AND mask: 1 bit per pixel. Each row is padded to a multiple of 32 bits.
    // Compute the number of bytes per row in the mask.
    unsigned int maskRowBytes = ((width + 31) / 32) * 4;
    std::vector<uint8_t> mask(maskRowBytes * height, 0);

    // For each pixel (using the original top-down order from "pixels"),
    // if the alpha is 0 then set the corresponding mask bit to 1.
    for (UINT y = 0; y < height; y++) {
        for (UINT x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 4;
            uint8_t alpha = pixels[idx + 3];
            if(alpha == 0) {
                // Determine bit position in the mask for pixel (x, y)
                unsigned int byteIndex = (y * maskRowBytes) + (x / 8);
                unsigned int bitIndex = 7 - (x % 8);
                mask[byteIndex] |= (1 << bitIndex);
            }
        }
    }
    // The AND mask in the ICO file is also stored in bottom-up order.
    for (int y = height - 1; y >= 0; y--) {
        data.insert(data.end(), &mask[y * maskRowBytes], &mask[y * maskRowBytes] + maskRowBytes);
    }

    return data;
}

//---------------------------------------------------------
// Main program entry point.
int main(int argc, char* argv[])
{
    if(argc < 3) {
        std::cout << "Usage: " << argv[0] << " input.png output.ico" << std::endl;
        return 1;
    }
    std::string inputFilename = argv[1];
    std::string outputFilename = argv[2];

    // Initialize GDI+
    ULONG_PTR gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;
    if(GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        std::cerr << "Failed to initialize GDI+." << std::endl;
        return 1;
    }

    // Load the PNG image using GDI+
    Bitmap* pngBitmap = new Bitmap(std::wstring(inputFilename.begin(), inputFilename.end()).c_str());
    if(pngBitmap->GetLastStatus() != Ok) {
        std::cerr << "Failed to load image: " << inputFilename << std::endl;
        delete pngBitmap;
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    // Define desired icon sizes (in pixels)
    std::vector<UINT> sizes = { 16, 32, 48, 256 };

    // Structure to hold each icon image’s data
    struct IconEntry {
        UINT width;
        std::vector<uint8_t> data;
    };
    std::vector<IconEntry> icons;

    // For each size, create a thumbnail and convert it to ICO image data.
    for(auto size : sizes) {
        Image* thumbImg = pngBitmap->GetThumbnailImage(size, size, NULL, NULL);
        if (!thumbImg) {
            std::cerr << "Failed to create thumbnail for size " << size << "x" << size << std::endl;
            continue;
        }
        Bitmap* thumb = Bitmap::FromImage(thumbImg);
        delete thumbImg; // free the temporary Image
        if(!thumb || thumb->GetLastStatus() != Ok) {
            std::cerr << "Thumbnail isn’t a Bitmap for size " << size << "x" << size << std::endl;
            delete thumb;
            continue;
        }
        std::vector<uint8_t> iconData = CreateIconImageFromBitmap(thumb);
        delete thumb;
        if(iconData.empty()) {
            std::cerr << "Failed to process thumbnail for size " << size << "x" << size << std::endl;
            continue;
        }
        icons.push_back({ size, iconData });
    }
    delete pngBitmap;

    if(icons.empty()) {
        std::cerr << "No icon images were created." << std::endl;
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    // Build the ICO file.
    // ICO file header (ICONDIR) is 6 bytes: reserved (2 bytes), type (2 bytes), count (2 bytes).
    // Each icon (ICONDIRENTRY) is 16 bytes.
    std::vector<uint8_t> ico;
    write_u16(ico, 0);                    // Reserved
    write_u16(ico, 1);                    // Type: 1 for icon
    write_u16(ico, (uint16_t)icons.size()); // Number of images

    // The first image’s data will start at offset:
    uint32_t offset = 6 + (uint32_t)icons.size() * 16;
    // Write one ICONDIRENTRY for each image.
    for(auto& entry : icons) {
        // In ICONDIRENTRY, width and height are stored as 1 byte each; 0 means 256.
        uint8_t w = (entry.width >= 256) ? 0 : (uint8_t)entry.width;
        uint8_t h = (entry.width >= 256) ? 0 : (uint8_t)entry.width;
        ico.push_back(w);           // Width
        ico.push_back(h);           // Height
        ico.push_back(0);           // Color count (0 for 32bpp)
        ico.push_back(0);           // Reserved
        write_u16(ico, 1);          // Color planes
        write_u16(ico, 32);         // Bits per pixel
        write_u32(ico, (uint32_t)entry.data.size()); // Size of the image data in bytes
        write_u32(ico, offset);     // Offset of the image data from the beginning of the file
        offset += (uint32_t)entry.data.size();
    }

    // Append each icon image’s data
    for(auto& entry : icons) {
        ico.insert(ico.end(), entry.data.begin(), entry.data.end());
    }

    // Write the ICO file to disk.
    std::ofstream outfile(outputFilename, std::ios::binary);
    if(!outfile) {
        std::cerr << "Failed to open output file: " << outputFilename << std::endl;
        GdiplusShutdown(gdiplusToken);
        return 1;
    }
    outfile.write(reinterpret_cast<const char*>(ico.data()), ico.size());
    outfile.close();

    GdiplusShutdown(gdiplusToken);
    std::cout << "ICO file created successfully: " << outputFilename << std::endl;
    return 0;
}
