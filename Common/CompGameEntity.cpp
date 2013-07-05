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

#include "CompGameEntity.hpp"
#include "World.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "SceneGraph/_aux.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"


/**********************/
/*** SharedTerrainT ***/
/**********************/

SharedTerrainT::SharedTerrainT(const BoundingBox3dT& BB_, unsigned long SideLength_, const ArrayT<unsigned short>& HeightData_, MaterialT* Material_)
    : BB(BB_),
      SideLength(SideLength_),
      HeightData(HeightData_),
      Material(Material_),
      Terrain(&HeightData[0], SideLength, BB.AsBoxOfFloat())
{
}


SharedTerrainT::SharedTerrainT(std::istream& InFile)
{
    using namespace cf::SceneGraph;

    BB.Min    =aux::ReadVector3d(InFile);
    BB.Max    =aux::ReadVector3d(InFile);
    SideLength=aux::ReadUInt32(InFile);
    Material  =MaterialManager->GetMaterial(aux::ReadString(InFile));

    HeightData.PushBackEmptyExact(SideLength*SideLength);

    for (unsigned long i=0; i<HeightData.Size(); i++)
        HeightData[i]=aux::ReadUInt16(InFile);

    Terrain=TerrainT(&HeightData[0], SideLength, BB.AsBoxOfFloat());
}


void SharedTerrainT::WriteTo(std::ostream& OutFile) const
{
    using namespace cf::SceneGraph;

    aux::Write(OutFile, BB.Min);
    aux::Write(OutFile, BB.Max);
    aux::Write(OutFile, aux::cnc_ui32(SideLength));
    aux::Write(OutFile, Material->Name);    // There are only few terrains, no need for the Pool here.

    for (unsigned long i=0; i<HeightData.Size(); i++)
        aux::Write(OutFile, HeightData[i]);
}


/***********************/
/*** CompGameEntityT ***/
/***********************/

CompGameEntityT::CompGameEntityT(unsigned long MFIndex_)
    : m_MFIndex(MFIndex_),
      m_BspTree(NULL),
      m_CollModel(NULL)
{
}


CompGameEntityT::CompGameEntityT(std::istream& InFile, cf::SceneGraph::aux::PoolT& Pool, ModelManagerT& ModelMan, cf::SceneGraph::LightMapManT& LightMapMan, cf::SceneGraph::SHLMapManT& SHLMapMan, PlantDescrManT& PlantDescrMan)
    : m_MFIndex(cf::SceneGraph::aux::ReadUInt32(InFile)),
      m_BspTree(NULL),
      m_CollModel(NULL)
{
    // Read the shared terrain data.
    ArrayT<const TerrainT*> ShTe_SceneGr;
    ArrayT<cf::ClipSys::CollisionModelStaticT::TerrainRefT> ShTe_CollDet;

    for (unsigned long TerrainNr = cf::SceneGraph::aux::ReadUInt32(InFile); TerrainNr > 0; TerrainNr--)
    {
        SharedTerrainT* ShTe = new SharedTerrainT(InFile);

        m_Terrains.PushBack(ShTe);
        ShTe_SceneGr.PushBack(&ShTe->Terrain);
        ShTe_CollDet.PushBack(cf::ClipSys::CollisionModelStaticT::TerrainRefT(&ShTe->Terrain, ShTe->Material, ShTe->BB));
    }

    // Read the SceneGraph BSP tree.
    // Don't call cf::SceneGraph::BspTreeNodeT::CreateFromFile_cw() directly, because it would expect
    // that the "BspTree" string that states the identity of this NodeT has already been read!
    m_BspTree = dynamic_cast<cf::SceneGraph::BspTreeNodeT*>(cf::SceneGraph::GenericNodeT::CreateFromFile_cw(InFile, Pool,
        LightMapMan, SHLMapMan, PlantDescrMan, ShTe_SceneGr, ModelMan));

    if (m_BspTree == NULL)
    {
        // TODO: Clean-up...
        throw WorldT::LoadErrorT("Could not read scene graph bsp tree node for game entity!");
    }

    // Read the ClipSys collision model.
    bool HasCollisionModel = false;
    assert(sizeof(HasCollisionModel) == 1);
    InFile.read((char*)&HasCollisionModel, sizeof(HasCollisionModel));

    m_CollModel = HasCollisionModel ? new cf::ClipSys::CollisionModelStaticT(InFile, Pool, ShTe_CollDet) : NULL;

    // Read the origin.
    InFile.read((char*)&m_Origin.x, sizeof(m_Origin.x));
    InFile.read((char*)&m_Origin.y, sizeof(m_Origin.y));
    InFile.read((char*)&m_Origin.z, sizeof(m_Origin.z));

    // Read the property pairs.
    for (unsigned long NrOfPropertyPairs = cf::SceneGraph::aux::ReadUInt32(InFile); NrOfPropertyPairs > 0; NrOfPropertyPairs--)
    {
        const std::string Key   = cf::SceneGraph::aux::ReadString(InFile);
        const std::string Value = cf::SceneGraph::aux::ReadString(InFile);

        m_Properties[Key] = Value;
    }
}


CompGameEntityT::CompGameEntityT(const CompGameEntityT& Comp)
    : m_MFIndex(Comp.m_MFIndex),
      m_Origin(Comp.m_Origin)
{
    // TODO...
    assert(false);
}


CompGameEntityT::~CompGameEntityT()
{
    delete m_BspTree;
    delete m_CollModel;

    // Delete the terrains that were shared by the BspTree and the CollModel.
    for (unsigned int TerrainNr = 0; TerrainNr < m_Terrains.Size(); TerrainNr++)
        delete m_Terrains[TerrainNr];
}


CompGameEntityT* CompGameEntityT::Clone() const
{
    return new CompGameEntityT(*this);
}


void CompGameEntityT::WriteTo(std::ostream& OutFile, cf::SceneGraph::aux::PoolT& Pool) const
{
    // Write the how many-th entity in the map file this game entity corresponds to.
    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(m_MFIndex));

    // Write the shared terrain data.
    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(m_Terrains.Size()));

    for (unsigned long TerrainNr = 0; TerrainNr < m_Terrains.Size(); TerrainNr++)
        m_Terrains[TerrainNr]->WriteTo(OutFile);

    // Write the SceneGraph BSP tree.
    m_BspTree->WriteTo(OutFile, Pool);

    // Write the ClipSys collision model.
    const bool HasCollisionModel = (m_CollModel!=NULL);
    assert(sizeof(HasCollisionModel) == 1);

    OutFile.write((char*)&HasCollisionModel, sizeof(HasCollisionModel));
    if (m_CollModel)
        m_CollModel->SaveToFile(OutFile, Pool);

    // Write the origin.
    OutFile.write((char*)&m_Origin.x, sizeof(m_Origin.x));
    OutFile.write((char*)&m_Origin.y, sizeof(m_Origin.y));
    OutFile.write((char*)&m_Origin.z, sizeof(m_Origin.z));

    // Write the property pairs.
    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(m_Properties.size()));

    for (std::map<std::string, std::string>::const_iterator It = m_Properties.begin(); It != m_Properties.end(); ++It)
    {
        cf::SceneGraph::aux::Write(OutFile, It->first );
        cf::SceneGraph::aux::Write(OutFile, It->second);
    }
}
