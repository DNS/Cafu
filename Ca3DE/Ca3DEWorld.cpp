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

#include "wx/msgdlg.h"

#include "Ca3DEWorld.hpp"
#include "EngineEntity.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "ConsoleCommands/Console.hpp"      // For cf::va().
#include "GameSys/World.hpp"
#include "GuiSys/AllComponents.hpp"         // for initing the m_ScriptState_NEW
#include "GuiSys/GuiImpl.hpp"               // for initing the m_ScriptState_NEW
#include "GuiSys/Window.hpp"                // for initing the m_ScriptState_NEW
#include "MaterialSystem/Material.hpp"
#include "Models/ModelManager.hpp"
#include "../Common/CompGameEntity.hpp"
#include "../Common/WorldMan.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "String.hpp"
#include "../Games/Game.hpp"


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


static WorldManT WorldMan;


Ca3DEWorldT::Ca3DEWorldT(cf::GameSys::GameInfoI* GameInfo, cf::GameSys::GameI* Game, const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction) /*throw (WorldT::LoadErrorT)*/
    : m_Game(Game),
      m_World(WorldMan.LoadWorld(FileName, ModelMan, GuiRes, InitForGraphics, ProgressFunction)),
      m_ClipWorld(new cf::ClipSys::ClipWorldT(m_World->m_StaticEntityData[0]->m_CollModel)),
      m_PhysicsWorld(m_World->m_StaticEntityData[0]->m_CollModel),
      m_ScriptState_OLD(GameInfo, m_Game),
      m_ScriptState_NEW(new cf::UniScriptStateT),   // Need a pointer because the dtor order is important.
      m_ScriptWorld(NULL),
      m_EngineEntities(),
      m_ModelMan(ModelMan),
      m_GuiRes(GuiRes)
{
    cf::GameSys::WorldT::InitScriptState(*m_ScriptState_NEW);

#if 0
    // We cannot use this method, which in fact is kind of obsolete:
    // It would attempt to re-register the Console and ConsoleInterface libraries,
    // which was already done above in cf::GameSys::WorldT::InitScriptState().
    // (Both InitScriptState() methods should probably be removed / refactored.)
    cf::GuiSys::GuiImplT::InitScriptState(*m_ScriptState_NEW);
#else
    {
        // For each class that the TypeInfoManTs know about, add a (meta-)table to the registry of the LuaState.
        // The (meta-)table holds the Lua methods that the respective class implements in C++,
        // and is to be used as metatable for instances of this class.
        cf::ScriptBinderT Binder(m_ScriptState_NEW->GetLuaState());

        Binder.Init(cf::GuiSys::GetGuiTIM());
        Binder.Init(cf::GuiSys::GetWindowTIM());
        Binder.Init(cf::GuiSys::GetComponentTIM());
    }
#endif

    try
    {
        std::string ScriptName = cf::String::StripExt(FileName) + ".cent";
        ScriptName = cf::String::Replace(ScriptName, "/Worlds/", "/Maps/");
        ScriptName = cf::String::Replace(ScriptName, "\\Worlds\\", "\\Maps\\");

        m_ScriptWorld = new cf::GameSys::WorldT(
            *m_ScriptState_NEW,
            ModelMan,
            GuiRes,
            *cf::ClipSys::CollModelMan,   // TODO: The CollModelMan should not be a global, but rather be instantiated along with the ModelMan and GuiRes.
            m_ClipWorld,
            &m_PhysicsWorld);

        cf::GameSys::WorldT::LoadScript(
            m_ScriptWorld,
            ScriptName,
            0 /*cf::GameSys::WorldT::InitFlag_InMapEditor*/);
    }
    catch (const cf::GameSys::WorldT::InitErrorT& IE)
    {
        throw WorldT::LoadErrorT(IE.what());
    }


    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    // Create a matching
    //     - CompGameEntityT instance and
    //     - EngineEntityT instance
    // for each entity in the script world (including the world (root) entity).
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        // This is also checked in the `cf::GameSys::WorldT` ctor, see there for details.
        // It is repeated here as a remainder: entity IDs are used as indices into m_World->m_StaticEntityData[].
        assert(AllEnts[EntNr]->GetID() == EntNr);

        IntrusivePtrT<CompGameEntityT> GE =
            new CompGameEntityT(EntNr < m_World->m_StaticEntityData.Size() ? m_World->m_StaticEntityData[EntNr] : NULL);

        AllEnts[EntNr]->SetApp(GE);

        // Note that this can fail (and thus not add anything to the m_EngineEntities array),
        // especially for now unsupported entity classes like "func_group"!
        CreateNewEntityFromBasicInfo(GE, 1 /*ServerFrameNr*/);
    }
}


