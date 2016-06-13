/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
    const char* FlagsIsLuaFileName[] = { "IsLuaFileName",   NULL };
}


const char* ComponentCarriedWeaponT::DocClass =
    "This component represents a weapon that a player can pick up and use.";


const cf::TypeSys::VarsDocT ComponentCarriedWeaponT::DocVars[] =
{
    { "Label",            "A short informational name for this weapon. Used for reference e.g. in the Map Editor, in log output, or in script code (therefore, changing it for existing weapons may require a review of the related script code)." },
    { "IsAvail",          "Is this weapon available to the player? Normally `false` when the player spawns. Switched to `true` when the player picks up the weapon for the first time, whereupon it can be selected and drawn." },
    { "Script",           "The filename of the script that implements the behaviour of this weapon." },
    { "PrimaryAmmo",      "The current amount of ammo for the primary fire of this weapon." },
    { "MaxPrimaryAmmo",   "The maximum amount of ammo for the primary fire of this weapon." },
    { "SecondaryAmmo",    "The current amount of ammo for the secondary fire of this weapon." },
    { "MaxSecondaryAmmo", "The maximum amount of ammo for the secondary fire of this weapon." },
    { NULL, NULL }
};


ComponentCarriedWeaponT::ComponentCarriedWeaponT()
    : ComponentBaseT(),
      m_Label("Label", "weapon"),
      m_IsAvail("IsAvail", false),
      m_Script("Script", "", FlagsIsLuaFileName),
      m_PrimaryAmmo("PrimaryAmmo", 0),
      m_MaxPrimaryAmmo("MaxPrimaryAmmo", 0),
      m_SecondaryAmmo("SecondaryAmmo", 0),
      m_MaxSecondaryAmmo("MaxSecondaryAmmo", 0)
{
    FillMemberVars();
}


ComponentCarriedWeaponT::ComponentCarriedWeaponT(const ComponentCarriedWeaponT& Comp)
    : ComponentBaseT(Comp),
      m_Label(Comp.m_Label),
      m_IsAvail(Comp.m_IsAvail),
      m_Script(Comp.m_Script),
      m_PrimaryAmmo(Comp.m_PrimaryAmmo),
      m_MaxPrimaryAmmo(Comp.m_MaxPrimaryAmmo),
      m_SecondaryAmmo(Comp.m_SecondaryAmmo),
      m_MaxSecondaryAmmo(Comp.m_MaxSecondaryAmmo)
{
    FillMemberVars();
}


void ComponentCarriedWeaponT::FillMemberVars()
{
    GetMemberVars().Add(&m_Label);
    GetMemberVars().Add(&m_IsAvail);
    GetMemberVars().Add(&m_Script);
    GetMemberVars().Add(&m_PrimaryAmmo);
    GetMemberVars().Add(&m_MaxPrimaryAmmo);
    GetMemberVars().Add(&m_SecondaryAmmo);
    GetMemberVars().Add(&m_MaxSecondaryAmmo);
}


ComponentCarriedWeaponT* ComponentCarriedWeaponT::Clone() const
{
    return new ComponentCarriedWeaponT(*this);
}


void ComponentCarriedWeaponT::PreCache()
{
    CallLuaMethod("PreCache", 0);
}


void ComponentCarriedWeaponT::OnPostLoad(bool OnlyStatic)
{
    if (OnlyStatic) return;
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

const cf::TypeSys::MethsDocT ComponentCarriedWeaponT::DocCallbacks[] =
{
    { "IsIdle",
      "This method is called in order to learn if this weapon is currently idle.",
      "boolean", "" },
    { "Draw",
      "This method is called in order to let this weapon know that it is drawn.",
      "", "" },
    { "Holster",
      "This method is called in order to have this weapon holstered.",
      "boolean", "" },
    { "FirePrimary",
      "This method is called in order to have this weapon emit primary fire.",
      "", "(boolean ThinkingOnServerSide)" },
    { "FireSecondary",
      "This method is called in order to have this weapon emit secondary fire.",
      "", "(boolean ThinkingOnServerSide)" },
    { "PreCache",
      "This method is called in order to have the weapon pre-cache its resources.",
      "", "" },
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentCarriedWeaponT::TypeInfo(
    GetComponentTIM(),
    "GameSys::ComponentCarriedWeaponT",
    "GameSys::ComponentBaseT",
    ComponentCarriedWeaponT::CreateInstance,
    MethodsList,
    DocClass,
    DocMethods,
    DocCallbacks,
    DocVars);
