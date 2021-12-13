/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

/*
This is written based on the PNG specification found here:
http://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html

TODO:
- serious code clean-up pass
    split up methods
    make it all more readable
- handle PLTE chunks
- handle more bit depths and formats
- check and stop and IEND
- better error handling all the way around
- make sure we don't leak our temporary uncompressed scanlines buffer
- handle eXIf for rotation:  
        http://ftp-osl.osuosl.org/pub/libpng/documents/pngext-1.5.0.html#C.eXIf 
        https://home.jeita.or.jp/tsc/std-pdf/CP-3451D.pdf
        https://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/EXIF.html
*/


#include "PNGDecoder.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Compression/Compression.h>
#include <AzCore/Debug/Trace.h>




void PNGDecoder::ProcessBuffer(AZStd::vector<char>* pngBuffer, AZStd::vector<uint32_t>& pixelBuffer, uint32_t& height, uint32_t& width)
{
    struct pngChunkHeader
    {
        uint32_t length;
        uint32_t chunkType;
    };

    struct pngChunkFooter
    {
        uint32_t crc;
    };

    struct pngIHDR
    {
        uint32_t width;
        uint32_t height;
        uint8_t bitDepth;
        uint8_t colorType;
        uint8_t compressionMethod;
        uint8_t filterMethod;
        uint8_t interlaceMethod;
    };

    // Parse the PNG data into a CImageEx.

    int offset = 0;
    pngChunkHeader chunkHeader;
    pngChunkFooter chunkFooter;
    pngIHDR ihdr;

    auto ReadUint8 = [](auto pngBuffer, auto& offset)
    {
        uint8_t result;
        result = static_cast<uint8_t>((*pngBuffer)[offset + 0]);
        offset += 1;
        return result;
    };

    auto ReadUint32 = [](auto pngBuffer, auto& offset)
    {
        uint32_t result;
        result = (static_cast<uint8_t>((*pngBuffer)[offset + 0]) << 24) +
            (static_cast<uint8_t>((*pngBuffer)[offset + 1]) << 16) +
            (static_cast<uint8_t>((*pngBuffer)[offset + 2]) << 8) +
            (static_cast<uint8_t>((*pngBuffer)[offset + 3]) << 0);
        offset += 4;
        return result;
    };


    // Verify that the PNG file header is what we expect.
    {
        const uint8_t pngHeader[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
        if (memcmp(pngHeader, &((*pngBuffer)[0]), sizeof(pngHeader)) != 0)
        {
            AZ_Error("Terrain", false, "Invalid PNG Header.");
            return;
        }
        offset += sizeof(pngHeader);
    }

    // Verify that the PNG IHDR header is what we expect.
    {
        chunkHeader.length = ReadUint32(pngBuffer, offset);
        chunkHeader.chunkType = ReadUint32(pngBuffer, offset);
        if ((chunkHeader.length != 13) || (chunkHeader.chunkType != 'IHDR'))
        {
            AZ_Error("Terrain", false, "Invalid PNG IHDR chunk.");
            return;
        }

        ihdr.width = ReadUint32(pngBuffer, offset);
        ihdr.height = ReadUint32(pngBuffer, offset);
        ihdr.bitDepth = ReadUint8(pngBuffer, offset);
        ihdr.colorType = ReadUint8(pngBuffer, offset);
        ihdr.compressionMethod = ReadUint8(pngBuffer, offset);
        ihdr.filterMethod = ReadUint8(pngBuffer, offset);
        ihdr.interlaceMethod = ReadUint8(pngBuffer, offset);

        chunkFooter.crc = ReadUint32(pngBuffer, offset);
    }

    // Perform some header validations
    {

        if ((ihdr.width == 0) || (ihdr.height == 0))
        {
            AZ_Error("Terrain", false, "Invalid PNG header:  width=%d, height=%d", ihdr.width, ihdr.height);
            return;
        }

        if (ihdr.compressionMethod != 0)
        {
            AZ_Error("Terrain", false, "Invalid PNG header:  unrecognized compressionMethod=%d", ihdr.compressionMethod);
            return;

        }

        if (ihdr.filterMethod != 0)
        {
            AZ_Error("Terrain", false, "Invalid PNG header:  unrecognized filterMethod=%d", ihdr.filterMethod);
            return;

        }

        switch (ihdr.colorType)
        {
        case 0:
            // Greyscale
            if ((ihdr.bitDepth != 1) && (ihdr.bitDepth != 2) && (ihdr.bitDepth != 4) && (ihdr.bitDepth != 8) && (ihdr.bitDepth != 16))
            {
                AZ_Error("Terrain", false, "Invalid PNG header:  bitDepth=%d is invalid for greyscale images", ihdr.bitDepth);
                return;
            }
            break;
        case 2:
            // RGB triplet
            if ((ihdr.bitDepth != 8) && (ihdr.bitDepth != 16))
            {
                AZ_Error("Terrain", false, "Invalid PNG header:  bitDepth=%d is invalid for RGB images", ihdr.bitDepth);
                return;
            }
            break;
        case 3:
            // Palette, requires PLTE chunk
            if ((ihdr.bitDepth != 1) && (ihdr.bitDepth != 2) && (ihdr.bitDepth != 4) && (ihdr.bitDepth != 8))
            {
                AZ_Error("Terrain", false, "Invalid PNG header:  bitDepth=%d is invalid for palette images", ihdr.bitDepth);
                return;
            }
            break;
        case 4:
            // Greyscale + Alpha
            if ((ihdr.bitDepth != 8) && (ihdr.bitDepth != 16))
            {
                AZ_Error("Terrain", false, "Invalid PNG header:  bitDepth=%d is invalid for greyscale + alpha images", ihdr.bitDepth);
                return;
            }
            break;
        case 6:
            // RGBA
            if ((ihdr.bitDepth != 8) && (ihdr.bitDepth != 16))
            {
                AZ_Error("Terrain", false, "Invalid PNG header:  bitDepth=%d is invalid for RGBA images", ihdr.bitDepth);
                return;
            }
            break;
        default:
            AZ_Error("Terrain", false, "Invalid PNG header:  colorType=%d, bitDepth=%d", ihdr.colorType, ihdr.bitDepth);
            return;
        }

        if ((ihdr.interlaceMethod != 0) && (ihdr.interlaceMethod != 1))
        {
            AZ_Error("Terrain", false, "Invalid PNG header:  unrecognized interlaceMethod=%d", ihdr.interlaceMethod);
            return;
        }


        // Terrain-specific header validations:  colorType of 2 means RGB pixels (24-bit)
        if ((ihdr.width != 256) || (ihdr.height != 256) || (ihdr.colorType != 2) || (ihdr.bitDepth != 8))
        {
            AZ_Error("Terrain", false, "Unexpected PNG IHDR chunk.");
            return;
        }
    }

    // Reserve enough space for all of our raw pixels.  Each raw pixel is 24-bit.
    // The extra numFilterBytes are because the first byte of each scanline contains a "filter-type" byte.
    const uint32_t numFilterBytes = ihdr.height;
    const uint32_t uncompressedBufferSize = (ihdr.width * ihdr.height * 3) + numFilterBytes;
    char* uncompressedRawPixels = new char[uncompressedBufferSize];

    // Locate and decompress the raw scanline data from the PNG.  We'll need to do a second pass on this data to decode it,
    // due to the per-scanline "filtering" that can occur.
    {
        AZ::ZLib decompressor;
        decompressor.StartDecompressor();

        char* uncompressedBufferPtr = uncompressedRawPixels;
        char* uncompressedRawPixelsEnd = uncompressedRawPixels + uncompressedBufferSize;

        while (offset < pngBuffer->size())
        {
            chunkHeader.length = ReadUint32(pngBuffer, offset);
            chunkHeader.chunkType = ReadUint32(pngBuffer, offset);
            if (chunkHeader.chunkType == 'IDAT')
            {
                uint32_t compressedBytesHandled = 0;

                while (compressedBytesHandled < chunkHeader.length)
                {
                    uint32_t currentRemainingUncompressedBufferSize = static_cast<uint32_t>(uncompressedRawPixelsEnd - uncompressedBufferPtr);
                    uint32_t remainingUncompressedBufferSize = currentRemainingUncompressedBufferSize;

                    if (remainingUncompressedBufferSize == 0)
                    {
                        AZ_Assert(remainingUncompressedBufferSize > 0, "Ran out of buffer space!");
                        return;
                    }

                    uint32_t currentCompressedBytesHandled = decompressor.Decompress(&((*pngBuffer)[offset + compressedBytesHandled]), chunkHeader.length - compressedBytesHandled,
                        uncompressedBufferPtr, remainingUncompressedBufferSize);

                    compressedBytesHandled += currentCompressedBytesHandled;
                    if ((currentCompressedBytesHandled <= 0) || (compressedBytesHandled > chunkHeader.length))
                    {
                        AZ_Assert(currentCompressedBytesHandled > 0, "Failed to decompress.");
                        AZ_Assert(compressedBytesHandled <= chunkHeader.length, "Unexpected buffer overflow");
                        return;
                    }
                    uncompressedBufferPtr += (currentRemainingUncompressedBufferSize - remainingUncompressedBufferSize);
                }
            }

            offset += chunkHeader.length;
            chunkFooter.crc = ReadUint32(pngBuffer, offset);
        }

        decompressor.StopDecompressor();

        AZ_TracePrintf("Terrain", "Expected %d bytes, uncompressed %d bytes.", uncompressedBufferSize, (uint32_t)(uncompressedBufferPtr - uncompressedRawPixels));
    }

    // Decode PNG scanlines into our final pixel buffer
    {
        height = ihdr.height;
        width = ihdr.width;
        pixelBuffer.reserve(ihdr.width* ihdr.height);
        pixelBuffer.clear();


        auto GetDecodedPixel = [](uint32_t* decodedData, int bufferHeight, int bufferWidth, int x, int y)
        {
            if ((x < 0) || (x >= bufferWidth) || (y < 0) || (y >= bufferHeight))
            {
                return static_cast<uint32_t>(0);
            }

            return (decodedData[(bufferWidth * y) + x]);
        };

        auto GetPixel = [](char* rawBuffer, int bufferHeight, int bufferWidth, int x, int y)
        {
            if ((x < 0) || (x >= bufferWidth) || (y < 0) || (y >= bufferHeight))
            {
                return static_cast<uint32_t>(0);
            }

            uint32_t offset = (((bufferWidth * 3) + 1) * y) + (x * 3) + 1;

            return static_cast<uint32_t>((static_cast<uint8_t>(rawBuffer[offset + 0]) << 0) +
                (static_cast<uint8_t>(rawBuffer[offset + 1]) << 8) +
                (static_cast<uint8_t>(rawBuffer[offset + 2]) << 16) +
                (0));
        };



        auto GetFilterType = [](char* rawBuffer, int bufferHeight, int bufferWidth, int y)
        {
            if ((y < 0) || (y >= bufferHeight))
            {
                return static_cast<uint8_t>(0);
            }

            uint32_t offset = ((bufferWidth * 3) + 1) * y;

            return static_cast<uint8_t>(rawBuffer[offset]);
        };

        for (uint32_t y = 0; y < ihdr.height; y++)
        {
            uint8_t filterType = GetFilterType(uncompressedRawPixels, ihdr.height, ihdr.width, y);

            for (uint32_t x = 0; x < ihdr.width; x++)
            {
                uint32_t pixelValue = GetPixel(uncompressedRawPixels, ihdr.height, ihdr.width, x, y);

                switch (filterType)
                {
                case 0:
                    // Filter = none
                    break;
                case 1:
                {
                    // Filter = Sub
                    uint32_t leftPixel = GetDecodedPixel(pixelBuffer.begin(), ihdr.height, ihdr.width, x - 1, y);
                    uint8_t r = (((pixelValue >> 0) & 0xFF) + ((leftPixel >> 0) & 0xFF)) & 0xFF;
                    uint8_t g = (((pixelValue >> 8) & 0xFF) + ((leftPixel >> 8) & 0xFF)) & 0xFF;
                    uint8_t b = (((pixelValue >> 16) & 0xFF) + ((leftPixel >> 16) & 0xFF)) & 0xFF;
                    pixelValue = static_cast<uint32_t>((r << 0) + (g << 8) + (b << 16));
                }
                break;
                case 2:
                {
                    // Filter = Up
                    uint32_t upPixel = GetDecodedPixel(pixelBuffer.begin(), ihdr.height, ihdr.width, x, y - 1);
                    uint8_t r = (((pixelValue >> 0) & 0xFF) + ((upPixel >> 0) & 0xFF)) & 0xFF;
                    uint8_t g = (((pixelValue >> 8) & 0xFF) + ((upPixel >> 8) & 0xFF)) & 0xFF;
                    uint8_t b = (((pixelValue >> 16) & 0xFF) + ((upPixel >> 16) & 0xFF)) & 0xFF;
                    pixelValue = static_cast<uint32_t>((r << 0) + (g << 8) + (b << 16));

                }
                break;
                case 3:
                {
                    // Filter = Average
                    uint32_t leftPixel = GetDecodedPixel(pixelBuffer.begin(), ihdr.height, ihdr.width, x - 1, y);
                    uint32_t upPixel = GetDecodedPixel(pixelBuffer.begin(), ihdr.height, ihdr.width, x, y - 1);
                    uint8_t r = (((pixelValue >> 0) & 0xFF) + ((((leftPixel >> 0) & 0xFF) + ((upPixel >> 0) & 0xFF))) / 2) % 256;
                    uint8_t g = (((pixelValue >> 8) & 0xFF) + ((((leftPixel >> 8) & 0xFF) + ((upPixel >> 8) & 0xFF))) / 2) % 256;
                    uint8_t b = (((pixelValue >> 16) & 0xFF) + ((((leftPixel >> 16) & 0xFF) + ((upPixel >> 16) & 0xFF))) / 2) % 256;
                    pixelValue = static_cast<uint32_t>((r << 0) + (g << 8) + (b << 16));

                }
                break;
                case 4:
                {
                    // Filter = Paeth
                    uint32_t a = GetDecodedPixel(pixelBuffer.begin(), ihdr.height, ihdr.width, x - 1, y);
                    uint32_t b = GetDecodedPixel(pixelBuffer.begin(), ihdr.height, ihdr.width, x, y - 1);
                    uint32_t c = GetDecodedPixel(pixelBuffer.begin(), ihdr.height, ihdr.width, x - 1, y - 1);

                    int rp = ((a >> 0) & 0xFF) + ((b >> 0) & 0xFF) - ((c >> 0) & 0xFF);
                    int gp = ((a >> 8) & 0xFF) + ((b >> 8) & 0xFF) - ((c >> 8) & 0xFF);
                    int bp = ((a >> 16) & 0xFF) + ((b >> 16) & 0xFF) - ((c >> 16) & 0xFF);

                    int rpa = abs(rp - static_cast<uint8_t>((a >> 0) & 0xFF));
                    int gpa = abs(gp - static_cast<uint8_t>((a >> 8) & 0xFF));
                    int bpa = abs(bp - static_cast<uint8_t>((a >> 16) & 0xFF));

                    int rpb = abs(rp - static_cast<uint8_t>((b >> 0) & 0xFF));
                    int gpb = abs(gp - static_cast<uint8_t>((b >> 8) & 0xFF));
                    int bpb = abs(bp - static_cast<uint8_t>((b >> 16) & 0xFF));

                    int rpc = abs(rp - static_cast<uint8_t>((c >> 0) & 0xFF));
                    int gpc = abs(gp - static_cast<uint8_t>((c >> 8) & 0xFF));
                    int bpc = abs(bp - static_cast<uint8_t>((c >> 16) & 0xFF));

                    uint8_t rPaeth = ((rpa <= rpb) && (rpa <= rpc)) ? ((a >> 0) & 0xFF) : (rpb <= rpc) ? ((b >> 0) & 0xFF) : ((c >> 0) & 0xFF);
                    uint8_t gPaeth = ((gpa <= gpb) && (gpa <= gpc)) ? ((a >> 8) & 0xFF) : (gpb <= gpc) ? ((b >> 8) & 0xFF) : ((c >> 8) & 0xFF);
                    uint8_t bPaeth = ((bpa <= bpb) && (bpa <= bpc)) ? ((a >> 16) & 0xFF) : (bpb <= bpc) ? ((b >> 16) & 0xFF) : ((c >> 16) & 0xFF);

                    uint8_t r = (((pixelValue >> 0) & 0xFF) + rPaeth) & 0xFF;
                    uint8_t g = (((pixelValue >> 8) & 0xFF) + gPaeth) & 0xFF;
                    uint8_t bb = (((pixelValue >> 16) & 0xFF) + bPaeth) & 0xFF;

                    pixelValue = static_cast<uint32_t>((r << 0) + (g << 8) + (bb << 16));

                }
                break;
                default:
                    AZ_Error("Terrain", false, "Unknown scanline filter type: %d", filterType);
                    return;
                }

                pixelBuffer.push_back(pixelValue);
            }
        }

    }

    delete[] uncompressedRawPixels;
}

