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

#include "Window.hpp"
#include "../../LuaAux.hpp"

#include "GuiSys/Window.hpp"

#include <string>
#include <sstream>


using namespace GuiEditor;


EditorDataWindowT::EditorDataWindowT(cf::GuiSys::WindowT* Window, GuiDocumentT* GuiDocument)
    : cf::GuiSys::EditorDataT(Window),
      Selected(false),
      m_GuiDocument(GuiDocument),
      m_Counter(1)
{
    // If window has no name, create default name.
    if (Window->Name=="") Window->Name="Window";

    // Check window name uniqueness and repair it.
    RepairNameUniqueness();

    // Note: Since the name of a window comes from an already functional script or is checked when set
    // by the method below, we assert that the name is already LUA compatible here.
    wxASSERT(CheckLuaVarCompat(Window->Name));
}


bool EditorDataWindowT::SetName(const wxString& NewName)
{
    if (!CheckLuaVarCompat(NewName))
    {
        wxMessageBox("Window names must be LUA compatible:\n-Must only consist of letters, digits and underscores\n-Must not begin with digits\n-Must not be a LUA keyword or LUA global variable", "Error: Entity name is not LUA compatible.", wxOK | wxICON_ERROR);
        return false;
    }

    if (!CheckNameUniqueness(NewName))
    {
        wxMessageBox("The window name must be unique in this window hierarchy level. The window can't have a name that is already taken by one of its siblings.", "Error: The given window name is not unique.", wxOK | wxICON_ERROR);
        return false;
    }

    m_GuiWindow->Name=NewName.c_str();

    return true;
}


bool EditorDataWindowT::CheckNameUniqueness(const wxString& Name) const
{
    if (m_GuiWindow->GetRoot()==m_GuiWindow) return true; // Root window can have any name.

    // Get the siblings of this windows and check name uniqueness against them.
    cf::GuiSys::WindowT* Parent=m_GuiWindow->GetParent();
    ArrayT<cf::GuiSys::WindowT*> Siblings;
    Parent->GetChildren(Siblings);

    for (unsigned long SibNr=0; SibNr<Siblings.Size(); SibNr++)
    {
        if (Siblings[SibNr]==m_GuiWindow) continue; // Don't check against ourselves.

        if (Siblings[SibNr]->Name==Name) return false;
    }

    return true;
}


static wxString StripSuffix(const wxString& Str)
{
    const size_t Pos=Str.rfind("_");

    if (Pos==std::string::npos) return Str;

    for (size_t i=Pos+1; i<Str.length(); i++)
        if (!wxIsdigit(Str[i])) return Str;

    if (Pos==0) return "Window";

    return Str.Left(Pos);
}


void EditorDataWindowT::RepairNameUniqueness()
{
    const wxString BaseName=StripSuffix(m_GuiWindow->Name);
    wxString       NewName =m_GuiWindow->Name;

    while (!CheckNameUniqueness(NewName))
    {
        NewName=BaseName+"_"+wxString::Format("%u", m_Counter);
        m_Counter++;
    }

    m_GuiWindow->Name=NewName;
}
