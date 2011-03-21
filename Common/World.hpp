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

/**********************/
/*** World (Header) ***/
/**********************/

#ifndef _WORLD_HPP_
#define _WORLD_HPP_

#include "Templates/Array.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Plane3.hpp"
#include "Math3D/Polygon.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "SceneGraph/LightMapMan.hpp"
#include "SceneGraph/SHLMapMan.hpp"
#include "Terrain/Terrain.hpp"
#include "Plants/PlantDescrMan.hpp"

#include <string>
#include <map>


namespace cf { namespace SceneGraph { class BspTreeNodeT; } }
namespace cf { namespace ClipSys    { class CollisionModelStaticT; } }


struct PointLightT
{
    VectorT Origin;
    VectorT Dir;        ///< Direction of the cone.
    float   Angle;      ///< Cone opening angle, in radians.
    VectorT Intensity;  ///< Intensity in [W/sr].
};


struct MapT
{
    // TODO: Move into FaceNodeT!
    const static double RoundEpsilon;   ///< The maximum amount that is allowed for geometry-related rounding errors.
    const static double MinVertexDist;  ///< The minimum distance that vertices of faces and portals must be apart.
};


struct InfoPlayerStartT
{
    VectorT        Origin;
    unsigned short Heading;
    unsigned short Pitch;
    unsigned short Bank;
};


class SharedTerrainT
{
    public:

    // Note that these constructors can theoretically throw because the TerrainT constructor can throw.
    // In practice this should never happen though, because otherwise a .cmap or .cw file contained an invalid terrain.
    SharedTerrainT(const BoundingBox3dT& BB_, unsigned long SideLength_, const ArrayT<unsigned short>& HeightData_, MaterialT* Material_);
    SharedTerrainT(std::istream& InFile);

    void WriteTo(std::ostream& OutFile) const;


    BoundingBox3dT         BB;          ///< The lateral dimensions of the terrain.
    unsigned long          SideLength;  ///< Side length of the terrain height data.
    ArrayT<unsigned short> HeightData;  ///< The height data this terrain is created from (size==SideLength*SideLength).
    MaterialT*             Material;    ///< The material for the terrain surface.
    TerrainT               Terrain;
};


class GameEntityT
{
    public:

    GameEntityT(unsigned long MFIndex_);
    ~GameEntityT();

    const unsigned long                 MFIndex;    ///< This game entity corresponds to (was loaded from) this entity in the cmap file. This index must explicitly be kept here, because the array of GameEntityTs does usually not correspond to the array of cmap file entities, as the CaBSP loader code may rearrange it. Note that this index should always be checked for validity: It defaults to 0xFFFFFFFF when for any reason no corresponding cmap file entity is known.
    VectorT                             Origin;
    ArrayT<SharedTerrainT*>             Terrains;   ///< Terrains are shared among the BspTree (graphics world) and the CollModel (physics world).
    cf::SceneGraph::BspTreeNodeT*       BspTree;
    cf::ClipSys::CollisionModelStaticT* CollModel;
    std::map<std::string, std::string>  Properties;


    private:

    GameEntityT(const GameEntityT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const GameEntityT&);   ///< Use of the Assignment Operator is not allowed.
};


class WorldT
{
    public:

    struct LoadErrorT { const char* Msg; LoadErrorT(const char* Msg_) : Msg(Msg_) {} };
    struct SaveErrorT { const char* Msg; SaveErrorT(const char* Msg_) : Msg(Msg_) {} };

    typedef void (*ProgressFunctionT)(float ProgressPercent, const char* ProgressText);


    /// Constructor for creating an empty world.
    WorldT();

    /// Destructor.
    ~WorldT();

    /// Constructor for creating a world from a .cw file.
    WorldT(const char* FileName, ProgressFunctionT ProgressFunction=NULL) /*throw (LoadErrorT)*/;

    /// Saves the world to disk.
    void SaveToDisk(const char* FileName) const /*throw (SaveErrorT)*/;


    ArrayT<SharedTerrainT*>             Terrains;
    cf::SceneGraph::BspTreeNodeT*       BspTree;
    cf::ClipSys::CollisionModelStaticT* CollModel;
    ArrayT<InfoPlayerStartT>            InfoPlayerStarts;
    ArrayT<PointLightT>                 PointLights;
    ArrayT<GameEntityT*>                GameEntities;
    cf::SceneGraph::LightMapManT        LightMapMan;
    cf::SceneGraph::SHLMapManT          SHLMapMan;
    PlantDescrManT                      PlantDescrMan;


    private:

    WorldT(const WorldT&);              // Use of the Copy    Constructor is not allowed.
    void operator = (const WorldT&);    // Use of the Assignment Operator is not allowed.
};

#endif
