/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "Ca3DEWorld.hpp"
#include "EntityManager.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "MaterialSystem/Material.hpp"
#include "Models/ModelManager.hpp"
#include "../../Common/WorldMan.hpp"
#include "SceneGraph/BspTreeNode.hpp"


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


static WorldManT WorldMan;


Ca3DEWorldT::Ca3DEWorldT(const char* FileName, ModelManagerT& ModelMan, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction) /*throw (WorldT::LoadErrorT)*/
    : m_World(WorldMan.LoadWorld(FileName, ModelMan, InitForGraphics, ProgressFunction)),
      m_ClipWorld(new cf::ClipSys::ClipWorldT(m_World->CollModel)),
      m_EntityManager(new EntityManagerT(*this)),
      m_ModelMan(ModelMan)
{
}


Ca3DEWorldT::~Ca3DEWorldT()
{
    Clear();
}


void Ca3DEWorldT::Clear()
{
    // Note that the EntityManager must be destructed *before* the ClipWorld,
    // so that all entities properly remove their clip model from the clip world on destruction.
    // In fact, this (control over the order of destruction) is the only reason why the
    // EntityManager is a pointer instead of an immediate member.
    if (m_EntityManager)
    {
        delete m_EntityManager;
        m_EntityManager = NULL;
    }

    if (m_ClipWorld)
    {
        delete m_ClipWorld;
        m_ClipWorld = NULL;
    }

    if (m_World)
    {
        WorldMan.FreeWorld(m_World);
        m_World = NULL;
    }
}


cf::ClipSys::ClipWorldT& Ca3DEWorldT::GetClipWorld()
{
    return *m_ClipWorld;
}


Vector3fT Ca3DEWorldT::GetAmbientLightColorFromBB(const BoundingBox3T<double>& Dimensions, const VectorT& Origin) const
{
    const Vector3dT BBCenter=scale(Dimensions.Min+Dimensions.Max, 0.5)+Origin;

#if 0
    // Performance profiling revealed that this method is frequently called
    // and that the call to CollModel->TraceRay() is pretty expensive!
    const Vector3dT Ray     =Vector3dT(0.0, 0.0, -999999.0);

    cf::ClipSys::TraceResultT Result(1.0);
    CollModel->TraceRay(BBCenter, Ray, MaterialT::Clip_Radiance, Result);

    Vector3dT       Ground  =BBCenter+Ray*Result.Fraction+Vector3dT(0, 0, 0.2);
#else
    // We therefore revert to this call, which is similarly limited (considers faces only) but is *much* faster!
    // Note that the proper future solution is to reimplement this method to use a precomputed lighting grid!!
    const double    Trace   =m_World->BspTree->ClipLine(BBCenter, Vector3dT(0.0, 0.0, -1.0), 0.0, 999999.0);
    Vector3dT       Ground  =BBCenter+Vector3dT(0.0, 0.0, -(Trace-0.2));
#endif
    unsigned long   LeafNr  =m_World->BspTree->WhatLeaf(Ground);

    // This is a relatively cheap (dumb) trick for dealing with problematic input (like some static detail models).
    if (!m_World->BspTree->Leaves[LeafNr].IsInnerLeaf)
    {
        const VectorT TestPoints[4]={ VectorT(Dimensions.Min.x, Dimensions.Min.y, Dimensions.Max.z)+Origin,
                                      VectorT(Dimensions.Min.x, Dimensions.Max.y, Dimensions.Max.z)+Origin,
                                      VectorT(Dimensions.Max.x, Dimensions.Min.y, Dimensions.Max.z)+Origin,
                                      VectorT(Dimensions.Max.x, Dimensions.Max.y, Dimensions.Max.z)+Origin };

        Ground.z=-999999.0;

        for (unsigned long TestNr=0; TestNr<4; TestNr++)
        {
#if 0
            // Same performance issue as above...
            CollModel->TraceRay(TestPoints[TestNr], Ray, MaterialT::Clip_Radiance, Result);     // BUG / FIXME: I think we have to re-init Result here!
            const Vector3dT     TestGround=TestPoints[TestNr]+Ray*Result.Fraction+Vector3dT(0, 0, 0.2);
#else
            const double        TestTrace =m_World->BspTree->ClipLine(TestPoints[TestNr], Vector3dT(0.0, 0.0, -1.0), 0.0, 999999.0);
            const Vector3dT     TestGround=TestPoints[TestNr]+Vector3dT(0.0, 0.0, -(TestTrace-0.2));
#endif
            const unsigned long TestLeafNr=m_World->BspTree->WhatLeaf(TestGround);

            if (!m_World->BspTree->Leaves[TestLeafNr].IsInnerLeaf) continue;

            if (TestGround.z>Ground.z)
            {
                Ground=TestGround;
                LeafNr=TestLeafNr;
            }
        }
    }

    for (unsigned long FNr=0; FNr<m_World->BspTree->Leaves[LeafNr].FaceChildrenSet.Size(); FNr++)
    {
        cf::SceneGraph::FaceNodeT* FaceNode=m_World->BspTree->FaceChildren[m_World->BspTree->Leaves[LeafNr].FaceChildrenSet[FNr]];
        Vector3fT                  AmbientLightColor;

        if (FaceNode->GetLightmapColorNearPosition(Ground, AmbientLightColor))
            return AmbientLightColor;
    }

    return Vector3fT(1.0f, 1.0f, 1.0f);
}


const ArrayT<unsigned long>& Ca3DEWorldT::GetAllEntityIDs() const
{
    static ArrayT<unsigned long> AllEntityIDs;

    AllEntityIDs.Clear();
    m_EntityManager->GetAllEntityIDs(AllEntityIDs);

    return AllEntityIDs;
}


BaseEntityT* Ca3DEWorldT::GetBaseEntityByID(unsigned long EntityID) const
{
    return m_EntityManager->GetBaseEntityByID(EntityID);
}


unsigned long Ca3DEWorldT::CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin)
{
    return m_EntityManager->CreateNewEntityFromBasicInfo(Properties, NULL, NULL, (unsigned long)(-1), (unsigned long)(-1), CreationFrameNr, Origin);
}


void Ca3DEWorldT::RemoveEntity(unsigned long EntityID)
{
    m_EntityManager->RemoveEntity(EntityID);
}


const CafuModelT* Ca3DEWorldT::GetModel(const std::string& FileName) const
{
    return m_ModelMan.GetModel(FileName);
}
