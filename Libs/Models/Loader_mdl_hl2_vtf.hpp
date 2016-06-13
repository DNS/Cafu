/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_HL2_MDL_VTF_TEXTURE_FILE_HPP_INCLUDED
#define CAFU_HL2_MDL_VTF_TEXTURE_FILE_HPP_INCLUDED

#include "Templates/Array.hpp"
#include <stdexcept>

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif

#define HL2_VTF_MAX_RES 32


namespace HL2mdl
{
    /// Possible image formats that can occur in VTF files.
    /// Named indices into an internal array of vtfImageFormatInfoT.
    enum vtfImageFormatT
    {
        IMAGE_FORMAT_NONE = -1,
        IMAGE_FORMAT_RGBA8888 = 0,          ///< Red, Green, Blue, Alpha - 32 bpp
        IMAGE_FORMAT_ABGR8888,              ///< Alpha, Blue, Green, Red - 32 bpp
        IMAGE_FORMAT_RGB888,                ///< Red, Green, Blue - 24 bpp
        IMAGE_FORMAT_BGR888,                ///< Blue, Green, Red - 24 bpp
        IMAGE_FORMAT_RGB565,                ///< Red, Green, Blue - 16 bpp
        IMAGE_FORMAT_I8,                    ///< Luminance - 8 bpp
        IMAGE_FORMAT_IA88,                  ///< Luminance, Alpha - 16 bpp
        IMAGE_FORMAT_P8,                    ///< Paletted - 8 bpp
        IMAGE_FORMAT_A8,                    ///< Alpha - 8 bpp
        IMAGE_FORMAT_RGB888_BLUESCREEN,     ///< Red, Green, Blue - 24 bpp, (0, 0, 255) is translucent
        IMAGE_FORMAT_BGR888_BLUESCREEN,     ///< Blue, Green, Red - 24 bpp, (0, 0, 255) is translucent
        IMAGE_FORMAT_ARGB8888,              ///< Alpha, Red, Green, Blue - 32 bpp
        IMAGE_FORMAT_BGRA8888,              ///< Blue, Green, Red, Alpha - 32 bpp
        IMAGE_FORMAT_DXT1,                  ///< DXT1 compressed format - 4 bpp
        IMAGE_FORMAT_DXT3,                  ///< DXT3 compressed format - 8 bpp
        IMAGE_FORMAT_DXT5,                  ///< DXT5 compressed format - 8 bpp
        IMAGE_FORMAT_BGRX8888,              ///< Blue, Green, Red, Unused - 32 bpp
        IMAGE_FORMAT_BGR565,                ///< Blue, Green, Red - 16 bpp
        IMAGE_FORMAT_BGRX5551,              ///< Blue, Green, Red, Unused - 16 bpp
        IMAGE_FORMAT_BGRA4444,              ///< Red, Green, Blue, Alpha - 16 bpp
        IMAGE_FORMAT_DXT1_ONEBITALPHA,      ///< DXT1 compressed format with 1-bit alpha - 4 bpp
        IMAGE_FORMAT_BGRA5551,              ///< Blue, Green, Red, Alpha - 16 bpp
        IMAGE_FORMAT_UV88,                  ///< 2 channel format for DuDv/Normal maps - 16 bpp
        IMAGE_FORMAT_UVWQ8888,              ///< 4 channel format for DuDv/Normal maps - 32 bpp
        IMAGE_FORMAT_RGBA16161616F,         ///< Red, Green, Blue, Alpha - 64 bpp
        IMAGE_FORMAT_RGBA16161616,          ///< Red, Green, Blue, Alpha signed with mantissa - 64 bpp
        IMAGE_FORMAT_UVLX8888,              ///< 4 channel format for DuDv/Normal maps - 32 bpp
        IMAGE_FORMAT_R32F,                  ///< Luminance - 32 bpp
        IMAGE_FORMAT_RGB323232F,            ///< Red, Green, Blue - 96 bpp
        IMAGE_FORMAT_RGBA32323232F,         ///< Red, Green, Blue, Alpha - 128 bpp
        IMAGE_FORMAT_NV_DST16,
        IMAGE_FORMAT_NV_DST24,
        IMAGE_FORMAT_NV_INTZ,
        IMAGE_FORMAT_NV_RAWZ,
        IMAGE_FORMAT_ATI_DST16,
        IMAGE_FORMAT_ATI_DST24,
        IMAGE_FORMAT_NV_NULL,
        IMAGE_FORMAT_ATI1N,
        IMAGE_FORMAT_ATI2N,
     /* IMAGE_FORMAT_X360_DST16,            ///< XBox formats.
        IMAGE_FORMAT_X360_DST24,
        IMAGE_FORMAT_X360_DST24F,
        IMAGE_FORMAT_LINEAR_BGRX8888,       ///< Blue, Green, Red, Unused - 32 bpp
        IMAGE_FORMAT_LINEAR_RGBA8888,       ///< Red, Green, Blue, Alpha - 32 bpp
        IMAGE_FORMAT_LINEAR_ABGR8888,       ///< Alpha, Blue, Green, Red - 32 bpp
        IMAGE_FORMAT_LINEAR_ARGB8888,       ///< Alpha, Red, Green, Blue - 32 bpp
        IMAGE_FORMAT_LINEAR_BGRA8888,       ///< Blue, Green, Red, Alpha - 32 bpp
        IMAGE_FORMAT_LINEAR_RGB888,         ///< Red, Green, Blue - 24 bpp
        IMAGE_FORMAT_LINEAR_BGR888,         ///< Blue, Green, Red - 24 bpp
        IMAGE_FORMAT_LINEAR_BGRX5551,       ///< Blue, Green, Red, Unused - 16 bpp
        IMAGE_FORMAT_LINEAR_I8,             ///< Luminance - 8 bpp
        IMAGE_FORMAT_LINEAR_RGBA16161616,   ///< Red, Green, Blue, Alpha signed with mantissa - 64 bpp
        IMAGE_FORMAT_LE_BGRX8888,           ///< Blue, Green, Red, Unused - 32 bpp
        IMAGE_FORMAT_LE_BGRA8888,           ///< Blue, Green, Red, Alpha - 32 bpp */
        IMAGE_FORMAT_COUNT
    };


