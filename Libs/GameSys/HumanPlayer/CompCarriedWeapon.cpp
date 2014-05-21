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

#include "CompCarriedWeapon.hpp"
#include "../AllComponents.hpp"
#include "../Entity.hpp"
#include "../World.hpp"

#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


/*******************************/
/*** ComponentCarriedWeaponT ***/
/*******************************/

namespace
{
    const char* FlagsIsModelFileName[] = { "IsModelFileName", NULL };
    const char* FlagsIsLuaFileName[]   = { "IsLuaFileName",   NULL };
}


const char* ComponentCarriedWeaponT::DocClass =
    "This component represents a weapon that a player can pick up and use.";


const cf::TypeSys::VarsDocT ComponentCarriedWeaponT::DocVars[] =
{
    { "Label",            "A short informational name for this weapon. Used for reference e.g. in the Map Editor, in log output, or in script code (therefore, changing it for existing weapons may require a review of the related script code)." },
    { "IsAvail",          "Is this weapon available to the player? Normally `false` when the player spawns. Switched to `true` when the player picks up the weapon for the first time, whereupon it can be selected and drawn." },
    { "Script",           "The filename of the script that implements the behaviour of this weapon." },
    { "Model1stPerson",   "The name of the 1st-person (\"view\") model of this weapon." },
    { "Model3rdPerson",   "The name of the 3rd-person (\"player\") model of this weapon." },
    { "AmmoPrimary",      "The current amount of ammo for the primary fire of this weapon." },
    { "MaxAmmoPrimary",   "The maximum amount of ammo for the primary fire of this weapon." },
    { "AmmoSecondary",    "The current amount of ammo for the secondary fire of this weapon." },
    { "MaxAmmoSecondary", "The maximum amount of ammo for the secondary fire of this weapon." },
    { NULL, NULL }
};


ComponentCarriedWeaponT::ComponentCarriedWeaponT()
    : ComponentBaseT(),
      m_Label("Label", "weapon"),
      m_IsAvail("IsAvail", false),
      m_Script("Script", "", FlagsIsLuaFileName),
      m_Model1stPerson("Model1stPerson", "", FlagsIsModelFileName),
      m_Model3rdPerson("Model3rdPerson", "", FlagsIsModelFileName),
      m_AmmoPrimary("AmmoPrimary", 0),
      m_MaxAmmoPrimary("MaxAmmoPrimary", 0),
      m_AmmoSecondary("AmmoSecondary", 0),
      m_MaxAmmoSecondary("MaxAmmoSecondary", 0)
{
    FillMemberVars();
}


ComponentCarriedWeaponT::ComponentCarriedWeaponT(const ComponentCarriedWeaponT& Comp)
    : ComponentBaseT(Comp),
      m_Label(Comp.m_Label),
      m_IsAvail(Comp.m_IsAvail),
      m_Script(Comp.m_Script),
      m_Model1stPerson(Comp.m_Model1stPerson),
      m_Model3rdPerson(Comp.m_Model3rdPerson),
      m_AmmoPrimary(Comp.m_AmmoPrimary),
      m_MaxAmmoPrimary(Comp.m_MaxAmmoPrimary),
      m_AmmoSecondary(Comp.m_AmmoSecondary),
      m_MaxAmmoSecondary(Comp.m_MaxAmmoSecondary)
{
    FillMemberVars();
}


void ComponentCarriedWeaponT::FillMemberVars()
{
    GetMemberVars().Add(&m_Label);
    GetMemberVars().Add(&m_IsAvail);
    GetMemberVars().Add(&m_Script);
    GetMemberVars().Add(&m_Model1stPerson);
    GetMemberVars().Add(&m_Model3rdPerson);
    GetMemberVars().Add(&m_AmmoPrimary);
    GetMemberVars().Add(&m_MaxAmmoPrimary);
    GetMemberVars().Add(&m_AmmoSecondary);
    GetMemberVars().Add(&m_MaxAmmoSecondary);
}


ComponentCarriedWeaponT* ComponentCarriedWeaponT::Clone() const
{
    return new ComponentCarriedWeaponT(*this);
}


void ComponentCarriedWeaponT::OnPostLoad(bool InEditor)
{
    if (InEditor) return;
    if (!GetEntity()) return;
    if (m_Script.Get() == "") return;

    cf::UniScriptStateT& ScriptState = GetEntity()->GetWorld().GetScriptState();
    lua_State*           LuaState    = ScriptState.GetLuaState();
    const char*          INT_GLOBAL  = "__CAFU_INTERNAL__";
    const StackCheckerT  StackChecker(LuaState);
    cf::ScriptBinderT    Binder(LuaState);

    // Using the INT_GLOBAL here is a trick to overcome the limitations of the parameter-passing to DoFile().
    //
    // Ideally, we would write
    //     ScriptState.DoFile(m_Script.Get().c_str(), "O", IntrusivePtrT<ComponentCarriedWeaponT>(this));
    // but implementing this properly (type safe) requires variadic templates.
    //
    // As an alternative, it would be possible to augment the interface of DoFile() to take a number of
    // "extra" arguments, very much like `UniScriptStateT::StartNewCoroutine(int NumExtraArgs, [...]` does
    // anyway.
    Binder.Push(IntrusivePtrT<ComponentCarriedWeaponT>(this));
    lua_setglobal(LuaState, INT_GLOBAL);

    ScriptState.DoFile(m_Script.Get().c_str(), "G", INT_GLOBAL);

    lua_pushnil(LuaState);
    lua_setglobal(LuaState, INT_GLOBAL);

    // CallLuaMethod("Init", 0, "b", ClientOrServer);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentCarriedWeaponT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "carried weapon component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentCarriedWeaponT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentCarriedWeaponT();
}

const luaL_Reg ComponentCarriedWeaponT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentCarriedWeaponT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentCarriedWeaponT::TypeInfo(GetComponentTIM(), "GameSys::ComponentCarriedWeaponT", "GameSys::ComponentBaseT", ComponentCarriedWeaponT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
