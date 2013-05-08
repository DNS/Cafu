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

#include "CompTransform.hpp"
#include "AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentTransformT::DocClass =
    "This component adds information about the position and orientation of the entity.";


const cf::TypeSys::VarsDocT ComponentTransformT::DocVars[] =
{
    { "Origin",      "The origin of the entity (in the coordinate system of its parent)." },
 // { "Orientation", "The orientation of the entity (in the coordinate system of its parent)." },
    { NULL, NULL }
};


ComponentTransformT::ComponentTransformT()
    : ComponentBaseT(),
      m_Origin("Origin", Vector3fT(0.0f, 0.0f, 0.0f))
{
    GetMemberVars().Add(&m_Origin);
}


ComponentTransformT::ComponentTransformT(const ComponentTransformT& Comp)
    : ComponentBaseT(Comp),
      m_Origin(Comp.m_Origin)
{
    GetMemberVars().Add(&m_Origin);
}


ComponentTransformT* ComponentTransformT::Clone() const
{
    return new ComponentTransformT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentTransformT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "transform component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentTransformT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTransformT();
}

const luaL_reg ComponentTransformT::MethodsList[] =
{
    { "__tostring", ComponentTransformT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentTransformT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTransformT::TypeInfo(GetComponentTIM(), "ComponentTransformT", "ComponentBaseT", ComponentTransformT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
