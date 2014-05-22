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

#include "CompInventory.hpp"
#include "../AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentInventoryT::DocClass =
    "This component represents the player's inventory.";


const cf::TypeSys::VarsDocT ComponentInventoryT::DocVars[] =
{
    { "Shells",    "The current amount of shells." },
    { "MaxShells", "The maximum amount of shells." },
    { NULL, NULL }
};


ComponentInventoryT::ComponentInventoryT()
    : ComponentBaseT(),
      m_Shells("Shells", 0),
      m_MaxShells("MaxShells", 0)
{
    FillMemberVars();
}


ComponentInventoryT::ComponentInventoryT(const ComponentInventoryT& Comp)
    : ComponentBaseT(Comp),
      m_Shells(Comp.m_Shells),
      m_MaxShells(Comp.m_MaxShells)
{
    FillMemberVars();
}


void ComponentInventoryT::FillMemberVars()
{
    GetMemberVars().Add(&m_Shells);
    GetMemberVars().Add(&m_MaxShells);
}


ComponentInventoryT* ComponentInventoryT::Clone() const
{
    return new ComponentInventoryT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentInventoryT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "inventory component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentInventoryT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentInventoryT();
}

const luaL_Reg ComponentInventoryT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentInventoryT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentInventoryT::TypeInfo(GetComponentTIM(), "GameSys::ComponentInventoryT", "GameSys::ComponentBaseT", ComponentInventoryT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
