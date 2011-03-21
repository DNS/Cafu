/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "WindowChoice.hpp"
#include "GuiImpl.hpp"
#include "WindowCreateParams.hpp"
#include "OpenGL/OpenGLWindow.hpp"      // for CaKeyboardEventT

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


using namespace cf::GuiSys;


/*** Begin of TypeSys related definitions for this class. ***/

void* ChoiceT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ChoiceT(*static_cast<const cf::GuiSys::WindowCreateParamsT*>(&Params));
}

const luaL_reg ChoiceT::MethodsList[]=
{
    { "Clear",         ChoiceT::Clear },
    { "Append",        ChoiceT::Append },
    { "Insert",        ChoiceT::Insert },
    { "GetNumChoices", ChoiceT::GetNumChoices },
    { "GetChoice",     ChoiceT::GetChoice },
    { "SetChoice",     ChoiceT::SetChoice },
    { "GetSelection",  ChoiceT::GetSelection },
    { "SetSelection",  ChoiceT::SetSelection },
    { "__gc",          WindowT::Destruct },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ChoiceT::TypeInfo(GetWindowTIM(), "ChoiceT", "WindowT", ChoiceT::CreateInstance, MethodsList);

/*** End of TypeSys related definitions for this class. ***/


ChoiceT::ChoiceT(const cf::GuiSys::WindowCreateParamsT& Params)
    : WindowT(Params),
      SelectedChoice(0xFFFFFFFF)
{
    FillMemberVars();
}


ChoiceT::ChoiceT(const ChoiceT& Window, bool Recursive)
    : WindowT(Window, Recursive),
      Choices(Window.Choices),
      SelectedChoice(Window.SelectedChoice)
{
    FillMemberVars();
}


ChoiceT* ChoiceT::Clone(bool Recursive) const
{
    return new ChoiceT(*this, Recursive);
}


ChoiceT::~ChoiceT()
{
}


void ChoiceT::Render() const
{
    WindowT::Render();
}


bool ChoiceT::OnInputEvent(const CaKeyboardEventT& KE)
{
    // 1. The relevant Lua OnKeyDown/Up() or OnChar() methods didn't handle this event, so we got here (see GuiT::ProcessDeviceEvent()).
    // 2. Now see if we want to and can handle the event here.
    if (KE.Type==CaKeyboardEventT::CKE_KEYDOWN)
    {
        switch (KE.Key)
        {
            case CaKeyboardEventT::CK_UP:       // UpArrow on arrow keypad.
            case CaKeyboardEventT::CK_LEFT:     // LeftArrow on arrow keypad.
                // Select the previous choice, wrapping.
                if (Choices.Size()>0)
                {
                    SelectedChoice--;   // This works even if SelectedChoice==0xFFFFFFFF.
                    if (SelectedChoice>=Choices.Size()) SelectedChoice=Choices.Size()-1;    // Wrap.

                    Text=Choices[SelectedChoice];
                    CallLuaMethod("OnSelectionChanged", "i", SelectedChoice);
                }
                return true;

            case CaKeyboardEventT::CK_DOWN:     // DownArrow on arrow keypad.
            case CaKeyboardEventT::CK_RIGHT:    // RightArrow on arrow keypad.
                // Select the next choice, wrapping.
                if (Choices.Size()>0)
                {
                    SelectedChoice++;   // This works even if SelectedChoice==0xFFFFFFFF.
                    if (SelectedChoice>=Choices.Size()) SelectedChoice=0;   // Wrap.

                    Text=Choices[SelectedChoice];
                    CallLuaMethod("OnSelectionChanged", "i", SelectedChoice);
                }
                return true;

            case CaKeyboardEventT::CK_HOME:     // Home on arrow keypad.
            case CaKeyboardEventT::CK_PGUP:     // PgUp on arrow keypad.
                // Move the selection to the first choice.
                if (Choices.Size()>0 && SelectedChoice!=0)
                {
                    SelectedChoice=0;

                    Text=Choices[SelectedChoice];
                    CallLuaMethod("OnSelectionChanged", "i", SelectedChoice);
                }
                return true;

            case CaKeyboardEventT::CK_END:      // End on arrow keypad.
            case CaKeyboardEventT::CK_PGDN:     // PgDn on arrow keypad.
                // Move the selection to the last choice.
                if (Choices.Size()>0 && SelectedChoice!=Choices.Size()-1)
                {
                    SelectedChoice=Choices.Size()-1;

                    Text=Choices[SelectedChoice];
                    CallLuaMethod("OnSelectionChanged", "i", SelectedChoice);
                }
                return true;
        }
    }

    // 3. Nobody handled the event so far, now propagate it to the base class(es),
    //    where it will (when still unhandled), travel up to the (class hierarchy of the) parent window(s).
    return WindowT::OnInputEvent(KE);
}


bool ChoiceT::OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY)
{
    // 1. The relevant Lua OnMouse...() methods didn't handle this event, so we got here (see GuiT::ProcessDeviceEvent()).
    // 2. Now see if we want to and can handle the event here.
    if (Choices.Size()>0 && ME.Type==CaMouseEventT::CM_BUTTON0 && ME.Amount==0)
    {
        SelectedChoice++;   // This works even if SelectedChoice==0xFFFFFFFF.
        if (SelectedChoice>=Choices.Size()) SelectedChoice=0;

        Text=Choices[SelectedChoice];
        CallLuaMethod("OnSelectionChanged", "i", SelectedChoice);
        return true;
    }

    // 3. Nobody handled the event so far, now propagate it to the base class(es),
    //    where it will (when still unhandled), travel up to the (class hierarchy of the) parent window(s).
    return WindowT::OnInputEvent(ME, PosX, PosY);
}


