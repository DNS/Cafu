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

#include "CompText.hpp"
#include "AllComponents.hpp"
#include "GuiImpl.hpp"
#include "GuiResources.hpp"
#include "Window.hpp"

#include "Fonts/FontTT.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning C4355: 'this' : used in base member initializer list.
    #pragma warning(disable:4355)
#endif

using namespace cf::GuiSys;


/************************************/
/*** ComponentTextT::VarFontNameT ***/
/************************************/

ComponentTextT::VarFontNameT::VarFontNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentTextT& CompText)
    : TypeSys::VarT<std::string>(Name, Value, Flags),
      m_CompText(CompText)
{
}


// The compiler-written copy constructor would copy m_CompText from Var.m_CompText,
// but we must obviously use the reference to the proper parent instance instead.
ComponentTextT::VarFontNameT::VarFontNameT(const VarFontNameT& Var, ComponentTextT& CompText)
    : TypeSys::VarT<std::string>(Var),
      m_CompText(CompText)
{
}


void ComponentTextT::VarFontNameT::Set(const std::string& v)
{
    // Make sure that m_CompText actually refers to the ComponentTextT instance that contains us!
    assert(this == &m_CompText.m_FontName);

    TypeSys::VarT<std::string>::Set(v);

    m_CompText.m_FontInst = NULL;

    if (m_CompText.GetWindow() && v != "")
    {
        m_CompText.m_FontInst = m_CompText.GetWindow()->GetGui().GetGuiResources().GetFont(v);
    }
}


/****************************************/
/*** ComponentTextT::VarTextAlignHorT ***/
/****************************************/

ComponentTextT::VarTextAlignHorT::VarTextAlignHorT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentTextT::VarTextAlignHorT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("left");   Values.PushBack(LEFT);
    Strings.PushBack("center"); Values.PushBack(CENTER);
    Strings.PushBack("right");  Values.PushBack(RIGHT);
}


/****************************************/
/*** ComponentTextT::VarTextAlignVerT ***/
/****************************************/

ComponentTextT::VarTextAlignVerT::VarTextAlignVerT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentTextT::VarTextAlignVerT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("top");    Values.PushBack(TOP);
    Strings.PushBack("middle"); Values.PushBack(MIDDLE);
    Strings.PushBack("bottom"); Values.PushBack(BOTTOM);
}


/**********************/
/*** ComponentTextT ***/
/**********************/

void* ComponentTextT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTextT();
}

