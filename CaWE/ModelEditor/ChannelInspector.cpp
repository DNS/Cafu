/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChannelInspector.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Rename.hpp"
#include "Commands/UpdateChannel.hpp"

#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(ChannelInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, ChannelInspectorT::OnPropertyGridChanging)
END_EVENT_TABLE()


ChannelInspectorT::ChannelInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Channel Properties");

    m_ModelDoc->RegisterObserver(this);
    RefreshPropGrid();
}


ChannelInspectorT::~ChannelInspectorT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void ChannelInspectorT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=CHAN) return;

    RefreshPropGrid();
}


void ChannelInspectorT::Notify_ChannelChanged(SubjectT* Subject, unsigned int ChannelNr)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshPropGrid();
}


void ChannelInspectorT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;
    ClearPage(0);
}


void ChannelInspectorT::RefreshPropGrid()
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<CafuModelT::JointT>&   Joints   =m_ModelDoc->GetModel()->GetJoints();
    const ArrayT<CafuModelT::ChannelT>& Channels =m_ModelDoc->GetModel()->GetChannels();
    const ArrayT<unsigned int>&         Selection=m_ModelDoc->GetSelection(CHAN);

    // Currently (with wx(MSW)-2.9.1), we don't use Freeze() and Thaw(), because with them, a click
    // into the Joints Hierarchy tree (to select another joint) brings us here (via Notify_SelectionChanged()), where this
    // code (when Freeze() and Thaw() are used) steals the input focus from the Joints Hierarchy back to this property grid,
    // rendering the selected joint in the Joints Hierarchy grey (focus is elsewhere) instead of blue (tree has the focus).
    // This should be re-tested with wx(MSW)-2.9.2, which will contain fix <http://trac.wxwidgets.org/changeset/67142>.
    // Freeze();
    ClearPage(0);

    if (Selection.Size()==0)
    {
        Append(new wxStringProperty("Name", wxPG_LABEL, "all (default)"));
        wxPGProperty* JointsProp=Append(new wxStringProperty("Joints", wxPG_LABEL, "<composed>"));

        for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
            DisableProperty(AppendIn(JointsProp, new wxBoolProperty(Joints[JointNr].Name, wxPG_LABEL, true)));
    }
    else if (Selection.Size()==1)
    {
        const CafuModelT::ChannelT& Ch=Channels[Selection[0]];

        Append(new wxStringProperty("Name", wxPG_LABEL, Ch.Name));
        wxPGProperty* JointsProp=Append(new wxStringProperty("Joints", wxPG_LABEL, "<composed>"));

        for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
            AppendIn(JointsProp, new wxBoolProperty(Joints[JointNr].Name, wxPG_LABEL, Ch.IsMember(JointNr)));
    }
    else
    {
        const wxString InfoMessage="Multiple selection";

        // Multiple selection is handled by showing an info message in the property grid.
        wxPGProperty* Info=Append(new wxStringProperty("Info", wxPG_LABEL, InfoMessage));
        DisableProperty(Info);
    }

    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);  // Use checkboxes instead of choices.
    RefreshGrid();
    // Thaw();
}


void ChannelInspectorT::OnPropertyGridChanging(wxPropertyGridEvent& Event)
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<unsigned int>& Selection=m_ModelDoc->GetSelection(CHAN);
    if (Selection.Size()!=1) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();

    const unsigned int ChanNr  =Selection[0];
    const wxString     PropName=Event.GetPropertyName();

    m_IsRecursiveSelfNotify=true;
    bool ok=true;

    if (PropName=="Name")
    {
        ok=m_Parent->SubmitCommand(new CommandRenameT(m_ModelDoc, CHAN, ChanNr, Event.GetValue().GetString()));
    }
    else if (PropName.StartsWith("Joints."))
    {
        ok=m_Parent->SubmitCommand(new CommandUpdateChannelT(m_ModelDoc, ChanNr, Event.GetProperty()->GetIndexInParent(), Event.GetValue().GetBool()));
    }
    else
    {
        // Changing child properties (e.g. "Pos.x" to "5") also generates events for the composite parent (e.g. "Pos" to "(5, 0, 0)")!
        // That is, if the following line is uncommented, it produces false warnings as well:
        // wxMessageBox("Unknown property label \""+Name+"\".", "Warning", wxOK | wxICON_ERROR);
    }

    wxASSERT(Event.CanVeto());    // EVT_PG_CHANGING events can be vetoed (as opposed to EVT_PG_CHANGED events).
    if (!ok) Event.Veto();
    m_IsRecursiveSelfNotify=false;
}
