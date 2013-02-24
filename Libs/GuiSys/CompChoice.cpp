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

#include "CompChoice.hpp"
#include "AllComponents.hpp"
#include "CompText.hpp"
#include "Window.hpp"

#include "OpenGL/OpenGLWindow.hpp"      // for CaKeyboardEventT
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


void* ComponentChoiceT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentChoiceT();
}

const luaL_reg ComponentChoiceT::MethodsList[] =
{
    { "GetSelItem", ComponentChoiceT::GetSelItem },
    { "__tostring", ComponentChoiceT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentChoiceT::TypeInfo(GetComponentTIM(), "ComponentChoiceT", "ComponentBaseT", ComponentChoiceT::CreateInstance, MethodsList);


ComponentChoiceT::ComponentChoiceT()
    : ComponentBaseT(),
      m_TextComp(NULL),
      m_Choices("Choices", ArrayT<std::string>()),
      m_Selection("Selection", 0)   // 0 is "no selection", 1 is the first choice.
{
    GetMemberVars().Add(&m_Choices);
    GetMemberVars().Add(&m_Selection);
}


ComponentChoiceT::ComponentChoiceT(const ComponentChoiceT& Comp)
    : ComponentBaseT(Comp),
      m_TextComp(NULL),
      m_Choices(Comp.m_Choices),
      m_Selection(Comp.m_Selection)
{
    GetMemberVars().Add(&m_Choices);
    GetMemberVars().Add(&m_Selection);
}


ComponentChoiceT* ComponentChoiceT::Clone() const
{
    return new ComponentChoiceT(*this);
}


void ComponentChoiceT::UpdateDependencies(WindowT* Window)
{
    ComponentBaseT::UpdateDependencies(Window);

    // The window may or may not have changed, and/or the components of the window may have changed.
    m_TextComp = NULL;

    if (GetWindow())
    {
        for (unsigned int CompNr = 0; m_TextComp == NULL && CompNr < GetWindow()->GetComponents().Size(); CompNr++)
        {
            m_TextComp = dynamic_pointer_cast<ComponentTextT>(GetWindow()->GetComponents()[CompNr]);
        }
    }
}


void ComponentChoiceT::OnPostLoad(bool InEditor)
{
    if (!InEditor) Sync();
}


bool ComponentChoiceT::OnInputEvent(const CaKeyboardEventT& KE)
{
    if (m_TextComp == NULL) return false;
    if (m_Choices.Get().Size() == 0) return false;
    if (KE.Type != CaKeyboardEventT::CKE_KEYDOWN) return false;

    // Note that the range of Sel is 1 ... Size, not 0 ... Size-1.
    const unsigned int Num = m_Choices.Get().Size();
    const unsigned int Sel = m_Selection.Get();

    switch (KE.Key)
    {
        case CaKeyboardEventT::CK_UP:       // UpArrow on arrow keypad.
        case CaKeyboardEventT::CK_LEFT:     // LeftArrow on arrow keypad.
            // Select the previous choice, wrapping.
            m_Selection.Set(Sel > 1 ? Sel-1 : Num);

            Sync();
            CallLuaMethod("OnSelectionChanged", "i", m_Selection.Get());
            return true;

        case CaKeyboardEventT::CK_DOWN:     // DownArrow on arrow keypad.
        case CaKeyboardEventT::CK_RIGHT:    // RightArrow on arrow keypad.
            // Select the next choice, wrapping.
            m_Selection.Set(Sel < Num ? Sel+1 : 1);

            Sync();
            CallLuaMethod("OnSelectionChanged", "i", m_Selection.Get());
            return true;

        case CaKeyboardEventT::CK_HOME:     // Home on arrow keypad.
        case CaKeyboardEventT::CK_PGUP:     // PgUp on arrow keypad.
            // Move the selection to the first choice.
            if (Sel != 1)
            {
                m_Selection.Set(1);

                Sync();
                CallLuaMethod("OnSelectionChanged", "i", m_Selection.Get());
            }
            return true;

        case CaKeyboardEventT::CK_END:      // End on arrow keypad.
        case CaKeyboardEventT::CK_PGDN:     // PgDn on arrow keypad.
            // Move the selection to the last choice.
            if (Sel != Num)
            {
                m_Selection.Set(Num);

                Sync();
                CallLuaMethod("OnSelectionChanged", "i", m_Selection.Get());
            }
            return true;
    }

    // We didn't handle this event.
    return false;
}


bool ComponentChoiceT::OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY)
{
    if (m_TextComp == NULL) return false;
    if (m_Choices.Get().Size() == 0) return false;
    if (ME.Type != CaMouseEventT::CM_BUTTON0) return false;

    if (ME.Amount == 0)
    {
        const unsigned int Sel = m_Selection.Get();

        // Note that the range of Sel is 1 ... Size, not 0 ... Size-1.
        m_Selection.Set(Sel < m_Choices.Get().Size() ? Sel+1 : 1);

        Sync();
        CallLuaMethod("OnSelectionChanged", "i", m_Selection.Get());
        return true;
    }

    // We didn't handle this event.
    return false;
}


void ComponentChoiceT::Sync()
{
    const unsigned int Sel = m_Selection.Get();

    if (m_TextComp == NULL) return;
    if (Sel < 1) return;
    if (Sel > m_Choices.Get().Size()) return;

    // Note that the range of Sel is 1 ... Size, not 0 ... Size-1.
    m_TextComp->SetText(m_Choices.Get()[Sel-1]);
}


int ComponentChoiceT::GetSelItem(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentChoiceT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentChoiceT> >(1);

    if (Comp->m_Selection.Get() < 1 || Comp->m_Selection.Get() > Comp->m_Choices.Get().Size())
        lua_pushnil(LuaState);
    else
        lua_pushstring(LuaState, Comp->m_Choices.Get()[Comp->m_Selection.Get() - 1].c_str());

    return 1;
}


int ComponentChoiceT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "choice component");
    return 1;
}
