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

#include "GameImpl.hpp"

#include "cw.hpp"
#include "EntityCreateParams.hpp"
#include "HumanPlayer.hpp"
#include "PhysicsWorld.hpp"
#include "ScriptState.hpp"
#include "TypeSys.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"
#include "_ResourceManager.hpp"

#include "ConsoleCommands/Console.hpp"
#include "Models/Model_proxy.hpp"

#include <map>

#if defined(_WIN32) && defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#if defined(__linux__)
    #define _stricmp strcasecmp
    #include <dirent.h>
#endif


using namespace cf::GameSys;


// Static method that returns the singleton instance.
cf::GameSys::GameImplT& cf::GameSys::GameImplT::GetInstance()
{
    static GameImplT GI;

    return GI;
}


// Provide a definition for Game, the global (DLL-wide) pointer to a GameI implementation.
cf::GameSys::GameI* cf::GameSys::Game=&cf::GameSys::GameImplT::GetInstance();


cf::GameSys::GameImplT::GameImplT()
    : RunningAsClient(false),
      RunningAsServer(false),
      Sv_PhysicsWorld(NULL),
      Cl_PhysicsWorld(NULL),
      ScriptState(NULL),
      IsThinking(false)
{
}


void cf::GameSys::GameImplT::Initialize(bool AsClient, bool AsServer)
{
    RunningAsClient=AsClient;
    RunningAsServer=AsServer;

    if (RunningAsClient)
    {
        // We can initialize most MatSys-related resources only AFTER the global pointers have been set:
        // MatSys::Renderer, MatSys::TextureMapManager, and MaterialSystem.
        // These point to the implementations that are provided by the engine main exe module,
        // and are accessed e.g. by the models and particles constructors.
        ResMan.Init();


        // Materials that should be pre-cached (as many as possible) must be registered with the MatSys *before* the engine triggers the MatSys's pre-caching.
        // On the other hand, registering materials can only be done *after* GetGame() was called.
        // So a good place for registering materials that we need within this DLL is for example at the end of GetGame(), or right here.
        // Registering the materials that our models will use is quite easy, because we just need to instantiate a ModelProxyT of each.
        // (Nothing breaks if the following list is simply deleted, these models just aren't pre-cached then.)
        //
        // It *is* however quite stupid that this list is not more tighly coupled to the relevant entity classes.
        // However, as the time of entity creation (and thus first model access and thus material registry time) is essentially unknown,
        // it seems simplest to list the models desired for pre-caching right here.
        //
        // Another curiosity that I just noticed: Not pre-caching here means not only to register the materials later (on first use),
        // but also to effectively load and initialize the models only at that later time, what might be very expensive, too!
        {
            // Simply loop over all weapon slots for getting all possibly carried weapons, and touch their models.
            for (char cwNr=0; cwNr<13; cwNr++)
            {
                CarriedWeaponT::GetCarriedWeapon(cwNr)->GetViewWeaponModel();
                CarriedWeaponT::GetCarriedWeapon(cwNr)->GetPlayerWeaponModel();
            }

            // Simply loop over all possible player models (all valid State.ModelIndex values for EntHumanPlayerTs).
            for (unsigned long pmNr=0; pmNr<7; pmNr++)
                EntHumanPlayerT::GetModelFromPlayerModelIndex(pmNr);

            // And the rest. Observe that static detail models are NOT mentioned (how could they?).
            static ModelProxyT M01("Games/DeathMatch/Models/Items/Ammo_DartGun.mdl");
            static ModelProxyT M02("Games/DeathMatch/Models/Items/Ammo_DesertEagle.mdl");

            static ModelProxyT M03("Games/DeathMatch/Models/LifeForms/Butterfly.mdl");
            static ModelProxyT M04("Games/DeathMatch/Models/LifeForms/Eagle.mdl");
            static ModelProxyT M05("Games/DeathMatch/Models/LifeForms/FaceHugger.mdl");

            static ModelProxyT M06("Games/DeathMatch/Models/Weapons/9mmAR_w.mdl");
            static ModelProxyT M07("Games/DeathMatch/Models/Weapons/BattleScythe_w.mdl");
            static ModelProxyT M08("Games/DeathMatch/Models/Weapons/Bazooka_w.mdl");
            static ModelProxyT M09("Games/DeathMatch/Models/Weapons/Beretta_w.mdl");
            static ModelProxyT M10("Games/DeathMatch/Models/Weapons/DartGun_w.mdl");
            static ModelProxyT M11("Games/DeathMatch/Models/Weapons/DesertEagle_w.mdl");
            static ModelProxyT M12("Games/DeathMatch/Models/Weapons/Egon_w.mdl");
            static ModelProxyT M13("Games/DeathMatch/Models/Weapons/FaceHugger_w.mdl");
            static ModelProxyT M14("Games/DeathMatch/Models/Weapons/Gauss_w.mdl");
            static ModelProxyT M15("Games/DeathMatch/Models/Weapons/Grenade_w.mdl");
            static ModelProxyT M16("Games/DeathMatch/Models/Weapons/HornetGun_w.mdl");
            static ModelProxyT M17("Games/DeathMatch/Models/Weapons/Shotgun_w.mdl");
            static ModelProxyT M18("Games/DeathMatch/Models/Weapons/Tripmine_w.mdl");
        }


        // Precache all sounds known to be used here.
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/PickUp")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/Respawn")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Ambient/Jungle")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/DesertEagle_Shot1")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/9mmAR_GLauncher")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/9mmAR_Shot1")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/BattleScythe")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/FaceHugger_Throw")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_sBarrel")));
        PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_dBarrel")));
    }

    if (RunningAsServer)
    {
    }
}


