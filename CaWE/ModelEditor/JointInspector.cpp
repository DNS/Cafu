/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "JointInspector.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Rename.hpp"
#include "Commands/TransformJoint.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(JointInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, JointInspectorT::OnPropertyGridChanging)
END_EVENT_TABLE()


JointInspectorT::JointInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Joint Properties");

    m_ModelDoc->RegisterObserver(this);
    RefreshPropGrid();
}


JointInspectorT::~JointInspectorT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void JointInspectorT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=JOINT) return;

    RefreshPropGrid();
}


void JointInspectorT::Notify_JointChanged(SubjectT* Subject, unsigned int JointNr)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshPropGrid();
}


void JointInspectorT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;
    ClearPage(0);
}


void JointInspectorT::RefreshPropGrid()
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<CafuModelT::JointT>& Joints   =m_ModelDoc->GetModel()->GetJoints();
    const ArrayT<unsigned int>&       Selection=m_ModelDoc->GetSelection(JOINT);

    // Currently (with wx(MSW)-2.9.1), we don't use Freeze() and Thaw(), because with them, a click
    // into the Joints Hierarchy tree (to select another joint) brings us here (via Notify_SelectionChanged()), where this
    // code (when Freeze() and Thaw() are used) steals the input focus from the Joints Hierarchy back to this property grid,
    // rendering the selected joint in the Joints Hierarchy grey (focus is elsewhere) instead of blue (tree has the focus).
    // This should be re-tested with wx(MSW)-2.9.2, which will contain fix <http://trac.wxwidgets.org/changeset/67142>.
    // Freeze();
    ClearPage(0);

    if (Selection.Size()==1)
    {
        const CafuModelT::JointT& Joint=Joints[Selection[0]];

        /*wxPGProperty* GeneralCat=*/Append(new wxPropertyCategory("General"));

        Append(new wxStringProperty("Name", wxPG_LABEL, Joint.Name));
        wxPGProperty* JointParent=Append(new wxIntProperty("Parent", wxPG_LABEL, Joint.Parent));
        DisableProperty(JointParent);

        /*wxPGProperty* BindPoseCat=*/Append(new wxPropertyCategory("Bind pose (no effect on animations)"));

        wxPGProperty* JointPos =Append(new wxStringProperty("Pos", wxPG_LABEL, "<composed>"));
        wxPGProperty* JointPosX=AppendIn(JointPos, new wxFloatProperty("x", wxPG_LABEL, Joint.Pos.x)); JointPosX->SetTextColour(wxColour(200, 0, 0));
        wxPGProperty* JointPosY=AppendIn(JointPos, new wxFloatProperty("y", wxPG_LABEL, Joint.Pos.y)); JointPosY->SetTextColour(wxColour(0, 200, 0));
        wxPGProperty* JointPosZ=AppendIn(JointPos, new wxFloatProperty("z", wxPG_LABEL, Joint.Pos.z)); JointPosZ->SetTextColour(wxColour(0, 0, 200));
        Collapse(JointPos);

        wxPGProperty* JointQtr =Append(new wxStringProperty("Qtr", wxPG_LABEL, "<composed>"));
        AppendIn(JointQtr, new wxFloatProperty("x", wxPG_LABEL, Joint.Qtr.x));
        AppendIn(JointQtr, new wxFloatProperty("y", wxPG_LABEL, Joint.Qtr.y));
        AppendIn(JointQtr, new wxFloatProperty("z", wxPG_LABEL, Joint.Qtr.z));
        Collapse(JointQtr);

        wxPGProperty* JointScale =Append(new wxStringProperty("Scale", wxPG_LABEL, "<composed>"));
        wxPGProperty* JointScaleX=AppendIn(JointScale, new wxFloatProperty("x", wxPG_LABEL, Joint.Scale.x)); JointScaleX->SetTextColour(wxColour(200, 0, 0));
        wxPGProperty* JointScaleY=AppendIn(JointScale, new wxFloatProperty("y", wxPG_LABEL, Joint.Scale.y)); JointScaleY->SetTextColour(wxColour(0, 200, 0));
        wxPGProperty* JointScaleZ=AppendIn(JointScale, new wxFloatProperty("z", wxPG_LABEL, Joint.Scale.z)); JointScaleZ->SetTextColour(wxColour(0, 0, 200));
        Collapse(JointScale);
    }
    else
    {
        const wxString InfoMessage=(Selection.Size()==0) ? "No joint selected" : "Multiple selection";

        // Multiple selection and no selection are handled by showing an info message in the property grid.
        wxPGProperty* Info=Append(new wxStringProperty("Info", wxPG_LABEL, InfoMessage));
        DisableProperty(Info);
    }

    RefreshGrid();
    // Thaw();
}


void JointInspectorT::OnPropertyGridChanging(wxPropertyGridEvent& Event)
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<unsigned int>& Selection=m_ModelDoc->GetSelection(JOINT);
    if (Selection.Size()!=1) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();

    const ArrayT<CafuModelT::JointT>& Joints =m_ModelDoc->GetModel()->GetJoints();
    const unsigned int                JointNr=Selection[0];

    const wxString PropName  =Event.GetPropertyName();
    double         PropValueD=0.0;
    const float    PropValueF=Event.GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;

    Vector3fT Pos  =Joints[JointNr].Pos;
    Vector3fT Qtr  =Joints[JointNr].Qtr;
    Vector3fT Scale=Joints[JointNr].Scale;

    m_IsRecursiveSelfNotify=true;
    bool ok=true;

         if (PropName=="Name"   ) ok=m_Parent->SubmitCommand(new CommandRenameT(m_ModelDoc, JOINT, JointNr, Event.GetValue().GetString()));
 // else if (PropName=="Parent" ) ;
    else if (PropName=="Pos.x"  ) { Pos.x  =PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 'p', Pos  )); }
    else if (PropName=="Pos.y"  ) { Pos.y  =PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 'p', Pos  )); }
    else if (PropName=="Pos.z"  ) { Pos.z  =PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 'p', Pos  )); }
    else if (PropName=="Qtr.x"  ) { Qtr.x  =PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 'q', Qtr  )); }
    else if (PropName=="Qtr.y"  ) { Qtr.y  =PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 'q', Qtr  )); }
    else if (PropName=="Qtr.z"  ) { Qtr.z  =PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 'q', Qtr  )); }
    else if (PropName=="Scale.x") { Scale.x=PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 's', Scale)); }
    else if (PropName=="Scale.y") { Scale.y=PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 's', Scale)); }
    else if (PropName=="Scale.z") { Scale.z=PropValueF; ok=m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, JointNr, 's', Scale)); }
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
