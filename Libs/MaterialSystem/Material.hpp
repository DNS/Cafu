/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATSYS_MATERIAL_HPP_INCLUDED
#define CAFU_MATSYS_MATERIAL_HPP_INCLUDED

#include "Expression.hpp"
#include "MapComposition.hpp"


// *Somewhere* (in some Linux clib header?!) apparently   #define None 0L   occurs,
// which causes problems below. Grrr. TODO: Use g++ -E to find out where this comes from.
#undef None


/// This class represents a surface material ("A datastructural representation of a scripts material def.").
/// It's definition is usually obtained with the help of the MaterialManager from a material script file (.cmat).
/// Note that materials are entirely independent from the renderer implementation!
class MaterialT
{
    public:

    enum BlendFactorT { None, Zero, One, DstColor, SrcColor, OneMinusDstColor, OneMinusSrcColor, DstAlpha, SrcAlpha, OneMinusDstAlpha, OneMinusSrcAlpha };
    enum TexCoordGenT { Disabled, ObjectSpacePlane, EyeSpacePlane, SphereMap };
    enum PolygonModeT { Filled, Wireframe, Points };

#if 0   // I think it should be like this:
    /// The properties of the surfaces that this material is assigned to.
    enum SurfacePropertiesT
    {
        SP_ClipPlayers     = 0x0001,
        ...
        SP_BlockBspPortals = ...,
        ...
        SP_Ladder          = ...
    }

    /// The properties of the volume that is defined by the brush on whose surface this material is used.
    enum VolumePropertiesT
    {
        VP_Trigger = 0x0001
    }
#else
    // Note that Materials are applied to surfaces, and thus they inherently can have surface flags/properties/attributes/types,
    // but not easily (naturally) describe volume/contents properties...
    // Volume properties should only be contents we can teleport into or out of, e.g. water, trigger volumes, etc.
    enum ClipFlagsT
    {
        Clip_Players    =0x0001,
        Clip_Monsters   =0x0002,
        Clip_Moveables  =0x0004,
        Clip_IK         =0x0008,
        Clip_Projectiles=0x0010,
        Clip_Sight      =0x0020,
        Clip_BspPortals =0x0040,
        Clip_Radiance   =0x0080,
        Clip_AllBlocking=0x00FF,
        Clip_BlkButUtils=Clip_AllBlocking & ~Clip_BspPortals & ~Clip_Radiance,  // 0x003F
        Clip_Trigger    =0x0100,
        SP_Ladder       =0x0200
    };

    // TODO: Rename ClipFlagsT to ContentsT (as in Q3, D3 etc.)?  Or is contents==volume, and thus something different from ClipFlags?
    // TODO: Extend this to not only have flags relevant to traces, but also to position (volume) tests (e.g. water, trigger, ladder, ...)??
    // (Consider if we should have a separate VolumeFlagsT enum (orthogonal to ClipFlagsT), or if this enum should be a part of the ClipFlagsT enum.)
    // (Idea: One reason for having the VolumeFlagsT seperate/orthogonal is that brushes can have "mixed" clipflags sides, but not mixed volumeflags sides.)
    // TODO: BspPortals and Radiance are actually "meta-flags" for CaBSP and CaLight, respectively, not really for the ClipSys / game-code:
    // BspPortals is for CaBSP only. CaLight may use the ClipSys or the draw BSP tree for its purposes, so Radiance is half a meta, and half a clip flag...
    // NOTE: A reaonsable consideration seems to think of maps as being made of triangle lists by 3D Modelling programs, not as brushes by CaWE.
    //       Then, it makes sense to have a VolumeFlagsT member that keeps track of the *volume* properties of the material, e.g. the "contents"
    //       of the volume like WATER, TRIGGER, PAIN, etc.  Separately stored are the *surface* properties, which may be a set of flags plus the
    //       surface type, etc. The clip flags would then be a subset of the surface properties/flags. Only remaining issue: This is currently
    //       a bit inconsistent with the assumptions of the ClipSys... REVISE!
#endif

    enum SurfaceTypeT { ST_None, ST_Stone, ST_Metal, ST_Sand, ST_Wood, ST_Liquid, ST_Glass, ST_Plastic };


    std::string             Name;

    std::string             AmbientShaderName;
    std::string             LightShaderName;

    MapCompositionT         DiffMapComp;
    MapCompositionT         NormMapComp;
    MapCompositionT         SpecMapComp;
    MapCompositionT         LumaMapComp;

    MapCompositionT         LightMapComp;   ///< This is normally empty or "$lightmap". Everything else works, too, but doesn't make much sense.
    MapCompositionT         SHLMapComp;     ///< This is normally empty or "$shlmap".   Everything else works, too, but doesn't make much sense.

    MapCompositionT         CubeMap1Comp;   ///< This materials 1st cubemap. Requires Ambient- and/or LightShaderName to be explicitly set -- the auto-detection doesn't take cubemaps into account. Use '#' as a placeholder for the actual side suffixes.
    MapCompositionT         CubeMap2Comp;   ///< This materials 2nd cubemap. Requires Ambient- and/or LightShaderName to be explicitly set -- the auto-detection doesn't take cubemaps into account. Use '#' as a placeholder for the actual side suffixes.

    ArrayT<ExpressionT>     ShaderParamExpr;///< Parameters for the shader that renders this material. The meanings depend on the shader!
    ArrayT<MapCompositionT> ShaderParamMapC;///< Parameters for the shader that renders this material. The meanings depend on the shader!

