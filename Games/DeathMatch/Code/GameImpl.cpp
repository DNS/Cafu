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

#include "GameImpl.hpp"

#include "cw.hpp"
#include "cw_357.hpp"
#include "cw_9mmAR.hpp"
#include "cw_BattleScythe.hpp"
#include "cw_CrossBow.hpp"
#include "cw_Egon.hpp"
#include "cw_FaceHugger.hpp"
#include "cw_Gauss.hpp"
#include "cw_Grenade.hpp"
#include "cw_Pistol.hpp"
#include "cw_RPG.hpp"
#include "cw_Shotgun.hpp"
#include "EntityCreateParams.hpp"

#include "ARGrenade.hpp"
#include "Butterfly.hpp"
#include "CompanyBot.hpp"
#include "Corpse.hpp"
#include "Eagle.hpp"
#include "FaceHugger.hpp"
#include "FuncDoor.hpp"
#include "FuncLadder.hpp"
#include "HandGrenade.hpp"
#include "HumanPlayer.hpp"
#include "InfoGeneric.hpp"
#include "InfoPlayerStart.hpp"
#include "Item.hpp"
#include "ItemAmmo357.hpp"
#include "ItemAmmoArrow.hpp"
#include "MonsterMaker.hpp"
#include "Mover.hpp"
#include "PointLightSource.hpp"
#include "RigidBody.hpp"
#include "Rocket.hpp"
#include "Speaker.hpp"
#include "StaticDetailModel.hpp"
#include "Trigger.hpp"
#include "Weapon.hpp"
#include "Weapon357.hpp"
#include "Weapon9mmAR.hpp"
#include "WeaponBattleScythe.hpp"
#include "WeaponCrossbow.hpp"
#include "WeaponEgon.hpp"
#include "WeaponFaceHugger.hpp"
#include "WeaponGauss.hpp"
#include "WeaponGrenade.hpp"
#include "WeaponHornetGun.hpp"
#include "WeaponPistol.hpp"
#include "WeaponRPG.hpp"
#include "WeaponShotgun.hpp"
#include "WeaponTripmine.hpp"

#include "TypeSys.hpp"
#include "Models/ModelManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"
#include "_ResourceManager.hpp"
#include "Libs/LookupTables.hpp"

#include "ConsoleCommands/Console.hpp"

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


namespace
{
    /*
     * The purpose of this code is to make sure that the constructors of all static TypeInfoT
     * members of all entity classes have been run and thus the TypeInfoTs all registered
     * themselves at the global type info manager (TypeInfoManT).
     *
     * Q: Why isn't that automatically the case, given that all TypeInfoTs are *static* members of
     *    their classes and supposed to be initialized before main() begins anyway?
     *
     * First, the C++ standard does not guarantee that nonlocal objects with static storage duration
     * are initialized before main() begins. Rather, their initialization can be deferred until
     * before their first use. This problem would be fixed by calling this function early in main(),
     * but as has been convincingly explained by James Kanze in [1], that is not an issue anyway:
     * Compilers just do not implement deferred initialization, mostly for backward-compatibility.
     *
     * The second and more important factor is the linker:
     * Both under Windows and Linux (and probably everywhere else), linkers include the symbols in
     * static libraries only in the executables if they resolve an unresolved external. This is
     * contrary to .obj files that are given to the linker directly [1].
     *
     * Thus, with the entity files all being part of a library, there are only two approaches to
     * make sure that all relevant units make it into the executable: Either pass the object files
     * directly and individually to the linker, or employ code like below.
     *
     * The problem with passing the individual object files is that this is difficult to implement
     * in SCons, and probably any other build system. Therefore, the only method for solving the
     * problem a reliable and portable manner that works well with any build system seems to be
     * the use of a method like below.
     *
     * [1] For more details, see the thread "Can initialization of static class members be forced
     *     before main?" that I've begun on 2008-Apr-03 in comp.lang.c++:
     *     http://groups.google.de/group/comp.lang.c++/browse_thread/thread/e264caa531ff52a9/
     *     Another very good explanation is at:
     *     http://blog.copton.net/articles/linker/index.html#linker-dependencies
     */
    const cf::TypeSys::TypeInfoT* AllTypeInfos[] = {
        &EntARGrenadeT::TypeInfo,
        &EntButterflyT::TypeInfo,
        &EntCompanyBotT::TypeInfo,
        &EntCorpseT::TypeInfo,
        &EntEagleT::TypeInfo,
        &EntFaceHuggerT::TypeInfo,
        &EntFuncDoorT::TypeInfo,
        &EntFuncLadderT::TypeInfo,
        &EntHandGrenadeT::TypeInfo,
        &EntHumanPlayerT::TypeInfo,
        &EntInfoGenericT::TypeInfo,
        &EntInfoPlayerStartT::TypeInfo,
        &EntItemT::TypeInfo,
        &EntItemAmmo357T::TypeInfo,
        &EntItemAmmoArrowT::TypeInfo,
        &EntMonsterMakerT::TypeInfo,
        &EntFuncMoverT::TypeInfo,
        &EntPointLightSourceT::TypeInfo,
        &EntRigidBodyT::TypeInfo,
        &EntRocketT::TypeInfo,
        &EntSpeakerT::TypeInfo,
        &EntStaticDetailModelT::TypeInfo,
        &EntTriggerT::TypeInfo,
        &EntWeaponT::TypeInfo,
        &EntWeapon357T::TypeInfo,
        &EntWeapon9mmART::TypeInfo,
        &EntWeaponBattleScytheT::TypeInfo,
        &EntWeaponCrossbowT::TypeInfo,
        &EntWeaponEgonT::TypeInfo,
        &EntWeaponFaceHuggerT::TypeInfo,
        &EntWeaponGaussT::TypeInfo,
        &EntWeaponGrenadeT::TypeInfo,
        &EntWeaponHornetGunT::TypeInfo,
        &EntWeaponPistolT::TypeInfo,
        &EntWeaponRPGT::TypeInfo,
        &EntWeaponShotgunT::TypeInfo,
        &EntWeaponTripmineT::TypeInfo,
    };
}


