/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompPlayerStart.hpp"
#include "AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentPlayerStartT::DocClass =
    "This component marks its entity as possible spawn point for human players\n"
    "that begin a single-player level or join a multi-player game.";


const cf::TypeSys::VarsDocT ComponentPlayerStartT::DocVars[] =
{
    { "SinglePlayer", "If checked, players can be spawned here in single-player games." },
    { "MultiPlayer",  "If checked, players can be spawned here in multi-player games." },
    { NULL, NULL }
};


ComponentPlayerStartT::ComponentPlayerStartT()
    : ComponentBaseT(),
      m_SinglePlayer("SinglePlayer", true),
      m_MultiPlayer("MultiPlayer", true)
{
    GetMemberVars().Add(&m_SinglePlayer);
    GetMemberVars().Add(&m_MultiPlayer);
}


ComponentPlayerStartT::ComponentPlayerStartT(const ComponentPlayerStartT& Comp)
    : ComponentBaseT(Comp),
      m_SinglePlayer(Comp.m_SinglePlayer),
      m_MultiPlayer(Comp.m_MultiPlayer)
{
    GetMemberVars().Add(&m_SinglePlayer);
    GetMemberVars().Add(&m_MultiPlayer);
}


ComponentPlayerStartT* ComponentPlayerStartT::Clone() const
{
    return new ComponentPlayerStartT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentPlayerStartT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "player start component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentPlayerStartT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentPlayerStartT();
}

const luaL_Reg ComponentPlayerStartT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentPlayerStartT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentPlayerStartT::TypeInfo(GetComponentTIM(), "GameSys::ComponentPlayerStartT", "GameSys::ComponentBaseT", ComponentPlayerStartT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
