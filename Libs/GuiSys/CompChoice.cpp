/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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


const char* ComponentChoiceT::DocClass =
    "This components add the behaviour of a choice field to its window.\n"
    "It requires that the window also has a text component, whose value it\n"
    "updates according to user interaction to one of the available choices.";


const cf::TypeSys::VarsDocT ComponentChoiceT::DocVars[] =
{
    { "Choices",   "The list of available choices." },
    { "Selection", "The index number of the currently selected choice, where 1 corresponds to the first choice (as per Lua convention). Use 0 for \"no selection\"." },
    { NULL, NULL }
};


ComponentChoiceT::ComponentChoiceT()
    : ComponentBaseT(),
      m_TextComp(NULL),
      m_Choices("Choices", 0, ""),
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


void ComponentChoiceT::OnPostLoad(bool OnlyStatic)
{
    if (!OnlyStatic) Sync();
}


bool ComponentChoiceT::OnInputEvent(const CaKeyboardEventT& KE)
{
    if (m_TextComp == NULL) return false;
    if (m_Choices.Size() == 0) return false;
    if (KE.Type != CaKeyboardEventT::CKE_KEYDOWN) return false;

    // Note that the range of Sel is 1 ... Size, not 0 ... Size-1.
    const unsigned int Num = m_Choices.Size();
    const unsigned int Sel = m_Selection.Get();

    switch (KE.Key)
    {
        case CaKeyboardEventT::CK_UP:       // UpArrow on arrow keypad.
        case CaKeyboardEventT::CK_LEFT:     // LeftArrow on arrow keypad.
            // Select the previous choice, wrapping.
            m_Selection.Set(Sel > 1 ? Sel-1 : Num);

            Sync();
            CallLuaMethod("OnSelectionChanged");
            return true;

        case CaKeyboardEventT::CK_DOWN:     // DownArrow on arrow keypad.
        case CaKeyboardEventT::CK_RIGHT:    // RightArrow on arrow keypad.
            // Select the next choice, wrapping.
            m_Selection.Set(Sel < Num ? Sel+1 : 1);

            Sync();
            CallLuaMethod("OnSelectionChanged");
            return true;

        case CaKeyboardEventT::CK_HOME:     // Home on arrow keypad.
        case CaKeyboardEventT::CK_PGUP:     // PgUp on arrow keypad.
            // Move the selection to the first choice.
            if (Sel != 1)
            {
                m_Selection.Set(1);

                Sync();
                CallLuaMethod("OnSelectionChanged");
            }
            return true;

        case CaKeyboardEventT::CK_END:      // End on arrow keypad.
        case CaKeyboardEventT::CK_PGDN:     // PgDn on arrow keypad.
            // Move the selection to the last choice.
            if (Sel != Num)
            {
                m_Selection.Set(Num);

                Sync();
                CallLuaMethod("OnSelectionChanged");
            }
            return true;
    }

    // We didn't handle this event.
    return false;
}


bool ComponentChoiceT::OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY)
{
    if (m_TextComp == NULL) return false;
    if (m_Choices.Size() == 0) return false;
    if (ME.Type != CaMouseEventT::CM_BUTTON0) return false;

    if (ME.Amount == 0)
    {
        const unsigned int Sel = m_Selection.Get();

        // Note that the range of Sel is 1 ... Size, not 0 ... Size-1.
        m_Selection.Set(Sel < m_Choices.Size() ? Sel+1 : 1);

        Sync();
        CallLuaMethod("OnSelectionChanged");
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
    if (Sel > m_Choices.Size()) return;

    // Note that the range of Sel is 1 ... Size, not 0 ... Size-1.
    m_TextComp->SetText(m_Choices[Sel-1]);
}


static const cf::TypeSys::MethsDocT META_Set =
{
    "set",
    "An override of the base class method that also calls Sync().",
    "", "(string var_name, any)"
};

int ComponentChoiceT::Set(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentChoiceT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentChoiceT> >(1);

    const int Result = ComponentBaseT::Set(LuaState);

    Comp->Sync();

    return Result;
}


static const cf::TypeSys::MethsDocT META_GetSelItem =
{
    "GetSelItem",
    "Returns the currently selected item (or `nil` if no item is selected).",
    "string", "()"
};

int ComponentChoiceT::GetSelItem(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentChoiceT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentChoiceT> >(1);

    if (Comp->m_Selection.Get() < 1 || Comp->m_Selection.Get() > Comp->m_Choices.Size())
        lua_pushnil(LuaState);
    else
        lua_pushstring(LuaState, Comp->m_Choices[Comp->m_Selection.Get() - 1].c_str());

    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentChoiceT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "choice component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentChoiceT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentChoiceT();
}

const luaL_Reg ComponentChoiceT::MethodsList[] =
{
    { "set",        Set },
    { "GetSelItem", GetSelItem },
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentChoiceT::DocMethods[] =
{
    META_Set,
    META_GetSelItem,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentChoiceT::DocCallbacks[] =
{
    { "OnSelectionChanged",
      "This method is called when the choice's selection has changed.",
      "", "" },
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentChoiceT::TypeInfo(
    GetComponentTIM(),
    "GuiSys::ComponentChoiceT",
    "GuiSys::ComponentBaseT",
    ComponentChoiceT::CreateInstance,
    MethodsList,
    DocClass,
    DocMethods,
    DocCallbacks,
    DocVars);
