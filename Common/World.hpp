/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************/
/*** World (Header) ***/
/**********************/

#ifndef CAFU_WORLD_HPP_INCLUDED
#define CAFU_WORLD_HPP_INCLUDED

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


namespace cf { namespace SceneGraph { class BspTreeNodeT; } }
namespace cf { namespace ClipSys    { class CollisionModelStaticT; } }
namespace cf { namespace GameSys    { class WorldT; } }
namespace cf { namespace GuiSys     { class GuiResourcesT; } }


struct MapT
{
    // TODO: Move into FaceNodeT!
    const static double RoundEpsilon;   ///< The maximum amount that is allowed for geometry-related rounding errors.
    const static double MinVertexDist;  ///< The minimum distance that vertices of faces and portals must be apart.
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


class StaticEntityDataT
{
    public:

    StaticEntityDataT();
    StaticEntityDataT(std::istream& InFile, cf::SceneGraph::aux::PoolT& Pool, ModelManagerT& ModelMan, cf::SceneGraph::LightMapManT& LightMapMan, cf::SceneGraph::SHLMapManT& SHLMapMan, PlantDescrManT& PlantDescrMan);

    ~StaticEntityDataT();

    void WriteTo(std::ostream& OutFile, cf::SceneGraph::aux::PoolT& Pool) const;


    ArrayT<SharedTerrainT*>             m_Terrains;   ///< Terrains are shared among the BspTree (graphics world) and the CollModel (physics world).
    cf::SceneGraph::BspTreeNodeT*       m_BspTree;
    cf::ClipSys::CollisionModelStaticT* m_CollModel;


    private:

    StaticEntityDataT(const StaticEntityDataT&);    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const StaticEntityDataT&);     ///< Use of the Assignment Operator is not allowed.
};


class WorldT
{
    public:

    struct LoadErrorT { const char* Msg; LoadErrorT(const char* Msg_) : Msg(Msg_) {} };
    struct SaveErrorT { const char* Msg; SaveErrorT(const char* Msg_) : Msg(Msg_) {} };

    typedef void (*ProgressFunctionT)(float ProgressPercent, const char* ProgressText);


    /// Constructor for creating an empty world.
    WorldT();

    /// Constructor for creating a world from a .cw file.
    WorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, ProgressFunctionT ProgressFunction=NULL) /*throw (LoadErrorT)*/;

    /// Destructor.
    ~WorldT();

    /// Saves the world to disk.
    void SaveToDisk(const char* FileName) const /*throw (SaveErrorT)*/;


    ArrayT<StaticEntityDataT*>   m_StaticEntityData;
    cf::SceneGraph::LightMapManT LightMapMan;
    cf::SceneGraph::SHLMapManT   SHLMapMan;
    PlantDescrManT               PlantDescrMan;


    private:

    WorldT(const WorldT&);              // Use of the Copy    Constructor is not allowed.
    void operator = (const WorldT&);    // Use of the Assignment Operator is not allowed.
};

#endif
