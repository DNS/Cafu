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

#include "GlobalsInspector.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(GlobalsInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGED(wxID_ANY, GlobalsInspectorT::OnPropertyGridChanged)
END_EVENT_TABLE()


GlobalsInspectorT::GlobalsInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Global Model Properties");

    m_ModelDoc->RegisterObserver(this);
    RefreshPropGrid();
}


GlobalsInspectorT::~GlobalsInspectorT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void GlobalsInspectorT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;
    ClearPage(0);
}


void GlobalsInspectorT::RefreshPropGrid()
{
    if (m_ModelDoc==NULL) return;

    // Currently (with wx(MSW)-2.9.1), we don't use Freeze() and Thaw(), because with them, a click
    // into the Joints Hierarchy tree (to select another joint) brings us here (via Notify_SelectionChanged()), where this
    // code (when Freeze() and Thaw() are used) steals the input focus from the Joints Hierarchy back to this property grid,
    // rendering the selected joint in the Joints Hierarchy grey (focus is elsewhere) instead of blue (tree has the focus).
    // This should be re-tested with wx(MSW)-2.9.2, which will contain fix <http://trac.wxwidgets.org/changeset/67142>.
    // Freeze();
    ClearPage(0);

    Append(new wxBoolProperty("Use given TS", wxPG_LABEL, m_ModelDoc->GetModel()->GetUseGivenTS()));

    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);  // Use checkboxes instead of choice.
    RefreshGrid();
    // Thaw();
}


void GlobalsInspectorT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
    if (m_ModelDoc==NULL) return;

    const wxPGProperty* Prop=Event.GetProperty();
    if (!Prop) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();

    // const wxString PropName  =Prop->GetName();
    // double         PropValueD=0.0;
    // const float    PropValueF=Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;

    m_IsRecursiveSelfNotify=true;

/*  if (PropName=="Name"   ) m_Parent->SubmitCommand(new CommandRenameT(m_ModelDoc, JOINT, JointNr, Prop->GetValueAsString()));
    else
    {
        // Changing child properties (e.g. "Pos.x" to "5") also generates events for the composite parent (e.g. "Pos" to "(5, 0, 0)")!
        // That is, if the following line is uncommented, it produces false warnings as well:
        // wxMessageBox("Unknown property label \""+Name+"\".", "Warning", wxOK | wxICON_ERROR);
    } */

    m_IsRecursiveSelfNotify=false;
}
