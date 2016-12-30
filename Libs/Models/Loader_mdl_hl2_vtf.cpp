/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_mdl_hl2_vtf.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <math.h>


using namespace HL2mdl;


namespace
{
    void ToLuminance(uint16_t& R, uint16_t& G, uint16_t& B, uint16_t& A)
    {
        const float sLuminanceWeightR = 0.299f;
        const float sLuminanceWeightG = 0.587f;
        const float sLuminanceWeightB = 0.114f;

        R = G = B = (uint16_t)(sLuminanceWeightR * (float)R + sLuminanceWeightG * (float)G + sLuminanceWeightB * (float)B);
    }


    void FromLuminance(uint16_t& R, uint16_t& G, uint16_t& B, uint16_t& A)
    {
        B = G = R;
    }


    void ToBlueScreen(uint16_t& R, uint16_t& G, uint16_t& B, uint16_t& A)
    {
        const uint16_t uiBlueScreenMaskR = 0x0000;
        const uint16_t uiBlueScreenMaskG = 0x0000;
        const uint16_t uiBlueScreenMaskB = 0xffff;

        if (A == 0x0000)
        {
            R = uiBlueScreenMaskR;
            G = uiBlueScreenMaskG;
            B = uiBlueScreenMaskB;
        }

        A = 0xffff;
    }


    void FromBlueScreen(uint16_t& R, uint16_t& G, uint16_t& B, uint16_t& A)
    {
        const uint16_t uiBlueScreenMaskR = 0x0000;
        const uint16_t uiBlueScreenMaskG = 0x0000;
        const uint16_t uiBlueScreenMaskB = 0xffff;

        const uint16_t uiBlueScreenClearR = 0x0000;
        const uint16_t uiBlueScreenClearG = 0x0000;
        const uint16_t uiBlueScreenClearB = 0x0000;

        if (R == uiBlueScreenMaskR && G == uiBlueScreenMaskG && B == uiBlueScreenMaskB)
        {
            R = uiBlueScreenClearR;
            G = uiBlueScreenClearG;
            B = uiBlueScreenClearB;
            A = 0x0000;
        }
        else
        {
            A = 0xffff;
        }
    }


    float ClampFP16(float sValue)
    {
        if (sValue < 0.0f)
            sValue = 0.0f;

        if (sValue > 65335.0f)
            sValue = 65335.0f;

        return sValue;
    }


    float sHDRLogAverageLuminance;


    void ToFP16(uint16_t& R, uint16_t& G, uint16_t& B, uint16_t& A)
    {
    }


    // Reference:
    // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/programmingguide/advancedtopics/HDRLighting/HDRLighting.asp
    void FromFP16(uint16_t& R, uint16_t& G, uint16_t& B, uint16_t& A)
    {
        const float sR = (float)R, sG = (float)G, sB = (float)B;//, sA = (float)A;

        const float sY = sR * 0.299f + sG * 0.587f + sB * 0.114f;

        const float sU = (sB - sY) * 0.565f;
        const float sV = (sR - sY) * 0.713f;

        float sTemp = sY;

        const float sFP16HDRKey = 4.0f;
        const float sFP16HDRShift = 0.0f;
        const float sFP16HDRGamma = 2.25f;

        sTemp = sFP16HDRKey * sTemp / sHDRLogAverageLuminance;
        sTemp = sTemp / (1.0f + sTemp);

        sTemp = sTemp / sY;

        R = (uint16_t)ClampFP16(pow((sY + 1.403f * sV) * sTemp + sFP16HDRShift, sFP16HDRGamma) * 65535.0f);
        G = (uint16_t)ClampFP16(pow((sY - 0.344f * sU - 0.714f * sV) * sTemp + sFP16HDRShift, sFP16HDRGamma) * 65535.0f);
        B = (uint16_t)ClampFP16(pow((sY + 1.770f * sU) * sTemp + sFP16HDRShift, sFP16HDRGamma) * 65535.0f);
    }


