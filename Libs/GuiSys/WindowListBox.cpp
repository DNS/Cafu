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

#include "WindowListBox.hpp"
#include "GuiImpl.hpp"
#include "WindowCreateParams.hpp"
#include "Fonts/FontTT.hpp"
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

void* ListBoxT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ListBoxT(*static_cast<const cf::GuiSys::WindowCreateParamsT*>(&Params));
}

const luaL_reg ListBoxT::MethodsList[]=
{
    { "Clear",              ListBoxT::Clear },
    { "Append",             ListBoxT::Append },
    { "Insert",             ListBoxT::Insert },
    { "GetNumRows",         ListBoxT::GetNumRows },
    { "GetRowText",         ListBoxT::GetRowText },
    { "SetRowText",         ListBoxT::SetRowText },
    { "GetSelection",       ListBoxT::GetSelection },
    { "SetSelection",       ListBoxT::SetSelection },
    { "GetRowHeight",       ListBoxT::GetRowHeight },
    { "SetRowHeight",       ListBoxT::SetRowHeight },
    { "SetOddRowBgColor",   ListBoxT::SetOddRowBgColor },
    { "SetEvenRowBgColor",  ListBoxT::SetEvenRowBgColor },
    { "SetRowTextColor",    ListBoxT::SetRowTextColor },
    { "SetSelRowBgColor",   ListBoxT::SetSelRowBgColor },
    { "SetSelRowTextColor", ListBoxT::SetSelRowTextColor },
    { "__gc",               WindowT::Destruct },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ListBoxT::TypeInfo(GetWindowTIM(), "ListBoxT", "WindowT", ListBoxT::CreateInstance, MethodsList);

/*** End of TypeSys related definitions for this class. ***/


ListBoxT::ListBoxT(const cf::GuiSys::WindowCreateParamsT& Params)
    : WindowT(Params),
      SelectedRow(0xFFFFFFFF),
      RowHeight(20.0f)
{
    for (unsigned long c=0; c<4; c++)
    {
        OddRowBgColor[c]=0.3f;
        EvenRowBgColor[c]=0.4f;
        RowTextColor[c]=float(c & 1);
        SelectedRowBgColor[c]=0.8f;
        SelectedRowTextColor[c]=(c<2) ? 0.0f : 1.0f;
    }

    FillMemberVars();
}


ListBoxT::ListBoxT(const ListBoxT& Window, bool Recursive)
    : WindowT(Window, Recursive),
      SelectedRow(Window.SelectedRow),
      RowHeight(Window.RowHeight)
{
    for (unsigned long i=0; i<4; i++)
    {
        OddRowBgColor       [i]=Window.OddRowBgColor       [i];
        EvenRowBgColor      [i]=Window.EvenRowBgColor      [i];
        RowTextColor        [i]=Window.RowTextColor        [i];
        SelectedRowBgColor  [i]=Window.SelectedRowBgColor  [i];
        SelectedRowTextColor[i]=Window.SelectedRowTextColor[i];
    }

    for (unsigned long RowNr=0; RowNr<Window.Rows.Size(); RowNr++)
    {
        Rows.PushBack(Window.Rows[RowNr]->Clone(true));
        Rows[RowNr]->Parent=this;
    }

    FillMemberVars();
}


ListBoxT* ListBoxT::Clone(bool Recursive) const
{
    return new ListBoxT(*this, Recursive);
}


ListBoxT::~ListBoxT()
{
    for (unsigned long RowNr=0; RowNr<Rows.Size(); RowNr++)
    {
        delete Rows[RowNr];
        Rows[RowNr]=NULL;
    }
}


void ListBoxT::Insert(unsigned long RowNr, const std::string& RowText)
{
    Rows.InsertAt(RowNr, new WindowT(WindowCreateParamsT(m_Gui)));

    // Initialize the new row window.
    Rows[RowNr]->Parent      =this;        // Set the proper parent so that the rendering of the row is relative to this window.
    Rows[RowNr]->Text        =RowText;
    Rows[RowNr]->TextScale   =TextScale;
    Rows[RowNr]->TextAlignVer=middle;
    Rows[RowNr]->Rect[2]     =Rect[2];
    Rows[RowNr]->Rect[3]     =RowHeight;

    for (unsigned long c=0; c<4; c++)
    {
        Rows[RowNr]->TextColor[c]=RowTextColor[c];
        Rows[RowNr]->BackColor[c]=(RowNr % 2)==0 ? OddRowBgColor[c] : EvenRowBgColor[c];
    }


    // Re-compute the positions of all row subwindows.
    for (RowNr=0; RowNr<Rows.Size(); RowNr++)
    {
        Rows[RowNr]->Rect[1]=RowNr*RowHeight;
    }
}


void ListBoxT::Render() const
{
    WindowT::Render();

    for (unsigned long RowNr=0; RowNr<Rows.Size(); RowNr++)
    {
        float NormalBackColor[4]={ 0, 0, 0, 0 };
        float NormalTextColor[4]={ 0, 0, 0, 0 };

        if (RowNr==SelectedRow)
        {
            // Save the normal background and text colors,
            // the set the colors for the *selected* row.
            for (unsigned long c=0; c<4; c++)
            {
                NormalBackColor[c]=Rows[RowNr]->BackColor[c];
                NormalTextColor[c]=Rows[RowNr]->TextColor[c];

                Rows[RowNr]->BackColor[c]=SelectedRowBgColor[c];
                Rows[RowNr]->TextColor[c]=SelectedRowTextColor[c];
            }
        }

        Rows[RowNr]->Render();

        if (RowNr==SelectedRow)
        {
            // Restore the normal colors for the selected row.
            for (unsigned long c=0; c<4; c++)
            {
                Rows[RowNr]->BackColor[c]=NormalBackColor[c];
                Rows[RowNr]->TextColor[c]=NormalTextColor[c];
            }
        }
    }
}


bool ListBoxT::OnInputEvent(const CaKeyboardEventT& KE)
{
    // 1. The relevant Lua OnKeyDown/Up() or OnChar() methods didn't handle this event, so we got here (see GuiT::ProcessDeviceEvent()).
    // 2. Now see if we want to and can handle the event here.
    if (KE.Type==CaKeyboardEventT::CKE_KEYDOWN)
    {
        switch (KE.Key)
        {
            case CaKeyboardEventT::CK_UP:       // UpArrow on arrow keypad.
            case CaKeyboardEventT::CK_LEFT:     // LeftArrow on arrow keypad.
                // Move the selection one row up.
                if (SelectedRow!=0xFFFFFFFF && SelectedRow>0)
                {
                    SelectedRow--;
                    CallLuaMethod("OnSelectionChanged", "i", SelectedRow);
                }
                return true;

            case CaKeyboardEventT::CK_DOWN:     // DownArrow on arrow keypad.
            case CaKeyboardEventT::CK_RIGHT:    // RightArrow on arrow keypad.
                // Move the selection one row down.
                if (SelectedRow!=0xFFFFFFFF && SelectedRow+1<Rows.Size())
                {
                    SelectedRow++;
                    CallLuaMethod("OnSelectionChanged", "i", SelectedRow);
                }
                return true;

            case CaKeyboardEventT::CK_HOME:     // Home on arrow keypad.
            case CaKeyboardEventT::CK_PGUP:     // PgUp on arrow keypad.
                // Move the selection to the first row.
                if (Rows.Size()>0 && SelectedRow>0)
                {
                    SelectedRow=0;
                    CallLuaMethod("OnSelectionChanged", "i", SelectedRow);
                }
                return true;

            case CaKeyboardEventT::CK_END:      // End on arrow keypad.
            case CaKeyboardEventT::CK_PGDN:     // PgDn on arrow keypad.
                // Move the selection to the last row.
                if (Rows.Size()>0 && SelectedRow+1<Rows.Size())
                {
                    SelectedRow=Rows.Size()-1;
                    CallLuaMethod("OnSelectionChanged", "i", SelectedRow);
                }
                return true;
        }
    }

    // 3. Nobody handled the event so far, now propagate it to the base class(es),
    //    where it will (when still unhandled), travel up to the (class hierarchy of the) parent window(s).
    return WindowT::OnInputEvent(KE);
}


bool ListBoxT::OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY)
{
    // 1. The relevant Lua OnMouse...() methods didn't handle this event, so we got here (see GuiT::ProcessDeviceEvent()).
    // 2. Now see if we want to and can handle the event here.
    if (ME.Type==CaMouseEventT::CM_BUTTON0 && ME.Amount>0)
    {
        for (unsigned long RowNr=0; RowNr<Rows.Size(); RowNr++)
        {
            if (RowNr==SelectedRow) continue;

            if (PosY>=Rows[RowNr]->Rect[1] && PosY<=Rows[RowNr]->Rect[1]+Rows[RowNr]->Rect[3])
            {
                SelectedRow=RowNr;
                CallLuaMethod("OnSelectionChanged", "i", SelectedRow);
                return true;
            }
        }
    }

    // 3. Nobody handled the event so far, now propagate it to the base class(es),
    //    where it will (when still unhandled), travel up to the (class hierarchy of the) parent window(s).
    return WindowT::OnInputEvent(ME, PosX, PosY);
}


