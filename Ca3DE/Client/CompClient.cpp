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

#include "CompClient.hpp"
#include "Client.hpp"

#include "GuiSys/AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


void* ComponentClientT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentClientT();
}

const luaL_reg ComponentClientT::MethodsList[] =
{
    { "__tostring", ComponentClientT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentClientT::TypeInfo(GetComponentTIM(), "ComponentClientT", "ComponentBaseT", ComponentClientT::CreateInstance, MethodsList);


ComponentClientT::ComponentClientT()
    : ComponentBaseT(),
      m_Client(NULL),
      m_LastFrameTime(0.0f)
{
}


ComponentClientT::ComponentClientT(const ComponentClientT& Comp)
    : ComponentBaseT(Comp),
      m_Client(NULL),
      m_LastFrameTime(0.0f)
{
}


void ComponentClientT::SetClient(ClientT* Cl)
{
    m_Client = Cl;
}


ComponentClientT* ComponentClientT::Clone() const
{
    return new ComponentClientT(*this);
}


void ComponentClientT::Render() const
{
    // Call the clients render methods here.
    m_Client->Render(m_LastFrameTime);
}


bool ComponentClientT::OnInputEvent(const CaKeyboardEventT& KE)
{
    return m_Client->ProcessInputEvent(KE);
}


bool ComponentClientT::OnInputEvent(const CaMouseEventT& ME, float /*PosX*/, float /*PosY*/)
{
    return m_Client->ProcessInputEvent(ME);
}


void ComponentClientT::OnClockTickEvent(float t)
{
    ComponentBaseT::OnClockTickEvent(t);

    m_LastFrameTime = t;
}


int ComponentClientT::toString(lua_State* LuaState)
{
    // cf::ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "client component");
    return 1;
}