void cf::GameSys::GameImplT::Release()
{
    assert(Sv_PhysicsWorld==NULL);
    assert(Cl_PhysicsWorld==NULL);

    if (RunningAsClient)
    {
        ResMan.ShutDown();

        // Remove reference to precached sounds here.
        for (unsigned long i=0; i<PreCacheSounds.Size(); i++)
            delete PreCacheSounds[i];
    }

    if (RunningAsServer)
    {
    }
}


void cf::GameSys::GameImplT::Sv_PrepareNewWorld(const char* /*WorldFileName*/, const cf::ClipSys::CollisionModelT* WorldCollMdl)
{
    assert(ScriptState==NULL);
    ScriptState=new cf::GameSys::ScriptStateT;

    assert(Sv_PhysicsWorld==NULL);
    Sv_PhysicsWorld=new PhysicsWorldT(WorldCollMdl);
}


void cf::GameSys::GameImplT::Sv_FinishNewWorld(const char* WorldFileName)
{
    assert(ScriptState!=NULL);

    std::string  LuaScriptName=WorldFileName;
    const size_t SuffixPos    =LuaScriptName.rfind(".cw");

    if (SuffixPos==std::string::npos) LuaScriptName+=".lua";
                                 else LuaScriptName.replace(SuffixPos, 3, ".lua");

    ScriptState->LoadMapScript(LuaScriptName);


    // Call each entities OnInit() script method here???
    // Finally call the Lua OnInit() method of each entity.
    //for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
    //{
    //    AllChildren[ChildNr]->OnLuaEventHandler(LuaState, "OnInit");
    //}
}


void cf::GameSys::GameImplT::Sv_BeginThinking(float FrameTime)
{
    IsThinking=true;

    Sv_PhysicsWorld->Think(FrameTime);

    ScriptState->RunPendingCoroutines(FrameTime);   // Should do this early rather than in Sv_EndThinking(), because new coroutines are usually added "during" thinking.
    ScriptState->RunMapCmdsFromConsole();
}


void cf::GameSys::GameImplT::Sv_EndThinking()
{
    IsThinking=false;
}


void cf::GameSys::GameImplT::Sv_UnloadWorld()
{
    assert(ScriptState!=NULL);

    // All entities should have been deleted by now, and their dtors should have removed their Lua associated instances.
    // ScriptState->PrintGlobalVars();
    assert(!ScriptState->HasEntityInstances());

    delete ScriptState;
    ScriptState=NULL;

    assert(Sv_PhysicsWorld!=NULL);
    delete Sv_PhysicsWorld;
    Sv_PhysicsWorld=NULL;
}


void cf::GameSys::GameImplT::Cl_LoadWorld(const char* /*WorldFileName*/, const cf::ClipSys::CollisionModelT* WorldCollMdl)
{
    assert(Cl_PhysicsWorld==NULL);
    Cl_PhysicsWorld=new PhysicsWorldT(WorldCollMdl);
}


void cf::GameSys::GameImplT::Cl_UnloadWorld()
{
    assert(Cl_PhysicsWorld!=NULL);
    delete Cl_PhysicsWorld;
    Cl_PhysicsWorld=NULL;
}


