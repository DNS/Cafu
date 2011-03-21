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

#include "GuiInspector.hpp"

#include "ChildFrame.hpp"
#include "GuiDocument.hpp"

#include "Commands/ModifyGui.hpp"


using namespace GuiEditor;


BEGIN_EVENT_TABLE(GuiInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGED(wxID_ANY, GuiInspectorT::OnPropertyGridChanged)
END_EVENT_TABLE()


GuiInspectorT::GuiInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_GuiDocument(Parent->GetGuiDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfUpdate(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Gui Properties");
}


void GuiInspectorT::NotifySubjectChanged_GuiPropertyModified(SubjectT* Subject)
{
    if (m_IsRecursiveSelfUpdate) return;

    RefreshPropGrid();
}


void GuiInspectorT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_GuiDocument);

    m_GuiDocument=NULL;

    ClearPage(0);
}


void GuiInspectorT::RefreshPropGrid()
{
    if (m_GuiDocument==NULL) return;

    ClearPage(0);

    Append(new wxBoolProperty  ("Activate",      wxPG_LABEL, m_GuiDocument->GetGuiProperties().Activate));
    Append(new wxBoolProperty  ("Interactive",   wxPG_LABEL, m_GuiDocument->GetGuiProperties().Interactive));
    Append(new wxBoolProperty  ("ShowMouse",     wxPG_LABEL, m_GuiDocument->GetGuiProperties().ShowMouse));
    Append(new wxStringProperty("DefaultFocus",  wxPG_LABEL, m_GuiDocument->GetGuiProperties().DefaultFocus));

    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX,true); // Use checkboxes instead of choice.

    RefreshGrid();
}


void GuiInspectorT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
    const wxPGProperty* Prop    =Event.GetProperty();
    const wxString      PropName=Prop->GetName();

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    ClearSelection();

    m_IsRecursiveSelfUpdate=true;

    if (PropName=="Activate")
        m_Parent->SubmitCommand(CommandModifyGuiT::Create(m_GuiDocument, "Activate", Prop->GetValue().GetBool()));
    else if (PropName=="Interactive")
        m_Parent->SubmitCommand(CommandModifyGuiT::Create(m_GuiDocument, "Interactive", Prop->GetValue().GetBool()));
    else if (PropName=="ShowMouse")
        m_Parent->SubmitCommand(CommandModifyGuiT::Create(m_GuiDocument, "ShowMouse", Prop->GetValue().GetBool()));
    else if (PropName=="DefaultFocus")
        m_Parent->SubmitCommand(CommandModifyGuiT::Create(m_GuiDocument, "DefaultFocus", Prop->GetValueAsString()));
    else
        wxASSERT(false);

    m_IsRecursiveSelfUpdate=false;
}
