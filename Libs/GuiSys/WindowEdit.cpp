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

#include "WindowEdit.hpp"
#include "GuiImpl.hpp"
#include "WindowCreateParams.hpp"
#include "Fonts/FontTT.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "OpenGL/OpenGLWindow.hpp"      // for CaKeyboardEventT
#include "TypeSys.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


using namespace cf::GuiSys;


/*** Begin of TypeSys related definitions for this class. ***/

void* EditWindowT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EditWindowT(*static_cast<const cf::GuiSys::WindowCreateParamsT*>(&Params));
}

const luaL_reg EditWindowT::MethodsList[]=
{
    { "set",                EditWindowT::Set },
    { "GetTextCursorPos",   EditWindowT::GetTextCursorPos },
    { "SetTextCursorPos",   EditWindowT::SetTextCursorPos },
    { "SetTextCursorType",  EditWindowT::SetTextCursorType },
    { "SetTextCursorRate",  EditWindowT::SetTextCursorRate },
    { "SetTextCursorColor", EditWindowT::SetTextCursorColor },
    { "__gc",               WindowT::Destruct },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT EditWindowT::TypeInfo(GetWindowTIM(), "EditWindowT", "WindowT", EditWindowT::CreateInstance, MethodsList);

/*** End of TypeSys related definitions for this class. ***/


EditWindowT::EditWindowT(const cf::GuiSys::WindowCreateParamsT& Params)
    : WindowT(Params),
      m_TextCursorPos((unsigned int)Text.length()),
      m_TextCursorType(0),
      m_TextCursorRate(1.0f),
      m_TextCursorTime(0)
{
    for (unsigned long c=0; c<4; c++)
    {
        m_TextCursorColor[c]=(c==2) ? 0.0f : 1.0f;
    }

    FillMemberVars();
}


EditWindowT::EditWindowT(const EditWindowT& Window, bool Recursive)
    : WindowT(Window, Recursive),
      m_TextCursorPos(Window.m_TextCursorPos),
      m_TextCursorType(Window.m_TextCursorType),
      m_TextCursorRate(Window.m_TextCursorRate),
      m_TextCursorTime(Window.m_TextCursorTime)
{
    for (unsigned long i=0; i<4; i++)
        m_TextCursorColor[i]=Window.m_TextCursorColor[i];

    FillMemberVars();
}


EditWindowT* EditWindowT::Clone(bool Recursive) const
{
    return new EditWindowT(*this, Recursive);
}


// The code in this method is much like that of WindowT::Render(), and likely to break
// (that is, in need for adjustment) when the code in WindowT::Render() is changed.
void EditWindowT::Render() const
{
    WindowT::Render();


    // Render the text cursor.
    if (!ShowWindow) return;
    if (Font==NULL) return;
    if (m_Gui.GetFocusWindow().GetRaw()!=this) return;      // Only render the text cursor if we have the keyboard input focus.
    if (m_TextCursorTime>=0.5f*m_TextCursorRate) return;    // Only render the text cursor during one half of the blink cycle.

    float x1;
    float y1;

    GetAbsolutePos(x1, y1);

    const float x2=x1+Rect[2];
    const float y2=y1+Rect[3];

    const float b=BorderWidth;


    // Save the current matrices.
    if (RotAngle!=0)
    {
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, x1+Rect[2]/2.0f, y1+Rect[3]/2.0f, 0.0f);
        MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD, RotAngle);
        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, -(x1+Rect[2]/2.0f), -(y1+Rect[3]/2.0f), 0.0f);
    }


    // This is the actual rendering of the cursor.
    int          LineCount=1;
    const size_t TextLength=Text.length();

    for (size_t i=0; i+1<TextLength; i++)
        if (Text[i]=='\n')
            LineCount++;

    unsigned long r_=(unsigned long)(m_TextCursorColor[0]*255.0f);
    unsigned long g_=(unsigned long)(m_TextCursorColor[1]*255.0f);
    unsigned long b_=(unsigned long)(m_TextCursorColor[2]*255.0f);
    unsigned long a_=(unsigned long)(m_TextCursorColor[3]*255.0f);

    const float MaxTop     =Font->GetAscender(TextScale);
    const float LineSpacing=Font->GetLineSpacing(TextScale);
    float       LineOffsetY=0.0;
    size_t      LineStart  =0;

    while (true)
    {
        const size_t      NextNewline=Text.find('\n', LineStart);
        const size_t      LineEnd    =(NextNewline==std::string::npos) ? TextLength : NextNewline;
        const std::string Line       =Text.substr(LineStart, LineEnd-LineStart);

        float AlignX=0.0f;
        float AlignY=0.0f;

        switch (TextAlignHor)
        {
            case left:  AlignX=b; break;
            case right: AlignX=x2-x1-Font->GetWidth(Line, TextScale)-b; break;
            default:    AlignX=(x2-x1-Font->GetWidth(Line, TextScale))/2.0f; break;
        }

        switch (TextAlignVer)
        {
            case top:    AlignY=b+MaxTop; break;   // Without the +MaxTop, the text baseline ("___") is at the top border of the window.
            case bottom: AlignY=y2-y1-b-(LineCount-1)*LineSpacing; break;
            default:     AlignY=(y2-y1-LineCount*LineSpacing)/2.0f+MaxTop; break;
        }

        // If the text cursor is in this line, render the cursor and break.
        if (LineStart<=m_TextCursorPos && m_TextCursorPos<=LineEnd)
        {
            const std::string LineUntilCursor =std::string(Line, 0, m_TextCursorPos-LineStart);
            const float       WidthUntilCursor=Font->GetWidth(LineUntilCursor, TextScale);

            if (m_TextCursorType==1)
            {
                Font->Print(x1+AlignX+WidthUntilCursor, y1+AlignY+LineOffsetY, TextScale, (a_ << 24) | (r_ << 16) | (g_ << 8) | (b_ << 0), "_");
            }
            else
            {
                const float OfsX=Font->GetWidth("|", TextScale)*0.5f;

                Font->Print(x1+AlignX+WidthUntilCursor-OfsX, y1+AlignY+LineOffsetY, TextScale, (a_ << 24) | (r_ << 16) | (g_ << 8) | (b_ << 0), "|");
            }
            break;
        }

        if (NextNewline==std::string::npos) break;  // Emergency break, in case m_TextCursorPos is not in Text.
        LineStart=LineEnd+1;
        LineOffsetY+=LineSpacing;
    }


    if (RotAngle!=0)
    {
        // Restore the previously active matrices.
        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    }
}