    vtfImageFormatInfoT VTFImageFormatInfo[] =
    {
        { "RGBA8888",             32,  4,    8,  8,  8,  8,    0,  1,  2,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_RGBA8888 },
        { "ABGR8888",             32,  4,    8,  8,  8,  8,    3,  2,  1,  0,   false, true,    NULL,    NULL,        IMAGE_FORMAT_ABGR8888 },
        { "RGB888",               24,  3,    8,  8,  8,  0,    0,  1,  2, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_RGB888 },
        { "BGR888",               24,  3,    8,  8,  8,  0,    2,  1,  0, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_BGR888 },
        { "RGB565",               16,  2,    5,  6,  5,  0,    0,  1,  2, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_RGB565 },
        { "I8",                    8,  1,    8,  8,  8,  0,    0, -1, -1, -1,   false, true,    ToLuminance, FromLuminance, IMAGE_FORMAT_I8 },
        { "IA88",                 16,  2,    8,  8,  8,  8,    0, -1, -1,  1,   false, true,    ToLuminance, FromLuminance, IMAGE_FORMAT_IA88 },
        { "P8",                    8,  1,    0,  0,  0,  0,   -1, -1, -1, -1,   false, false,   NULL,    NULL,        IMAGE_FORMAT_P8 },
        { "A8",                    8,  1,    0,  0,  0,  8,   -1, -1, -1,  0,   false, true,    NULL,    NULL,        IMAGE_FORMAT_A8 },
        { "RGB888 Bluescreen",    24,  3,    8,  8,  8,  8,    0,  1,  2, -1,   false, true,    ToBlueScreen, FromBlueScreen, IMAGE_FORMAT_RGB888_BLUESCREEN },
        { "BGR888 Bluescreen",    24,  3,    8,  8,  8,  8,    2,  1,  0, -1,   false, true,    ToBlueScreen, FromBlueScreen, IMAGE_FORMAT_BGR888_BLUESCREEN },
        { "ARGB8888",             32,  4,    8,  8,  8,  8,    3,  0,  1,  2,   false, true,    NULL,    NULL,        IMAGE_FORMAT_ARGB8888 },
        { "BGRA8888",             32,  4,    8,  8,  8,  8,    2,  1,  0,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_BGRA8888 },
        { "DXT1",                  4,  0,    0,  0,  0,  0,   -1, -1, -1, -1,   true,  true,    NULL,    NULL,        IMAGE_FORMAT_DXT1 },
        { "DXT3",                  8,  0,    0,  0,  0,  8,   -1, -1, -1, -1,   true,  true,    NULL,    NULL,        IMAGE_FORMAT_DXT3 },
        { "DXT5",                  8,  0,    0,  0,  0,  8,   -1, -1, -1, -1,   true,  true,    NULL,    NULL,        IMAGE_FORMAT_DXT5 },
        { "BGRX8888",             32,  4,    8,  8,  8,  0,    2,  1,  0, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_BGRX8888 },
        { "BGR565",               16,  2,    5,  6,  5,  0,    2,  1,  0, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_BGR565 },
        { "BGRX5551",             16,  2,    5,  5,  5,  0,    2,  1,  0, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_BGRX5551 },
        { "BGRA4444",             16,  2,    4,  4,  4,  4,    2,  1,  0,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_BGRA4444 },
        { "DXT1 One Bit Alpha",    4,  0,    0,  0,  0,  1,   -1, -1, -1, -1,   true,  true,    NULL,    NULL,        IMAGE_FORMAT_DXT1_ONEBITALPHA },
        { "BGRA5551",             16,  2,    5,  5,  5,  1,    2,  1,  0,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_BGRA5551 },
        { "UV88",                 16,  2,    8,  8,  0,  0,    0,  1, -1, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_UV88 },
        { "UVWQ8888",             32,  4,    8,  8,  8,  8,    0,  1,  2,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_UVWQ8888 },
        { "RGBA16161616F",        64,  8,   16, 16, 16, 16,    0,  1,  2,  3,   false, true,    ToFP16,  FromFP16,    IMAGE_FORMAT_RGBA16161616F },
        { "RGBA16161616",         64,  8,   16, 16, 16, 16,    0,  1,  2,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_RGBA16161616 },
        { "UVLX8888",             32,  4,    8,  8,  8,  8,    0,  1,  2,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_UVLX8888 },
        { "R32F",                 32,  4,   32,  0,  0,  0,    0, -1, -1, -1,   false, false,   NULL,    NULL,        IMAGE_FORMAT_R32F },
        { "RGB323232F",           96, 12,   32, 32, 32,  0,    0,  1,  2, -1,   false, false,   NULL,    NULL,        IMAGE_FORMAT_RGB323232F },
        { "RGBA32323232F",       128, 16,   32, 32, 32, 32,    0,  1,  2,  3,   false, false,   NULL,    NULL,        IMAGE_FORMAT_RGBA32323232F },
        { "nVidia DST16",         16,  2,   16,  0,  0,  0,    0, -1, -1, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_NV_DST16 },
        { "nVidia DST24",         24,  3,   24,  0,  0,  0,    0, -1, -1, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_NV_DST24 },
        { "nVidia INTZ",          32,  4,    0,  0,  0,  0,   -1, -1, -1, -1,   false, false,   NULL,    NULL,        IMAGE_FORMAT_NV_INTZ },
        { "nVidia RAWZ",          24,  3,    0,  0,  0,  0,   -1, -1, -1, -1,   false, false,   NULL,    NULL,        IMAGE_FORMAT_NV_RAWZ },
        { "ATI DST16",            16,  2,   16,  0,  0,  0,    0, -1, -1, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_ATI_DST16 },
        { "ATI DST24",            24,  3,   24,  0,  0,  0,    0, -1, -1, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_ATI_DST24 },
        { "nVidia NULL",          32,  4,    0,  0,  0,  0,   -1, -1, -1, -1,   false, false,   NULL,    NULL,        IMAGE_FORMAT_NV_NULL },
        { "ATI1N",                 4,  0,    0,  0,  0,  0,   -1, -1, -1, -1,   true,  false,   NULL,    NULL,        IMAGE_FORMAT_ATI1N },
        { "ATI2N",                 8,  0,    0,  0,  0,  0,   -1, -1, -1, -1,   true,  false,   NULL,    NULL,        IMAGE_FORMAT_ATI2N },
     /* { "Xbox360 DST16",        16,  2,   16,  0,  0,  0,    0, -1, -1, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_X360_DST16 },
        { "Xbox360 DST24",        24,  3,   24,  0,  0,  0,    0, -1, -1, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_X360_DST24 },
        { "Xbox360 DST24F",       24,  3,    0,  0,  0,  0,   -1, -1, -1, -1,   false, false,   NULL,    NULL,        IMAGE_FORMAT_X360_DST24F },
        { "Linear BGRX8888",      32,  4,    8,  8,  8,  0,    2,  1,  0, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_BGRX8888 },
        { "Linear RGBA8888",      32,  4,    8,  8,  8,  8,    0,  1,  2,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_RGBA8888 },
        { "Linear ABGR8888",      32,  4,    8,  8,  8,  8,    3,  2,  1,  0,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_ABGR8888 },
        { "Linear ARGB8888",      32,  4,    8,  8,  8,  8,    3,  0,  1,  2,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_ARGB8888 },
        { "Linear BGRA8888",      32,  4,    8,  8,  8,  8,    2,  1,  0,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_BGRA8888 },
        { "Linear RGB888",        32,  4,    8,  8,  8,  8,    0,  1,  2, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_RGB888 },
        { "Linear BGR888",        32,  4,    8,  8,  8,  8,    2,  1,  0, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_BGR888 },
        { "Linear BGRX5551",      16,  2,    5,  5,  5,  0,    2,  1,  0, -1,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_BGRX5551 },
        { "Linear I8",             8,  1,    8,  8,  8,  0,    0, -1, -1, -1,   false, true,    ToLuminance, FromLuminance, IMAGE_FORMAT_LINEAR_I8 },
        { "Linear RGBA16161616",  64,  8,   16, 16, 16, 16,    0,  1,  2,  3,   false, true,    NULL,    NULL,        IMAGE_FORMAT_LINEAR_RGBA16161616 },
        { "LE BGRX8888",          32,  4,    8,  8,  8,  0,   -1, -1, -1, -1,   false, true     NULL,    NULL,        IMAGE_FORMAT_LE_BGRX8888 },
        { "LE BGRA8888",          32,  4,    8,  8,  8,  8,   -1, -1, -1, -1,   false, true     NULL,    NULL,        IMAGE_FORMAT_LE_BGRA8888 } */
    };
}


