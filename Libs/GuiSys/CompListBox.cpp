/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompListBox.hpp"
#include "AllComponents.hpp"
#include "CompText.hpp"
#include "GuiImpl.hpp"
#include "Window.hpp"

#include "OpenGL/OpenGLWindow.hpp"      // for CaKeyboardEventT
#include "UniScriptState.hpp"
#include "Fonts/FontTT.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

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


/*************************/
/*** ComponentListBoxT ***/
/*************************/

namespace
{
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


const char* ComponentListBoxT::DocClass =
    "This components turns its window into a list-box control.\n"
    "It requires that in the same window a text component is available where the aspects of text rendering are\n"
    "configured (but that has empty text contents itself).";


const cf::TypeSys::VarsDocT ComponentListBoxT::DocVars[] =
{
    { "Items",        "The list of available items." },
    { "Selection",    "The index number of the currently selected item, where 1 corresponds to the first item (as per Lua convention). Use 0 for \"no selection\"." },
    { "BgColorOdd",   "The background color for odd rows." },
    { "BgAlphaOdd",   "The background alpha for odd rows." },
    { "BgColorEven",  "The background color for even rows." },
    { "BgAlphaEven",  "The background alpha for even rows." },
    { "BgColorSel",   "The background color for selected rows." },
    { "BgAlphaSel",   "The background alpha for selected rows." },
    { "TextColorSel", "The foreground color for selected rows." },
    { "TextAlphaSel", "The foreground alpha for selected rows." },
    { NULL, NULL }
};


ComponentListBoxT::ComponentListBoxT()
    : ComponentBaseT(),
      m_TextComp(NULL),
      m_Items("Items", 0, ""),
      m_Selection("Selection", 1),  // 0 is "no selection", 1 is the first item.
      m_BgColorOdd("BgColorOdd", Vector3fT(0.0f, 0.396078f, 0.792157f), FlagsIsColor),
      m_BgAlphaOdd("BgAlphaOdd", 0.9f),
      m_BgColorEven("BgColorEven", Vector3fT(0.0f, 0.5f, 1.0f), FlagsIsColor),
      m_BgAlphaEven("BgAlphaEven", 0.9f),
      m_BgColorSel("BgColorSel", Vector3fT(1.0f, 1.0f, 0.756863f), FlagsIsColor),
      m_BgAlphaSel("BgAlphaSel", 1.0f),
      m_TextColorSel("TextColorSel", Vector3fT(0.5f, 0.0f, 0.0f), FlagsIsColor),
      m_TextAlphaSel("TextAlphaSel", 1.0f)
{
    GetMemberVars().Add(&m_Items);
    GetMemberVars().Add(&m_Selection);
    GetMemberVars().Add(&m_BgColorOdd);
    GetMemberVars().Add(&m_BgAlphaOdd);
    GetMemberVars().Add(&m_BgColorEven);
    GetMemberVars().Add(&m_BgAlphaEven);
    GetMemberVars().Add(&m_BgColorSel);
    GetMemberVars().Add(&m_BgAlphaSel);
    GetMemberVars().Add(&m_TextColorSel);
    GetMemberVars().Add(&m_TextAlphaSel);
}


ComponentListBoxT::ComponentListBoxT(const ComponentListBoxT& Comp)
    : ComponentBaseT(Comp),
      m_TextComp(NULL),
      m_Items(Comp.m_Items),
      m_Selection(Comp.m_Selection),
      m_BgColorOdd(Comp.m_BgColorOdd),
      m_BgAlphaOdd(Comp.m_BgAlphaOdd),
      m_BgColorEven(Comp.m_BgColorEven),
      m_BgAlphaEven(Comp.m_BgAlphaEven),
      m_BgColorSel(Comp.m_BgColorSel),
      m_BgAlphaSel(Comp.m_BgAlphaSel),
      m_TextColorSel(Comp.m_TextColorSel),
      m_TextAlphaSel(Comp.m_TextAlphaSel)
{
    GetMemberVars().Add(&m_Items);
    GetMemberVars().Add(&m_Selection);
    GetMemberVars().Add(&m_BgColorOdd);
    GetMemberVars().Add(&m_BgAlphaOdd);
    GetMemberVars().Add(&m_BgColorEven);
    GetMemberVars().Add(&m_BgAlphaEven);
    GetMemberVars().Add(&m_BgColorSel);
    GetMemberVars().Add(&m_BgAlphaSel);
    GetMemberVars().Add(&m_TextColorSel);
    GetMemberVars().Add(&m_TextAlphaSel);
}


ComponentListBoxT* ComponentListBoxT::Clone() const
{
    return new ComponentListBoxT(*this);
}


void ComponentListBoxT::UpdateDependencies(WindowT* Window)
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


void ComponentListBoxT::Render() const
{
    if (m_TextComp == NULL) return;
    if (!m_TextComp->m_FontInst) return;
    if (m_Items.Size() == 0) return;

    const TrueTypeFontT* Font     = m_TextComp->m_FontInst;
    const float          Scale    = m_TextComp->m_Scale.Get();
    const float          PaddingX = m_TextComp->m_Padding.Get().x;
    const float          PaddingY = m_TextComp->m_Padding.Get().y;

    const float x1 = 0.0f;
    const float y1 = 0.0f;
    const float x2 = GetWindow()->GetTransform()->GetSize().x;
    const float y2 = GetWindow()->GetTransform()->GetSize().y;

    unsigned int TextCol = 0;
    TextCol |= (unsigned int)(m_TextComp->m_Alpha.Get()    * 255.0f) << 24;
    TextCol |= (unsigned int)(m_TextComp->m_Color.Get()[0] * 255.0f) << 16;
    TextCol |= (unsigned int)(m_TextComp->m_Color.Get()[1] * 255.0f) << 8;
    TextCol |= (unsigned int)(m_TextComp->m_Color.Get()[2] * 255.0f) << 0;

    unsigned int TextSel = 0;
    TextSel |= (unsigned int)(m_TextAlphaSel.Get()    * 255.0f) << 24;
    TextSel |= (unsigned int)(m_TextColorSel.Get()[0] * 255.0f) << 16;
    TextSel |= (unsigned int)(m_TextColorSel.Get()[1] * 255.0f) << 8;
    TextSel |= (unsigned int)(m_TextColorSel.Get()[2] * 255.0f) << 0;

    const float LineSpacing = Font->GetLineSpacing(Scale);

    float LineOffsetY = 0.0f;

    switch (m_TextComp->m_AlignVer.Get())
    {
        case ComponentTextT::VarTextAlignVerT::TOP:    LineOffsetY = y1 + PaddingY; break;
        case ComponentTextT::VarTextAlignVerT::MIDDLE: LineOffsetY = y1 + (y2 - y1 - m_Items.Size()*LineSpacing) / 2.0f; break;
        case ComponentTextT::VarTextAlignVerT::BOTTOM: LineOffsetY = y2 - PaddingY - m_Items.Size()*LineSpacing; break;
    }

    // Render the row background rectangles.
    {
        float yTop = LineOffsetY;

        for (unsigned int ItemNr = 0; ItemNr < m_Items.Size(); ItemNr++)
        {
         // MatSys::Renderer->SetCurrentAmbientLightColor(BorderColor);
            MatSys::Renderer->SetCurrentMaterial(GetWindow()->GetGui().GetDefaultRM() /*BorderMaterial*/);

            static MatSys::MeshT Mesh(MatSys::MeshT::Quads);

            Mesh.Vertices.Overwrite();
            Mesh.Vertices.PushBackEmpty(4);

            for (unsigned int VertexNr = 0; VertexNr < Mesh.Vertices.Size(); VertexNr++)
            {
                // Note that the range of m_Selection is 1 ... Size, not 0 ... Size-1.
                if (ItemNr+1 == m_Selection.Get())
                {
                    for (unsigned int i = 0; i < 3; i++)
                        Mesh.Vertices[VertexNr].Color[i] = m_BgColorSel.Get()[i];

                    Mesh.Vertices[VertexNr].Color[3] = m_BgAlphaSel.Get();
                }
                else
                {
                    for (unsigned int i = 0; i < 3; i++)
                        Mesh.Vertices[VertexNr].Color[i] = (ItemNr % 2) == 0 ? m_BgColorOdd.Get()[i] : m_BgColorEven.Get()[i];

                    Mesh.Vertices[VertexNr].Color[3] = (ItemNr % 2) == 0 ? m_BgAlphaOdd.Get() : m_BgAlphaEven.Get();
                }
            }

            Mesh.Vertices[0].SetOrigin(x1, yTop); Mesh.Vertices[0].SetTextureCoord(0.0f, 0.0f);
            Mesh.Vertices[1].SetOrigin(x2, yTop); Mesh.Vertices[1].SetTextureCoord(1.0f, 0.0f);

            yTop += LineSpacing;

            Mesh.Vertices[2].SetOrigin(x2, yTop); Mesh.Vertices[2].SetTextureCoord(1.0f, 1.0f);
            Mesh.Vertices[3].SetOrigin(x1, yTop); Mesh.Vertices[3].SetTextureCoord(0.0f, 1.0f);

            MatSys::Renderer->RenderMesh(Mesh);
        }
    }

    // Without adding the ascender, the text baseline ("___") is at the top border of the window.
    LineOffsetY += Font->GetAscender(Scale);

    for (unsigned int ItemNr = 0; ItemNr < m_Items.Size(); ItemNr++)
    {
        const std::string& Item = m_Items[ItemNr];
        float AlignX = 0.0f;

        switch (m_TextComp->m_AlignHor.Get())
        {
            case ComponentTextT::VarTextAlignHorT::LEFT:   AlignX = PaddingX; break;
            case ComponentTextT::VarTextAlignHorT::CENTER: AlignX = (x2 - x1 - Font->GetWidth(Item, Scale))/2.0f; break;
            case ComponentTextT::VarTextAlignHorT::RIGHT:  AlignX = x2 - x1 - Font->GetWidth(Item, Scale) - PaddingX; break;
        }

        Font->Print(x1 + AlignX, LineOffsetY, Scale, ItemNr+1 == m_Selection.Get() ? TextSel : TextCol, "%s", Item.c_str());

        LineOffsetY += LineSpacing;
    }
}


bool ComponentListBoxT::OnInputEvent(const CaKeyboardEventT& KE)
{
    if (m_TextComp == NULL) return false;
    if (m_Items.Size() == 0) return false;
    if (KE.Type != CaKeyboardEventT::CKE_KEYDOWN) return false;

    // Note that the range of Sel is 1 ... Size, not 0 ... Size-1.
    const unsigned int Num = m_Items.Size();
    const unsigned int Sel = m_Selection.Get();

    switch (KE.Key)
    {
        case CaKeyboardEventT::CK_UP:       // UpArrow on arrow keypad.
        case CaKeyboardEventT::CK_LEFT:     // LeftArrow on arrow keypad.
            // Select the previous item.
            if (Sel > 1)
            {
                m_Selection.Set(Sel-1);
                CallLuaMethod("OnSelectionChanged");
            }
            return true;

        case CaKeyboardEventT::CK_DOWN:     // DownArrow on arrow keypad.
        case CaKeyboardEventT::CK_RIGHT:    // RightArrow on arrow keypad.
            // Select the next item.
            if (Sel < Num)
            {
                m_Selection.Set(Sel+1);
                CallLuaMethod("OnSelectionChanged");
            }
            return true;

        case CaKeyboardEventT::CK_HOME:     // Home on arrow keypad.
        case CaKeyboardEventT::CK_PGUP:     // PgUp on arrow keypad.
            // Move the selection to the first item.
            if (Sel != 1)
            {
                m_Selection.Set(1);
                CallLuaMethod("OnSelectionChanged");
            }
            return true;

        case CaKeyboardEventT::CK_END:      // End on arrow keypad.
        case CaKeyboardEventT::CK_PGDN:     // PgDn on arrow keypad.
            // Move the selection to the last item.
            if (Sel != Num)
            {
                m_Selection.Set(Num);
                CallLuaMethod("OnSelectionChanged");
            }
            return true;
    }

    // We didn't handle this event.
    return false;
}


bool ComponentListBoxT::OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY)
{
    if (m_TextComp == NULL) return false;
    if (!m_TextComp->m_FontInst) return false;
    if (m_Items.Size() == 0) return false;
    if (ME.Type != CaMouseEventT::CM_BUTTON0) return false;
    if (ME.Amount == 0) return false;

    const TrueTypeFontT* Font     = m_TextComp->m_FontInst;
    const float          Scale    = m_TextComp->m_Scale.Get();
    const float          PaddingY = m_TextComp->m_Padding.Get().y;

    const float x1 = 0.0f;
    const float y1 = 0.0f;
    const float x2 = GetWindow()->GetTransform()->GetSize().x;
    const float y2 = GetWindow()->GetTransform()->GetSize().y;

    const float LineSpacing = Font->GetLineSpacing(Scale);

    float LineOffsetY = 0.0f;

    switch (m_TextComp->m_AlignVer.Get())
    {
        case ComponentTextT::VarTextAlignVerT::TOP:    LineOffsetY = y1 + PaddingY; break;
        case ComponentTextT::VarTextAlignVerT::MIDDLE: LineOffsetY = y1 + (y2 - y1 - m_Items.Size()*LineSpacing) / 2.0f; break;
        case ComponentTextT::VarTextAlignVerT::BOTTOM: LineOffsetY = y2 - PaddingY - m_Items.Size()*LineSpacing; break;
    }

    for (unsigned int ItemNr = 0; ItemNr < m_Items.Size(); ItemNr++)
    {
        // Note that the range of m_Selection is 1 ... Size, not 0 ... Size-1.
        if (ItemNr+1 != m_Selection.Get() && PosX >= x1 && PosX < x2 && PosY >= LineOffsetY && PosY < LineOffsetY + LineSpacing)
        {
            m_Selection.Set(ItemNr+1);
            CallLuaMethod("OnSelectionChanged");
            return true;
        }

        LineOffsetY += LineSpacing;
    }

    // We didn't handle this event.
    return false;
}


