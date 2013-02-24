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

#include "CompTextEdit.hpp"
#include "AllComponents.hpp"
#include "CompText.hpp"
#include "GuiImpl.hpp"
#include "Window.hpp"

#include "OpenGL/OpenGLWindow.hpp"      // for CaKeyboardEventT
#include "UniScriptState.hpp"
#include "Fonts/FontTT.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


/******************************************/
/*** ComponentTextEditT::VarCursorTypeT ***/
/******************************************/

ComponentTextEditT::VarCursorTypeT::VarCursorTypeT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentTextEditT::VarCursorTypeT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack(" | "); Values.PushBack(0);
    Strings.PushBack(" _ "); Values.PushBack(1);
}


/**************************/
/*** ComponentTextEditT ***/
/**************************/

void* ComponentTextEditT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTextEditT();
}

const luaL_reg ComponentTextEditT::MethodsList[] =
{
    { "SetText",    ComponentTextEditT::SetText },
    { "__tostring", ComponentTextEditT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTextEditT::TypeInfo(GetComponentTIM(), "ComponentTextEditT", "ComponentBaseT", ComponentTextEditT::CreateInstance, MethodsList);


namespace
{
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


ComponentTextEditT::ComponentTextEditT()
    : ComponentBaseT(),
      m_TextComp(NULL),
      m_CursorTime(0.0f),
      m_CursorPos("CursorPos", 0),
      m_CursorType("CursorType", 0),
      m_CursorRate("CursorRate", 1.0f),
      m_CursorColor("CursorColor", Vector3fT(1.0f, 1.0f, 0.0f), FlagsIsColor),
      m_CursorAlpha("CursorAlpha", 1.0f)
{
    GetMemberVars().Add(&m_CursorPos);
    GetMemberVars().Add(&m_CursorType);
    GetMemberVars().Add(&m_CursorRate);
    GetMemberVars().Add(&m_CursorColor);
    GetMemberVars().Add(&m_CursorAlpha);
}


ComponentTextEditT::ComponentTextEditT(const ComponentTextEditT& Comp)
    : ComponentBaseT(Comp),
      m_TextComp(NULL),
      m_CursorTime(Comp.m_CursorTime),
      m_CursorPos(Comp.m_CursorPos),
      m_CursorType(Comp.m_CursorType),
      m_CursorRate(Comp.m_CursorRate),
      m_CursorColor(Comp.m_CursorColor),
      m_CursorAlpha(Comp.m_CursorAlpha)
{
    GetMemberVars().Add(&m_CursorPos);
    GetMemberVars().Add(&m_CursorType);
    GetMemberVars().Add(&m_CursorRate);
    GetMemberVars().Add(&m_CursorColor);
    GetMemberVars().Add(&m_CursorAlpha);
}


ComponentTextEditT* ComponentTextEditT::Clone() const
{
    return new ComponentTextEditT(*this);
}


void ComponentTextEditT::UpdateDependencies(WindowT* Window)
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


// The code in this method is much like that of ComponentTextT::Render(), and likely to break
// (that is, in need for an adjustment) when the code in ComponentTextEditT::Render() is changed.
void ComponentTextEditT::Render() const
{
    if (m_TextComp == NULL) return;
    if (!m_TextComp->m_FontInst) return;
    if (m_CursorTime >= 0.5f * m_CursorRate.Get()) return;              // Only render the text cursor during one half of the blink cycle.
    if (GetWindow()->GetGui().GetFocusWindow() != GetWindow()) return;  // Only render the text cursor if we have the keyboard input focus.

    const TrueTypeFontT* Font     = m_TextComp->m_FontInst;
    const std::string&   Text     = m_TextComp->m_Text.Get();
    const float          Scale    = m_TextComp->m_Scale.Get();
    const float          PaddingX = m_TextComp->m_Padding.Get().x;
    const float          PaddingY = m_TextComp->m_Padding.Get().y;

    // Make sure that the cursor position is valid.
    // Not possible here though, because Render() is const.
    // if (m_CursorPos > Text.length()) m_CursorPos = Text.length();

    const float x1 = 0.0f;
    const float y1 = 0.0f;
    const float x2 = GetWindow()->Rect[2];
    const float y2 = GetWindow()->Rect[3];

    int LineCount = 1;
    const size_t TextLength = Text.length();

    for (size_t i = 0; i+1 < TextLength; i++)
        if (Text[i] == '\n')
            LineCount++;

    unsigned int CursorCol = 0;
    CursorCol |= (unsigned int)(m_CursorAlpha.Get()    * 255.0f) << 24;
    CursorCol |= (unsigned int)(m_CursorColor.Get()[0] * 255.0f) << 16;
    CursorCol |= (unsigned int)(m_CursorColor.Get()[1] * 255.0f) << 8;
    CursorCol |= (unsigned int)(m_CursorColor.Get()[2] * 255.0f) << 0;

    const float MaxTop      = Font->GetAscender(Scale);
    const float LineSpacing = Font->GetLineSpacing(Scale);
    float       LineOffsetY = 0.0f;

    size_t LineStart = 0;

    while (true)
    {
        const size_t      NextNewline = Text.find('\n', LineStart);
        const size_t      LineEnd     = (NextNewline == std::string::npos) ? TextLength : NextNewline;
        const std::string Line        = Text.substr(LineStart, LineEnd - LineStart);

        float AlignX = 0.0f;
        float AlignY = 0.0f;

        switch (m_TextComp->m_AlignHor.Get())
        {
            case ComponentTextT::VarTextAlignHorT::LEFT:  AlignX = PaddingX; break;
            case ComponentTextT::VarTextAlignHorT::RIGHT: AlignX = x2 - x1 - Font->GetWidth(Line, Scale) - PaddingX; break;
            default:                                      AlignX = (x2 - x1 - Font->GetWidth(Line, Scale))/2.0f; break;
        }

        switch (m_TextComp->m_AlignVer.Get())
        {
            case ComponentTextT::VarTextAlignVerT::TOP:    AlignY = PaddingY + MaxTop; break;   // Without the +MaxTop, the text baseline ("___") is at the top border of the window.
            case ComponentTextT::VarTextAlignVerT::BOTTOM: AlignY = y2 - y1 - PaddingY - (LineCount-1)*LineSpacing; break;
            default:                                       AlignY = (y2 - y1 - LineCount*LineSpacing)/2.0f + MaxTop; break;
        }

        // If the text cursor is in this line, render the cursor and break.
        if (LineStart <= m_CursorPos.Get() && m_CursorPos.Get() <= LineEnd)
        {
            const std::string LineUntilCursor  = std::string(Line, 0, m_CursorPos.Get() - LineStart);
            const float       WidthUntilCursor = Font->GetWidth(LineUntilCursor, Scale);

            if (m_CursorType.Get() == 1)
            {
                Font->Print(x1 + AlignX + WidthUntilCursor, y1 + AlignY + LineOffsetY, Scale, CursorCol, "_");
            }
            else
            {
                const float OfsX = Font->GetWidth("|", Scale) * 0.5f;

                Font->Print(x1 + AlignX + WidthUntilCursor - OfsX, y1 + AlignY + LineOffsetY, Scale, CursorCol, "|");
            }

            break;
        }

        // Did we reach the end of text without having seen the cursor?
        if (NextNewline == std::string::npos) break;

        LineStart = LineEnd + 1;
        LineOffsetY += LineSpacing;
    }
}


bool ComponentTextEditT::OnInputEvent(const CaKeyboardEventT& KE)
{
    if (m_TextComp == NULL) return false;

    const std::string& Text = m_TextComp->m_Text.Get();

    // Make sure that the cursor position is valid.
    if (m_CursorPos.Get() > Text.length()) m_CursorPos.Set(Text.length());

    if (KE.Type == CaKeyboardEventT::CKE_KEYDOWN)
    {
        switch (KE.Key)
        {
            case CaKeyboardEventT::CK_BACKSPACE:
                if (m_CursorPos.Get() > 0)
                {
                    std::string NewText = Text;

                    NewText.erase(m_CursorPos.Get() - 1, 1);
                    m_TextComp->SetText(NewText);

                    m_CursorPos.Set(m_CursorPos.Get() - 1);   // m_CursorPos--;
                    m_CursorTime = 0;
                }
                return true;

            case CaKeyboardEventT::CK_DELETE:
                if (m_CursorPos.Get() < Text.length())
                {
                    std::string NewText = Text;

                    NewText.erase(m_CursorPos.Get(), 1);
                    m_TextComp->SetText(NewText);
                }
                return true;

            case CaKeyboardEventT::CK_LEFT:
                if (m_CursorPos.Get() > 0)
                {
                    m_CursorPos.Set(m_CursorPos.Get() - 1);   // m_CursorPos--;
                    m_CursorTime = 0;
                }
                return true;

            case CaKeyboardEventT::CK_RIGHT:
                if (m_CursorPos.Get() < Text.length())
                {
                    m_CursorPos.Set(m_CursorPos.Get() + 1);   // m_CursorPos++;
                    m_CursorTime = 0;
                }
                return true;

            case CaKeyboardEventT::CK_HOME:
                m_CursorPos.Set(0);
                m_CursorTime = 0;
                return true;

            case CaKeyboardEventT::CK_END:
                m_CursorPos.Set(Text.length());
                m_CursorTime = 0;
                return true;
        }
    }
    else if (KE.Type == CaKeyboardEventT::CKE_CHAR)
    {
        // Well, yes, we copy the entire Text string at each keystroke.
        // But then it's not like we were editing the whole of "Ice and Fire" here.  :)
        std::string NewText = Text;

        NewText.insert(m_CursorPos.Get(), 1, char(KE.Key));
        m_TextComp->SetText(NewText);

        m_CursorPos.Set(m_CursorPos.Get() + 1);   // m_CursorPos++;
        m_CursorTime = 0;
        return true;
    }

    // We didn't handle this event.
    return false;
}


void ComponentTextEditT::OnClockTickEvent(float t)
{
    if (m_CursorRate.Get() > 0.0f)
    {
        m_CursorTime = fmod(m_CursorTime + t, m_CursorRate.Get());
    }
}


int ComponentTextEditT::SetText(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentTextEditT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTextEditT> >(1);
    const char* s = luaL_checkstring(LuaState, 2);

    luaL_argcheck(LuaState, Comp->m_TextComp != NULL, 1, "This component has no Text sibling component associated to it.");

    Comp->m_TextComp->m_Text.Set(s);
    Comp->m_CursorPos.Set(strlen(s));
    return 0;
}


int ComponentTextEditT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "text edit component");
    return 1;
}