    /// The vtfImageFormatInfoT struct provides information about VTF image formats.
    struct vtfImageFormatInfoT
    {
        typedef void (*TransformProc)(uint16_t& R, uint16_t& G, uint16_t& B, uint16_t& A);

        const char*     Name;               ///< Enumeration text equivalent.
        uint32_t        BitsPerPixel;       ///< Bits per pixel.
        uint32_t        BytesPerPixel;      ///< Bytes per pixel.
        uint32_t        RedBitsPerPixel;    ///< Red bits per pixel.
        uint32_t        GreenBitsPerPixel;  ///< Green bits per pixel.
        uint32_t        BlueBitsPerPixel;   ///< Blue bits per pixel.
        uint32_t        AlphaBitsPerPixel;  ///< Alpha bits per pixel.
        int32_t         IndexRed;           ///< Red index.
        int32_t         IndexGreen;         ///< Green index.
        int32_t         IndexBlue;          ///< Blue index.
        int32_t         IndexAlpha;         ///< Alpha index.
        bool            IsCompressed;       ///< Is compressed (DXT).
        bool            IsSupported;        ///< Format is supported by the implementation.
        TransformProc   pToTransform;       ///< Custom transform to function.
        TransformProc   pFromTransform;     ///< Custom transform from function.
        vtfImageFormatT Format;
    };


    /// Maps a vtfImageFormatT constant to an vtfImageFormatInfoT instance.
    const vtfImageFormatInfoT& GetImageFormatInfo(vtfImageFormatT ImageFormat);

    /// Convert an image from any format to IMAGE_FORMAT_RGBA8888.
    ///
    /// \param lpSource is a pointer to the source image data.
    /// \param lpDest is a pointer to the buffer for the converted data.
    /// \param Width is the width of the source image in pixels.
    /// \param Height is the height of the source image in pixels.
    /// \param SourceFormat is the image format you are converting from.
    /// \return true on successful conversion, otherwise false.
    bool Convert(uint8_t* lpSource, uint8_t* lpDest, uint32_t Width, uint32_t Height, vtfImageFormatT SourceFormat);