const vtfImageFormatInfoT& HL2mdl::GetImageFormatInfo(vtfImageFormatT ImageFormat)
{
    assert(ImageFormat >= 0 && ImageFormat < IMAGE_FORMAT_COUNT);

    return VTFImageFormatInfo[ImageFormat];
}


namespace
{
    uint32_t ComputeImageSize(uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth, vtfImageFormatT ImageFormat)
    {
        switch (ImageFormat)
        {
            case IMAGE_FORMAT_DXT1:
            case IMAGE_FORMAT_DXT1_ONEBITALPHA:
                return ((uiWidth + 3) / 4) * ((uiHeight + 3) / 4) * 8 * uiDepth;

            case IMAGE_FORMAT_DXT3:
            case IMAGE_FORMAT_DXT5:
                return ((uiWidth + 3) / 4) * ((uiHeight + 3) / 4) * 16 * uiDepth;

            default:
                return uiWidth * uiHeight * uiDepth * GetImageFormatInfo(ImageFormat).BytesPerPixel;
        }
    }


    uint32_t ComputeImageSize(uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth, uint32_t uiMipmaps, vtfImageFormatT ImageFormat)
    {
        uint32_t uiImageSize = 0;

        assert(uiWidth != 0 && uiHeight != 0 && uiDepth != 0);

        for (uint32_t i = 0; i < uiMipmaps; i++)
        {
            uiImageSize += ComputeImageSize(uiWidth, uiHeight, uiDepth, ImageFormat);

            uiWidth >>= 1;
            uiHeight >>= 1;
            uiDepth >>= 1;

            if (uiWidth < 1)
                uiWidth = 1;

            if (uiHeight < 1)
                uiHeight = 1;

            if (uiDepth < 1)
                uiDepth = 1;
        }

        return uiImageSize;
    }


    uint32_t ComputeMipmapSize(uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth, uint32_t uiMipmapLevel, vtfImageFormatT ImageFormat)
    {
        return ComputeImageSize(
            std::max(uiWidth >> uiMipmapLevel, 1u),
            std::max(uiHeight >> uiMipmapLevel, 1u),
            std::max(uiDepth >> uiMipmapLevel, 1u), ImageFormat);
    }


    uint8_t* GetRawBytes(const std::string& Name, ArrayT<uint8_t>& RawBytes)
    {
        FILE* InFile = fopen(Name.c_str(), "rb");

        if (!InFile) throw vtfLoadErrorT("Could not open the " + Name + " file.");

        fseek(InFile, 0, SEEK_END); RawBytes.PushBackEmptyExact(ftell(InFile));
        fseek(InFile, 0, SEEK_SET);

        if (fread(&RawBytes[0], RawBytes.Size(), 1, InFile) == 0) { }   // Must check the return value of fread() with GCC 4.3...

        fclose(InFile);
        InFile = NULL;

        return &RawBytes[0];
    }


