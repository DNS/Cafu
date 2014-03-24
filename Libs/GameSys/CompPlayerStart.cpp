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

const cf::TypeSys::TypeInfoT ComponentPlayerStartT::TypeInfo(GetComponentTIM(), "GameSys::ComponentPlayerStartT", "GameSys::ComponentBaseT", ComponentPlayerStartT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