    /// Constants used for the Flags member in the file header.
    enum vtfFlagsT
    {
        TEXTUREFLAGS_POINTSAMPLE                              = 0x00000001,
        TEXTUREFLAGS_TRILINEAR                                = 0x00000002,
        TEXTUREFLAGS_CLAMPS                                   = 0x00000004,
        TEXTUREFLAGS_CLAMPT                                   = 0x00000008,
        TEXTUREFLAGS_ANISOTROPIC                              = 0x00000010,
        TEXTUREFLAGS_HINT_DXT5                                = 0x00000020,
        TEXTUREFLAGS_DEPRECATED_NOCOMPRESS                    = 0x00000040,
        TEXTUREFLAGS_NORMAL                                   = 0x00000080,
        TEXTUREFLAGS_NOMIP                                    = 0x00000100,
        TEXTUREFLAGS_NOLOD                                    = 0x00000200,
        TEXTUREFLAGS_MINMIP                                   = 0x00000400,
        TEXTUREFLAGS_PROCEDURAL                               = 0x00000800,
        TEXTUREFLAGS_ONEBITALPHA                              = 0x00001000,
        TEXTUREFLAGS_EIGHTBITALPHA                            = 0x00002000,
        TEXTUREFLAGS_ENVMAP                                   = 0x00004000,
        TEXTUREFLAGS_RENDERTARGET                             = 0x00008000,
        TEXTUREFLAGS_DEPTHRENDERTARGET                        = 0x00010000,
        TEXTUREFLAGS_NODEBUGOVERRIDE                          = 0x00020000,
        TEXTUREFLAGS_SINGLECOPY                               = 0x00040000,
        TEXTUREFLAGS_DEPRECATED_ONEOVERMIPLEVELINALPHA        = 0x00080000,
        TEXTUREFLAGS_DEPRECATED_PREMULTCOLORBYONEOVERMIPLEVEL = 0x00100000,
        TEXTUREFLAGS_DEPRECATED_NORMALTODUDV                  = 0x00200000,
        TEXTUREFLAGS_DEPRECATED_ALPHATESTMIPGENERATION        = 0x00400000,
        TEXTUREFLAGS_NODEPTHBUFFER                            = 0x00800000,
        TEXTUREFLAGS_DEPRECATED_NICEFILTERED                  = 0x01000000,
        TEXTUREFLAGS_CLAMPU                                   = 0x02000000,
        TEXTUREFLAGS_VERTEXTEXTURE                            = 0x04000000,
        TEXTUREFLAGS_SSBUMP                                   = 0x08000000,
        TEXTUREFLAGS_DEPRECATED_UNFILTERABLE_OK               = 0x10000000,
        TEXTUREFLAGS_BORDER                                   = 0x20000000,
        TEXTUREFLAGS_DEPRECATED_SPECVAR_RED                   = 0x40000000,
        TEXTUREFLAGS_DEPRECATED_SPECVAR_ALPHA                 = 0x80000000,
        TEXTUREFLAGS_LAST                                     = 0x20000000
    };


#if defined(_MSC_VER)
    #pragma pack(push, 1)
    #define GCC_PACKED
#elif defined(__GNUG__)
    #define GCC_PACKED __attribute__ ((packed))
#endif


    struct vtfResourceT
    {
        union
        {
            uint32_t Type;
            struct
            {
                uint8_t ID[3];  ///< Resource ID
                uint8_t Flags;  ///< Resource flags
            };
        };

        /// Depending on the type, this can be the immediate resource data, e.g. a CRC,
        /// or the offset from start of the file to an uint32_t with the number of bytes, followed by the bytes.
        uint32_t Data;
    } GCC_PACKED;


    /// This is the header for VTF files, covering all 7.x versions up to 7.5 (current).
    class vtfHeaderT
    {
        public:

