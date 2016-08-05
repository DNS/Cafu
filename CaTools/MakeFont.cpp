/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <limits>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Bitmap/Bitmap.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "Templates/Array.hpp"

#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER<1300)
    #define vsnprintf _vsnprintf
#endif


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;


bool DebugPNGs=false;


struct GlyphInfoT
{
    unsigned long GlyphIndex;

    float BearingX;     ///< The horizontal offset of the bitmap relative to the cursor position.
    float BearingY;     ///< The vertical   offset of the bitmap relative to the cursor position (y-axis points up!).
    float AdvanceX;     ///< How much the cursor position should be advanced horizontally for rendering the next character.

    int   Width;        ///< The width  in pixels of the bitmap of this glyph.
    int   Height;       ///< The height in pixels of the bitmap of this glyph.

    int   BitmapNr;     ///< The index of the larger bitmap this glyphs bitmap is embedded in.
    float s1;           ///< The s1 tex-coord into the larger bitmap.
    float t1;           ///< The t1 tex-coord into the larger bitmap.
    float s2;           ///< The s2 tex-coord into the larger bitmap.
    float t2;           ///< The t2 tex-coord into the larger bitmap.
};


std::string va(const char* FormatString, ...)
{
    va_list ArgList;
    char    Buffer[1024];

    if (!FormatString) return "";

    va_start(ArgList, FormatString);
    vsnprintf(Buffer, 1024-1, FormatString, ArgList);
    Buffer[1024-1]=0;
    va_end(ArgList);

    return Buffer;
}


/// This class "allocates" rectangular areas in larger bitmaps.
class RectBitmapAllocatorT
{
    public:

    RectBitmapAllocatorT(uint32_t BitmapInitColor, unsigned long BitmapSizeS=256, unsigned long BitmapSizeT=256)
        : BITMAP_SIZE_S(BitmapSizeS),
          BITMAP_SIZE_T(BitmapSizeT),
          BITMAP_INIT_COLOR(BitmapInitColor)
    {
        for (unsigned long s=0; s<BITMAP_SIZE_S; s++)
            BitmapAllocated.PushBack(BITMAP_SIZE_T);
    }

    ~RectBitmapAllocatorT()
    {
        for (unsigned long BitmapNr=0; BitmapNr<Bitmaps.Size(); BitmapNr++)
            delete Bitmaps[BitmapNr];
    }

    bool Allocate(unsigned long SizeS, unsigned long SizeT, unsigned long& BitmapNr, unsigned long& PosS, unsigned long& PosT)
    {
        if (SizeS>BITMAP_SIZE_S) return false;
        if (SizeT>BITMAP_SIZE_T) return false;

        BitmapNr=Bitmaps.Size()-1;
        if (AllocateHelper(SizeS, SizeT, PosS, PosT)) return true;

        BitmapT* Bitmap=new BitmapT(BITMAP_SIZE_S, BITMAP_SIZE_T);
        for (unsigned long i=0; i<Bitmap->Data.Size(); i++)
            Bitmap->Data[i]=BITMAP_INIT_COLOR;
        Bitmaps.PushBack(Bitmap);

        for (unsigned long s=0; s<BITMAP_SIZE_S; s++)
            BitmapAllocated[s]=0;

        BitmapNr=Bitmaps.Size()-1;
        return AllocateHelper(SizeS, SizeT, PosS, PosT);
    }


    const unsigned long BITMAP_SIZE_S;
    const unsigned long BITMAP_SIZE_T;
    const uint32_t      BITMAP_INIT_COLOR;
    ArrayT<BitmapT*>    Bitmaps;


    private:

    RectBitmapAllocatorT(const RectBitmapAllocatorT&);      ///< Use of the Copy Constructor    is not allowed.
    void operator = (const RectBitmapAllocatorT&);          ///< Use of the Assignment Operator is not allowed.

    bool AllocateHelper(unsigned long SizeS, unsigned long SizeT, unsigned long& PosS, unsigned long& PosT)
    {
        unsigned long Best=BITMAP_SIZE_T;

        for (unsigned long s=0; s<=BITMAP_SIZE_S-SizeS; s++)
        {
            unsigned long Best2=0;
            unsigned long s2;

            for (s2=0; s2<SizeS; s2++)
            {
                if (BitmapAllocated[s+s2]>=Best) break;
                if (BitmapAllocated[s+s2]>Best2) Best2=BitmapAllocated[s+s2];
            }

            if (s2==SizeS)
            {
                // Gültige Position gefunden
                PosS=s;
                PosT=Best=Best2;
            }
        }

        if (Best+SizeT>BITMAP_SIZE_T) return false;

        for (unsigned long s=0; s<SizeS; s++) BitmapAllocated[PosS+s]=Best+SizeT;
        return true;
    }

