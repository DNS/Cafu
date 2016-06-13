/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "FontTT.hpp"
#include "ConsoleCommands/Console.hpp"      // For the cf::va() function.
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "TextParser/TextParser.hpp"

#include <cstdarg>
#include <cstdio>

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define vsnprintf _vsnprintf
        #define for if (false) ; else for
    #endif
#endif


using namespace cf;


TrueTypeFontT::GlyphInfoT::GlyphInfoT(TextParserT& TP, const ArrayT<MatSys::RenderMaterialT*>& RenderMaterials)
{
    TP.AssertAndSkipToken("glyph");
    TP.GetNextToken();  // Skip the sequential number.

    unsigned long RMNr=0;

    BearingX=TP.GetNextTokenAsFloat();
    BearingY=TP.GetNextTokenAsFloat();
    AdvanceX=TP.GetNextTokenAsFloat();
    Width   =TP.GetNextTokenAsInt();
    Height  =TP.GetNextTokenAsInt();
    RMNr    =TP.GetNextTokenAsInt(); if (RMNr>=RenderMaterials.Size()) throw TextParserT::ParseError();
    RM      =RenderMaterials[RMNr];
    s1      =TP.GetNextTokenAsFloat();
    t1      =TP.GetNextTokenAsFloat();
    s2      =TP.GetNextTokenAsFloat();
    t2      =TP.GetNextTokenAsFloat();
}


TrueTypeFontT::FontInfoT::FontInfoT(const std::string& FontName, int SizeInPixels_)
    : SizeInPixels(float(SizeInPixels_))
{
    for (unsigned long c=0; c<256; c++) CharToGlyphIndex[c]=0;

    const std::string cfontFileName=FontName+"/"+va("FontDescr_%i.cfont", SizeInPixels_);
    TextParserT TP(cfontFileName.c_str());


    try
    {
        TP.AssertAndSkipToken("global");
        Ascender =TP.GetNextTokenAsFloat();
        Descender=TP.GetNextTokenAsFloat();
        Height   =TP.GetNextTokenAsFloat();

        for (unsigned long c=0; c<256; c++) CharToGlyphIndex[c]=TP.GetNextTokenAsInt();

        while (!TP.IsAtEOF() && TP.PeekNextToken()=="matname")
        {
            TP.AssertAndSkipToken("matname");

            MaterialT* Material=MaterialManager->GetMaterial(TP.GetNextToken());

            Materials.PushBack(Material);
            RenderMaterials.PushBack(MatSys::Renderer->RegisterMaterial(Material));
        }

        while (!TP.IsAtEOF() && TP.PeekNextToken()=="glyph")
        {
            GlyphInfos.PushBack(new TrueTypeFontT::GlyphInfoT(TP, RenderMaterials));
        }

        // Read the kerning table.
        KerningTable.PushBackEmpty(GlyphInfos.Size());

        while (!TP.IsAtEOF())
        {
            TP.AssertAndSkipToken("k");

            int Glyph1=TP.GetNextTokenAsInt();
            int Glyph2=TP.GetNextTokenAsInt();

            KerningTable[Glyph1][Glyph2]=TP.GetNextTokenAsFloat();
        }
    }
    catch (const TextParserT::ParseError& PE)
    {
        Console->Warning(std::string("Could not parse ")+cfontFileName+va(" near byte %lu.\n", TP.GetReadPosByte()));
        throw PE;
    }


#if 0
    // Run some tests on the kerning table.
    const char First=' ';
    const char Last =125;

    for (char c1=First; c1<=Last; c1++)
        for (char c2=First; c2<=Last; c2++)
        {
            const unsigned long index1=CharToGlyphIndex[c1];
            const unsigned long index2=CharToGlyphIndex[c2];

            // What I *would* like to write here is:
            //     const float Kerning=KerningTable[index1][index2];
            // But unfortunately, there is no const version of the [] operator of a std::map,
            // and therefore we have to employ this clumsy work-around:
            std::map<int, float>::const_iterator It=KerningTable[index1].find(index2);

            const float Kerning=(It!=KerningTable[index1].end()) ? It->second : 0.0f;

            if (Kerning!=0.0f)
            {
                Console->Print(cfontFileName+va(": Kerning between %c%c is %f.\n", c1, c2, Kerning));
            }
        }
#endif
}


TrueTypeFontT::FontInfoT::~FontInfoT()
{
    for (unsigned long GINr=0; GINr<GlyphInfos.Size(); GINr++)
        delete GlyphInfos[GINr];

    for (unsigned long MaterialNr=0; MaterialNr<RenderMaterials.Size(); MaterialNr++)
        MatSys::Renderer->FreeMaterial(RenderMaterials[MaterialNr]);
}


TrueTypeFontT::TrueTypeFontT(const std::string& FontName_)
    : FontName(FontName_),
      DEFAULT_FONT_SCALE(48.0f)
{
    // Register all the cmat files in the font directory (which happens to be the FontName).
    // Note that we use FontName also as the base directory for the textures of these materials.
    // Also note that the MaterialManager makes sure that no cmat script is ever registered multiply,
    // e.g. in case there are multiple instances of the TrueTypeFontT class.
    MaterialManager->RegisterMaterialScriptsInDir(FontName, FontName+"/");

    FontInfoSmall =new FontInfoT(FontName, 12);
    FontInfoMedium=new FontInfoT(FontName, 24);
    FontInfoLarge =new FontInfoT(FontName, 48);
}


