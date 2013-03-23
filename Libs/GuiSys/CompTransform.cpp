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

using namespace cf::GuiSys;


void* ComponentTransformT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTransformT();
}

const luaL_reg ComponentTransformT::MethodsList[] =
{
    { "__tostring", ComponentTransformT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTransformT::TypeInfo(GetComponentTIM(), "ComponentTransformT", "ComponentBaseT", ComponentTransformT::CreateInstance, MethodsList);


ComponentTransformT::ComponentTransformT()
    : ComponentBaseT(),
      m_Pos("Pos", Vector2fT(0.0f, 0.0f)),
      m_Size("Size", Vector2fT(80.0f, 60.0f)),
      m_RotAngle("Rotation", 0.0f)
{
    GetMemberVars().Add(&m_Pos);
    GetMemberVars().Add(&m_Size);
    GetMemberVars().Add(&m_RotAngle);
}


ComponentTransformT::ComponentTransformT(const ComponentTransformT& Comp)
    : ComponentBaseT(Comp),
      m_Pos(Comp.m_Pos),
      m_Size(Comp.m_Size),
      m_RotAngle(Comp.m_RotAngle)
{
    GetMemberVars().Add(&m_Pos);
    GetMemberVars().Add(&m_Size);
    GetMemberVars().Add(&m_RotAngle);
}


ComponentTransformT* ComponentTransformT::Clone() const
{
    return new ComponentTransformT(*this);
}


int ComponentTransformT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "transform component");
    return 1;
}
