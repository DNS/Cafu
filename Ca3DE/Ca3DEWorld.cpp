/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Ca3DEWorld.hpp"
#include "EngineEntity.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "ConsoleCommands/Console.hpp"      // For cf::va().
#include "GameSys/World.hpp"
#include "GuiSys/AllComponents.hpp"         // for initing the m_ScriptState
#include "GuiSys/GuiImpl.hpp"               // for initing the m_ScriptState
#include "GuiSys/Window.hpp"                // for initing the m_ScriptState
#include "MaterialSystem/Material.hpp"
#include "Models/ModelManager.hpp"
#include "../Common/CompGameEntity.hpp"
#include "../Common/WorldMan.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "String.hpp"


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


static WorldManT WorldMan;


Ca3DEWorldT::Ca3DEWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction) /*throw (WorldT::LoadErrorT)*/
    : m_World(WorldMan.LoadWorld(FileName, ModelMan, GuiRes, InitForGraphics, ProgressFunction)),
      m_ClipWorld(new cf::ClipSys::ClipWorldT(m_World->m_StaticEntityData[0]->m_CollModel)),
      m_PhysicsWorld(m_World->m_StaticEntityData[0]->m_CollModel),
      m_ScriptState(new cf::UniScriptStateT),   // Need a pointer because the dtor order is important.
      m_ScriptWorld(NULL),
      m_EngineEntities()
{
    cf::GameSys::WorldT::InitScriptState(*m_ScriptState);

#if 0
    // We cannot use this method, which in fact is kind of obsolete:
    // It would attempt to re-register the Console and ConsoleInterface libraries,
    // which was already done above in cf::GameSys::WorldT::InitScriptState().
    // (Both InitScriptState() methods should probably be removed / refactored.)
    cf::GuiSys::GuiImplT::InitScriptState(*m_ScriptState);
#else
    {
        // For each class that the TypeInfoManTs know about, add a (meta-)table to the registry of the LuaState.
        // The (meta-)table holds the Lua methods that the respective class implements in C++,
        // and is to be used as metatable for instances of this class.
        cf::ScriptBinderT Binder(m_ScriptState->GetLuaState());

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
            InitForGraphics ? cf::GameSys::WorldT::RealmClient : cf::GameSys::WorldT::RealmServer,
            *m_ScriptState,
            ModelMan,
            GuiRes,
            *cf::ClipSys::CollModelMan,   // TODO: The CollModelMan should not be a global, but rather be instantiated along with the ModelMan and GuiRes.
            m_ClipWorld,
            &m_PhysicsWorld);

        m_ScriptWorld->LoadScript(ScriptName, 0 /*cf::GameSys::WorldT::InitFlag_OnlyStatic*/);
    }
    catch (const cf::GameSys::WorldT::InitErrorT& IE)
    {
        throw WorldT::LoadErrorT(IE.what());
    }


    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    // For player prototype entities make sure that their models are not shown.
    // Showing their models may be desirable in the Map Editor, but not in the Engine.
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        if (AllEnts[EntNr]->GetComponent("HumanPlayer") != NULL)
        {
            for (unsigned int i = 0; true; i++)
            {
                IntrusivePtrT<cf::GameSys::ComponentBaseT> ModelComp = AllEnts[EntNr]->GetComponent("Model", i);

                if (ModelComp == NULL) break;
                ModelComp->SetMember("Show", false);
            }
        }
    }

    // Create a matching
    //     - CompGameEntityT instance and
    //     - EngineEntityT instance
    // for each entity in the script world (including the world (root) entity).
    if (AllEnts.Size() != m_World->m_StaticEntityData.Size())
        throw WorldT::LoadErrorT("The number of entities in the .cent file must match the number of entities in the .bsp file.");

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        // This is also checked in the `cf::GameSys::WorldT` ctor, see there for details.
        // It is repeated here as a reminder: entity IDs are used as indices into m_World->m_StaticEntityData[].
        assert(AllEnts[EntNr]->GetID() == EntNr);

        AllEnts[EntNr]->SetApp(new CompGameEntityT(m_World->m_StaticEntityData[EntNr]));
        CreateNewEntityFromBasicInfo(AllEnts[EntNr], 1 /*ServerFrameNr*/);
    }

    // Have all components of all entities precache their resources.
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        const ArrayT< IntrusivePtrT<cf::GameSys::ComponentBaseT> >& Components = AllEnts[EntNr]->GetComponents();

        for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
        {
            Components[CompNr]->PreCache();
        }
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
    delete m_ScriptState;
    m_ScriptState = NULL;

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


void Ca3DEWorldT::CreateNewEntityFromBasicInfo(IntrusivePtrT<cf::GameSys::EntityT> Ent, unsigned long CreationFrameNr)
{
    const unsigned long NewEntityID = Ent->GetID();

    while (m_EngineEntities.Size() <= NewEntityID)
        m_EngineEntities.PushBack(NULL);

    // Well... quite clearly, EngineEntityT should be merged into the App component...!
    // However, note that there is one very important requirement:
    // Iterating over the m_EngineEntities array iterates the entities in the order of increasing ID.
    // The CaServerWorldT::WriteClientDeltaUpdateMessages() and the related client code *rely* on this order!
    assert(m_EngineEntities[NewEntityID] == NULL);
    delete m_EngineEntities[NewEntityID];
    m_EngineEntities[NewEntityID] = new EngineEntityT(Ent, CreationFrameNr);
}
