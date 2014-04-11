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
#include "CompPlayerPhysics.hpp"
#include "Entity.hpp"

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
 // { "SinglePlayer", "If checked, players can be spawned here in single-player games." },
 // { "MultiPlayer",  "If checked, players can be spawned here in multi-player games." },
    { NULL, NULL }
};


ComponentHumanPlayerT::ComponentHumanPlayerT()
    : ComponentBaseT(),
      m_PlayerCommands()
  //  m_SinglePlayer("SinglePlayer", true),
  //  m_MultiPlayer("MultiPlayer", true)
{
    // GetMemberVars().Add(&m_SinglePlayer);
}


ComponentHumanPlayerT::ComponentHumanPlayerT(const ComponentHumanPlayerT& Comp)
    : ComponentBaseT(Comp),
      m_PlayerCommands()
  //  m_SinglePlayer(Comp.m_SinglePlayer),
  //  m_MultiPlayer(Comp.m_MultiPlayer)
{
    // GetMemberVars().Add(&m_SinglePlayer);
}


Vector3dT ComponentHumanPlayerT::GetPlayerVelocity() const
{
    if (!GetEntity())
        return Vector3dT();

    IntrusivePtrT<cf::GameSys::ComponentPlayerPhysicsT> CompPlayerPhysics =
        dynamic_pointer_cast<cf::GameSys::ComponentPlayerPhysicsT>(GetEntity()->GetComponent("PlayerPhysics"));

    if (CompPlayerPhysics == NULL)
        return Vector3dT();

    return CompPlayerPhysics->GetVelocity();
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
