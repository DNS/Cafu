/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "AnimInspector.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Rename.hpp"
#include "Commands/SetAnimFPS.hpp"
#include "Commands/SetAnimNext.hpp"
#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(AnimInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, AnimInspectorT::OnPropertyGridChanging)
END_EVENT_TABLE()


AnimInspectorT::AnimInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Anim Properties");

    m_ModelDoc->RegisterObserver(this);
    RefreshPropGrid();
}


AnimInspectorT::~AnimInspectorT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void AnimInspectorT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=ANIM) return;

    RefreshPropGrid();
}


void AnimInspectorT::Notify_AnimChanged(SubjectT* Subject, unsigned int AnimNr)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshPropGrid();
}


void AnimInspectorT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;
    ClearPage(0);
}


void AnimInspectorT::RefreshPropGrid()
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<CafuModelT::AnimT>& Anims    =m_ModelDoc->GetModel()->GetAnims();
    const ArrayT<unsigned int>&      Selection=m_ModelDoc->GetSelection(ANIM);

    // Currently (with wx(MSW)-2.9.1), we don't use Freeze() and Thaw(), because with them, a click
    // into the Joints Hierarchy tree (to select another joint) brings us here (via Notify_SelectionChanged()), where this
    // code (when Freeze() and Thaw() are used) steals the input focus from the Joints Hierarchy back to this property grid,
    // rendering the selected joint in the Joints Hierarchy grey (focus is elsewhere) instead of blue (tree has the focus).
    // This should be re-tested with wx(MSW)-2.9.2, which will contain fix <http://trac.wxwidgets.org/changeset/67142>.
    // Freeze();
    ClearPage(0);

    if (Selection.Size()==1)
    {
        const CafuModelT::AnimT& Anim=Anims[Selection[0]];

        Append(new wxStringProperty("Name", wxPG_LABEL, Anim.Name));
        Append(new wxFloatProperty("FPS", wxPG_LABEL, Anim.FPS));

        wxPGProperty* NumFrames=Append(new wxIntProperty("Num Frames", wxPG_LABEL, Anim.Frames.Size()));
        DisableProperty(NumFrames);

        Append(new wxIntProperty("Next sequence", wxPG_LABEL, Anim.Next));

        // if (Anim.IsLastFrameDup())
        // {
        //     wxPGProperty* Note=Append(new wxStringProperty("Note", wxPG_LABEL, "0 == last"));
        //     DisableProperty(Note);
        // }
    }
    else
    {
        const wxString InfoMessage=(Selection.Size()==0) ? "No animation selected" : "Multiple selection";

        // Multiple selection and no selection are handled by showing an info message in the property grid.
        wxPGProperty* Info=Append(new wxStringProperty("Info", wxPG_LABEL, InfoMessage));
        DisableProperty(Info);
    }

    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);  // Use checkboxes instead of choice.
    RefreshGrid();
    // Thaw();
}


void AnimInspectorT::OnPropertyGridChanging(wxPropertyGridEvent& Event)
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<unsigned int>& Selection=m_ModelDoc->GetSelection(ANIM);
    if (Selection.Size()!=1) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();

    const unsigned int AnimNr  =Selection[0];
    const wxString     PropName=Event.GetPropertyName();

    m_IsRecursiveSelfNotify=true;
    bool ok=true;

         if (PropName=="Name"         ) ok=m_Parent->SubmitCommand(new CommandRenameT(m_ModelDoc, ANIM, AnimNr, Event.GetValue().GetString()));
    else if (PropName=="FPS"          ) ok=m_Parent->SubmitCommand(new CommandSetAnimFPST(m_ModelDoc, AnimNr, Event.GetValue().GetDouble()));
    else if (PropName=="Next sequence") ok=m_Parent->SubmitCommand(new CommandSetAnimNextT(m_ModelDoc, AnimNr, Event.GetValue().GetLong()));
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