void ChoiceT::FillMemberVars()
{
    WindowT::FillMemberVars();

    // TODO Should we add an unsigned long type to member vars?
    MemberVars["selectedChoice"]=MemberVarT(MemberVarT(MemberVarT::TYPE_INT, &SelectedChoice));
}


int ChoiceT::Clear(lua_State* LuaState)
{
    ChoiceT* Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    Choice->Choices.Overwrite();
    Choice->Text="";
    Choice->SelectedChoice=0xFFFFFFFF;
    return 0;
}


int ChoiceT::Append(lua_State* LuaState)
{
    ChoiceT* Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    Choice->Choices.PushBack(luaL_checkstring(LuaState, 2));
    return 0;
}


int ChoiceT::Insert(lua_State* LuaState)
{
    ChoiceT*      Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);
    unsigned long ChNr  =luaL_checkinteger(LuaState, 2);
    const char*   ChText=luaL_checkstring(LuaState, 3);

    luaL_argcheck(LuaState, ChNr<=Choice->Choices.Size(), 2, "Insertion index too large.");
    Choice->Choices.InsertAt(ChNr, ChText);
    return 0;
}


int ChoiceT::GetNumChoices(lua_State* LuaState)
{
    ChoiceT* Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    lua_pushinteger(LuaState, Choice->Choices.Size());
    return 1;
}


int ChoiceT::GetChoice(lua_State* LuaState)
{
    ChoiceT*      Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);
    unsigned long ChNr  =luaL_checkinteger(LuaState, 2);

    luaL_argcheck(LuaState, ChNr<Choice->Choices.Size(), 2, "Index out of range.");
    lua_pushstring(LuaState, Choice->Choices[ChNr].c_str());
    return 1;
}


int ChoiceT::SetChoice(lua_State* LuaState)
{
    ChoiceT*      Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);
    unsigned long ChNr  =luaL_checkinteger(LuaState, 2);

    luaL_argcheck(LuaState, ChNr<Choice->Choices.Size(), 2, "Index out of range.");
    Choice->Choices[ChNr]=luaL_checkstring(LuaState, 3);
    return 0;
}


int ChoiceT::GetSelection(lua_State* LuaState)
{
    ChoiceT* Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    lua_pushinteger(LuaState, Choice->SelectedChoice>=Choice->Choices.Size() ? -1 : Choice->SelectedChoice);
    return 1;
}


int ChoiceT::SetSelection(lua_State* LuaState)
{
    ChoiceT* Choice=(ChoiceT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    Choice->SelectedChoice=luaL_checkinteger(LuaState, 2);

    // Anything out-of-range is a "none" selection, set them uniquely to 0xFFFFFFFF.
    if (Choice->SelectedChoice >= Choice->Choices.Size())
    {
        Choice->Text="";
        Choice->SelectedChoice=0xFFFFFFFF;
    }
    else
    {
        Choice->Text=Choice->Choices[Choice->SelectedChoice];
    }

    return 0;
}