static const cf::TypeSys::MethsDocT META_GetSelItem =
{
    "GetSelItem",
    "Returns the currently selected item (or nil if no item is selected).",
    "string", "()"
};

int ComponentListBoxT::GetSelItem(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentListBoxT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentListBoxT> >(1);

    if (Comp->m_Selection.Get() < 1 || Comp->m_Selection.Get() > Comp->m_Items.Size())
        lua_pushnil(LuaState);
    else
        lua_pushstring(LuaState, Comp->m_Items[Comp->m_Selection.Get() - 1].c_str());

    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentListBoxT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "list-box component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentListBoxT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentListBoxT();
}

const luaL_Reg ComponentListBoxT::MethodsList[] =
{
    { "GetSelItem", GetSelItem },
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentListBoxT::DocMethods[] =
{
    META_GetSelItem,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentListBoxT::DocCallbacks[] =
{
    { "OnSelectionChanged",
      "This method is called when the list box's selection has changed.",
      "", "" },
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentListBoxT::TypeInfo(
    GetComponentTIM(),
    "GuiSys::ComponentListBoxT",
    "GuiSys::ComponentBaseT",
    ComponentListBoxT::CreateInstance,
    MethodsList,
    DocClass,
    DocMethods,
    DocCallbacks,
    DocVars);
