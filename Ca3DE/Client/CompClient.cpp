/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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

const luaL_Reg ComponentClientT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentClientT::TypeInfo(GetComponentTIM(), "Cafu::ComponentClientT", "GuiSys::ComponentBaseT", ComponentClientT::CreateInstance, MethodsList);


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