void ListBoxT::FillMemberVars()
{
    WindowT::FillMemberVars();

    MemberVars["selectedRow"]=MemberVarT(MemberVarT::TYPE_INT, &SelectedRow);
    MemberVars["rowHeight"]=MemberVarT(RowHeight);
    MemberVars["oddRowBgColor"]=MemberVarT(MemberVarT::TYPE_FLOAT4, &OddRowBgColor[0]);
    MemberVars["oddRowBgColor.r"]=MemberVarT(OddRowBgColor[0]);
    MemberVars["oddRowBgColor.g"]=MemberVarT(OddRowBgColor[1]);
    MemberVars["oddRowBgColor.b"]=MemberVarT(OddRowBgColor[2]);
    MemberVars["oddRowBgColor.a"]=MemberVarT(OddRowBgColor[3]);
    MemberVars["evenRowBgColor"]=MemberVarT(MemberVarT::TYPE_FLOAT4, &EvenRowBgColor[0]);
    MemberVars["evenRowBgColor.r"]=MemberVarT(EvenRowBgColor[0]);
    MemberVars["evenRowBgColor.g"]=MemberVarT(EvenRowBgColor[1]);
    MemberVars["evenRowBgColor.b"]=MemberVarT(EvenRowBgColor[2]);
    MemberVars["evenRowBgColor.a"]=MemberVarT(EvenRowBgColor[3]);
    MemberVars["rowTextColor"]=MemberVarT(MemberVarT::TYPE_FLOAT4, &RowTextColor[0]);
    MemberVars["rowTextColor.r"]=MemberVarT(RowTextColor[0]);
    MemberVars["rowTextColor.g"]=MemberVarT(RowTextColor[1]);
    MemberVars["rowTextColor.b"]=MemberVarT(RowTextColor[2]);
    MemberVars["rowTextColor.a"]=MemberVarT(RowTextColor[3]);
    MemberVars["selectedRowBgColor"]=MemberVarT(MemberVarT::TYPE_FLOAT4, &SelectedRowBgColor[0]);
    MemberVars["selectedRowBgColor.r"]=MemberVarT(SelectedRowBgColor[0]);
    MemberVars["selectedRowBgColor.g"]=MemberVarT(SelectedRowBgColor[1]);
    MemberVars["selectedRowBgColor.b"]=MemberVarT(SelectedRowBgColor[2]);
    MemberVars["selectedRowBgColor.a"]=MemberVarT(SelectedRowBgColor[3]);
    MemberVars["selectedRowTextColor"]=MemberVarT(MemberVarT::TYPE_FLOAT4, &SelectedRowTextColor[0]);
    MemberVars["selectedRowTextColor.r"]=MemberVarT(SelectedRowTextColor[0]);
    MemberVars["selectedRowTextColor.g"]=MemberVarT(SelectedRowTextColor[1]);
    MemberVars["selectedRowTextColor.b"]=MemberVarT(SelectedRowTextColor[2]);
    MemberVars["selectedRowTextColor.a"]=MemberVarT(SelectedRowTextColor[3]);
}


