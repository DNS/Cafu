/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONTWIZARD_FONT_GENERATOR_HPP_INCLUDED
#define CAFU_FONTWIZARD_FONT_GENERATOR_HPP_INCLUDED

#include <ft2build.h>
#include FT_FREETYPE_H

#include "wx/wx.h"

#include "Templates/Array.hpp"


struct GlyphInfoT;
class RectBitmapAllocatorT;
struct BitmapT;


class FontGeneratorT
{
    public:

    struct FontDataT
    {
        FT_Face               FTFontFace;
        RectBitmapAllocatorT* RBA;
        ArrayT<unsigned long> CharToGlyphInfoNr;
        ArrayT<GlyphInfoT>    GlyphInfos;
    };


    FontGeneratorT();
    ~FontGeneratorT();

    /// Generates a Cafu font from a .ttf font file.
    /// @param FontFile    The ttf file to create the Cafu font from.
    /// @param DebugPNGs   Add debug information to the created texture images?
    /// @return Whether the font has been successfully generated.
    bool GenerateFont(const wxString& FontFile, bool DebugPNGs=false);

    /// Saves all files related to this font into a directory.
    /// @param Directory          The directory into which the font files are saved.
    /// @param MaterialBaseName   The name of the material to be created for this font. Empty string means that no material is created.
    void SaveFont(const wxString& Directory, const wxString& MaterialBaseName="") const;

    /// Gets the number of sizes created for this font.
    /// @return Number of font sizes.
    unsigned long GetNrOfSizes() const;

    /// Gets an array of font bitmaps for the passed font size.
    /// @param SizeNr The size number for which the bitmaps should be returned (if this size number is not existant an empty array is returned).
    /// @return Array of all font bitmaps for this font size.
    ArrayT<BitmapT*> GetBitmaps(unsigned long SizeNr) const;


    private:

    FT_Library m_FTLib;
    bool m_FTInited;
    bool m_DebugPNGs;

    ArrayT<FontDataT> m_FontData; ///< Contains font data for different font sizes.


    void ProcessNewGlyph(FT_Face& Face, GlyphInfoT& Glyph, RectBitmapAllocatorT& RBA);
    void ClearFontData();

    FontGeneratorT(const FontGeneratorT&);   ///< Use of the Copy    Constructor is not allowed.
    void operator = (const FontGeneratorT&); ///< Use of the Assignment Operator is not allowed.
};

#endif
