/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Material.hpp"
#include "Bitmap/Bitmap.hpp"

#include <ostream>


namespace
{
    const char* BlendFactorTokens[]=
    {
        "none",
        "zero",
        "one",
        "dst_color",
        "src_color",
        "one_minus_dst_color",
        "one_minus_src_color",
        "dst_alpha",
        "src_alpha",
        "one_minus_dst_alpha",
        "one_minus_src_alpha"
    };

    const char* SurfaceTypeTokens[]=
    {
        "none",
        "stone",
        "metal",
        "sand",
        "wood",
        "liquid",
        "glass",
        "plastic"
    };
}


MaterialT::MaterialT()
    : CubeMap1Comp(),
      CubeMap2Comp(),
      ShaderParamExpr(),
      ShaderParamMapC(),
      NoDraw(false),
      TwoSided(false),
      DepthOffset(0.0f),
      PolygonMode(Filled),
      AlphaTestValue(-1.0f),
      BlendFactorSrc(None),
      BlendFactorDst(None),
      RedGen  (1.0f),
      GreenGen(1.0f),
      BlueGen (1.0f),
      AlphaGen(1.0f),
      UseMeshColors(false),
      NoDynLight(false),
      NoShadows(false),
      ClipFlags(Clip_AllBlocking),
      SurfaceType(ST_None),
      meta_EditorImage(),
      meta_EditorSave(false),
      meta_RadiantExitance_ByImage_Scale(1.0),
      meta_AlphaModulatesRadiosityLight(false),
   // BaseDir("."),
      PixelSizeX(0),
      PixelSizeY(0)
{
    meta_RadiantExitance_Values[0]=0.0;
    meta_RadiantExitance_Values[1]=0.0;
    meta_RadiantExitance_Values[2]=0.0;

    meta_SunLight_Irr[0]=0.0;
    meta_SunLight_Irr[1]=0.0;
    meta_SunLight_Irr[2]=0.0;

    meta_SunLight_Dir[0]=0.0;
    meta_SunLight_Dir[1]=0.0;
    meta_SunLight_Dir[2]=0.0;

    for (unsigned long MaskBit=0; MaskBit<5; MaskBit++)
    {
        AmbientMask[MaskBit]=true;
        LightMask  [MaskBit]=true;
    }
}


static MaterialT::BlendFactorT ParseBlendFactor(const std::string& bf)
{
    if (bf=="zero"               ) return MaterialT::Zero;
    if (bf=="one"                ) return MaterialT::One;
    if (bf=="dst_color"          ) return MaterialT::DstColor;
    if (bf=="src_color"          ) return MaterialT::SrcColor;
    if (bf=="one_minus_dst_color") return MaterialT::OneMinusDstColor;
    if (bf=="one_minus_src_color") return MaterialT::OneMinusSrcColor;
    if (bf=="dst_alpha"          ) return MaterialT::DstAlpha;
    if (bf=="src_alpha"          ) return MaterialT::SrcAlpha;
    if (bf=="one_minus_dst_alpha") return MaterialT::OneMinusDstAlpha;
    if (bf=="one_minus_src_alpha") return MaterialT::OneMinusSrcAlpha;

    return MaterialT::None;
}


static MaterialT::SurfaceTypeT ParseSurfaceType(const std::string& st)
{
 // if (st=="none"   ) return MaterialT::ST_None;
    if (st=="stone"  ) return MaterialT::ST_Stone;
    if (st=="metal"  ) return MaterialT::ST_Metal;
    if (st=="sand"   ) return MaterialT::ST_Sand;
    if (st=="wood"   ) return MaterialT::ST_Wood;
    if (st=="liquid" ) return MaterialT::ST_Liquid;
    if (st=="glass"  ) return MaterialT::ST_Glass;
    if (st=="plastic") return MaterialT::ST_Plastic;

    return MaterialT::ST_None;
}


