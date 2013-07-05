/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_COMPONENT_GAME_ENTITY_HPP_INCLUDED
#define CAFU_COMPONENT_GAME_ENTITY_HPP_INCLUDED

#include "GameSys/CompBase.hpp"
#include "GameSys/Entity.hpp"       // For GetGameEnt() only.
#include "Terrain/Terrain.hpp"

namespace cf { namespace ClipSys    { class CollisionModelStaticT; } }
namespace cf { namespace SceneGraph { namespace aux { class PoolT; } } }
namespace cf { namespace SceneGraph { class BspTreeNodeT; } }
namespace cf { namespace SceneGraph { class LightMapManT; } }
namespace cf { namespace SceneGraph { class SHLMapManT; } }

class MaterialT;
class ModelManagerT;
class PlantDescrManT;


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


/// This component houses the "engine-specific" parts of its entity.
/// It is intended for use by the implementing applications only (map compilers, engine), that is,
/// as the "App" component of `cf::GameSys::EntityT`s. It is not intended for use in game scripts.
/// As such, it doesn't integrate with the TypeSys, and thus isn't available for scripting and
/// whereever else we need the related meta-data.
class CompGameEntityT : public cf::GameSys::ComponentBaseT
{
    public:

    /// The constructor.
    CompGameEntityT(unsigned long MFIndex_);
    CompGameEntityT(std::istream& InFile, cf::SceneGraph::aux::PoolT& Pool, ModelManagerT& ModelMan, cf::SceneGraph::LightMapManT& LightMapMan, cf::SceneGraph::SHLMapManT& SHLMapMan, PlantDescrManT& PlantDescrMan);

    /// The copy constructor. It creates a new component as a copy of another component.
    /// @param Comp   The component to copy-construct this component from.
    CompGameEntityT(const CompGameEntityT& Comp);

    /// The destructor.
    ~CompGameEntityT();

    // Base class overrides.
    CompGameEntityT* Clone() const;
    const char* GetName() const { return "GameEntity"; }

    void WriteTo(std::ostream& OutFile, cf::SceneGraph::aux::PoolT& Pool) const;


    const unsigned long                 m_MFIndex;    ///< This game entity corresponds to (was loaded from) this entity in the cmap file. This index must explicitly be kept here, because the array of GameEntityTs does usually not correspond to the array of cmap file entities, as the CaBSP loader code may rearrange it. Note that this index should always be checked for validity: It defaults to 0xFFFFFFFF when for any reason no corresponding cmap file entity is known.
    Vector3dT                           m_Origin;
    ArrayT<SharedTerrainT*>             m_Terrains;   ///< Terrains are shared among the BspTree (graphics world) and the CollModel (physics world).
    cf::SceneGraph::BspTreeNodeT*       m_BspTree;
    cf::ClipSys::CollisionModelStaticT* m_CollModel;
    std::map<std::string, std::string>  m_Properties;
};


inline IntrusivePtrT<CompGameEntityT> GetGameEnt(IntrusivePtrT<cf::GameSys::EntityT> Entity)
{
    return dynamic_pointer_cast<CompGameEntityT>(Entity->GetApp());
}

#endif
