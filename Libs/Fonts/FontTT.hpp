/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONT_TRUETYPE_HPP_INCLUDED
#define CAFU_FONT_TRUETYPE_HPP_INCLUDED

#include <string>
#include <map>
#include "Templates/Array.hpp"

class MaterialT;
namespace MatSys { class RenderMaterialT; }
class TextParserT;


namespace cf
{
    /// A class for rendering fonts that have been created with the Cafu MakeFont tool.
    class TrueTypeFontT
    {
        public:

        struct GlyphInfoT
        {
            GlyphInfoT(TextParserT& TP, const ArrayT<MatSys::RenderMaterialT*>& RenderMaterials);

            float                    BearingX;  ///< The horizontal offset of the bitmap relative to the cursor position.
            float                    BearingY;  ///< The vertical   offset of the bitmap relative to the cursor position (y-axis points up!).
            float                    AdvanceX;  ///< How much the cursor position should be advanced horizontally for rendering the next character.

            int                      Width;     ///< The width  in pixels of the bitmap of this glyph.
            int                      Height;    ///< The height in pixels of the bitmap of this glyph.

            MatSys::RenderMaterialT* RM;        ///< The RenderMaterialT that represents the larger bitmap that this glyph is embedded in. Points into the FontInfoT::RenderMaterials array of its FontInfoT.
            float                    s1;        ///< The s1 tex-coord into the larger bitmap.
            float                    t1;        ///< The t1 tex-coord into the larger bitmap.
            float                    s2;        ///< The s2 tex-coord into the larger bitmap.
            float                    t2;        ///< The t2 tex-coord into the larger bitmap.
        };

        class FontInfoT
        {
            public:

            /// The constructor.
            /// @throws TextParserT::ParseErrorT if the cfont file for the desired pixel size could not be successfully opened and parsed.
            FontInfoT(const std::string& FontName, int SizeInPixels_);

            /// The destructor.
            ~FontInfoT();

            float                            SizeInPixels;          ///< The size of this font in pixels, i.e. 12, 24 or 48.
            float                            Ascender;              ///< The highest coordinate above the baseline in this font face, in pixels.
            float                            Descender;             ///< The lowest  coordinate above the baseline in this font face, in pixels.
            float                            Height;                ///< The height of this font face, in pixels. Usually larger than Ascender-Descender, as this is to be used as the default value for the line spacing ("line gap").
            unsigned long                    CharToGlyphIndex[256]; ///< Maps each ASCII character to the index into GlyphInfos of its related GlyphInfoT.
            ArrayT<GlyphInfoT*>              GlyphInfos;            ///< The GlyphInfos for this font.
            ArrayT< std::map<int, float> >   KerningTable;          ///< The kerning table, expressed as a "half-sparse" matrix.
            ArrayT<MaterialT*>               Materials;             ///< The materials with the larger bitmaps that contain the glyphs.
            ArrayT<MatSys::RenderMaterialT*> RenderMaterials;       ///< The render materials matching the Materials array.


            private:

            FontInfoT(const FontInfoT&);            ///< Use of the Copy    Constructor is not allowed.
            void operator = (const FontInfoT&);     ///< Use of the Assignment Operator is not allowed.
        };

        /// The constructor.
        /// @throws TextParserT::ParseErrorT if one of the related cfont files could not be successfully opened and parsed.
        TrueTypeFontT(const std::string& FontName_);

        /// The destructor.
        ~TrueTypeFontT();

        /// Returns the name of this font.
        const std::string& GetName() const { return FontName; }

        /// Returns the width of string Text at scale Scale in pixels. Useful e.g. for aligning a string to the center or the right of a window.
        /// This method does *not* take newline characters ('\n') into account, the caller should pass pre-parsed strings instead!
        float GetWidth(const std::string& Text, float Scale) const;

        /// Returns how far the highest glyph of this font extends above the baseline ("____") at scale Scale in pixels.
        float GetAscender(float Scale) const;

        /// Returns the default line-spacing of this font at scale Scale in pixels.
        float GetLineSpacing(float Scale) const;

        /// Prints PrintString at (PosX, PosY) in color Color.
        /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices: it's up to the caller to do that!
        void Print(float PosX, float PosY, float Scale, unsigned long Color, const char* PrintString, ...) const;


        private:

        TrueTypeFontT(const TrueTypeFontT&);        ///< Use of the Copy    Constructor is not allowed.
        void operator = (const TrueTypeFontT&);     ///< Use of the Assignment Operator is not allowed.

        /// Returns the FontInfoT suitable for scale Scale (one of the members FontInfoSmall, FontInfoMedium and FontInfoLarge).
        const FontInfoT& GetFontInfo(float Scale) const;

        std::string FontName;
        FontInfoT*  FontInfoSmall;
        FontInfoT*  FontInfoMedium;
        FontInfoT*  FontInfoLarge;
        const float DEFAULT_FONT_SCALE;     ///< This is set to 48.0f, so that a scale of 1.0 corresponds to 48.0 (virtual GUI-) pixels.
    };
}

#endif
