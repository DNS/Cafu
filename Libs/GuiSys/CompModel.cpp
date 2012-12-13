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

#include "CompModel.hpp"
#include "AllComponents.hpp"
#include "CompTransform.hpp"
#include "Window.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


void* ComponentModelT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentModelT();
}

const luaL_reg ComponentModelT::MethodsList[] =
{
    { "__tostring", ComponentModelT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentModelT::TypeInfo(GetComponentTIM(), "ComponentModelT", "ComponentBaseT", ComponentModelT::CreateInstance, MethodsList);


ComponentModelT::ComponentModelT()
    : ComponentBaseT(),
      m_Transform(NULL)
{
}


ComponentModelT::ComponentModelT(const ComponentModelT& Comp)
    : ComponentBaseT(Comp),
      m_Transform(NULL)
{
}


ComponentModelT* ComponentModelT::Clone() const
{
    return new ComponentModelT(*this);
}


void ComponentModelT::UpdateDependencies(WindowT* Window)
{
    ComponentBaseT::UpdateDependencies(Window);

    m_Transform = NULL;

    if (!GetWindow()) return;

    // It would be possible to break this loop as soon as we have assigned a non-NULL pointer to m_Transform.
    // However, this is only because the Transform component is, at this time, the only sibling component that
    // we're interested in, whereas the loop below is suitable for resolving additional dependencies, too.
    for (unsigned int CompNr = 0; CompNr < GetWindow()->GetComponents().Size(); CompNr++)
    {
        IntrusivePtrT<ComponentBaseT> Comp = GetWindow()->GetComponents()[CompNr];

        if (m_Transform == NULL)
            m_Transform = dynamic_pointer_cast<ComponentTransformT>(Comp);
    }
}


int ComponentModelT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "model component");
    return 1;
}