bool EditWindowT::OnInputEvent(const CaKeyboardEventT& KE)
{
    // 1. The relevant Lua OnKeyDown/Up() or OnChar() methods didn't handle this event, so we got here (see GuiT::ProcessDeviceEvent()).
    // 2. Now see if we want to and can handle the event here.
    if (KE.Type==CaKeyboardEventT::CKE_KEYDOWN)
    {
        switch (KE.Key)
        {
            case CaKeyboardEventT::CK_BACKSPACE:
                if (m_TextCursorPos>0)
                {
                    Text.erase(m_TextCursorPos-1, 1);
                    m_TextCursorPos--;
                    m_TextCursorTime=0;
                }
                return true;

            case CaKeyboardEventT::CK_DELETE:
                if (m_TextCursorPos<Text.length())
                {
                    Text.erase(m_TextCursorPos, 1);
                }
                return true;

            case CaKeyboardEventT::CK_LEFT:
                if (m_TextCursorPos>0)
                {
                    m_TextCursorPos--;
                    m_TextCursorTime=0;
                }
                return true;

            case CaKeyboardEventT::CK_RIGHT:
                if (m_TextCursorPos<Text.length())
                {
                    m_TextCursorPos++;
                    m_TextCursorTime=0;
                }
                return true;

            case CaKeyboardEventT::CK_HOME:
                m_TextCursorPos=0;
                m_TextCursorTime=0;
                return true;

            case CaKeyboardEventT::CK_END:
                m_TextCursorPos=(unsigned int)Text.length();
                m_TextCursorTime=0;
                return true;
        }
    }
    else if (KE.Type==CaKeyboardEventT::CKE_CHAR)
    {
        Text.insert(m_TextCursorPos, 1, char(KE.Key));
        m_TextCursorPos++;
        m_TextCursorTime=0;
        return true;
    }

    // 3. Nobody handled the event so far, now propagate it to the base class(es),
    //    where it will (when still unhandled), travel up to the (class hierarchy of the) parent window(s).
    return WindowT::OnInputEvent(KE);
}


