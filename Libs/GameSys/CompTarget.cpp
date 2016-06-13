/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompTarget.hpp"
#include "AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


namespace
{
    const char* FlagsIsEntityName[] = { "IsEntityName", NULL };
}


const char* ComponentTargetT::DocClass =
    "This component connects its entity to another.\n"
    "It is used by Script or GUI (Model) components in order to learn which other entity\n"
    "is related and should possibly be acted upon. For example, Target components are\n"
    "often used to let generic \"open door\" GUIs know which door they should actually open.";


const cf::TypeSys::VarsDocT ComponentTargetT::DocVars[] =
{
    { "Target", "The name of another entity that scripts and GUIs should act upon." },
    { NULL, NULL }
};


ComponentTargetT::ComponentTargetT()
    : ComponentBaseT(),
      m_TargetName("Target", "", FlagsIsEntityName)
{
    GetMemberVars().Add(&m_TargetName);
}


ComponentTargetT::ComponentTargetT(const ComponentTargetT& Comp)
    : ComponentBaseT(Comp),
      m_TargetName(Comp.m_TargetName)
{
    GetMemberVars().Add(&m_TargetName);
}


ComponentTargetT* ComponentTargetT::Clone() const
{
    return new ComponentTargetT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentTargetT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "target component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentTargetT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTargetT();
}

const luaL_Reg ComponentTargetT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentTargetT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTargetT::TypeInfo(GetComponentTIM(), "GameSys::ComponentTargetT", "GameSys::ComponentBaseT", ComponentTargetT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