    struct Colour8888
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };


    struct Colour565
    {
        uint16_t nBlue  : 5;
        uint16_t nGreen : 6;
        uint16_t nRed   : 5;
    };


    //-----------------------------------------------------------------------------------------------------
    // DXTn decompression code is based on examples on Microsofts website and from the
    // Developers Image Library (http://www.imagelib.org) (c) Denton Woods.
    //
    //-----------------------------------------------------------------------------------------------------
    // DecompressDXT1(uint8_t *src, uint8_t *dst, uint32_t uiWidth, uint32_t uiHeight)
    //
    // Converts data from the DXT1 to RGBA8888 format. Data is read from *src
    // and written to *dst. Width and height are needed to it knows how much data to process
    //-----------------------------------------------------------------------------------------------------
    bool DecompressDXT1(uint8_t* src, uint8_t* dst, uint32_t uiWidth, uint32_t uiHeight)
    {
        uint8_t*      Temp;
        Colour565*    color_0, *color_1;
        Colour8888    colours[4], *col;
        uint32_t      bitmask, Offset;

        uint8_t nBpp = 4;                        // bytes per pixel (4 channels (RGBA))
        uint8_t nBpc = 1;                        // bytes per channel (1 byte per channel)
        uint32_t iBps = nBpp * nBpc * uiWidth;        // bytes per scanline

        Temp = src;

        for (uint32_t y = 0; y < uiHeight; y += 4)
        {
            for (uint32_t x = 0; x < uiWidth; x += 4)
            {
                color_0 = ((Colour565*)Temp);
                color_1 = ((Colour565*)(Temp + 2));
                bitmask = ((uint32_t*)Temp)[1];
                Temp += 8;

                colours[0].r = color_0->nRed << 3;
                colours[0].g = color_0->nGreen << 2;
                colours[0].b = color_0->nBlue << 3;
                colours[0].a = 0xFF;

                colours[1].r = color_1->nRed << 3;
                colours[1].g = color_1->nGreen << 2;
                colours[1].b = color_1->nBlue << 3;
                colours[1].a = 0xFF;

                if (*((uint16_t*)color_0) > *((uint16_t*)color_1))
                {
                    // Four-color block: derive the other two colors.
                    // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
                    // These 2-bit codes correspond to the 2-bit fields
                    // stored in the 64-bit block.
                    colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
                    colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
                    colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
                    colours[2].a = 0xFF;

                    colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
                    colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
                    colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
                    colours[3].a = 0xFF;
                }
                else
                {
                    // Three-color block: derive the other color.
                    // 00 = color_0,  01 = color_1,  10 = color_2,
                    // 11 = transparent.
                    // These 2-bit codes correspond to the 2-bit fields
                    // stored in the 64-bit block.
                    colours[2].b = (colours[0].b + colours[1].b) / 2;
                    colours[2].g = (colours[0].g + colours[1].g) / 2;
                    colours[2].r = (colours[0].r + colours[1].r) / 2;
                    colours[2].a = 0xFF;

                    colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
                    colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
                    colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
                    colours[3].a = 0x00;
                }

                for (uint32_t j = 0, k = 0; j < 4; j++)
                {
                    for (uint32_t i = 0; i < 4; i++, k++)
                    {
                        const uint32_t Select = (bitmask & (0x03 << k * 2)) >> k * 2;
                        col = &colours[Select];

                        if (((x + i) < uiWidth) && ((y + j) < uiHeight))
                        {
                            Offset = (y + j) * iBps + (x + i) * nBpp;
                            dst[Offset + 0] = col->r;
                            dst[Offset + 1] = col->g;
                            dst[Offset + 2] = col->b;
                            dst[Offset + 3] = col->a;
                        }
                    }
                }
            }
        }
        return true;
    }


    //-----------------------------------------------------------------------------------------------------
    // DecompressDXT3(uint8_t *src, uint8_t *dst, uint32_t uiWidth, uint32_t uiHeight)
    //
    // Converts data from the DXT3 to RGBA8888 format. Data is read from *src
    // and written to *dst. Width and height are needed to it knows how much data to process
    //-----------------------------------------------------------------------------------------------------
    bool DecompressDXT3(uint8_t* src, uint8_t* dst, uint32_t uiWidth, uint32_t uiHeight)
    {
        uint8_t*      Temp;
        Colour565*    color_0, *color_1;
        Colour8888    colours[4], *col;
        uint32_t      bitmask, Offset;

        uint8_t nBpp = 4;                        // bytes per pixel (4 channels (RGBA))
        uint8_t nBpc = 1;                        // bytes per channel (1 byte per channel)
        uint32_t iBps = nBpp * nBpc * uiWidth;        // bytes per scanline

        Temp = src;

        for (uint32_t y = 0; y < uiHeight; y += 4)
        {
            for (uint32_t x = 0; x < uiWidth; x += 4)
            {
                const uint16_t* alphaRows = (uint16_t*)Temp;    // An "alpha block", consisting of four "rows", where each row is one uint16_t value.
                Temp += 8;
                color_0 = ((Colour565*)Temp);
                color_1 = ((Colour565*)(Temp + 2));
                bitmask = ((uint32_t*)Temp)[1];
                Temp += 8;

                colours[0].r = color_0->nRed << 3;
                colours[0].g = color_0->nGreen << 2;
                colours[0].b = color_0->nBlue << 3;
                colours[0].a = 0xFF;

                colours[1].r = color_1->nRed << 3;
                colours[1].g = color_1->nGreen << 2;
                colours[1].b = color_1->nBlue << 3;
                colours[1].a = 0xFF;

                // Four-color block: derive the other two colors.
                // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
                // These 2-bit codes correspond to the 2-bit fields
                // stored in the 64-bit block.
                colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
                colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
                colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
                colours[2].a = 0xFF;

                colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
                colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
                colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
                colours[3].a = 0xFF;

                uint32_t k = 0;
                for (uint32_t j = 0; j < 4; j++)
                {
                    for (uint32_t i = 0; i < 4; i++, k++)
                    {
                        const uint32_t Select = (bitmask & (0x03 << k * 2)) >> k * 2;
                        col = &colours[Select];

                        if (((x + i) < uiWidth) && ((y + j) < uiHeight))
                        {
                            Offset = (y + j) * iBps + (x + i) * nBpp;
                            dst[Offset + 0] = col->r;
                            dst[Offset + 1] = col->g;
                            dst[Offset + 2] = col->b;
                        }
                    }
                }

                for (uint32_t j = 0; j < 4; j++)
                {
                    uint16_t word = alphaRows[j];

                    for (uint32_t i = 0; i < 4; i++)
                    {
                        if (((x + i) < uiWidth) && ((y + j) < uiHeight))
                        {
                            Offset = (y + j) * iBps + (x + i) * nBpp + 3;
                            dst[Offset] = word & 0x0F;
                            dst[Offset] = dst[Offset] | (dst[Offset] << 4);
                        }

                        word >>= 4;
                    }
                }
            }
        }
        return true;
    }


    //-----------------------------------------------------------------------------------------------------
    // DecompressDXT5(uint8_t *src, uint8_t *dst, uint32_t uiWidth, uint32_t uiHeight)
    //
    // Converts data from the DXT5 to RGBA8888 format. Data is read from *src
    // and written to *dst. Width and height are needed to it knows how much data to process
    //-----------------------------------------------------------------------------------------------------
    bool DecompressDXT5(uint8_t* src, uint8_t* dst, uint32_t uiWidth, uint32_t uiHeight)
    {
        uint8_t*      Temp;
        Colour565*    color_0, *color_1;
        Colour8888    colours[4], *col;
        uint32_t      bitmask, Offset;
        uint8_t       alphas[8], *alphamask;
        uint32_t      bits;

        uint8_t nBpp = 4;                        // bytes per pixel (4 channels (RGBA))
        uint8_t nBpc = 1;                        // bytes per channel (1 byte per channel)
        uint32_t iBps = nBpp * nBpc * uiWidth;        // bytes per scanline

        Temp = src;

        for (uint32_t y = 0; y < uiHeight; y += 4)
        {
            for (uint32_t x = 0; x < uiWidth; x += 4)
            {
                //if (y >= uiHeight || x >= uiWidth)
                //        break;

                alphas[0] = Temp[0];
                alphas[1] = Temp[1];
                alphamask = Temp + 2;
                Temp += 8;
                color_0 = ((Colour565*)Temp);
                color_1 = ((Colour565*)(Temp + 2));
                bitmask = ((uint32_t*)Temp)[1];
                Temp += 8;

                colours[0].r = color_0->nRed << 3;
                colours[0].g = color_0->nGreen << 2;
                colours[0].b = color_0->nBlue << 3;
                colours[0].a = 0xFF;

                colours[1].r = color_1->nRed << 3;
                colours[1].g = color_1->nGreen << 2;
                colours[1].b = color_1->nBlue << 3;
                colours[1].a = 0xFF;

                // Four-color block: derive the other two colors.
                // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
                // These 2-bit codes correspond to the 2-bit fields
                // stored in the 64-bit block.
                colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
                colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
                colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
                colours[2].a = 0xFF;

                colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
                colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
                colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
                colours[3].a = 0xFF;

                uint32_t k = 0;
                for (uint32_t j = 0; j < 4; j++)
                {
                    for (uint32_t i = 0; i < 4; i++, k++)
                    {
                        const uint32_t Select = (bitmask & (0x03 << k * 2)) >> k * 2;
                        col = &colours[Select];

                        // only put pixels out < width or height
                        if (((x + i) < uiWidth) && ((y + j) < uiHeight))
                        {
                            Offset = (y + j) * iBps + (x + i) * nBpp;
                            dst[Offset + 0] = col->r;
                            dst[Offset + 1] = col->g;
                            dst[Offset + 2] = col->b;
                        }
                    }
                }

                // 8-alpha or 6-alpha block?
                if (alphas[0] > alphas[1])
                {
                    // 8-alpha block:  derive the other six alphas.
                    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
                    alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;    // bit code 010
                    alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;    // bit code 011
                    alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;    // bit code 100
                    alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;    // bit code 101
                    alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;    // bit code 110
                    alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;    // bit code 111
                }
                else
                {
                    // 6-alpha block.
                    // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
                    alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;    // Bit code 010
                    alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;    // Bit code 011
                    alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;    // Bit code 100
                    alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;    // Bit code 101
                    alphas[6] = 0x00;                                        // Bit code 110
                    alphas[7] = 0xFF;                                        // Bit code 111
                }

                // Note: Have to separate the next two loops,
                //    it operates on a 6-byte system.

                // First three bytes
                bits = *((int*)alphamask);
                for (uint32_t j = 0; j < 2; j++)
                {
                    for (uint32_t i = 0; i < 4; i++)
                    {
                        // only put pixels out < width or height
                        if (((x + i) < uiWidth) && ((y + j) < uiHeight))
                        {
                            Offset = (y + j) * iBps + (x + i) * nBpp + 3;
                            dst[Offset] = alphas[bits & 0x07];
                        }
                        bits >>= 3;
                    }
                }

                // Last three bytes
                bits = *((int*)&alphamask[3]);
                for (uint32_t j = 2; j < 4; j++)
                {
                    for (uint32_t i = 0; i < 4; i++)
                    {
                        // only put pixels out < width or height
                        if (((x + i) < uiWidth) && ((y + j) < uiHeight))
                        {
                            Offset = (y + j) * iBps + (x + i) * nBpp + 3;
                            dst[Offset] = alphas[bits & 0x07];
                        }
                        bits >>= 3;
                    }
                }
            }
        }
        return true;
    }


    // Get each channel's shift and mask (for encoding and decoding).
    template<typename T>
    void GetShiftAndMask(const vtfImageFormatInfoT& Info, T& uiRShift, T& uiGShift, T& uiBShift, T& uiAShift, T& uiRMask, T& uiGMask, T& uiBMask, T& uiAMask)
    {
        if (Info.IndexRed >= 0)
        {
            if (Info.IndexGreen >= 0 && Info.IndexGreen < Info.IndexRed)
                uiRShift += (T)Info.GreenBitsPerPixel;

            if (Info.IndexBlue >= 0 && Info.IndexBlue < Info.IndexRed)
                uiRShift += (T)Info.BlueBitsPerPixel;

            if (Info.IndexAlpha >= 0 && Info.IndexAlpha < Info.IndexRed)
                uiRShift += (T)Info.AlphaBitsPerPixel;

            uiRMask = (T)(~0) >> (T)((sizeof(T) * 8) - Info.RedBitsPerPixel); // Mask is for down shifted values.
        }

        if (Info.IndexGreen >= 0)
        {
            if (Info.IndexRed >= 0 && Info.IndexRed < Info.IndexGreen)
                uiGShift += (T)Info.RedBitsPerPixel;

            if (Info.IndexBlue >= 0 && Info.IndexBlue < Info.IndexGreen)
                uiGShift += (T)Info.BlueBitsPerPixel;

            if (Info.IndexAlpha >= 0 && Info.IndexAlpha < Info.IndexGreen)
                uiGShift += (T)Info.AlphaBitsPerPixel;

            uiGMask = (T)(~0) >> (T)((sizeof(T) * 8) - Info.GreenBitsPerPixel);
        }

        if (Info.IndexBlue >= 0)
        {
            if (Info.IndexRed >= 0 && Info.IndexRed < Info.IndexBlue)
                uiBShift += (T)Info.RedBitsPerPixel;

            if (Info.IndexGreen >= 0 && Info.IndexGreen < Info.IndexBlue)
                uiBShift += (T)Info.GreenBitsPerPixel;

            if (Info.IndexAlpha >= 0 && Info.IndexAlpha < Info.IndexBlue)
                uiBShift += (T)Info.AlphaBitsPerPixel;

            uiBMask = (T)(~0) >> (T)((sizeof(T) * 8) - Info.BlueBitsPerPixel);
        }

        if (Info.IndexAlpha >= 0)
        {
            if (Info.IndexRed >= 0 && Info.IndexRed < Info.IndexAlpha)
                uiAShift += (T)Info.RedBitsPerPixel;

            if (Info.IndexGreen >= 0 && Info.IndexGreen < Info.IndexAlpha)
                uiAShift += (T)Info.GreenBitsPerPixel;

            if (Info.IndexBlue >= 0 && Info.IndexBlue < Info.IndexAlpha)
                uiAShift += (T)Info.BlueBitsPerPixel;

            uiAMask = (T)(~0) >> (T)((sizeof(T) * 8) - Info.AlphaBitsPerPixel);
        }
    }


    // Downsample a channel.
    template<typename T>
    T Shrink(T S, T SourceBits, T DestBits)
    {
        if (SourceBits == 0 || DestBits == 0)
            return 0;

        return S >> (SourceBits - DestBits);
    }


    // Upsample a channel.
    template<typename T>
    T Expand(T S, T SourceBits, T DestBits)
    {
        if (SourceBits == 0 || DestBits == 0)
            return 0;

        T D = 0;

        // Repeat source bit pattern as much as possible.
        while (DestBits >= SourceBits)
        {
            D <<= SourceBits;
            D |= S;
            DestBits -= SourceBits;
        }

        // Add most significant part of source bit pattern to least significant part of dest bit pattern.
        if (DestBits)
        {
            S >>= SourceBits - DestBits;
            D <<= DestBits;
            D |= S;
        }

        return D;
    }


    // Run custom transformation functions.
    template<typename T, typename U>
    void Transform(vtfImageFormatInfoT::TransformProc pTransform1, vtfImageFormatInfoT::TransformProc pTransform2,
        T SR, T SG, T SB, T SA, T SRBits, T SGBits, T SBBits, T SABits, U& DR, U& DG, U& DB, U& DA, U DRBits, U DGBits, U DBBits, U DABits)
    {
        uint16_t TR, TG, TB, TA;

        // Expand from source to 16 bits for transform functions.
        SRBits&& SRBits < 16 ? TR = (uint16_t)Expand<T>(SR, SRBits, 16) : TR = (uint16_t)SR;
        SGBits&& SGBits < 16 ? TG = (uint16_t)Expand<T>(SG, SGBits, 16) : TG = (uint16_t)SG;
        SBBits&& SBBits < 16 ? TB = (uint16_t)Expand<T>(SB, SBBits, 16) : TB = (uint16_t)SB;
        SABits&& SABits < 16 ? TA = (uint16_t)Expand<T>(SA, SABits, 16) : TA = (uint16_t)SA;

        // Source transform then dest transform.
        if (pTransform1)
            pTransform1(TR, TG, TB, TA);
        if (pTransform2)
            pTransform2(TR, TG, TB, TA);

        // Shrink to dest from 16 bits.
        DRBits&& DRBits < 16 ? DR = (U)Shrink<uint16_t>(TR, 16, (uint16_t)DRBits) : DR = (U)TR;
        DGBits&& DGBits < 16 ? DG = (U)Shrink<uint16_t>(TG, 16, (uint16_t)DGBits) : DG = (U)TG;
        DBBits&& DBBits < 16 ? DB = (U)Shrink<uint16_t>(TB, 16, (uint16_t)DBBits) : DB = (U)TB;
        DABits&& DABits < 16 ? DA = (U)Shrink<uint16_t>(TA, 16, (uint16_t)DABits) : DA = (U)TA;
    }


    // Convert source to dest using required storage requirments (hence the template).
    template<typename T>
    bool ConvertTemplated(uint8_t* lpSource, uint8_t* lpDest, uint32_t uiWidth, uint32_t uiHeight, const vtfImageFormatInfoT& SourceInfo, const vtfImageFormatInfoT& DestInfo)
    {
        uint16_t uiSourceRShift = 0, uiSourceGShift = 0, uiSourceBShift = 0, uiSourceAShift = 0;
        uint16_t uiSourceRMask = 0, uiSourceGMask = 0, uiSourceBMask = 0, uiSourceAMask = 0;

        uint16_t uiDestRShift = 0, uiDestGShift = 0, uiDestBShift = 0, uiDestAShift = 0;
        uint16_t uiDestRMask = 0, uiDestGMask = 0, uiDestBMask = 0, uiDestAMask = 0;

        GetShiftAndMask<uint16_t>(SourceInfo, uiSourceRShift, uiSourceGShift, uiSourceBShift, uiSourceAShift, uiSourceRMask, uiSourceGMask, uiSourceBMask, uiSourceAMask);
        GetShiftAndMask<uint16_t>(DestInfo, uiDestRShift, uiDestGShift, uiDestBShift, uiDestAShift, uiDestRMask, uiDestGMask, uiDestBMask, uiDestAMask);

        // If we are in the FP16 HDR format we will need a log average.
        if (SourceInfo.Format == IMAGE_FORMAT_RGBA16161616F)
        {
            uint8_t* lpFPSource = lpSource;

            sHDRLogAverageLuminance = 0.0f;

            uint8_t* lpFPSourceEnd = lpFPSource + (uiWidth * uiHeight * SourceInfo.BytesPerPixel);
            for (; lpFPSource < lpFPSourceEnd; lpFPSource += SourceInfo.BytesPerPixel)
            {
                uint16_t* p = (uint16_t*)lpFPSource;

                const float sLuminance = (float)p[0] * 0.299f + (float)p[1] * 0.587f + (float)p[2] * 0.114f;

                sHDRLogAverageLuminance += log(0.0000000001f + sLuminance);
            }

            sHDRLogAverageLuminance = exp(sHDRLogAverageLuminance / (float)(uiWidth * uiHeight));
        }

        uint8_t* lpSourceEnd = lpSource + (uiWidth * uiHeight * SourceInfo.BytesPerPixel);
        for (; lpSource < lpSourceEnd; lpSource += SourceInfo.BytesPerPixel, lpDest += DestInfo.BytesPerPixel)
        {
            // read source into single variable
            uint32_t i;
            T Source = 0;
            for (i = 0; i < SourceInfo.BytesPerPixel; i++)
            {
                Source |= (T)lpSource[i] << ((T)i * 8);
            }

            uint16_t SR = 0, SG = 0, SB = 0, SA = ~0;
            uint16_t DR = 0, DG = 0, DB = 0, DA = ~0;    // default values

            // read source values
            if (uiSourceRMask)
                SR = (uint16_t)(Source >> (T)uiSourceRShift) & uiSourceRMask;    // isolate R channel

            if (uiSourceGMask)
                SG = (uint16_t)(Source >> (T)uiSourceGShift) & uiSourceGMask;    // isolate G channel

            if (uiSourceBMask)
                SB = (uint16_t)(Source >> (T)uiSourceBShift) & uiSourceBMask;    // isolate B channel

            if (uiSourceAMask)
                SA = (uint16_t)(Source >> (T)uiSourceAShift) & uiSourceAMask;    // isolate A channel

            if (SourceInfo.pFromTransform || DestInfo.pToTransform)
            {
                // transform values
                Transform<uint16_t, uint16_t>(SourceInfo.pFromTransform, DestInfo.pToTransform, SR, SG, SB, SA, SourceInfo.RedBitsPerPixel, SourceInfo.GreenBitsPerPixel, SourceInfo.BlueBitsPerPixel, SourceInfo.AlphaBitsPerPixel, DR, DG, DB, DA, DestInfo.RedBitsPerPixel, DestInfo.GreenBitsPerPixel, DestInfo.BlueBitsPerPixel, DestInfo.AlphaBitsPerPixel);
            }
            else
            {
                // default value transform
                if (uiSourceRMask && uiDestRMask)
                {
                    if (DestInfo.RedBitsPerPixel < SourceInfo.RedBitsPerPixel)   // downsample
                        DR = Shrink<uint16_t>(SR, SourceInfo.RedBitsPerPixel, DestInfo.RedBitsPerPixel);
                    else if (DestInfo.RedBitsPerPixel > SourceInfo.RedBitsPerPixel)   // upsample
                        DR = Expand<uint16_t>(SR, SourceInfo.RedBitsPerPixel, DestInfo.RedBitsPerPixel);
                    else
                        DR = SR;
                }

                if (uiSourceGMask && uiDestGMask)
                {
                    if (DestInfo.GreenBitsPerPixel < SourceInfo.GreenBitsPerPixel)   // downsample
                        DG = Shrink<uint16_t>(SG, SourceInfo.GreenBitsPerPixel, DestInfo.GreenBitsPerPixel);
                    else if (DestInfo.GreenBitsPerPixel > SourceInfo.GreenBitsPerPixel)   // upsample
                        DG = Expand<uint16_t>(SG, SourceInfo.GreenBitsPerPixel, DestInfo.GreenBitsPerPixel);
                    else
                        DG = SG;
                }

                if (uiSourceBMask && uiDestBMask)
                {
                    if (DestInfo.BlueBitsPerPixel < SourceInfo.BlueBitsPerPixel)   // downsample
                        DB = Shrink<uint16_t>(SB, SourceInfo.BlueBitsPerPixel, DestInfo.BlueBitsPerPixel);
                    else if (DestInfo.BlueBitsPerPixel > SourceInfo.BlueBitsPerPixel)   // upsample
                        DB = Expand<uint16_t>(SB, SourceInfo.BlueBitsPerPixel, DestInfo.BlueBitsPerPixel);
                    else
                        DB = SB;
                }

                if (uiSourceAMask && uiDestAMask)
                {
                    if (DestInfo.AlphaBitsPerPixel < SourceInfo.AlphaBitsPerPixel)   // downsample
                        DA = Shrink<uint16_t>(SA, SourceInfo.AlphaBitsPerPixel, DestInfo.AlphaBitsPerPixel);
                    else if (DestInfo.AlphaBitsPerPixel > SourceInfo.AlphaBitsPerPixel)   // upsample
                        DA = Expand<uint16_t>(SA, SourceInfo.AlphaBitsPerPixel, DestInfo.AlphaBitsPerPixel);
                    else
                        DA = SA;
                }
            }

            // write source to single variable
            const uint32_t Dest = ((uint32_t)(DR & uiDestRMask) << (uint32_t)uiDestRShift) |
                                  ((uint32_t)(DG & uiDestGMask) << (uint32_t)uiDestGShift) |
                                  ((uint32_t)(DB & uiDestBMask) << (uint32_t)uiDestBShift) |
                                  ((uint32_t)(DA & uiDestAMask) << (uint32_t)uiDestAShift);

            for (i = 0; i < DestInfo.BytesPerPixel; i++)
            {
                lpDest[i] = (uint8_t)((Dest >> ((T)i * 8)) & 0xff);
            }
        }

        return true;
    }
}