// Static method that returns the singleton instance.
cf::GameSys::GameImplT& cf::GameSys::GameImplT::GetInstance()
{
    static GameImplT GI;

    return GI;
}


cf::GameSys::GameImplT::GameImplT()
    : RunningAsClient(false),
      RunningAsServer(false)
{
    LookupTables::Initialize();

    GetBaseEntTIM().Init();
}


void cf::GameSys::GameImplT::Initialize(bool AsClient, bool AsServer, ModelManagerT& ModelMan)
{
    m_PlayerModels.PushBack(ModelMan.GetModel("Games/DeathMatch/Models/Players/Alien/Alien.cmdl"));
    m_PlayerModels.PushBack(ModelMan.GetModel("Games/DeathMatch/Models/Players/James/James.cmdl"));
    m_PlayerModels.PushBack(ModelMan.GetModel("Games/DeathMatch/Models/Players/Punisher/Punisher.cmdl"));
    m_PlayerModels.PushBack(ModelMan.GetModel("Games/DeathMatch/Models/Players/Sentinel/Sentinel.cmdl"));
    m_PlayerModels.PushBack(ModelMan.GetModel("Games/DeathMatch/Models/Players/Skeleton/Skeleton.cmdl"));
    m_PlayerModels.PushBack(ModelMan.GetModel("Games/DeathMatch/Models/Players/T801/T801.cmdl"));
    m_PlayerModels.PushBack(ModelMan.GetModel("Games/DeathMatch/Models/Players/Trinity/Trinity.cmdl"));

    m_CarriedWeapons.PushBack(new CarriedWeaponBattleScytheT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeapon357T(ModelMan));     // The .357 acts as "dummy" implementation.
    m_CarriedWeapons.PushBack(new CarriedWeaponPistolT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeapon357T(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeaponShotgunT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeapon9mmART(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeaponCrossBowT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeaponRPGT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeaponGaussT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeaponEgonT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeaponGrenadeT(ModelMan));
    m_CarriedWeapons.PushBack(new CarriedWeapon357T(ModelMan));     // The .357 acts as "dummy" implementation.
    m_CarriedWeapons.PushBack(new CarriedWeaponFaceHuggerT(ModelMan));

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
        // Registering the materials that our models will use is quite easy, because we just need to find them in the ModelMan each.
        // (Nothing breaks if the following list is simply deleted, these models just aren't pre-cached then.)
        //
        // It *is* however quite stupid that this list is not more tighly coupled to the relevant entity classes.
        // However, as the time of entity creation (and thus first model access and thus material registry time) is essentially unknown,
        // it seems simplest to list the models desired for pre-caching right here.
        //
        // Another curiosity that I just noticed: Not pre-caching here means not only to register the materials later (on first use),
        // but also to effectively load and initialize the models only at that later time, what might be very expensive, too!
        {
            // And the rest. Observe that static detail models are NOT mentioned (how could they?).
            ModelMan.GetModel("Games/DeathMatch/Models/Items/Ammo_DartGun/Ammo_DartGun.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Items/Ammo_DesertEagle/Ammo_DesertEagle.cmdl");

            ModelMan.GetModel("Games/DeathMatch/Models/LifeForms/Butterfly/Butterfly.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/LifeForms/Eagle/Eagle.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/LifeForms/FaceHugger/FaceHugger.cmdl");

            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/9mmAR/9mmAR_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/BattleScythe/BattleScythe_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Bazooka/Bazooka_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Beretta/Beretta_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DartGun/DartGun_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Egon/Egon_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/FaceHugger/FaceHugger_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Gauss/Gauss_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/HornetGun/HornetGun_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Shotgun/Shotgun_w.cmdl");
            ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Tripmine/Tripmine_w.cmdl");
        }


        // Precache all sounds known to be used here.
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/PickUp")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/Respawn")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Ambient/Jungle")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/DesertEagle_Shot1")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/9mmAR_GLauncher")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/9mmAR_Shot1")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/BattleScythe")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/FaceHugger_Throw")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_sBarrel")));
        m_PreCacheSounds.PushBack(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_dBarrel")));
    }

    if (RunningAsServer)
    {
    }
}