int ListBoxT::Clear(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    for (unsigned long RowNr=0; RowNr<ListBox->Rows.Size(); RowNr++)
    {
        delete ListBox->Rows[RowNr];
    }

    ListBox->Rows.Overwrite();
    ListBox->SelectedRow=0xFFFFFFFF;
    return 0;
}


int ListBoxT::Append(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);
    unsigned long RowNr  =ListBox->Rows.Size();
    const char*   RowText=luaL_checkstring(LuaState, 2);

    ListBox->Insert(RowNr, RowText);
    return 0;
}


int ListBoxT::Insert(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);
    unsigned long RowNr  =luaL_checkinteger(LuaState, 2);
    const char*   RowText=luaL_checkstring(LuaState, 3);

    luaL_argcheck(LuaState, RowNr<=ListBox->Rows.Size(), 2, "Insertion index too large.");
    ListBox->Insert(RowNr, RowText);
    return 0;
}


int ListBoxT::GetNumRows(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    lua_pushinteger(LuaState, ListBox->Rows.Size());
    return 1;
}


int ListBoxT::GetRowText(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);
    unsigned long RowNr  =luaL_checkinteger(LuaState, 2);

    luaL_argcheck(LuaState, RowNr<ListBox->Rows.Size(), 2, "Index out of range.");
    lua_pushstring(LuaState, ListBox->Rows[RowNr]->Text.c_str());
    return 1;
}