bool HL2mdl::Convert(uint8_t* lpSource, uint8_t* lpDest, uint32_t uiWidth, uint32_t uiHeight, vtfImageFormatT SourceFormat)
{
    assert(lpSource != 0);
    assert(lpDest != 0);

    assert(SourceFormat >= 0 && SourceFormat < IMAGE_FORMAT_COUNT);

    const vtfImageFormatInfoT& SourceInfo = GetImageFormatInfo(SourceFormat);

    if (!SourceInfo.IsSupported)
    {
        // "Source image format is not supported."
        assert(false);
        return false;
    }

    // Optimize common conversions.
    switch (SourceFormat)
    {
        case IMAGE_FORMAT_RGBA8888:
            memcpy(lpDest, lpSource, ComputeImageSize(uiWidth, uiHeight, 1, IMAGE_FORMAT_RGBA8888));
            return true;

        case IMAGE_FORMAT_RGB888:
        {
            uint8_t* lpLast = lpSource + ComputeImageSize(uiWidth, uiHeight, 1, SourceFormat);

            for (; lpSource < lpLast; lpSource += 3, lpDest += 4)
            {
                lpDest[0] = lpSource[0];
                lpDest[1] = lpSource[1];
                lpDest[2] = lpSource[2];
                lpDest[3] = 255;
            }

            return true;
        }

        case IMAGE_FORMAT_DXT1:
        case IMAGE_FORMAT_DXT1_ONEBITALPHA:
            return DecompressDXT1(lpSource, lpDest, uiWidth, uiHeight);

        case IMAGE_FORMAT_DXT3:
            return DecompressDXT3(lpSource, lpDest, uiWidth, uiHeight);

        case IMAGE_FORMAT_DXT5:
            return DecompressDXT5(lpSource, lpDest, uiWidth, uiHeight);

        default:
            // All other formats must be generically converted.
            break;
   }

    // Do general convertions.
    // convert from one variable order and bit format to another
    const vtfImageFormatInfoT& DestInfo = GetImageFormatInfo(IMAGE_FORMAT_RGBA8888);

    if (SourceInfo.BytesPerPixel == 1)
        return ConvertTemplated<uint8_t>(lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo);

    if (SourceInfo.BytesPerPixel == 2)
        return ConvertTemplated<uint16_t>(lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo);

    if (SourceInfo.BytesPerPixel == 4)
        return ConvertTemplated<uint32_t>(lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo);

    if (SourceInfo.BytesPerPixel == 8)
        return ConvertTemplated<uint64_t>(lpSource, lpDest, uiWidth, uiHeight, SourceInfo, DestInfo);

    return false;
}