    // Global material rendering parameters (never looked at by the map compile tools).
    bool                    NoDraw;         ///< If true, this material does not render at all. Mostly useful for debugging.
    bool                    TwoSided;       ///< Normally, back-face culling is enabled per default. If TwoSided is true however, culling gets disabled.
    float                   DepthOffset;    ///< Depth buffer offset to combat z-fighting. Useful e.g. for decals or CaWE materials.
    PolygonModeT            PolygonMode;    ///< The mode in which the polygon is rendered: filled, wireframe, or as points. Applies to both the front- and back-side of the polygon.

    // Ambient material rendering parameters (only relevant for the *ambient* contribution, and never looked at by the map compile tools <-- WRONG! E.g. CaBSP looks for blended or perforated materials when calculating portals...).
    ExpressionT             AlphaTestValue; ///< The value for the alpha test (alpha > AmbientTestValue?). Negative for no test.
    BlendFactorT            BlendFactorSrc; ///< The source      factor of the blend function for the ambient contribution.
    BlendFactorT            BlendFactorDst; ///< The destination factor of the blend function for the ambient contribution.
    ExpressionT             RedGen;
    ExpressionT             GreenGen;
    ExpressionT             BlueGen;
    ExpressionT             AlphaGen;
    bool                    AmbientMask[5]; ///< Buffer mask for the ambient contribution. Elements 0 to 4 correspond to red, green, blue, alpha and depth.
    bool                    UseMeshColors;  ///< Modulates the RGBA color with the colors specified at the mesh vertices. Normally, the mesh vertex colors are ignored.

    // Light material rendering parameters (only relevant for the *lighting* contribution, and never looked at by the map compile tools).
    bool                    NoDynLight;     ///< Entirely turns off per-lightsource interaction, that is, the complete light shader. If true, this material does not receive (or rather, reflect) light by dynamic light sources, only the ambient contribution is rendered. Useful e.g. for sky domes, additive effects like particles, translucent surfaces like glass etc. It may still cast shadows, though.
    bool                    NoShadows;      ///< Meshes with this material applied won't cast any (stencil-buffer) shadows if this is true. This should in a sense actually be a "meta" parameter, as it is taken into account only by the code that computes the shadow volumes, *not* by the MatSys! (It can't - materials are usually unknown to it while rendering stencil shadow volumes.)
    bool                    LightMask[5];   ///< Buffer mask for the lighting contribution. Elements 0 to 4 correspond to red, green, blue, alpha and depth.

    // Material (meta-)parameters for the compile tools, the game code etc. Not directly related to the rendering of the material.
    ClipFlagsT              ClipFlags;      ///< The collision detection (trace) code may want to consider only materials with certain clip flags set.
    SurfaceTypeT            SurfaceType;    ///< The game code usually wants to play footstep sounds and ricochet effects according to the surface type.

    // Material meta-parameters for the compile tools etc. Not directly related to the rendering of the material.
    MapCompositionT         meta_EditorImage;                       ///< Image shown in CaWE.
    bool                    meta_EditorSave;                        ///< If \c true, this is a material that the user has created and/or manipulated in the editor (CaWE) and that the editor thus should save, possibly overwriting a previous definition. This flag is used for keeping such materials separate from custom, hand-crafted material definitions that the editor should not touch or overwrite. The editor saves such materials typically in a separate file whose name ends like <tt>_editor.cmat</tt> that in turn is included from another <tt>.cmat</tt> file.
    float                   meta_RadiantExitance_Values[3];         ///< Radiant Exitance RGB values in [W/m^2]. Used by CaLight.
    MapCompositionT         meta_RadiantExitance_ByImage_FileName;  ///< Radiant Exitance RGB values from image file. Used by CaLight.
    float                   meta_RadiantExitance_ByImage_Scale;     ///< Radiant Exitance intensity (scale) for the RGB values from image file. Used by CaLight.
    float                   meta_SunLight_Irr[3];                   ///< Irradiance of the sunlight in Watt/m^2 that comes (or shines) through this material.
    float                   meta_SunLight_Dir[3];                   ///< The direction of the incoming sunlight rays. The z-component should be negative.
    bool                    meta_AlphaModulatesRadiosityLight;      ///< Makes CaLight handle the DiffMapComps alpha channel and the RGBAGens properly. For fences, grates, glass, water, etc.


    /// The default constructor.
    MaterialT();

    /// A constructor. Throws TextParserT::ParseError on failure.
    MaterialT(const std::string& MaterialName, const std::string& BaseDir, TextParserT& TP, const ArrayT<TableT*>& ListOfTables);

 // void         SetBaseDir(const std::string& BD);
    unsigned int GetPixelSizeX() const;
    unsigned int GetPixelSizeY() const;

    bool UsesGeneratedLightMap() const
    {
        return LightMapComp.GetString()=="$lightmap";
    }

    bool UsesGeneratedSHLMap() const
    {
        return SHLMapComp.GetString()=="$shlmap";
    }

    bool HasDefaultBlendFunc() const
    {
        if (BlendFactorSrc==MaterialT::None) return true;
        if (BlendFactorDst==MaterialT::None) return true;
        if (BlendFactorSrc==MaterialT::One && BlendFactorDst==MaterialT::Zero) return true;

        return false;
    }

    /// Saves the material into the given stream.
    void Save(std::ostream& OutStream) const;


    private:

 // std::string          BaseDir;
    mutable unsigned int PixelSizeX;
    mutable unsigned int PixelSizeY;
};

#endif