    ArrayT<unsigned long> BitmapAllocated;
};


void ProcessNewGlyph(FT_Face& Face, GlyphInfoT& Glyph, RectBitmapAllocatorT& RBA)
{
    unsigned long PosS=0;
    unsigned long PosT=0;

    // IMPORTANT:
    // Our bitmaps are rendered by the Cafu engine using bilinear filtering.
    // Therefore, we add a safety margin (a frame) of 1 pixel thickness around each glyph bitmap,
    // i.e. we increase its width and height by two pixels.
    // This safety margin is transparently added here in this function -- no user code is supposed to ever become aware of it!
    unsigned long BitmapNr;
    if (!RBA.Allocate(Face->glyph->bitmap.width+2, Face->glyph->bitmap.rows+2, BitmapNr, PosS, PosT))
    {
        std::cout << "WARNING: Unable to process glyph!\n";

        if (RBA.Bitmaps.Size()==0) return;      // That should never happen (but *could*, e.g. if the glpyh bitmap is larger than the bitmaps of the RBA).
        BitmapNr=0;
        PosS=0;
        PosT=0;
    }

    BitmapT* Bitmap=RBA.Bitmaps[BitmapNr];

    // Copy the glyphs bitmap into the larger bitmap.
    // The cast to `unsigned int` is needed because in FreeType, the type of the `rows` and
    // `width` fields was changed in version 2.5.4 (see its docs/CHANGES file for details)
    // and some Linux distros still ship with FreeType 2.5.3 or older, whereas others ship
    // with 2.5.4 or newer.
    for (unsigned int y = 0; y < (unsigned int)Face->glyph->bitmap.rows; y++)
        for (unsigned int x = 0; x < (unsigned int)Face->glyph->bitmap.width; x++)
        {
            const unsigned char GrayValue=Face->glyph->bitmap.buffer[x+y*Face->glyph->bitmap.pitch];

            if (DebugPNGs) Bitmap->SetPixel(PosS+x+1, PosT+y+1, GrayValue, GrayValue, GrayValue, 255);
                      else Bitmap->SetPixel(PosS+x+1, PosT+y+1, 255, 255, 255, GrayValue);
        }


    // Fill out the Glyph structure.
    Glyph.BearingX=float(Face->glyph->bitmap_left-1);       // The -1 is required in order to compensate for our "safety frame".
    Glyph.BearingY=float(Face->glyph->bitmap_top+1);        // The +1 is required in order to compensate for our "safety frame". (Note that the bearing is relative to the *UPPER* left corner of the bitmap, not the lower left.)
    Glyph.AdvanceX=float(Face->glyph->advance.x)/64.0f;
 // Glyph.AdvanceX=float(Face->glyph->linearHoriAdvance)/65536.0f;

    Glyph.Width =Face->glyph->bitmap.width+2;
    Glyph.Height=Face->glyph->bitmap.rows +2;

    Glyph.BitmapNr=BitmapNr;
    Glyph.s1=float(PosS)/float(RBA.BITMAP_SIZE_S);
    Glyph.t1=float(PosT)/float(RBA.BITMAP_SIZE_T);
    Glyph.s2=float(PosS+Face->glyph->bitmap.width+2)/float(RBA.BITMAP_SIZE_S);
    Glyph.t2=float(PosT+Face->glyph->bitmap.rows +2)/float(RBA.BITMAP_SIZE_T);
}


int Usage()
{
    std::cout << "\nUSAGE: MakeFont FontFileName [OPTIONS]\n";
    std::cout << "\n";
    std::cout << "OPTIONS:\n";
    std::cout << "-m=MaterialBaseName\n";
    std::cout << "   If this is given, a Font.cmat material definition file will be created\n";
    std::cout << "   automatically, using MaterialBaseName as a basis for creating the material\n";
    std::cout << "   names.\n";
    std::cout << "\n";
    std::cout << "-debug\n";
    std::cout << "   Creates the FontImage_X_Y.png files with false colors.\n";
    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << "   MakeFont c:\\WINNT\\Fonts\\arial.ttf -m=Arial\n";
    return 1;
}


