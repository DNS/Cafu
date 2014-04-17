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

#include "CompHumanPlayer.hpp"
#include "AllComponents.hpp"
#include "CompPhysics.hpp"
#include "CompPlayerPhysics.hpp"
#include "CompScript.hpp"
#include "Entity.hpp"
#include "World.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentHumanPlayerT::DocClass =
    "Entities with this component are associated with a client connection\n"
    "at whose end is a human player who provides input to control the entity.";


const cf::TypeSys::VarsDocT ComponentHumanPlayerT::DocVars[] =
{
    { "PlayerName",          "The name that the player chose for himself." },
    { "State",               "For the player's main state machine, e.g. spectator, dead, alive, ..." },
    { "Health",              "Health." },
    { "Armor",               "Armor." },
    { "HaveItems",           "Bit field, entity can carry 32 different items." },
    { "HaveWeapons",         "Bit field, entity can carry 32 different weapons." },
    { "ActiveWeaponSlot",    "Index into m_HaveWeapons, m_HaveAmmoInWeapons, and for determining the weapon model index." },
    { "ActiveWeaponSequNr",  "The weapon anim sequence that we see (the local clients 1st person ('view') weapon model)." },
    { "ActiveWeaponFrameNr", "Respectively, this is the frame number of the current weapon sequence." },
    { "HaveAmmo",            "Entity can carry 16 different types of ammo (weapon independent). This is the amount of each." },
    { "HaveAmmoInWeapons",   "Entity can carry ammo in each of the 32 weapons. This is the amount of each." },
    { NULL, NULL }
};


ComponentHumanPlayerT::ComponentHumanPlayerT()
    : ComponentBaseT(),
      m_PlayerName("PlayerName", "Player"),
      m_StateOfExistence("State", 2 /*StateOfExistence_FrozenSpectator*/),
      m_Health("Health", 100),
      m_Armor("Armor", 0),
      m_HaveItems("HaveItems", 0),
      m_HaveWeapons("HaveWeapons", 0),
      m_ActiveWeaponSlot("ActiveWeaponSlot", 0),
      m_ActiveWeaponSequNr("ActiveWeaponSequNr", 0),
      m_ActiveWeaponFrameNr("ActiveWeaponFrameNr", 0),
      m_HaveAmmo("HaveAmmo", 16, 0),
      m_HaveAmmoInWeapons("HaveAmmoInWeapons", 32, 0),
      m_PlayerCommands()
{
    FillMemberVars();
}


ComponentHumanPlayerT::ComponentHumanPlayerT(const ComponentHumanPlayerT& Comp)
    : ComponentBaseT(Comp),
      m_PlayerName(Comp.m_PlayerName),
      m_StateOfExistence(Comp.m_StateOfExistence),
      m_Health(Comp.m_Health),
      m_Armor(Comp.m_Armor),
      m_HaveItems(Comp.m_HaveItems),
      m_HaveWeapons(Comp.m_HaveWeapons),
      m_ActiveWeaponSlot(Comp.m_ActiveWeaponSlot),
      m_ActiveWeaponSequNr(Comp.m_ActiveWeaponSequNr),
      m_ActiveWeaponFrameNr(Comp.m_ActiveWeaponFrameNr),
      m_HaveAmmo(Comp.m_HaveAmmo),
      m_HaveAmmoInWeapons(Comp.m_HaveAmmoInWeapons),
      m_PlayerCommands()
{
    FillMemberVars();
}


void ComponentHumanPlayerT::FillMemberVars()
{
    GetMemberVars().Add(&m_PlayerName);
    GetMemberVars().Add(&m_StateOfExistence);
    GetMemberVars().Add(&m_Health);
    GetMemberVars().Add(&m_Armor);
    GetMemberVars().Add(&m_HaveItems);
    GetMemberVars().Add(&m_HaveWeapons);
    GetMemberVars().Add(&m_ActiveWeaponSlot);
    GetMemberVars().Add(&m_ActiveWeaponSequNr);
    GetMemberVars().Add(&m_ActiveWeaponFrameNr);
    GetMemberVars().Add(&m_HaveAmmo);
    GetMemberVars().Add(&m_HaveAmmoInWeapons);
}