MaterialT::MaterialT(const std::string& MaterialName, const std::string& BaseDir, TextParserT& TP, const ArrayT<TableT*>& ListOfTables)
    : Name(MaterialName),
      CubeMap1Comp(),
      CubeMap2Comp(),
      ShaderParamExpr(),
      ShaderParamMapC(),
      NoDraw(false),
      TwoSided(false),
      DepthOffset(0.0f),
      PolygonMode(Filled),
      AlphaTestValue(-1.0f),
      BlendFactorSrc(None),
      BlendFactorDst(None),
      RedGen  (1.0f),
      GreenGen(1.0f),
      BlueGen (1.0f),
      AlphaGen(1.0f),
      UseMeshColors(false),
      NoDynLight(false),
      NoShadows(false),
      ClipFlags(Clip_AllBlocking),
      SurfaceType(ST_None),
      meta_EditorImage(),
      meta_EditorSave(false),
      meta_RadiantExitance_ByImage_Scale(1.0),
      meta_AlphaModulatesRadiosityLight(false),
   // BaseDir("."),
      PixelSizeX(0),
      PixelSizeY(0)
{
    meta_RadiantExitance_Values[0]=0.0;
    meta_RadiantExitance_Values[1]=0.0;
    meta_RadiantExitance_Values[2]=0.0;

    meta_SunLight_Irr[0]=0.0;
    meta_SunLight_Irr[1]=0.0;
    meta_SunLight_Irr[2]=0.0;

    meta_SunLight_Dir[0]=0.0;
    meta_SunLight_Dir[1]=0.0;
    meta_SunLight_Dir[2]=0.0;

    for (unsigned long MaskBit=0; MaskBit<5; MaskBit++)
    {
        AmbientMask[MaskBit]=true;
        LightMask  [MaskBit]=true;
    }


    if (TP.GetNextToken()!="{") throw TextParserT::ParseError();

    while (true)
    {
        std::string Token=TP.GetNextToken();

        if (Token=="}")
        {
            // End of material definition.
            break;
        }
        else if (Token=="diffusemap")
        {
            DiffMapComp=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="normalmap")
        {
            NormMapComp=MapCompositionT(TP, BaseDir, true);     // Normal-maps default to employ no compression.
        }
        else if (Token=="specularmap")
        {
            SpecMapComp=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="lumamap")
        {
            LumaMapComp=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="lightmap")
        {
            LightMapComp=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="shlmap")
        {
            SHLMapComp=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="cubeMap")
        {
            CubeMap1Comp=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="cubeMap2")
        {
            CubeMap2Comp=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="AmbientShader")
        {
            AmbientShaderName=TP.GetNextToken();
        }
        else if (Token=="LightShader")
        {
            LightShaderName=TP.GetNextToken();
        }
        else if (Token=="shaderParamExpr")
        {
            ShaderParamExpr.PushBack(ExpressionT(TP, ListOfTables));
        }
        else if (Token=="shaderParamMapC")
        {
            ShaderParamMapC.PushBack(MapCompositionT(TP, BaseDir));
        }
        else if (Token=="noDraw")
        {
            NoDraw=true;
        }
        else if (Token=="noDynLight")
        {
            NoDynLight=true;
        }
        else if (Token=="noShadows")
        {
            NoShadows=true;
        }
        else if (Token=="twoSided")
        {
            TwoSided=true;
        }
        else if (Token=="depthOffset")
        {
            DepthOffset=float(atof(TP.GetNextToken().c_str()));
        }
        else if (Token=="polygonMode")
        {
            const std::string ModeString=TP.GetNextToken();

                 if (ModeString=="filled"   ) PolygonMode=Filled;
            else if (ModeString=="wireframe") PolygonMode=Wireframe;
            else if (ModeString=="points"   ) PolygonMode=Points;
            else throw TextParserT::ParseError();
        }
        else if (Token=="alphaTest")
        {
            AlphaTestValue=ExpressionT(TP, ListOfTables);
        }
        else if (Token=="blendFunc")
        {
            BlendFactorSrc=ParseBlendFactor(TP.GetNextToken());
            BlendFactorDst=ParseBlendFactor(TP.GetNextToken());
        }
        else if (Token=="red")
        {
            RedGen=ExpressionT(TP, ListOfTables);
        }
        else if (Token=="green")
        {
            GreenGen=ExpressionT(TP, ListOfTables);
        }
        else if (Token=="blue")
        {
            BlueGen=ExpressionT(TP, ListOfTables);
        }
        else if (Token=="alpha")
        {
            AlphaGen=ExpressionT(TP, ListOfTables);
        }
        else if (Token=="rgb")
        {
            RedGen  =ExpressionT(TP, ListOfTables);
            GreenGen=RedGen;
            BlueGen =RedGen;
        }
        else if (Token=="rgba")
        {
            RedGen  =ExpressionT(TP, ListOfTables);
            GreenGen=RedGen;
            BlueGen =RedGen;
            AlphaGen=RedGen;
        }
        else if (Token=="ambientMask")
        {
            const std::string TurnOff=TP.GetNextToken();

            for (unsigned long c=0; c<TurnOff.length(); c++)
                switch (TurnOff.at(c))
                {
                    case 'r': AmbientMask[0]=false; break;
                    case 'g': AmbientMask[1]=false; break;
                    case 'b': AmbientMask[2]=false; break;
                    case 'a': AmbientMask[3]=false; break;
                    case 'd': AmbientMask[4]=false; break;
                }
        }
        else if (Token=="lightMask")
        {
            const std::string TurnOff=TP.GetNextToken();

            for (unsigned long c=0; c<TurnOff.length(); c++)
                switch (TurnOff.at(c))
                {
                    case 'r': LightMask[0]=false; break;
                    case 'g': LightMask[1]=false; break;
                    case 'b': LightMask[2]=false; break;
                    case 'a': LightMask[3]=false; break;
                    case 'd': LightMask[4]=false; break;
                }
        }
        else if (Token=="useMeshColors")
        {
            UseMeshColors=true;
        }
        else if (Token=="clip")
        {
            unsigned long cf=0;     // Start with all flags cleared.

            while (true)
            {
                const std::string ClipFlagStr=TP.GetNextToken();

                     if (ClipFlagStr=="players"    ) cf|=Clip_Players;
                else if (ClipFlagStr=="monsters"   ) cf|=Clip_Monsters;
                else if (ClipFlagStr=="moveables"  ) cf|=Clip_Moveables;
                else if (ClipFlagStr=="ik"         ) cf|=Clip_IK;
                else if (ClipFlagStr=="projectiles") cf|=Clip_Projectiles;
                else if (ClipFlagStr=="sight"      ) cf|=Clip_Sight;
                else if (ClipFlagStr=="bspPortals" ) cf|=Clip_BspPortals;
                else if (ClipFlagStr=="radiance"   ) cf|=Clip_Radiance;
                else if (ClipFlagStr=="all"        ) cf|=Clip_AllBlocking;
                else if (ClipFlagStr=="trigger"    ) cf|=Clip_Trigger;
                else if (ClipFlagStr=="ladder"     ) cf|=SP_Ladder;
                else if (ClipFlagStr=="nothing"    ) { }
                else throw TextParserT::ParseError();

                if (TP.PeekNextToken()!=",") break;
                TP.GetNextToken();  // Overread the "," token.
            }

            ClipFlags=ClipFlagsT(cf);
        }
        else if (Token=="_clip_int")    // "internal" or "integer" representation of the clip flags.
        {
            ClipFlags=ClipFlagsT(TP.GetNextTokenAsInt());
        }
        else if (Token=="surfaceType")
        {
            SurfaceType=ParseSurfaceType(TP.GetNextToken());
        }
        else if (Token=="meta_editorImage")
        {
            meta_EditorImage=MapCompositionT(TP, BaseDir);
        }
        else if (Token=="meta_editorSave")
        {
            meta_EditorSave=true;
        }
        else if (Token=="meta_radiantExitance")
        {
            meta_RadiantExitance_Values[0]=float(atof(TP.GetNextToken().c_str()));
            meta_RadiantExitance_Values[1]=float(atof(TP.GetNextToken().c_str()));
            meta_RadiantExitance_Values[2]=float(atof(TP.GetNextToken().c_str()));

            float Scale=float(atof(TP.GetNextToken().c_str()));

            meta_RadiantExitance_Values[0]*=Scale;
            meta_RadiantExitance_Values[1]*=Scale;
            meta_RadiantExitance_Values[2]*=Scale;
        }
        else if (Token=="meta_radiantExitance_byImage")
        {
            meta_RadiantExitance_ByImage_FileName=MapCompositionT(TP, BaseDir);
            meta_RadiantExitance_ByImage_Scale   =float(atof(TP.GetNextToken().c_str()));
        }
        else if (Token=="meta_sunlight")
        {
            // This keyword states that the materials casts sunlight.
            if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
            meta_SunLight_Irr[0]=float(atof(TP.GetNextToken().c_str()));
            meta_SunLight_Irr[1]=float(atof(TP.GetNextToken().c_str()));
            meta_SunLight_Irr[2]=float(atof(TP.GetNextToken().c_str()));
            if (TP.GetNextToken()!=")") throw TextParserT::ParseError();

            if (TP.GetNextToken()!="(") throw TextParserT::ParseError();
            meta_SunLight_Dir[0]=float(atof(TP.GetNextToken().c_str()));
            meta_SunLight_Dir[1]=float(atof(TP.GetNextToken().c_str()));
            meta_SunLight_Dir[2]=float(atof(TP.GetNextToken().c_str()));
            if (TP.GetNextToken()!=")") throw TextParserT::ParseError();
        }
        else if (Token=="meta_alphaModulatesRadiosityLight")
        {
            meta_AlphaModulatesRadiosityLight=true;
        }
        else throw TextParserT::ParseError();   // Unknown token!
    }
}