int main(int ArgC, const char* ArgV[])
{
    // Output header.
    std::cout << "The Cafu Font Maker, version " << __DATE__ << ".\n\n";
    std::cout << "Portions of this software are copyright (c) 2006 The FreeType Project\n(www.freetype.org). All rights reserved.\n\n";

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    // Parse the command-line arguments.
    std::string FontFileName="";
    std::string MaterialBaseName="";

    for (int ArgNr=1; ArgNr<ArgC; ArgNr++)
    {
             if (strncmp(ArgV[ArgNr], "-m=" , 3)==0) MaterialBaseName=ArgV[ArgNr]+3;
        else if (strcmp (ArgV[ArgNr], "-debug" )==0) DebugPNGs=true;
        else if (ArgV[ArgNr][0]=='-') return Usage();
        else if (FontFileName=="") FontFileName=ArgV[ArgNr];
        else return Usage();
    }

    if (FontFileName=="") return Usage();


    // Init the FreeType library.
    FT_Library ftLib;
    if (FT_Init_FreeType(&ftLib)!=0)
    {
        std::cout << "Error: Could not init the FreeType library.\n";
        return 1;
    }


    // Load the font face.
    FT_Face ftFontFace;
    if (FT_New_Face(ftLib, FontFileName.c_str(), 0, &ftFontFace)!=0)
    {
        std::cout << "Error: Could not load the font face from \"" << FontFileName << "\".\n";
        return 1;
    }


    const int PointSizes[]= { 12, 24, 48 };     // The sizes in points (1/72th of an inch).

    for (unsigned long SizeNr=0; SizeNr<3; SizeNr++)
    {
        const int SizeInPoints=PointSizes[SizeNr];

        // Set the character size to SizeInPoints points at 72 DPI, which implies that the size in pixels is the same as SizeInPoints.
        if (FT_Set_Char_Size(ftFontFace, SizeInPoints*64, 0, 72, 72)!=0)
        {
            std::cout << "Error: Could not set the character size to " << SizeInPoints << "pt.\n";
            continue;
        }

        RectBitmapAllocatorT  RBA(DebugPNGs ? 0xFFFF00FF : 0x00FFFFFF);     // The default color is "invisible white", that is, RGB=1 and A=0.
        ArrayT<unsigned long> CharToGlyphInfoNr;
        ArrayT<GlyphInfoT>    GlyphInfos;

        for (unsigned long CharNr=0; CharNr<256; CharNr++)
        {
         // const unsigned long GlyphIndex=(CharNr<128) ? FT_Get_Char_Index(ftFontFace, CharNr) : 0;
            const unsigned long GlyphIndex=FT_Get_Char_Index(ftFontFace, CharNr);

            // Determine if we already have a GlyphInfoT object with that GlyphIndex, that is, determine whether GlyphIndex has occurred before.
            unsigned long GlyphInfoNr;

            for (GlyphInfoNr=0; GlyphInfoNr<GlyphInfos.Size(); GlyphInfoNr++)
                if (GlyphInfos[GlyphInfoNr].GlyphIndex==GlyphIndex)
                    break;

            CharToGlyphInfoNr.PushBack(GlyphInfoNr);

            // If we already have such a GlyphInfoT, we're done with this character.
            if (GlyphInfoNr<GlyphInfos.Size()) continue;

            // Load the glyph at GlyphIndex into the glyph slot of ftFontFace (i.e. ftFontFace->glyph).
            if (FT_Load_Glyph(ftFontFace, GlyphIndex, FT_LOAD_RENDER)!=0)
            {
                std::cout << "Error: Could not obtain the glyph at index " << GlyphIndex << ".\n";
                continue;   // TODO: Is this proper error handling???
            }


            // Insert and store the new glyph info.
            GlyphInfos.PushBackEmpty();
            GlyphInfos[GlyphInfos.Size()-1].GlyphIndex=GlyphIndex;

            ProcessNewGlyph(ftFontFace, GlyphInfos[GlyphInfos.Size()-1], RBA);
        }


        // Save all resulting texture maps to disk.
        for (unsigned long TexMapNr=0; TexMapNr<RBA.Bitmaps.Size(); TexMapNr++)
            RBA.Bitmaps[TexMapNr]->SaveToDisk(va("FontImage_%i_%lu.png", SizeInPoints, TexMapNr).c_str());


        // Save the resulting font description to disk.
        std::ofstream FontFile(va("FontDescr_%i.cfont", SizeInPoints).c_str(), std::ios::out);
        // if (FontFile.bad()) ...

        const int sigdigits=std::numeric_limits<float>::digits10;
        FontFile << std::setprecision(sigdigits);

        FontFile << "// Values that are not per-glyph, but global to the font face at the current scale (" << SizeInPoints << " pixels).\n";
        FontFile << "// The values are: ascender, descender and height.\n";
        FontFile << "global " << float(ftFontFace->size->metrics.ascender )/64.0f << " "
                              << float(ftFontFace->size->metrics.descender)/64.0f << " "
                              << float(ftFontFace->size->metrics.height   )/64.0f << "\n";
        FontFile << "\n";

        FontFile << "// For each of the " << CharToGlyphInfoNr.Size() << " characters, record the index into the glyph infos below.\n";
        for (unsigned long CharNr=0; CharNr<CharToGlyphInfoNr.Size(); CharNr++)
        {
            if (CharNr>0)
            {
                if ((CharNr % 16)==0) FontFile << "\n"; else FontFile << " ";
            }

            FontFile << CharToGlyphInfoNr[CharNr];
        }
        FontFile << "\n\n";

        FontFile << "// The glyphs below refer to larger bitmaps, represented by the following materials.\n";
        for (unsigned long TexMapNr=0; TexMapNr<RBA.Bitmaps.Size(); TexMapNr++)
            FontFile << "matname Fonts/" << MaterialBaseName << "_" << SizeInPoints << "_" << TexMapNr << "\n";
        FontFile << "\n";

        FontFile << "// The glyphs (num, bearing x, bearing y, advance x, width, height, bitmap num, x1, y1, x2, y2).\n";
        for (unsigned long GINr=0; GINr<GlyphInfos.Size(); GINr++)
        {
            const GlyphInfoT& Glyph=GlyphInfos[GINr];

            FontFile << "glyph " << GINr
                << " " << Glyph.BearingX
                << " " << Glyph.BearingY
                << " " << Glyph.AdvanceX
                << " " << Glyph.Width
                << " " << Glyph.Height
                << " " << Glyph.BitmapNr
                << " " << Glyph.s1
                << " " << Glyph.t1
                << " " << Glyph.s2
                << " " << Glyph.t2
                << "\n";
        }
        FontFile << "\n";

        FontFile << "// The kerning table.\n";
        for (unsigned long GI1Nr=0; GI1Nr<GlyphInfos.Size(); GI1Nr++)
            for (unsigned long GI2Nr=0; GI2Nr<GlyphInfos.Size(); GI2Nr++)
            {
                const GlyphInfoT& gi1=GlyphInfos[GI1Nr];
                const GlyphInfoT& gi2=GlyphInfos[GI2Nr];

                FT_Vector KerningVec;

                if (FT_Get_Kerning(ftFontFace, gi1.GlyphIndex, gi2.GlyphIndex, FT_KERNING_DEFAULT /*FT_KERNING_UNFITTED*/, &KerningVec)!=0) continue;
                if (KerningVec.x==0) continue;

                const float HorKerning=float(KerningVec.x)/64.0f;

                FontFile << "k " << GI1Nr << " " << GI2Nr << " " << HorKerning << "\n";
            }


        // Auto-create a materials definition file if MaterialBaseName was given.
        if (MaterialBaseName!="")
        {
            std::ofstream CMatFile("Font.cmat", SizeNr==0 ? std::ios::out : std::ios::out | std::ios::app);

            for (unsigned long TexMapNr=0; TexMapNr<RBA.Bitmaps.Size(); TexMapNr++)
            {
                CMatFile << "Fonts/" << MaterialBaseName << "_" << SizeInPoints << "_" << TexMapNr << "\n";     // Fonts/Arial_24_0
                CMatFile << "{\n";
                CMatFile << "    diffusemap " << "FontImage" /*MaterialBaseName*/ << "_" << SizeInPoints << "_" << TexMapNr << ".png, minFilter linear, noScaleDown\n";
                CMatFile << "\n";
                CMatFile << "    noDynLight    // Should be replaced e.g. by \"LightShader none\"...\n";
                CMatFile << "\n";
                CMatFile << "    blendFunc src_alpha one_minus_src_alpha\n";
                CMatFile << "    red   ambientLightRed       // Hmmm. Maybe we should rather use fParam0...fParam2 here.\n";
                CMatFile << "    green ambientLightGreen\n";
                CMatFile << "    blue  ambientLightBlue\n";
                CMatFile << "    ambientMask d               // Don't write into the depth buffer!\n";
                CMatFile << "}\n";
                CMatFile << "\n";
            }
        }
    }


    // Clean up and quit.
    FT_Done_Face(ftFontFace);
    FT_Done_FreeType(ftLib);
    return 0;
}
