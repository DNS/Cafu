/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


void* ComponentTransformT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTransformT(static_cast<const cf::GuiSys::ComponentCreateParamsT&>(Params).m_Window);
}

const luaL_reg ComponentTransformT::MethodsList[] =
{
    { "__tostring", ComponentTransformT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTransformT::TypeInfo(GetComponentTIM(), "ComponentTransformT", "ComponentBaseT", ComponentTransformT::CreateInstance, MethodsList);


ComponentTransformT::ComponentTransformT(WindowT& Window)
    : ComponentBaseT(Window)
{
}


ComponentTransformT::ComponentTransformT(const ComponentTransformT& Comp, WindowT& Window)
    : ComponentBaseT(Comp, Window)
{
}


ComponentTransformT* ComponentTransformT::Clone(WindowT& Window) const
{
    return new ComponentTransformT(*this, Window);
}


int ComponentBaseT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "transform component");
    return 1;
}
