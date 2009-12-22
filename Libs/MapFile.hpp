/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CF_MAPFILE_HPP_
#define _CF_MAPFILE_HPP_

#include <map>

#include "Templates/Array.hpp"
#include "Math3D/Vector3.hpp"
#include "Math3D/Plane3.hpp"
#include "Math3D/BoundingBox.hpp"


class MaterialT;
class TextParserT;


namespace cf
{
    extern const double CA3DE_SCALE;


    /// This struct describes a plane (and thus one side) of a map brush.
    /// The members U, V, ShiftU and ShiftV together define the planar projection
    /// for computing the (u, v) texture coordinates at the vertices of the brush.
    struct MapFilePlaneT
    {
     // ArrayT<VectorT> Points;
        Plane3dT        Plane;
        MaterialT*      Material;       ///< The planes material.
        Vector3dT       U;              ///< The first  span vector of the texture projection plane.
        Vector3dT       V;              ///< The second span vector of the texture projection plane.
        double          ShiftU;         ///< Texture "scroll offset" in direction of U.
        double          ShiftV;         ///< Texture "scroll offset" in direction of V.
    };


    struct MapFileBrushT
    {
        /// The default constructor.
        MapFileBrushT() { }

        /// @throws TextParserT::ParseError on problems.
        MapFileBrushT(TextParserT& TP, unsigned long BrushNr);


        ArrayT<MapFilePlaneT> MFPlanes;
    };


    struct MapFileBezierPatchT
    {
        /// The default constructor.
        MapFileBezierPatchT() { }

        /// @throws TextParserT::ParseError on problems.
        MapFileBezierPatchT(TextParserT& TP);


        // TODO: Remove the SizeX, SizeY and ControlsPoints members, use a cf::math::BezierPatchT<float> instead!
        unsigned long SizeX;            ///< Nr of columns.
        unsigned long SizeY;            ///< Nr of rows.

        int           SubdivsHorz;      ///< Number of subdivisions in horizontal direction, or auto-detection if -1.
        int           SubdivsVert;      ///< Number of subdivisions in vertical   direction, or auto-detection if -1.

        MaterialT*    Material;         ///< The patches material.
        ArrayT<float> ControlPoints;    ///< The SizeX*SizeY*5 control points.
    };


    struct MapFileTerrainT
    {
        /// The default constructor.
        MapFileTerrainT() { }

        /// @throws TextParserT::ParseError on problems.
        MapFileTerrainT(TextParserT& TP);

        /// Returns the spatial coordinate for the given (logical) height field position.
        /// Note that for processing all vertices of a terrain quickly, specialized loops
        /// should be preferred over this relatively slow (repetitive) method.
        /// @param x   The horizontal component of the logical height field position.
        /// @param y   The vertical   component of the logical height field position.
        /// @returns the spatial coordinate for the given (logical) height field position.
        Vector3dT GetSpatial(unsigned long x, unsigned long y) const
        {
            Vector3dT Pos=Bounds.Min;

            Pos.x+=(Bounds.Max.x-Bounds.Min.x)*double(x)/double(SideLength-1);
            Pos.y+=(Bounds.Max.y-Bounds.Min.y)*double(y)/double(SideLength-1);
            Pos.z+=(Bounds.Max.z-Bounds.Min.z)*double(HeightData[SideLength*y+x])/65535.0;

            return Pos;
        }


        BoundingBox3dT         Bounds;     ///< The terrains bounds.
        MaterialT*             Material;   ///< The terrains material.
        unsigned long          SideLength; ///< Side length of the terrains height data.
        ArrayT<unsigned short> HeightData; ///< The SideLength*SideLength array of height data.
    };


    struct MapFilePlantT
    {
        /// The default constructor.
        MapFilePlantT() { }

        /// @throws TextParserT::ParseError on problems.
        MapFilePlantT(TextParserT& TP);


        std::string  DescrFileName;
        unsigned int RandomSeed;
        Vector3dT    Position;
        Vector3fT    Angles;
    };


    struct MapFileModelT
    {
        /// The default constructor.
        MapFileModelT() { }

        /// @throws TextParserT::ParseError on problems.
        MapFileModelT(TextParserT& TP);


        std::string Model;
        std::string CollModel;
        std::string Label;
        Vector3fT   Origin;
        Vector3fT   Angles;
        float       Scale;
        int         SeqNumber;
        float       FrameOffset;
        float       FrameTimeScale;
        bool        Animate;
    };


    struct MapFileEntityT
    {
        /// The default constructor.
        MapFileEntityT() : MFIndex(0) { }

        /// @throws TextParserT::ParseError on problems.
        MapFileEntityT(unsigned long Index, TextParserT& TP);


        unsigned long                      MFIndex;     ///< In the source cmap file, this was/is the MFIndex-th entity. Normally (e.g. immediately after loading the cmap file) this is identical to the index of this MapFileEntityT into the array of all entities. It is kept explicitly here in case the array of all entities is rearranged, as is done e.g. by the CaBSP loader.
        ArrayT<MapFileBrushT>              MFBrushes;
        ArrayT<MapFileBezierPatchT>        MFPatches;
        ArrayT<MapFileTerrainT>            MFTerrains;
        ArrayT<MapFilePlantT>              MFPlants;
        ArrayT<MapFileModelT>              MFModels;
        std::map<std::string, std::string> MFProperties;
    };
}

#endif