vtfFileT::vtfFileT(const std::string& FileName)
    : m_Header(NULL),
      m_ImageBuffer(NULL)
{
    assert(sizeof(vtfImageFormatT) == 4);
    assert(sizeof(vtfResourceT)    == 8);

    GetRawBytes(FileName, m_RawBytes);

    m_Header = (vtfHeaderT*)(&m_RawBytes[0]);

    if (m_RawBytes.Size() < 16)
        throw vtfLoadErrorT("File is much too small.");

    if (memcmp(m_Header->TypeString, "VTF\0", 4) != 0)
        throw vtfLoadErrorT("File ID is not \"VTF\".");

    if (m_RawBytes.Size() < m_Header->HeaderSize)
        throw vtfLoadErrorT("File is too small.");

    if (m_Header->GetVersion() < 70)
        throw vtfLoadErrorT("File version is older than 7.0.");

    if (m_Header->GetVersion() > 75)
        throw vtfLoadErrorT("File version is newer than 7.5.");

    if (m_Header->GetNumResources() > HL2_VTF_MAX_RES)
        throw vtfLoadErrorT("File has unexpectedly many resources.");

    const uint32_t ImageBufferSize = ComputeImageSize(m_Header->Width, m_Header->Height, m_Header->GetDepth(), m_Header->NumMipMaps, m_Header->ImageFormat) * m_Header->GetNumFaces() * m_Header->NumFrames;
    const uint32_t ThumbBufferSize = (m_Header->LowResImageFormat != IMAGE_FORMAT_NONE) ? ComputeImageSize(m_Header->LowResImageWidth, m_Header->LowResImageHeight, 1, m_Header->LowResImageFormat) : 0;

    uint32_t ThumbBufferOffset = m_Header->HeaderSize;
    uint32_t ImageBufferOffset = ThumbBufferOffset + ThumbBufferSize;

    for (uint32_t i = 0; i < m_Header->GetNumResources(); i++)
    {
        const uint32_t VTF_LEGACY_RSRC_THUMB = 0x01;
        const uint32_t VTF_LEGACY_RSRC_IMAGE = 0x30;

        switch (m_Header->GetResource(i).Type)
        {
            case VTF_LEGACY_RSRC_THUMB:
                if (m_Header->LowResImageFormat == IMAGE_FORMAT_NONE)
                    throw vtfLoadErrorT("Unexpected low-res image resource.");

                if (ThumbBufferOffset != 0)
                    throw vtfLoadErrorT("Multiple low-res image resources.");

                ThumbBufferOffset = m_Header->GetResource(i).Data;
                break;

            case VTF_LEGACY_RSRC_IMAGE:
                if (ImageBufferOffset != 0)
                    throw vtfLoadErrorT("Multiple image resources.");

                ImageBufferOffset = m_Header->GetResource(i).Data;
                break;

            default:
                // Ignore any other resource types.
                break;
        }
    }

    // More checks. Note that m_Header->HeaderSize includes the resoures.
    if (m_Header->HeaderSize > m_RawBytes.Size() || ThumbBufferOffset + ThumbBufferSize > m_RawBytes.Size() || ImageBufferOffset + ImageBufferSize > m_RawBytes.Size())
        throw vtfLoadErrorT("Image data exceeds file size.");

    if (m_Header->HeaderSize + ThumbBufferSize + ImageBufferSize != m_RawBytes.Size())
        throw vtfLoadErrorT("Unexpected file size.");

    m_ImageBuffer = &m_RawBytes[0] + ImageBufferOffset;
}


