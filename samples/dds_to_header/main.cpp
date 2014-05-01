//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <ddraw.h>
#include <d3d9types.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <array>

int main(int argc, char **argv)
{
    std::string programName(argv[0]);
    if (argc < 4)
    {
        std::cout << "usage:\n";
        std::cout << programName << "INPUT_FILE OUTPUT_C_ARRAY_NAME OUTPUT_FILE\n";
        return -1;
    }

    std::string inputFile(argv[1]);
    std::string outputName(argv[2]);
    std::string outputFile(argv[3]);
    std::ifstream readFile(inputFile, std::ios::binary | std::ios::in);

    readFile.seekg(0, std::ios::end);
    std::streamoff fileSize = readFile.tellg();
    readFile.seekg(0, std::ios::beg);

    const std::size_t minSize = sizeof(DDSURFACEDESC2) + sizeof(DWORD);
    if (fileSize < minSize)
    {
        std::cout << inputFile << " is only " << fileSize << " bytes, must be at greater than " << minSize
                  << " bytes to be a correct DDS image file.\n";
        return -2;
    }

    DWORD magicWord;
    readFile.read(reinterpret_cast<char*>(&magicWord), sizeof(DWORD));
    if (magicWord != MAKEFOURCC('D','D','S',' '))
    {
        std::cout << "Magic word must be " << MAKEFOURCC('D','D','S',' ') << ".\n";
        return -3;
    }

    DDSURFACEDESC2 header;
    readFile.read(reinterpret_cast<char*>(&header), sizeof(DDSURFACEDESC2));

    std::string formatName;
    std::size_t blockSize = 0;
    std::size_t blockWidth = 0;
    std::size_t blockHeight = 0;

    if (header.ddpfPixelFormat.dwFlags & DDPF_RGB)
    {
        blockSize = header.ddpfPixelFormat.dwRGBBitCount / 8;
        blockWidth = 1;
        blockHeight = 1;

        if (blockSize == 4)
        {
            if (header.ddpfPixelFormat.dwRBitMask        == 0x000000FF &&
                header.ddpfPixelFormat.dwGBitMask        == 0x0000FF00 &&
                header.ddpfPixelFormat.dwBBitMask        == 0x00FF0000 &&
                header.ddpfPixelFormat.dwRGBAlphaBitMask == 0xFF000000)
            {
                formatName = "RGBA8";
            }
            else if (header.ddpfPixelFormat.dwRBitMask       == 0x000000FF &&
                    header.ddpfPixelFormat.dwGBitMask        == 0x0000FF00 &&
                    header.ddpfPixelFormat.dwBBitMask        == 0x00FF0000 &&
                    header.ddpfPixelFormat.dwRGBAlphaBitMask == 0x00000000)
            {
                formatName = "RGBX8";
            }
            else if (header.ddpfPixelFormat.dwRBitMask       == 0x000003FF &&
                    header.ddpfPixelFormat.dwGBitMask        == 0x000FFC00 &&
                    header.ddpfPixelFormat.dwBBitMask        == 0x3FF00000 &&
                    header.ddpfPixelFormat.dwRGBAlphaBitMask == 0xC0000000)
            {
                formatName = "RGB10A2";
            }
            else if (header.ddpfPixelFormat.dwRBitMask       == 0x0000FFFF &&
                    header.ddpfPixelFormat.dwGBitMask        == 0xFFFF0000 &&
                    header.ddpfPixelFormat.dwBBitMask        == 0x00000000 &&
                    header.ddpfPixelFormat.dwRGBAlphaBitMask == 0x00000000)
            {
                formatName = "RG16";
            }
            else if (header.ddpfPixelFormat.dwRBitMask       == 0xFFFFFFFF &&
                    header.ddpfPixelFormat.dwGBitMask        == 0x00000000 &&
                    header.ddpfPixelFormat.dwBBitMask        == 0x00000000 &&
                    header.ddpfPixelFormat.dwRGBAlphaBitMask == 0x00000000)
            {
                formatName = "R32";
            }
            else
            {
                formatName = "UKNOWN";
            }
        }
    }
    else if (header.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
    {
        switch (header.ddpfPixelFormat.dwFourCC)
        {
          case MAKEFOURCC('D','X','T','1'):
            formatName = "DXT1";
            blockSize = 8;
            blockWidth = 4;
            blockHeight = 4;
            break;

          case MAKEFOURCC('D','X','T','3'):
            formatName = "DXT3";
            blockSize = 16;
            blockWidth = 4;
            blockHeight = 4;
            break;

          case MAKEFOURCC('D','X','T','5'):
            formatName = "DXT5";
            blockSize = 16;
            blockWidth = 4;
            blockHeight = 4;
            break;

          case D3DFMT_R32F:
            formatName = "R32F";
            blockSize = 4;
            blockWidth = 1;
            blockHeight = 1;

          case D3DFMT_G32R32F:
            formatName = "RG32F";
            blockSize = 8;
            blockWidth = 1;
            blockHeight = 1;

          case D3DFMT_A32B32G32R32F:
            formatName = "RGBA32F";
            blockSize = 16;
            blockWidth = 1;
            blockHeight = 1;

          case D3DFMT_R16F:
            formatName = "R16F";
            blockSize = 2;
            blockWidth = 1;
            blockHeight = 1;

          case D3DFMT_G16R16F:
            formatName = "RG16F";
            blockSize = 4;
            blockWidth = 1;
            blockHeight = 1;

          case D3DFMT_A16B16G16R16F:
            formatName = "RGBA16F";
            blockSize = 8;
            blockWidth = 1;
            blockHeight = 1;

          default:
            std::cout << "Unsupported FourCC format.\n";
            return -5;
        }
    }
    else
    {
        std::cout << "Unsupported DDS format.\n";
        return -6;
    }

    std::size_t height = header.dwHeight;
    std::size_t width = header.dwWidth;
    std::size_t levels = std::max<size_t>(1, header.dwMipMapCount);

    std::ofstream oss(outputFile);
    oss << "// Automatically generated header from " << inputFile << ", a " << width << "x" << height;
    if (levels > 1)
    {
        oss << " (" << levels << " mip levels)";
    }
    oss << "\n// " << formatName << " texture using " << programName << ".\n";

    oss << "static const size_t " << outputName << "_width = " << width << ";\n";
    oss << "static const size_t " << outputName << "_height = " << height << ";\n";
    oss << "static const size_t " << outputName << "_levels = " << levels << ";\n";
    oss << "\n";

    for (std::size_t i = 0; i < levels; ++i)
    {
        std::size_t widthAtLevel = std::max<size_t>(width >> i, 1);
        std::size_t heightAtLevel = std::max<size_t>(height >> i, 1);
        std::size_t sizeAtLevel = static_cast<std::size_t>(std::ceil(widthAtLevel / static_cast<float>(blockWidth)) *
                                                           std::ceil(heightAtLevel / static_cast<float>(blockHeight))) *
                                  blockSize;

        std::vector<unsigned char> data(sizeAtLevel);
        readFile.read(reinterpret_cast<char*>(data.data()), sizeAtLevel);

        oss << "static const size_t " << outputName << "_" << i << "_width = " << widthAtLevel << ";\n";
        oss << "static const size_t " << outputName << "_" << i << "_height = " << heightAtLevel << ";\n";
        oss << "static const size_t " << outputName << "_" << i << "_size = " << sizeAtLevel << ";\n";
        oss << "static const unsigned char " << outputName << "_" << i << "_data[" << sizeAtLevel << "] =\n";
        oss << "{";
        for (std::size_t j = 0; j < sizeAtLevel; j++)
        {
            if (j % 16 == 0)
            {
                oss << "\n    ";
            }

            char buffer[32];
            sprintf_s(buffer, "0x%02X,", data[j]);
            oss << std::string(buffer);
        }
        oss << "\n";
        oss << "};\n";

        if (i + 1 < levels)
        {
            oss << "\n";
        }
    }

    oss.close();
    readFile.close();

    return 0;
}
