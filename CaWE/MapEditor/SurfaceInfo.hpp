/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SURFACE_INFO_HPP_INCLUDED
#define CAFU_SURFACE_INFO_HPP_INCLUDED

#include "Math3D/Plane3.hpp"
#include "Templates/Array.hpp"


class TextParserT;


/// This enum describes the technique that is used to generate texture-coordinates for the associated map primitive.
enum TexCoordGenModeT
{
    Custom=0,   ///< The tex-coords are freely specified (per vertex).
    MatFit,     ///< Texture coordinates are created, so the material fits exactly on the patch.
    PlaneProj   ///< Texture coordinates are created using a plane projection with UV axes.
};


class TexCoordT
{
    public:

    TexCoordT() : u(0), v(0) { }

    float& operator [] (unsigned long Index)
    {
        assert(Index<2);
        return Index==0 ? u : v;
    }

    const float& operator [] (unsigned long Index) const
    {
        assert(Index<2);
        return Index==0 ? u : v;
    }

    float u;
    float v;
};


/// This class holds all information that is needed in order to compute the UV texture-space
/// coordinates of (and thus to apply a material onto) a primitive surface.
class SurfaceInfoT
{
    public:

    /// The default constructor.
    SurfaceInfoT();

    /// The constructor for creating a SurfaceInfoT in plane projection mode, matching the given plane.
    SurfaceInfoT(const Plane3fT& Plane, bool FaceAligned);

    /// Named constructor for loading a SurfaceInfoT from a cmap file.
    static SurfaceInfoT Create_cmap(TextParserT& TP);


    /// Serializes this instance into a cmap file.
    void Save_cmap(std::ostream& OutFile) const;

    /// Resets the texture-space u- and v-axes as appropriate for the given plane.
    /// @param Plane         The plane with reference to which the axes are reset.
    /// @param FaceAligned   When true, the axes are re-initialized face-aligned, world-aligned otherwise.
    void ResetUVAxes(const Plane3fT& Plane, bool FaceAligned);

    /// Wraps the Trans[] members so that they are in the interval [0, 1[.
    void WrapTranslations();

    /// Rotates the texture-space u- and v-axes by the given angle.
    /// @param Angle   The angle of rotation, in degrees.
    void RotateUVAxes(float Angle);

    /// Changes this SurfaceInfoT so that the material is aligned on the surface according to AlignKey.
    /// @param AlignKey   How the material is aligned wrt. the surface. Valid keys are "top", "bottom", "left", "right", "center" and "fit".
    /// @param Vertices   The vertices of the surface for which the alignment is to be computed.
    void AlignMaterial(const char* AlignKey, const ArrayT<Vector3fT>& Vertices);


    TexCoordGenModeT TexCoordGenMode;   ///< Determines the algorithm that is used to generate texture-coordinates for the associated map primitive.
    float            Trans[2];
    float            Scale[2];
    float            Rotate;
    Vector3fT        UAxis;
    Vector3fT        VAxis;
    float            LightmapScale;
};

#endif
