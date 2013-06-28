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

#include "CompCollisionModel.hpp"
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
    const char* FlagsIsFileName[] = { "IsGenericFileName", NULL };
}


const char* ComponentCollisionModelT::DocClass =
    "This component adds a collision model to its entity.";


const cf::TypeSys::VarsDocT ComponentCollisionModelT::DocVars[] =
{
    { "Name", "The file name of the collision model." },
    { NULL, NULL }
};


ComponentCollisionModelT::ComponentCollisionModelT()
    : ComponentBaseT(),
      m_CollMdlName("Name", "", FlagsIsFileName)
{
    GetMemberVars().Add(&m_CollMdlName);
}


ComponentCollisionModelT::ComponentCollisionModelT(const ComponentCollisionModelT& Comp)
    : ComponentBaseT(Comp),
      m_CollMdlName(Comp.m_CollMdlName)
{
    GetMemberVars().Add(&m_CollMdlName);
}


ComponentCollisionModelT* ComponentCollisionModelT::Clone() const
{
    return new ComponentCollisionModelT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentCollisionModelT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "collision model component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentCollisionModelT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentCollisionModelT();
}

const luaL_reg ComponentCollisionModelT::MethodsList[] =
{
    { "__tostring", ComponentCollisionModelT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentCollisionModelT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentCollisionModelT::TypeInfo(GetComponentTIM(), "ComponentCollisionModelT", "ComponentBaseT", ComponentCollisionModelT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
