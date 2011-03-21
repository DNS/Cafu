/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _FONTWIZARD_FONT_GENERATOR_HPP_
#define _FONTWIZARD_FONT_GENERATOR_HPP_

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
    /// @param FontFile The ttf file to create the Cafu font from.
    /// @param MaterialBaseName The name of the material to be created for this font. Empty string means that no material is created.
    /// @return Whether the font has been successfully generated.
    bool GenerateFont(const wxString& FontFile, bool DebugPNGs=false);

    /// Saves all files related to this font into a directory.
    /// @param Directory The directory into which the font files are saved.
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