TrueTypeFontT::~TrueTypeFontT()
{
    delete FontInfoSmall;
    delete FontInfoMedium;
    delete FontInfoLarge;
}


float TrueTypeFontT::GetWidth(const std::string& Text, float Scale) const
{
    const FontInfoT& fi=GetFontInfo(Scale);

    const float   s=Scale*DEFAULT_FONT_SCALE/fi.SizeInPixels;   // Compute the "total" scale.
    const size_t  l=Text.length();
    float         w=0.0f;
    unsigned long PrevGI=0;                                     // Glyph index of the previous character.

    for (size_t i=0; i<l; i++)
    {
        const unsigned long ThisGI=fi.CharToGlyphIndex[Text[i]];

        if (PrevGI!=0)
        {
            // What I *would* like to write here is:
            //     w+=fi.KerningTable[PrevGI][ThisGI];
            // But unfortunately, there is no const version of the [] operator of a std::map,
            // and therefore we have to employ this clumsy work-around:
            std::map<int, float>::const_iterator It=fi.KerningTable[PrevGI].find(ThisGI);

            if (It!=fi.KerningTable[PrevGI].end()) w+=It->second;
        }

        w+=fi.GlyphInfos[ThisGI]->AdvanceX;
        PrevGI=ThisGI;
    }

    return w*s;
}


float TrueTypeFontT::GetAscender(float Scale) const
{
    const FontInfoT& fi=GetFontInfo(Scale);

    return fi.Ascender*DEFAULT_FONT_SCALE/fi.SizeInPixels*Scale;
}


float TrueTypeFontT::GetLineSpacing(float Scale) const
{
    const FontInfoT& fi=GetFontInfo(Scale);

    return fi.Height*DEFAULT_FONT_SCALE/fi.SizeInPixels*Scale;
}


void TrueTypeFontT::Print(float PosX, float PosY, float Scale, unsigned long Color, const char* PrintString, ...) const
{
    if (!PrintString) return;

    va_list ArgList;
    char    PrintBuffer[256];

    va_start(ArgList, PrintString);
        vsnprintf(PrintBuffer, 256, PrintString, ArgList);
    va_end(ArgList);


    MatSys::Renderer->SetCurrentAmbientLightColor(char((Color >> 16) & 0xFF)/255.0f, char((Color >> 8) & 0xFF)/255.0f, char(Color & 0xFF)/255.0f);


    const FontInfoT& fi=GetFontInfo(Scale);
    const float      s =Scale*DEFAULT_FONT_SCALE/fi.SizeInPixels;   // Compute the "total" scale.
    unsigned long    PrevGI=0;                                      // Glyph index of the previous character.

    for (unsigned long c=0; PrintBuffer[c]; c++)
    {
        const unsigned long ThisGI=fi.CharToGlyphIndex[PrintBuffer[c]];
        const GlyphInfoT&   gi    =*fi.GlyphInfos[ThisGI];

        if (PrevGI!=0)
        {
            // What I *would* like to write here is:
            //     PosX+=fi.KerningTable[PrevGI][ThisGI]*s;
            // But unfortunately, there is no const version of the [] operator of a std::map,
            // and therefore we have to employ this clumsy work-around:
            std::map<int, float>::const_iterator It=fi.KerningTable[PrevGI].find(ThisGI);

            if (It!=fi.KerningTable[PrevGI].end()) PosX+=It->second*s;
        }

        const float x1=PosX+gi.BearingX*s;       // PosX is advanced below.
        const float y1=PosY-gi.BearingY*s;
        const float x2=x1+gi.Width*s;
        const float y2=y1+gi.Height*s;

        static MatSys::MeshT GlyphMesh(MatSys::MeshT::Quads);
        GlyphMesh.Vertices.Overwrite();
        GlyphMesh.Vertices.PushBackEmpty(4);

        GlyphMesh.Vertices[0].SetOrigin(x1, y1); GlyphMesh.Vertices[0].SetTextureCoord(gi.s1, gi.t1);
        GlyphMesh.Vertices[1].SetOrigin(x2, y1); GlyphMesh.Vertices[1].SetTextureCoord(gi.s2, gi.t1);
        GlyphMesh.Vertices[2].SetOrigin(x2, y2); GlyphMesh.Vertices[2].SetTextureCoord(gi.s2, gi.t2);
        GlyphMesh.Vertices[3].SetOrigin(x1, y2); GlyphMesh.Vertices[3].SetTextureCoord(gi.s1, gi.t2);

        MatSys::Renderer->SetCurrentMaterial(gi.RM);
        MatSys::Renderer->RenderMesh(GlyphMesh);

        PosX+=gi.AdvanceX*s;
        PrevGI=ThisGI;
    }
}


const TrueTypeFontT::FontInfoT& TrueTypeFontT::GetFontInfo(float Scale) const
{
    if (Scale<=0.3) return *FontInfoSmall;
    if (Scale<=0.6) return *FontInfoMedium;

    return *FontInfoLarge;
}