// Replaces all occurrences of '#' in BaseString with "_px".
static std::string GetFullCubeMapString(std::string BaseString)
{
    for (std::string::size_type i=BaseString.find("#"); i!=std::string::npos; i=BaseString.find("#"))
        BaseString.replace(i, 1, "_px");

    return BaseString;
}


unsigned int MaterialT::GetPixelSizeX() const
{
    if (PixelSizeX==0)
    {
        BitmapT* Bitmap=NULL;

        if (DiffMapComp.IsEmpty())
        {
            TextParserT TP(GetFullCubeMapString(CubeMap1Comp.GetString()).c_str(), "({[]}),", false);
            Bitmap=MapCompositionT(TP, CubeMap1Comp.GetBaseDir()).GetBitmap();
        }
        else Bitmap=DiffMapComp.GetBitmap();

        PixelSizeX=Bitmap->SizeX;
        PixelSizeY=Bitmap->SizeY;

        delete Bitmap;
    }

    return PixelSizeX;
}


unsigned int MaterialT::GetPixelSizeY() const
{
    if (PixelSizeY==0)
    {
        BitmapT* Bitmap=NULL;

        if (DiffMapComp.IsEmpty())
        {
            TextParserT TP(GetFullCubeMapString(CubeMap1Comp.GetString()).c_str(), "({[]}),", false);
            Bitmap=MapCompositionT(TP, CubeMap1Comp.GetBaseDir()).GetBitmap();
        }
        else Bitmap=DiffMapComp.GetBitmap();

        PixelSizeX=Bitmap->SizeX;
        PixelSizeY=Bitmap->SizeY;

        delete Bitmap;
    }

    return PixelSizeY;
}