Ca3DEWorldT::~Ca3DEWorldT()
{
    // Note that the engine entities must be destructed *before* the ClipWorld,
    // so that they properly remove their clip model from the clip world on destruction.
    for (unsigned long EntityNr=0; EntityNr<m_EngineEntities.Size(); EntityNr++)
        delete m_EngineEntities[EntityNr];

    m_EngineEntities.Clear();

    m_ScriptWorld = NULL;

    // The script state may still hold entities that have collision model components that have registered
    // collision models with the clip world. Thus, make sure to delete the script state before the clip world,
    // so that the clip world is left clean.
    delete m_ScriptState_NEW;
    m_ScriptState_NEW = NULL;

    // delete m_PhysicsWorld;
    // m_PhysicsWorld = NULL;

    delete m_ClipWorld;
    m_ClipWorld = NULL;

    if (m_World)
    {
        WorldMan.FreeWorld(m_World);
        m_World = NULL;
    }
}


cf::GameSys::GameI* Ca3DEWorldT::GetGame()
{
    return m_Game;
}


cf::ClipSys::ClipWorldT& Ca3DEWorldT::GetClipWorld()
{
    return *m_ClipWorld;
}


PhysicsWorldT& Ca3DEWorldT::GetPhysicsWorld()
{
    return m_PhysicsWorld;
}


Vector3fT Ca3DEWorldT::GetAmbientLightColorFromBB(const BoundingBox3T<double>& Dimensions, const VectorT& Origin) const
{
    const Vector3dT BBCenter = scale(Dimensions.Min+Dimensions.Max, 0.5) + Origin;
    const cf::SceneGraph::BspTreeNodeT* BspTree = m_World->m_StaticEntityData[0]->m_BspTree;

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
    const double    Trace   =BspTree->ClipLine(BBCenter, Vector3dT(0.0, 0.0, -1.0), 0.0, 999999.0);
    Vector3dT       Ground  =BBCenter+Vector3dT(0.0, 0.0, -(Trace-0.2));
#endif
    unsigned long   LeafNr  =BspTree->WhatLeaf(Ground);

    // This is a relatively cheap (dumb) trick for dealing with problematic input (like some static detail models).
    if (!BspTree->Leaves[LeafNr].IsInnerLeaf)
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
            const double        TestTrace =BspTree->ClipLine(TestPoints[TestNr], Vector3dT(0.0, 0.0, -1.0), 0.0, 999999.0);
            const Vector3dT     TestGround=TestPoints[TestNr]+Vector3dT(0.0, 0.0, -(TestTrace-0.2));
#endif
            const unsigned long TestLeafNr=BspTree->WhatLeaf(TestGround);

            if (!BspTree->Leaves[TestLeafNr].IsInnerLeaf) continue;

            if (TestGround.z>Ground.z)
            {
                Ground=TestGround;
                LeafNr=TestLeafNr;
            }
        }
    }

    for (unsigned long FNr=0; FNr<BspTree->Leaves[LeafNr].FaceChildrenSet.Size(); FNr++)
    {
        cf::SceneGraph::FaceNodeT* FaceNode=BspTree->FaceChildren[BspTree->Leaves[LeafNr].FaceChildrenSet[FNr]];
        Vector3fT                  AmbientLightColor;

        if (FaceNode->GetLightmapColorNearPosition(Ground, AmbientLightColor, BspTree->GetLightMapPatchSize()))
            return AmbientLightColor;
    }

    return Vector3fT(1.0f, 1.0f, 1.0f);
}


const ArrayT<unsigned long>& Ca3DEWorldT::GetAllEntityIDs() const
{
    static ArrayT<unsigned long> AllEntityIDs;

    AllEntityIDs.Overwrite();

    for (unsigned long EntityNr=0; EntityNr<m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr]!=NULL)
            AllEntityIDs.PushBack(EntityNr);

    return AllEntityIDs;
}