// This function is called by the server, in order to obtain a (pointer to a) 'BaseEntityT' from a map file entity.
// The server also provides the ID and engine function call-backs for the new entity.
//
// TODO: Diese Funktion sollte einen struct-Parameter haben, der enthält: std::map<> mit EntityDef (Properties), ID, EF, ptr auf SceneNode-Root, ptr auf ClipObject.
BaseEntityT* cf::GameSys::GameImplT::CreateBaseEntityFromMapFile(const std::map<std::string, std::string>& Properties,
    const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel, unsigned long ID,
    unsigned long WorldFileIndex, unsigned long MapFileIndex, cf::GameSys::GameWorldI* GameWorld, const Vector3T<double>& Origin)
{
    // 1. Determine from the entity class name (e.g. "monster_argrenade") the C++ class name (e.g. "EntARGrenadeT").
    std::map<std::string, std::string>::const_iterator EntClassNamePair=Properties.find("classname");

    if (EntClassNamePair==Properties.end()) return NULL;
    assert(ScriptState!=NULL);      // We are on the server-, not on the client-side, after all.

    const std::string EntClassName=EntClassNamePair->second;
    const std::string CppClassName=ScriptState->GetCppClassNameFromEntityClassName(EntClassName);

    if (CppClassName=="")
    {
        Console->Warning("C++ class name for entity class name \""+EntClassName+"\" not found.\n");
        return NULL;
    }


    // 2. Create an instance of the desired entity type.
    const cf::TypeSys::TypeInfoT* TI=GetBaseEntTIM().FindTypeInfoByName(CppClassName.c_str());

    if (TI==NULL)
    {
        Console->Warning("No type info found for entity class \""+EntClassName+"\" with C++ class name \""+CppClassName+"\".\n");
        return NULL;
    }

    assert(TI->CreateInstance!=NULL);

    // YES! THIS is how it SHOULD work!
    BaseEntityT* NewEnt=static_cast<BaseEntityT*>(TI->CreateInstance(
        EntityCreateParamsT(ID, Properties, RootNode, CollisionModel, WorldFileIndex, MapFileIndex, GameWorld, Sv_PhysicsWorld, Origin)));

    assert(NewEnt!=NULL);
    assert(NewEnt->GetType()==TI);

    if (NewEnt==NULL)
    {
        Console->Warning("Could not create instance for type \""+CppClassName+"\".\n");
        TI->Print(false /*Don't print the child classes.*/);
        return NULL;
    }


    // 3. Create a matching entity instance in the Lua ScriptState (only if a concrete object name (e.g. "Soldier_Barney") is given).
    if (NewEnt->Name!="")
    {
        assert(ScriptState!=NULL);      // We are on the server-, not on the client-side, after all.

        if (!ScriptState->AddEntityInstance(NewEnt))
        {
            // An error message was already printed by the AddEntityInstance() function.
            #if 1
                Console->Warning("Could not create scripting instance for entity \""+NewEnt->Name+"\" of class \""+EntClassName+"\" (\""+CppClassName+"\").\n");
            #else
                delete NewEnt;
                return NULL;
            #endif
        }

        Console->DevPrint("Info: Entity \""+NewEnt->Name+"\" of class \""+EntClassName+"\" (\""+CppClassName+"\") instantiated.\n");
    }

    // OPEN QUESTION:
    // Should we copy the Properties into the Lua entity instance, into the C++ entity instance, or nowhere (just keep the std::map<> pointer around)?
    // See   svn log -r 301   for one argument for the C++ instance.
    return NewEnt;
}


// This function is called by the client, in order to obtain a (pointer to a) 'BaseEntityT' for a new entity
// whose TypeNr and ID it got via a net message from the server.
// (It initializes the 'State' of the entity directly via the returned pointer.)
// The client also provides engine function call-backs, such that the prediction feature can work.
BaseEntityT* cf::GameSys::GameImplT::CreateBaseEntityFromTypeNr(unsigned long TypeNr, const std::map<std::string, std::string>& Properties,
    const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel,
    unsigned long ID, unsigned long WorldFileIndex, unsigned long MapFileIndex, cf::GameSys::GameWorldI* GameWorld)
{
    const cf::TypeSys::TypeInfoT* TI=GetBaseEntTIM().FindTypeInfoByNr(TypeNr);

    assert(TI!=NULL);
    assert(TI->TypeNr==TypeNr);
    assert(TI->CreateInstance!=NULL);

    return static_cast<BaseEntityT*>(TI->CreateInstance(
        EntityCreateParamsT(ID, Properties, RootNode, CollisionModel, WorldFileIndex, MapFileIndex, GameWorld, Cl_PhysicsWorld, VectorT())));
}


// Called by both the client and the server to release previously obtained 'BaseEntityT's.
// Note that simply deleting them directly is not possible (the "EXE vs. DLL boundary").
void cf::GameSys::GameImplT::FreeBaseEntity(BaseEntityT* BaseEntity)
{
    // If this entity is a server entity and has a script instance (an concrete entity name was given in the map file), remove it.
    // The RemoveEntityInstance() method makes sure that an instance for BaseEntity actually exists before it tries to remove it.
    if (ScriptState) ScriptState->RemoveEntityInstance(BaseEntity);

    delete BaseEntity;
}