bool EditWindowT::OnClockTickEvent(float t)
{
    m_TextCursorTime+=t;
    while (m_TextCursorTime>=m_TextCursorRate) m_TextCursorTime-=m_TextCursorRate;

    return WindowT::OnClockTickEvent(t);
}


void EditWindowT::FillMemberVars()
{
    WindowT::FillMemberVars();

    MemberVars["textCursorPos"]=MemberVarT(MemberVarT::TYPE_INT, &m_TextCursorPos);
    MemberVars["textCursorType"]=MemberVarT(MemberVarT::TYPE_INT, &m_TextCursorType);
    MemberVars["textCursorRate"]=MemberVarT(m_TextCursorRate);
    MemberVars["textCursorColor"]=MemberVarT(MemberVarT::TYPE_FLOAT4, &m_TextCursorColor[0]);
    MemberVars["textCursorColor.r"]=MemberVarT(m_TextCursorColor[0]);
    MemberVars["textCursorColor.g"]=MemberVarT(m_TextCursorColor[1]);
    MemberVars["textCursorColor.b"]=MemberVarT(m_TextCursorColor[2]);
    MemberVars["textCursorColor.a"]=MemberVarT(m_TextCursorColor[3]);
}


int EditWindowT::Set(lua_State* LuaState)
{
    const int Result=WindowT::Set(LuaState);

    EditWindowT* EditWin=(EditWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);
    std::string  VarName=luaL_checkstring(LuaState, 2);

    // If the text of this window was changed, set the text cursor position to its end.
    if (VarName=="text") EditWin->m_TextCursorPos=(unsigned int)EditWin->Text.length();
    return Result;
}


int EditWindowT::GetTextCursorPos(lua_State* LuaState)
{
    EditWindowT* EditWin=(EditWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    lua_pushinteger(LuaState, EditWin->m_TextCursorPos);
    return 1;
}


int EditWindowT::SetTextCursorPos(lua_State* LuaState)
{
    EditWindowT* EditWin=(EditWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    EditWin->m_TextCursorPos=lua_tointeger(LuaState, 2);
    return 0;
}


int EditWindowT::SetTextCursorType(lua_State* LuaState)
{
    EditWindowT* EditWin=(EditWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);
    const char*  Type=luaL_checkstring(LuaState, 2);

    EditWin->m_TextCursorType=(Type[0]=='_') ? 1 : 0;
    return 0;
}


int EditWindowT::SetTextCursorRate(lua_State* LuaState)
{
    EditWindowT* EditWin=(EditWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    EditWin->m_TextCursorRate=float(lua_tonumber(LuaState, 2));
    return 0;
}


int EditWindowT::SetTextCursorColor(lua_State* LuaState)
{
    EditWindowT* EditWin=(EditWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    EditWin->m_TextCursorColor[0]=float(lua_tonumber(LuaState, 2));
    EditWin->m_TextCursorColor[1]=float(lua_tonumber(LuaState, 3));
    EditWin->m_TextCursorColor[2]=float(lua_tonumber(LuaState, 4));
    EditWin->m_TextCursorColor[3]=lua_isnil(LuaState, 5) ? 1.0f : float(lua_tonumber(LuaState, 5));
    return 0;
}