IntrusivePtrT<GameEntityI> Ca3DEWorldT::GetGameEntityByID(unsigned long EntityID) const
{
    if (EntityID<m_EngineEntities.Size())
        if (m_EngineEntities[EntityID]!=NULL)
            return m_EngineEntities[EntityID]->GetGameEntity();

    return NULL;
}


const CafuModelT* Ca3DEWorldT::GetModel(const std::string& FileName) const
{
    return m_ModelMan.GetModel(FileName);
}


cf::GuiSys::GuiResourcesT& Ca3DEWorldT::GetGuiResources() const
{
    return m_GuiRes;
}


unsigned long Ca3DEWorldT::CreateNewEntityFromBasicInfo(IntrusivePtrT<const CompGameEntityT> CompGameEnt,
    unsigned long CreationFrameNr)
{
    try
    {
        // 1. Determine from the entity class name (e.g. "monster_argrenade") the C++ class name (e.g. "EntARGrenadeT").
        std::map<std::string, std::string>::const_iterator EntClassNamePair = CompGameEnt->GetStaticEntityData()->m_Properties.find("classname");

        const std::string EntClassName =
            (EntClassNamePair == CompGameEnt->GetStaticEntityData()->m_Properties.end()) ? std::string("unknown") : EntClassNamePair->second;

        std::string CppClassName = m_ScriptState_OLD.GetCppClassNameFromEntityClassName(EntClassName);

        if (CppClassName == "")
        {
            // This can happen for EntClassName's for which we never had an entry in EntityClassDefs.lua,
            // (e.g. for "ammo_buckshot"), or for EntClassName's for which we have an entry, but CppClass="",
            // (e.g. for "ammo_9mmclip"). As entity classes are obsolete anyway, the transitional fix is simple:
            CppClassName = "EntInfoGenericT";

            // throw std::runtime_error("C++ class name for entity class name \""+EntClassName+"\" not found.\n");
        }


        // 2. Find the related type info.
        const cf::TypeSys::TypeInfoT* TI = m_Game->GetEntityTIM().FindTypeInfoByName(CppClassName.c_str());

        if (TI==NULL)
        {
            // These days, this can happen for entity classes such as "EntButterflyT" that have already been fully
            // ported to the new component system, and whose old C++ class has already been removed.
            // Therefore, replace the old C++ class with one that deliberately does nothing:
            TI = m_Game->GetEntityTIM().FindTypeInfoByName("EntInfoGenericT");

            if (TI==NULL)
                throw std::runtime_error("No type info found for entity class \""+EntClassName+"\" with C++ class name \""+CppClassName+"\".\n");
        }


        // 3. Create an instance of the desired entity type.
        const unsigned long NewEntityID = CompGameEnt->GetEntity()->GetID();

        IntrusivePtrT<GameEntityI> NewEntity = m_Game->CreateGameEntity(
            TI, CompGameEnt->GetEntity(), CompGameEnt->GetStaticEntityData()->m_Properties,
            CompGameEnt->GetStaticEntityData()->m_BspTree, NewEntityID, this);

        if (NewEntity.IsNull())
            throw std::runtime_error("Could not create entity of class \""+EntClassName+"\" with C++ class name \""+CppClassName+"\".\n");

        while (m_EngineEntities.Size() <= NewEntityID)
            m_EngineEntities.PushBack(NULL);

        // Well... quite clearly, EngineEntityT should be merged into the App component...!
        // However, note that there is one very important requirement:
        // Iterating over the m_EngineEntities array iterates the entities in the order of increasing ID.
        // The CaServerWorldT::WriteClientDeltaUpdateMessages() and the related client code *rely* on this order!
        assert(m_EngineEntities[NewEntityID] == NULL);
        delete m_EngineEntities[NewEntityID];
        m_EngineEntities[NewEntityID] = new EngineEntityT(NewEntity, CompGameEnt->GetEntity(), CreationFrameNr);

        return NewEntityID;
    }
    catch (const std::runtime_error& RE)
    {
        Console->Warning(RE.what());
    }

    // Return error code.
    return 0xFFFFFFFF;
}