int ListBoxT::SetRowText(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);
    unsigned long RowNr  =luaL_checkinteger(LuaState, 2);

    luaL_argcheck(LuaState, RowNr<ListBox->Rows.Size(), 2, "Index out of range.");
    ListBox->Rows[RowNr]->Text=luaL_checkstring(LuaState, 3);
    return 0;
}


int ListBoxT::GetSelection(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    lua_pushinteger(LuaState, ListBox->SelectedRow>=ListBox->Rows.Size() ? -1 : ListBox->SelectedRow);
    return 1;
}


int ListBoxT::SetSelection(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ListBox->SelectedRow=luaL_checkinteger(LuaState, 2);

    // Anything out-of-range is a "none" selection, set them uniquely to 0xFFFFFFFF.
    if (ListBox->SelectedRow >= ListBox->Rows.Size()) ListBox->SelectedRow=0xFFFFFFFF;

    return 0;
}


int ListBoxT::GetRowHeight(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    lua_pushnumber(LuaState, ListBox->RowHeight);
    return 1;
}


int ListBoxT::SetRowHeight(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ListBox->RowHeight=float(lua_tonumber(LuaState, 2));

    if (ListBox->RowHeight<0)
    {
        // Reset to font default.
        ListBox->RowHeight=ListBox->Font->GetLineSpacing(ListBox->TextScale) * -(ListBox->RowHeight);
    }

    // Re-compute the positions of all row subwindows.
    for (unsigned long RowNr=0; RowNr<ListBox->Rows.Size(); RowNr++)
    {
        ListBox->Rows[RowNr]->Rect[1]=RowNr*ListBox->RowHeight;
        ListBox->Rows[RowNr]->Rect[3]=ListBox->RowHeight;
    }

    return 0;
}


int ListBoxT::SetOddRowBgColor(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ListBox->OddRowBgColor[0]=float(lua_tonumber(LuaState, 2));
    ListBox->OddRowBgColor[1]=float(lua_tonumber(LuaState, 3));
    ListBox->OddRowBgColor[2]=float(lua_tonumber(LuaState, 4));
    ListBox->OddRowBgColor[3]=lua_isnil(LuaState, 5) ? 1.0f : float(lua_tonumber(LuaState, 5));

    return 0;
}


int ListBoxT::SetEvenRowBgColor(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ListBox->EvenRowBgColor[0]=float(lua_tonumber(LuaState, 2));
    ListBox->EvenRowBgColor[1]=float(lua_tonumber(LuaState, 3));
    ListBox->EvenRowBgColor[2]=float(lua_tonumber(LuaState, 4));
    ListBox->EvenRowBgColor[3]=lua_isnil(LuaState, 5) ? 1.0f : float(lua_tonumber(LuaState, 5));

    return 0;
}


int ListBoxT::SetRowTextColor(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ListBox->RowTextColor[0]=float(lua_tonumber(LuaState, 2));
    ListBox->RowTextColor[1]=float(lua_tonumber(LuaState, 3));
    ListBox->RowTextColor[2]=float(lua_tonumber(LuaState, 4));
    ListBox->RowTextColor[3]=lua_isnil(LuaState, 5) ? 1.0f : float(lua_tonumber(LuaState, 5));

    return 0;
}


int ListBoxT::SetSelRowBgColor(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ListBox->SelectedRowBgColor[0]=float(lua_tonumber(LuaState, 2));
    ListBox->SelectedRowBgColor[1]=float(lua_tonumber(LuaState, 3));
    ListBox->SelectedRowBgColor[2]=float(lua_tonumber(LuaState, 4));
    ListBox->SelectedRowBgColor[3]=lua_isnil(LuaState, 5) ? 1.0f : float(lua_tonumber(LuaState, 5));

    return 0;
}


int ListBoxT::SetSelRowTextColor(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ListBoxT*     ListBox=(ListBoxT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ListBox->SelectedRowTextColor[0]=float(lua_tonumber(LuaState, 2));
    ListBox->SelectedRowTextColor[1]=float(lua_tonumber(LuaState, 3));
    ListBox->SelectedRowTextColor[2]=float(lua_tonumber(LuaState, 4));
    ListBox->SelectedRowTextColor[3]=lua_isnil(LuaState, 5) ? 1.0f : float(lua_tonumber(LuaState, 5));

    return 0;
}
