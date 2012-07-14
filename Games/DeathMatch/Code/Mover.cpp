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

#include "Mover.hpp"
#include "EntityCreateParams.hpp"
#include "TypeSys.hpp"
#include "UniScriptState.hpp"
#include "../../GameWorld.hpp"
#include "ClipSys/ClipModel.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModel_base.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "SceneGraph/Node.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntFuncMoverT::GetType() const
{
    return &TypeInfo;
 // return &EntFuncMoverT::TypeInfo;
}

void* EntFuncMoverT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntFuncMoverT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const luaL_Reg EntFuncMoverT::MethodsList[]=
{
    { "SetOrigin", EntFuncMoverT::SetOrigin },
    { "Translate", EntFuncMoverT::Translate },
    { "Rotate",    EntFuncMoverT::Rotate },
 // { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT EntFuncMoverT::TypeInfo(GetBaseEntTIM(), "EntFuncMoverT", "BaseEntityT", CreateInstance, MethodsList);


EntFuncMoverT::EntFuncMoverT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  Params.RootNode->GetBoundingBox(),
                  0,
                  EntityStateT(VectorT(),   // Velocity
                               0,
                               0,
                               0,           // ModelIndex
                               0,           // ModelSequNr
                               0.0,         // ModelFrameNr
                               0,           // Health
                               0,           // Armor
                               0,           // HaveItems
                               0,           // HaveWeapons
                               0,           // ActiveWeaponSlot
                               0,           // ActiveWeaponSequNr
                               0.0)),       // ActiveWeaponFrameNr
      TranslationSource(),
      TranslationDest(),
      TranslationLinTimeTotal(0.0f),
      TranslationLinTimeLeft(0.0f),
      RootNode(Params.RootNode)
{
    ClipModel.Register();
}


void EntFuncMoverT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();
}


void EntFuncMoverT::Think(float FrameTime, unsigned long ServerFrameNr)
{
    if (TranslationLinTimeLeft>0.0f)
    {
        cf::ClipSys::ContactsResultT Contacts;

        // Find out if anything touches us.
        // Some of these objects might want to ride with us.
        GameWorld->GetClipWorld().GetContacts(CollisionModel->GetBoundingBox(),
            m_Origin, (TranslationDest-TranslationSource)*(FrameTime/TranslationLinTimeTotal),
            MaterialT::Clip_Players | MaterialT::Clip_Monsters | MaterialT::Clip_Moveables, &ClipModel, Contacts);

        ArrayT<BaseEntityT*> AlreadySeen;   // List of entities we have already dealt with.

        for (unsigned long ContactNr=0; ContactNr<Contacts.NrOfRepContacts; ContactNr++)
        {
            BaseEntityT* ContactEntity=static_cast<BaseEntityT*>(Contacts.ClipModels[ContactNr]->GetUserData());

            // If we have a contact with a clip model with unknown owner entity, just ignore it.
            if (ContactEntity==NULL) continue;

            // Right now, there is no need to discriminate entities by type here (which ones should be pushed, which ones shouldn't?),
            // because of the contents flags (Clip_Players | Clip_Monsters | ...) that have been passed to the GetContacts() method above.
            // Well, maybe later...?
            // if (ContactEntity->GetType()==...) continue;

            // Here is such a case: movers don't push each other, no matter what!
            // Without this check, the two parts of the double slide door near the TechDemo map start are pushing each other
            // when they are both moving "down", into the ground.
            if (ContactEntity->GetType()==this->GetType()) continue;

            // Process each entity only once.
            if (AlreadySeen.Find(ContactEntity)>=0) continue;
            AlreadySeen.PushBack(ContactEntity);

            ArrayT<BaseEntityT*> Pushers;
            Pushers.PushBack(this);

            ContactEntity->OnPush(Pushers, (TranslationDest-TranslationSource)*(FrameTime/TranslationLinTimeTotal));
        }


        TranslationLinTimeLeft-=FrameTime;
        if (TranslationLinTimeLeft<0.0f) TranslationLinTimeLeft=0.0f;

        const float t=TranslationLinTimeLeft/TranslationLinTimeTotal;

        m_Origin=TranslationSource*t + TranslationDest*(1.0f-t);

        ClipModel.SetOrigin(m_Origin);
        ClipModel.Register();  // Re-register ourselves with the clip world.
    }
}


void EntFuncMoverT::Draw(bool FirstPersonView, float LodDist) const
{
    Vector3dT EyePos(MatSys::Renderer->GetCurrentEyePosition());
    Vector3dT LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    float     LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();


    // BIG PROBLEM:
    // The EyePos as obtained from MatSys::Renderer->GetCurrentEyePosition() currently oscillates up and down,
    // as added by the code in ClientWorld.cpp for eye-candy specular-effect...

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // MatSys::Renderer->...();
    MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, 1.0f/25.4f);
    MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, -90.0f);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // EyePos=EyePos-Entity->GetOrigin();         // Convert into unrotated model space.
    EyePos=scale(EyePos, 25.4);
    EyePos=EyePos.GetRotZ(90.0);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // LightPos=LightPos-Entity->GetOrigin();         // Convert into unrotated model space.
    LightPos=scale(LightPos, 25.4);
    LightPos=LightPos.GetRotZ(90.0);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
    LightRadius*=25.4f;

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
    // Set the modified (now in model space) lighting parameters.
    MatSys::Renderer->SetCurrentLightSourcePosition(float(LightPos.x), float(LightPos.y), float(LightPos.z));
    MatSys::Renderer->SetCurrentLightSourceRadius(LightRadius);
    MatSys::Renderer->SetCurrentEyePosition(float(EyePos.x), float(EyePos.y), float(EyePos.z));


    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
            RootNode->DrawAmbientContrib(EyePos);
            RootNode->DrawTranslucentContrib(EyePos);
            break;

        case MatSys::RendererI::LIGHTING:
            RootNode->DrawLightSourceContrib(EyePos, LightPos);
            break;

        case MatSys::RendererI::STENCILSHADOW:
            RootNode->DrawStencilShadowVolumes(LightPos, LightRadius);
            break;
    }
}


int EntFuncMoverT::SetOrigin(lua_State* LuaState)
{
    // Call the overridden base class implementation of this method first.
    BaseEntityT::SetOrigin(LuaState);

    cf::ScriptBinderT Binder(LuaState);
    EntFuncMoverT*    Ent=(EntFuncMoverT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    Ent->ClipModel.SetOrigin(Ent->GetOrigin());
    Ent->ClipModel.Register();  // Re-register ourselves with the clip world.
    return 0;
}


int EntFuncMoverT::Translate(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    EntFuncMoverT*    Ent=(EntFuncMoverT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    // If there is already a translation in progress, ignore additional requests.
    if (Ent->TranslationLinTimeLeft>0.0f) return 0;

    const double Ox=luaL_checknumber(LuaState, 2);
    const double Oy=luaL_checknumber(LuaState, 3);
    const double Oz=luaL_checknumber(LuaState, 4);

    Ent->TranslationSource=Ent->GetOrigin();
    Ent->TranslationDest=Vector3dT(Ox, Oy, Oz);
    Ent->TranslationLinTimeTotal=float(luaL_checknumber(LuaState, 5));
    Ent->TranslationLinTimeLeft=Ent->TranslationLinTimeTotal;

    return 0;
}


int EntFuncMoverT::Rotate(lua_State* LuaState)
{
    // cf::ScriptBinderT Binder(LuaState);
    // EntFuncMoverT*    Ent=(EntFuncMoverT*)Binder.GetCheckedObjectParam(TypeInfo);

    return 0;
}