const luaL_reg ComponentTextT::MethodsList[] =
{
    { "__tostring", ComponentTextT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTextT::TypeInfo(GetComponentTIM(), "ComponentTextT", "ComponentBaseT", ComponentTextT::CreateInstance, MethodsList);


namespace
{
    const char* FlagsPaddingLabels[] = { "Labels", "hor.", "vert.", NULL };
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


ComponentTextT::ComponentTextT()
    : ComponentBaseT(),
      m_FontName("Name", "Fonts/Arial", NULL, *this),
      m_FontInst(NULL),
      m_Text("Text", ""),
      m_Scale("Scale", 1.0f),
      m_Padding("Padding", Vector2fT(0.0f, 0.0f), FlagsPaddingLabels),
      m_Color("Color", Vector3fT(0.5f, 0.5f, 1.0f), FlagsIsColor),
      m_Alpha("Alpha", 1.0f),
      m_AlignHor("hor. Align", VarTextAlignHorT::LEFT),
      m_AlignVer("ver. Align", VarTextAlignVerT::TOP)
{
    FillMemberVars();
}


ComponentTextT::ComponentTextT(const ComponentTextT& Comp)
    : ComponentBaseT(Comp),
      m_FontName(Comp.m_FontName, *this),
      m_FontInst(NULL),
      m_Text(Comp.m_Text),
      m_Scale(Comp.m_Scale),
      m_Padding(Comp.m_Padding),
      m_Color(Comp.m_Color),
      m_Alpha(Comp.m_Alpha),
      m_AlignHor(Comp.m_AlignHor),
      m_AlignVer(Comp.m_AlignVer)
{
    FillMemberVars();
}


void ComponentTextT::FillMemberVars()
{
    GetMemberVars().Add(&m_FontName);
    GetMemberVars().Add(&m_Text);
    GetMemberVars().Add(&m_Scale);
    GetMemberVars().Add(&m_Padding);
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Alpha);
    GetMemberVars().Add(&m_AlignHor);
    GetMemberVars().Add(&m_AlignVer);
}


ComponentTextT* ComponentTextT::Clone() const
{
    return new ComponentTextT(*this);
}


void ComponentTextT::UpdateDependencies(WindowT* Window)
{
    const bool WindowChanged = Window != GetWindow();

    ComponentBaseT::UpdateDependencies(Window);


    // Update resources that are provided directly or indirectly by the window.
    if (WindowChanged)
    {
        m_FontInst = NULL;

        if (GetWindow() && m_FontName.Get() != "")
        {
            m_FontInst = GetWindow()->GetGui().GetGuiResources().GetFont(m_FontName.Get());
        }
    }


/*
    // The window may or may not have changed, and/or the components of the window may have changed.
    m_Transform = NULL;

    if (GetWindow())
    {
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
*/
}


void ComponentTextT::Render() const
{
    if (!m_FontInst) return;

    const float x1 = 0.0f;
    const float y1 = 0.0f;
    const float x2 = GetWindow()->GetSize().x;
    const float y2 = GetWindow()->GetSize().y;

    int LineCount = 1;
    const size_t TextLength = m_Text.Get().length();

    for (size_t i = 0; i+1 < TextLength; i++)
        if (m_Text.Get()[i] == '\n')
            LineCount++;

    unsigned int TextCol = 0;
    TextCol |= (unsigned int)(m_Alpha.Get()    * 255.0f) << 24;
    TextCol |= (unsigned int)(m_Color.Get()[0] * 255.0f) << 16;
    TextCol |= (unsigned int)(m_Color.Get()[1] * 255.0f) << 8;
    TextCol |= (unsigned int)(m_Color.Get()[2] * 255.0f) << 0;

    const float MaxTop      = m_FontInst->GetAscender(m_Scale.Get());
    const float LineSpacing = m_FontInst->GetLineSpacing(m_Scale.Get());
    float       LineOffsetY = 0.0f;

    size_t LineStart = 0;

    while (true)
    {
        const size_t LineEnd = m_Text.Get().find('\n', LineStart);
        std::string  Line    = (LineEnd==std::string::npos) ? m_Text.Get().substr(LineStart) : m_Text.Get().substr(LineStart, LineEnd-LineStart);

        float AlignX = 0.0f;
        float AlignY = 0.0f;

        switch (m_AlignHor.Get())
        {
            case VarTextAlignHorT::LEFT:  AlignX = m_Padding.Get().x; break;
            case VarTextAlignHorT::RIGHT: AlignX = x2 - x1 - m_FontInst->GetWidth(Line, m_Scale.Get()) - m_Padding.Get().x; break;
            default:                      AlignX = (x2 - x1 - m_FontInst->GetWidth(Line, m_Scale.Get()))/2.0f; break;
        }

        switch (m_AlignVer.Get())
        {
            case VarTextAlignVerT::TOP:    AlignY = m_Padding.Get().y + MaxTop; break;   // Without the +MaxTop, the text baseline ("___") is at the top border of the window.
            case VarTextAlignVerT::BOTTOM: AlignY = y2 - y1 - m_Padding.Get().y - (LineCount-1)*LineSpacing; break;
            default:                       AlignY = (y2 - y1 - LineCount*LineSpacing)/2.0f + MaxTop; break;
        }

        m_FontInst->Print(x1 + AlignX, y1 + AlignY + LineOffsetY, m_Scale.Get(), TextCol, "%s", Line.c_str());

        if (LineEnd == std::string::npos) break;
        LineStart = LineEnd+1;
        LineOffsetY += LineSpacing;
    }
}


int ComponentTextT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "text component");
    return 1;
}
