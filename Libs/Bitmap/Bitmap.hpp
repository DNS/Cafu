/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#ifndef _CA_BITMAP_LIB_HPP_
#define _CA_BITMAP_LIB_HPP_

#include "Templates/Array.hpp"

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


/// This class represents a RGBA bitmap.
struct BitmapT
{
    class LoadErrorT {};

    unsigned int     SizeX;
    unsigned int     SizeY;
    ArrayT<uint32_t> Data;


    /// Constructor that creates an empty bitmap.
    BitmapT();

    /// Constructor that creates a bitmap from the file 'FileName'.
    /// The file format is automatically determined (only!) by looking at the suffix of 'FileName'.
    /// Currently supported file formats are BMP (Windows Bitmaps), TGA (TrueVision Targa), JPG (JPEG) and PNG (Portable Network Graphics).
    /// A 'LoadErrorT' exception is thrown if the bitmap could not be loaded for any reason,
    /// including file not found errors, unsupported file formats, or unsupported sub-file formats (like paletted Windows bitmaps).
    ///
    /// Note that the Cafu file system (cf::FileSys) is used to load the image!
    /// That means that an appropriate file system must be mounted in order to be able to create/load an image.
    BitmapT(const char* FileName) /*throw (LoadErrorT)*/;

    /// Constructor that creates a bitmap from the memory pointed to by 'Buffer'.
    BitmapT(unsigned int Width, unsigned int Height, const uint32_t* Buffer=NULL);

    /// Named constructor which returns a built-in "?FILE NOT FOUND" bitmap.
    static BitmapT GetBuiltInFileNotFoundBitmap();


    /// These methods return and accept values in the range from 0 to 255.
    /// The SetPixel() method will clamp to this range if necessary.
    void GetPixel(unsigned int x, unsigned int y, int& r, int& g, int& b) const;
    void SetPixel(unsigned int x, unsigned int y, int  r, int  g, int  b);
    void GetPixel(unsigned int x, unsigned int y, int& r, int& g, int& b, int& a) const;
    void SetPixel(unsigned int x, unsigned int y, int  r, int  g, int  b, int  a);

    /// These methods return and accept values in the range from 0.0 to 1.0.
    /// The SetPixel() method will clamp to this range if necessary.
    void GetPixel(unsigned int x, unsigned int y, float& r, float& g, float& b) const;
    void SetPixel(unsigned int x, unsigned int y, float  r, float  g, float  b);
    void GetPixel(unsigned int x, unsigned int y, float& r, float& g, float& b, float& a) const;
    void SetPixel(unsigned int x, unsigned int y, float  r, float  g, float  b, float  a);


    /// This method applies the gamma value 'Gamma' to this bitmap.
    void ApplyGamma(float Gamma);

    /// Scales the bitmap to the new dimensions NewSizeX and NewSizeY.
    /// You may specify 0 for NewSizeX or NewSizeY in order to keep that direction unchanged.
    void Scale(unsigned int NewSizeX, unsigned int NewSizeY);

    /// This functions computes a paletted (8 BPP) image from this bitmap,
    /// using the NeuQuant Neural-Net quantization algorithm (c) by Anthony Dekker, 1994.
    /// See "Kohonen neural networks for optimal colour quantization"
    /// in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367.
    /// for a discussion of the algorithm.
    /// See also  http://members.ozemail.com.au/~dekker/NEUQUANT.HTML
    /// The returned pointer contains 256*3 palette bytes (256 RGB color triples),
    /// followed by 'SizeX*SizeY' bytes of image indices into the palette.
    /// It is the callers responsibility to call 'delete[]' on the returned pointer.
    char* GetPalettedImage() const;

    /// Writes the contents of this bitmap into a file with name 'FileName'.
    /// The file format is automatically determined by looking at the suffix of 'FileName'.
    /// Currently supported file formats are BMP (Windows Bitmaps), JPG (JPEG) and PNG (Portable Network Graphics).
    /// NOTE: When saving the bitmap in BMP or JPG file format, the alpha channel is lost!
    /// Returns 'true' on success, 'false' on failure.
    /// Reasons for failure include unknown file formats (e.g. passing "hello.tga" for 'FileName'),
    /// and problems with the file itself (like invalid path, no disk space left, and so on).
    bool SaveToDisk(const char* FileName) const;
};

#endif