Vector3dT ComponentHumanPlayerT::GetPlayerVelocity() const
{
    if (!GetEntity())
        return Vector3dT();

    IntrusivePtrT<ComponentPlayerPhysicsT> CompPlayerPhysics =
        dynamic_pointer_cast<ComponentPlayerPhysicsT>(GetEntity()->GetComponent("PlayerPhysics"));

    if (CompPlayerPhysics == NULL)
        return Vector3dT();

    return CompPlayerPhysics->GetVelocity();
}


Vector3dT ComponentHumanPlayerT::GetOriginWS() const
{
    if (!GetEntity()) return Vector3dT();

    return GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble();
}


Vector3dT ComponentHumanPlayerT::GetViewDirWS(double Random) const
{
    if (!GetEntity()) return Vector3dT();

    IntrusivePtrT<ComponentTransformT> Trafo = GetEntity()->GetTransform();

    if (GetEntity()->GetChildren().Size() > 0)
    {
        // The normal, expected case: Use the entity's camera transform.
        Trafo = GetEntity()->GetChildren()[0]->GetTransform();
    }

    const cf::math::Matrix3x3fT Mat(Trafo->GetQuatWS());

    Vector3dT ViewDir = Mat.GetAxis(0).AsVectorOfDouble();

    if (Random > 0.0)
    {
        ViewDir += Mat.GetAxis(0).AsVectorOfDouble() * ((rand() % 10000 - 5000) / 5000.0) * Random;
        ViewDir += Mat.GetAxis(2).AsVectorOfDouble() * ((rand() % 10000 - 5000) / 5000.0) * Random;

        ViewDir = normalizeOr0(ViewDir);
    }

    return ViewDir;
}


RayResultT ComponentHumanPlayerT::TracePlayerRay(const Vector3dT& Dir) const
{
    if (!GetEntity())
        return RayResultT(NULL);

    IntrusivePtrT<ComponentPhysicsT> Physics =
        dynamic_pointer_cast<ComponentPhysicsT>(GetEntity()->GetComponent("Physics"));

    RayResultT RayResult(Physics != NULL ? Physics->GetRigidBody() : NULL);

    GetEntity()->GetWorld().GetPhysicsWorld()->TraceRay(
        UnitsToPhys(GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble()),
        Dir * 9999.0, RayResult);

    return RayResult;
}


void ComponentHumanPlayerT::InflictDamage(EntityT* OtherEnt, float Amount, const Vector3dT& Dir) const
{
    if (!OtherEnt) return;

    IntrusivePtrT<ComponentScriptT> OtherScript =
        dynamic_pointer_cast<ComponentScriptT>(OtherEnt->GetComponent("Script"));

    if (OtherScript == NULL) return;

    cf::ScriptBinderT      OtherBinder(OtherEnt->GetWorld().GetScriptState().GetLuaState());
    IntrusivePtrT<EntityT> This = GetEntity();

    OtherBinder.Push(This);

    OtherScript->CallLuaMethod("TakeDamage", 1, "ffff", Amount, Dir.x, Dir.y, Dir.z);
}


ComponentHumanPlayerT* ComponentHumanPlayerT::Clone() const
{
    return new ComponentHumanPlayerT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentHumanPlayerT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "human player component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentHumanPlayerT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentHumanPlayerT();
}

const luaL_Reg ComponentHumanPlayerT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentHumanPlayerT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentHumanPlayerT::TypeInfo(GetComponentTIM(), "GameSys::ComponentHumanPlayerT", "GameSys::ComponentBaseT", ComponentHumanPlayerT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