void cf::GameSys::GameImplT::Release()
{
    if (RunningAsClient)
    {
        ResMan.ShutDown();

        // Remove reference to precached sounds here.
        for (unsigned long i=0; i<m_PreCacheSounds.Size(); i++)
            delete m_PreCacheSounds[i];
    }

    if (RunningAsServer)
    {
    }

    for (unsigned int cwNr=0; cwNr<m_CarriedWeapons.Size(); cwNr++)
        delete m_CarriedWeapons[cwNr];

    // for (unsigned int ModelNr=0; ModelNr<m_PlayerModels.Size(); ModelNr++)
    //     delete m_PlayerModels[ModelNr];  // No, don't delete them: they're from the ModelManager!
}


const cf::TypeSys::TypeInfoManT& cf::GameSys::GameImplT::GetEntityTIM() const
{
    return GetBaseEntTIM();
}


// This function is called by the server, in order to obtain a (pointer to a) 'BaseEntityT' from a map file entity.
// The server also provides the ID and engine function call-backs for the new entity.
//
// TODO: Diese Funktion sollte einen struct-Parameter haben, der enthält: std::map<> mit EntityDef (Properties), ID, EF, ptr auf SceneNode-Root, ptr auf ClipObject.
IntrusivePtrT<BaseEntityT> cf::GameSys::GameImplT::CreateBaseEntityFromMapFile(const cf::TypeSys::TypeInfoT* TI, const std::map<std::string, std::string>& Properties,
    const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel, unsigned long ID,
    unsigned long WorldFileIndex, unsigned long MapFileIndex, cf::GameSys::GameWorldI* GameWorld, const Vector3T<double>& Origin)
{
    // YES! THIS is how it SHOULD work!
    BaseEntityT* NewEnt=static_cast<BaseEntityT*>(TI->CreateInstance(
        EntityCreateParamsT(ID, Properties, RootNode, CollisionModel, WorldFileIndex, MapFileIndex, GameWorld, Origin)));

    assert(NewEnt!=NULL);
    assert(NewEnt->GetType()==TI);

    if (NewEnt==NULL)
    {
        Console->Warning("Could not create instance for type \"" + std::string(TI->ClassName) + "\".\n");
        TI->Print(false /*Don't print the child classes.*/);
        return NULL;
    }

    return NewEnt;
}


// This function is called by the client, in order to obtain a (pointer to a) 'BaseEntityT' for a new entity
// whose TypeNr and ID it got via a net message from the server.
// (It initializes the 'State' of the entity directly via the returned pointer.)
// The client also provides engine function call-backs, such that the prediction feature can work.
IntrusivePtrT<BaseEntityT> cf::GameSys::GameImplT::CreateBaseEntityFromTypeNr(unsigned long TypeNr, const std::map<std::string, std::string>& Properties,
    const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel,
    unsigned long ID, unsigned long WorldFileIndex, unsigned long MapFileIndex, cf::GameSys::GameWorldI* GameWorld)
{
    const cf::TypeSys::TypeInfoT* TI=GetBaseEntTIM().FindTypeInfoByNr(TypeNr);

    assert(TI!=NULL);
    assert(TI->TypeNr==TypeNr);
    assert(TI->CreateInstance!=NULL);

    return static_cast<BaseEntityT*>(TI->CreateInstance(
        EntityCreateParamsT(ID, Properties, RootNode, CollisionModel, WorldFileIndex, MapFileIndex, GameWorld, VectorT())));
}


const CafuModelT* cf::GameSys::GameImplT::GetPlayerModel(unsigned int ModelIndex) const
{
    return m_PlayerModels[ModelIndex<m_PlayerModels.Size() ? ModelIndex : 0];
}


const CarriedWeaponT* cf::GameSys::GameImplT::GetCarriedWeapon(unsigned int ActiveWeaponSlot) const
{
    return m_CarriedWeapons[ActiveWeaponSlot<m_CarriedWeapons.Size() ? ActiveWeaponSlot : 3];
}
