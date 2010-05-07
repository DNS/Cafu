/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "ModelProperties.hpp"
#include "ChildFrame.hpp"


BEGIN_EVENT_TABLE(ModelEditor::ModelPropertiesT, wxPropertyGridManager)
    EVT_PG_CHANGED(wxID_ANY, ModelEditor::ModelPropertiesT::OnPropertyGridChanged)
END_EVENT_TABLE()


ModelEditor::ModelPropertiesT::ModelPropertiesT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_Parent(Parent)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Model Setup");
}


void ModelEditor::ModelPropertiesT::RefreshPropGrid()
{
    // if (m_ModelDocument==NULL) return;

    ClearPage(0);
    Append(new wxStringProperty("Info", wxPG_LABEL, "Hello"));
    RefreshGrid();
}


void ModelEditor::ModelPropertiesT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
 // if (m_SelectedWindow==NULL) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
 // ClearSelection();

 // m_SelectedWindow->EditorHandlePGChange(Event, m_Parent);
}