        char            TypeString[4];      ///< "VTF\0"
        uint32_t        Version[2];
        uint32_t        HeaderSize;         ///< Size of the header struct (currently 80 bytes (+ resources(??))
        uint16_t        Width;              ///< Width at the largest MipMap level
        uint16_t        Height;             ///< Height at the largest MipMap level
        uint32_t        Flags;              ///< As defined in vtfFlagsT.
        uint16_t        NumFrames;          ///< It's a small movie if NumFrames > 1.
        uint16_t        StartFrame;
        uint8_t         Padding0[4];        ///< for 16 byte alignment
        float           Reflectivity[3];
        uint8_t         Padding1[4];        ///< for 16 byte alignment
        float           BumpScale;
        vtfImageFormatT ImageFormat;
        uint8_t         NumMipMaps;         ///< Number of MipMap levels (including the largest image)
        vtfImageFormatT LowResImageFormat;
        uint8_t         LowResImageWidth;
        uint8_t         LowResImageHeight;

        /// Returns the file version times ten, e.g. 75 for the current version.
        uint32_t GetVersion() const
        {
            return Version[0] * 10 + Version[1];
        }

        /// Returns the number of faces in the file (usually 1).
        uint32_t GetNumFaces() const
        {
            // Cubemaps have 6 or 7 faces, others just 1.
            if ((Flags & TEXTUREFLAGS_ENVMAP) == 0)
                return 1;

            // Spheremaps were only supported from version 7.1 to 7.4.
            if (StartFrame != 0xffff && 71 <= GetVersion() && GetVersion() <= 74)
                return 7;

            return 6;
        }

        /// Returns the depth (number of z-slices) of the image in pixels.
        uint16_t GetDepth() const
        {
            return GetVersion() < 72 ? 1 : m_Depth;
        }

        uint32_t GetNumResources() const
        {
            return GetVersion() < 73 ? 0 : m_ResourceCount;
        }

        const vtfResourceT& GetResource(uint32_t i) const
        {
            assert(i < GetNumResources());
            return m_Resources[i];
        }


        private:

        // These fields are only available in newer versions of the file format.
        // Use the Get*() methods in order to safely access their values.
        uint16_t       m_Depth;           ///< Depth at the largest MipMap level.
        uint8_t        m_Padding2[3];
        uint32_t       m_ResourceCount;   ///< Number of image resources.
        uint8_t        m_Padding3[8];

        // --- Up to here, 80 header bytes. ---
        vtfResourceT   m_Resources[HL2_VTF_MAX_RES];
    } GCC_PACKED;


#if defined(_MSC_VER)
    #pragma pack(pop)
#endif
#undef GCC_PACKED


    /// A class that is thrown on VTF load errors.
    class vtfLoadErrorT : public std::runtime_error
    {
        public:

        vtfLoadErrorT(const std::string& Message)
            : std::runtime_error(Message)
        {
        }
    };


    /**
    This class represents a VTF image file as documented at https://developer.valvesoftware.com/wiki/VTF

    In summary, the disk file format for VTF files is:
      - VTF header (80 bytes)
      - low-res image data
      - image data

    The image data is stored as follows:
      - for each mipmap level (starting with the smallest and getting larger)
          - for each frame
              - for each face (for cubemaps)
                  - for each z-slice
                      - image data
    */
    class vtfFileT
    {
        public:

        vtfFileT(const std::string& FileName);

        const vtfHeaderT* GetHeader() const { return m_Header; }

        /// Returns a pointer to the image data for a given frame, face, z-slice and MIP level.
        /// Frames start at index 0 for the first frame. Faces start at index 0
        /// for the first face. Cubemaps have 6 faces, others only 1.
        /// MIP levels start at index 0 for the largest image moving down in size.
        uint8_t* GetData(uint32_t Frame, uint32_t Face, uint32_t Slice, uint32_t MipmapLevel) const;


        private:

        uint32_t ComputeDataOffset(uint32_t Frame, uint32_t Face, uint32_t Slice, uint32_t MipmapLevel) const;

        ArrayT<uint8_t> m_RawBytes;
        vtfHeaderT*     m_Header;
        uint8_t*        m_ImageBuffer;
    };
}

#endif