// Computes where in the VTF image the desired data begins.
// Returns the offset in our HiResDataBuffer of the data for an image at the
// desired frame, face, slice and mip level.
// MIP level 0 is the largest moving up to MIP count-1 for the smallest.
// To get the first and largest image, you would use 0, 0, 0, 0.
uint32_t vtfFileT::ComputeDataOffset(uint32_t uiFrame, uint32_t uiFace, uint32_t uiSlice, uint32_t uiMipLevel) const
{
    uint32_t uiOffset = 0;

    const uint32_t uiFrameCount = m_Header->NumFrames;
    const uint32_t uiFaceCount = m_Header->GetNumFaces();
    const uint32_t uiSliceCount = m_Header->GetDepth();
    const uint32_t uiMipCount = m_Header->NumMipMaps;

    if (uiFrame >= uiFrameCount)
    {
        uiFrame = uiFrameCount - 1;
    }

    if (uiFace >= uiFaceCount)
    {
        uiFace = uiFaceCount - 1;
    }

    if (uiSlice >= uiSliceCount)
    {
        uiSlice = uiSliceCount - 1;
    }

    if (uiMipLevel >= uiMipCount)
    {
        uiMipLevel = uiMipCount - 1;
    }

    // Transverse past all frames and faces of each mipmap (up to the requested one).
    for (int32_t i = (int32_t)uiMipCount - 1; i > (int32_t)uiMipLevel; i--)
    {
        uiOffset += ComputeMipmapSize(m_Header->Width, m_Header->Height, m_Header->GetDepth(), i, m_Header->ImageFormat) * uiFrameCount * uiFaceCount;
    }

    const uint32_t uiTemp1 = ComputeMipmapSize(m_Header->Width, m_Header->Height, m_Header->GetDepth(), uiMipLevel, m_Header->ImageFormat);
    const uint32_t uiTemp2 = ComputeMipmapSize(m_Header->Width, m_Header->Height, 1, uiMipLevel, m_Header->ImageFormat);

    // Transverse past requested frames and faces of requested mipmap.
    uiOffset += uiTemp1 * uiFrame * uiFaceCount * uiSliceCount;
    uiOffset += uiTemp1 * uiFace * uiSliceCount;
    uiOffset += uiTemp2 * uiSlice;

    return uiOffset;
}


uint8_t* vtfFileT::GetData(uint32_t uiFrame, uint32_t uiFace, uint32_t uiSlice, uint32_t uiMipmapLevel) const
{
    return m_ImageBuffer + ComputeDataOffset(uiFrame, uiFace, uiSlice, uiMipmapLevel);
}