void MaterialT::Save(std::ostream& OutStream) const
{
    const static MaterialT RefMat;  // Use a reference object here, just in case we ever change the defaults.
    const char* INDENT="    ";

    OutStream << "\"" << Name << "\"\n";
    OutStream << "{\n";

    if (AmbientShaderName!="") OutStream << INDENT << "AmbientShader" << " " << AmbientShaderName << "\n";
    if (LightShaderName  !="") OutStream << INDENT << "LightShader" << " " << LightShaderName   << "\n";

    std::string mcs;
    mcs=DiffMapComp .GetStringWithOptions();     if (mcs!="") OutStream << INDENT << "diffusemap"  << " " << mcs << "\n";
    mcs=NormMapComp .GetStringWithOptions(true); if (mcs!="") OutStream << INDENT << "normalmap"   << " " << mcs << "\n";
    mcs=SpecMapComp .GetStringWithOptions();     if (mcs!="") OutStream << INDENT << "specularmap" << " " << mcs << "\n";
    mcs=LumaMapComp .GetStringWithOptions();     if (mcs!="") OutStream << INDENT << "lumamap"     << " " << mcs << "\n";
    mcs=LightMapComp.GetStringWithOptions();     if (mcs!="") OutStream << INDENT << "lightmap"    << " " << mcs << "\n";
    mcs=SHLMapComp  .GetStringWithOptions();     if (mcs!="") OutStream << INDENT << "shlmap"      << " " << mcs << "\n";
    mcs=CubeMap1Comp.GetStringWithOptions();     if (mcs!="") OutStream << INDENT << "cubeMap"     << " " << mcs << "\n";
    mcs=CubeMap2Comp.GetStringWithOptions();     if (mcs!="") OutStream << INDENT << "cubeMap2"    << " " << mcs << "\n";

    for (unsigned long i=0; i<ShaderParamExpr.Size(); i++) OutStream << INDENT << "shaderParamExpr" << " " << ShaderParamExpr[i].GetString() << "\n";
    for (unsigned long i=0; i<ShaderParamMapC.Size(); i++) OutStream << INDENT << "shaderParamMapC" << " " << ShaderParamMapC[i].GetStringWithOptions() << "\n";

    if (NoDraw)   OutStream << INDENT << "noDraw"   << "\n";
    if (TwoSided) OutStream << INDENT << "twoSided" << "\n";
    if (DepthOffset!=0.0f) OutStream << INDENT << "depthOffset" << " " << DepthOffset << "\n";
    if (PolygonMode==Wireframe) OutStream << INDENT << "polygonMode wireframe" << "\n";
    if (PolygonMode==Points)    OutStream << INDENT << "polygonMode points"    << "\n";

    mcs=AlphaTestValue.GetString(); if (mcs!="<expr>" && mcs!=RefMat.AlphaTestValue.GetString()) OutStream << INDENT << "alphaTest" << " " << mcs << "\n";
    mcs=RedGen        .GetString(); if (mcs!="<expr>" && mcs!=RefMat.RedGen        .GetString()) OutStream << INDENT << "red  "     << " " << mcs << "\n";
    mcs=GreenGen      .GetString(); if (mcs!="<expr>" && mcs!=RefMat.GreenGen      .GetString()) OutStream << INDENT << "green"     << " " << mcs << "\n";
    mcs=BlueGen       .GetString(); if (mcs!="<expr>" && mcs!=RefMat.BlueGen       .GetString()) OutStream << INDENT << "blue "     << " " << mcs << "\n";
    mcs=AlphaGen      .GetString(); if (mcs!="<expr>" && mcs!=RefMat.AlphaGen      .GetString()) OutStream << INDENT << "alpha"     << " " << mcs << "\n";

    if (BlendFactorSrc!=None || BlendFactorDst!=None)
        OutStream << INDENT << "blendFunc" << " " << BlendFactorTokens[BlendFactorSrc] << " " << BlendFactorTokens[BlendFactorDst] << "\n";

    const char BitToChannel[]={ 'r', 'g', 'b', 'a', 'd' };

    for (unsigned int MaskBit=0; MaskBit<5; MaskBit++)
        if (!AmbientMask[MaskBit]) OutStream << INDENT << "ambientMask " << BitToChannel[MaskBit] << "\n";

    for (unsigned int MaskBit=0; MaskBit<5; MaskBit++)
        if (!LightMask[MaskBit]) OutStream << INDENT << "lightMask " << BitToChannel[MaskBit] << "\n";

    if (UseMeshColors) OutStream << INDENT << "useMeshColors" << "\n";
    if (NoDynLight)    OutStream << INDENT << "noDynLight" << "\n";
    if (NoShadows)     OutStream << INDENT << "noShadows" << "\n";

    if (ClipFlags!=Clip_AllBlocking)
        OutStream << INDENT << "_clip_int" << " " << ClipFlags << "\n";

    if (SurfaceType!=ST_None)
        OutStream << INDENT << "surfaceType" << " " << SurfaceTypeTokens[SurfaceType] << "\n";

    mcs=meta_EditorImage.GetStringWithOptions(); if (mcs!="") OutStream << INDENT << "meta_editorImage" << " " << mcs << "\n";
    if (meta_EditorSave) OutStream << INDENT << "meta_editorSave" << "\n";
    mcs=meta_RadiantExitance_ByImage_FileName.GetStringWithOptions(); if (mcs!="") OutStream << INDENT << "meta_radiantExitance_byImage" << " " << mcs << " " << meta_RadiantExitance_ByImage_Scale << "\n";
    if (meta_AlphaModulatesRadiosityLight) OutStream << INDENT << "meta_alphaModulatesRadiosityLight" << "\n";

    if (meta_RadiantExitance_Values[0]!=0.0f || meta_RadiantExitance_Values[1]!=0.0f || meta_RadiantExitance_Values[2]!=0.0f)
    {
        OutStream << INDENT << "meta_radiantExitance"
                  << " " << meta_RadiantExitance_Values[0]
                  << " " << meta_RadiantExitance_Values[1]
                  << " " << meta_RadiantExitance_Values[2] << " 1.0\n";
    }

    if (meta_SunLight_Irr[0]!=0.0f || meta_SunLight_Irr[1]!=0.0f || meta_SunLight_Irr[2]!=0.0f)
    {
        OutStream << INDENT << "meta_sunlight "
                  << "(" << meta_SunLight_Irr[0]
                  << " " << meta_SunLight_Irr[1]
                  << " " << meta_SunLight_Irr[2] << ") "
                  << "(" << meta_SunLight_Dir[0]
                  << " " << meta_SunLight_Dir[1]
                  << " " << meta_SunLight_Dir[2] << ")\n";
    }

    OutStream << "}\n";
}
