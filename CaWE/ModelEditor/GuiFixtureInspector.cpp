/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "GuiFixtureInspector.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/UpdateGuiFixture.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(GuiFixInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, GuiFixInspectorT::OnPropertyGridChanging)
END_EVENT_TABLE()


GuiFixInspectorT::GuiFixInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("GUI Fixture Properties");

    m_ModelDoc->RegisterObserver(this);
    RefreshPropGrid();
}


GuiFixInspectorT::~GuiFixInspectorT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void GuiFixInspectorT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=GFIX) return;

    RefreshPropGrid();
}


void GuiFixInspectorT::Notify_GuiFixtureChanged(SubjectT* Subject, unsigned int GuiFixtureNr)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshPropGrid();
}


void GuiFixInspectorT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;
    ClearPage(0);
}


void GuiFixInspectorT::RefreshPropGrid()
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures=m_ModelDoc->GetModel()->GetGuiFixtures();
    const ArrayT<unsigned int>&            Selection  =m_ModelDoc->GetSelection(GFIX);

    // Currently (with wx(MSW)-2.9.1), we don't use Freeze() and Thaw(), because with them, a click
    // into the Joints Hierarchy tree (to select another joint) brings us here (via Notify_SelectionChanged()), where this
    // code (when Freeze() and Thaw() are used) steals the input focus from the Joints Hierarchy back to this property grid,
    // rendering the selected joint in the Joints Hierarchy grey (focus is elsewhere) instead of blue (tree has the focus).
    // This should be re-tested with wx(MSW)-2.9.2, which will contain fix <http://trac.wxwidgets.org/changeset/67142>.
    // Freeze();
    ClearPage(0);

    if (Selection.Size()==1)
    {
        const CafuModelT::GuiFixtureT& GF=GuiFixtures[Selection[0]];

        Append(new wxStringProperty("Name", wxPG_LABEL, GF.Name));

        for (unsigned int PointNr=0; PointNr<3; PointNr++)
        {
            const char* PointNames[3]={ "Origin", "X-endpoint", "Y-endpoint" };

            wxPGProperty* Point=Append(new wxStringProperty(PointNames[PointNr], wxPG_LABEL, "<composed>"));
            AppendIn(Point, new wxIntProperty("Mesh",   wxPG_LABEL, GF.Points[PointNr].MeshNr  ))->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsMeshNrOK  (GF, PointNr)   ? "#FFFFFF" : "#FFF3BB"));
            AppendIn(Point, new wxIntProperty("Vertex", wxPG_LABEL, GF.Points[PointNr].VertexNr))->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsVertexNrOK(GF, PointNr) ? "#FFFFFF" : "#FFF3BB"));
        }

        wxPGProperty* Trans=Append(new wxStringProperty("Translation", wxPG_LABEL, "<composed>"));
        AppendIn(Trans, new wxFloatProperty("x", wxPG_LABEL, GF.Trans[0]));
        AppendIn(Trans, new wxFloatProperty("y", wxPG_LABEL, GF.Trans[1]));
        Collapse(Trans);

        wxPGProperty* Scale=Append(new wxStringProperty("Scale", wxPG_LABEL, "<composed>"));
        AppendIn(Scale, new wxFloatProperty("x", wxPG_LABEL, GF.Scale[0]));
        AppendIn(Scale, new wxFloatProperty("y", wxPG_LABEL, GF.Scale[1]));
        Collapse(Scale);
    }
    else
    {
        const wxString InfoMessage=(Selection.Size()==0) ? "No GUI fixture selected" : "Multiple selection";

        // Multiple selection and no selection are handled by showing an info message in the property grid.
        wxPGProperty* Info=Append(new wxStringProperty("Info", wxPG_LABEL, InfoMessage));
        DisableProperty(Info);
    }

    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);  // Use checkboxes instead of choice.
    RefreshGrid();
    // Thaw();
}


void GuiFixInspectorT::OnPropertyGridChanging(wxPropertyGridEvent& Event)
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<unsigned int>& Selection=m_ModelDoc->GetSelection(GFIX);
    if (Selection.Size()!=1) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();

    const unsigned int GFixNr  =Selection[0];
    const wxString     PropName=Event.GetPropertyName();

    CafuModelT::GuiFixtureT GF=m_ModelDoc->GetModel()->GetGuiFixtures()[GFixNr];

         if (PropName=="Name"             ) GF.Name              =Event.GetValue().GetString();   // Using CommandRenameT here would achieve the same.
    else if (PropName=="Origin.Mesh"      ) { GF.Points[0].MeshNr  =Event.GetValue().GetLong(); Event.GetProperty()->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsMeshNrOK  (GF, 0) ? "#FFFFFF" : "#FFF3BB")); }
    else if (PropName=="Origin.Vertex"    ) { GF.Points[0].VertexNr=Event.GetValue().GetLong(); Event.GetProperty()->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsVertexNrOK(GF, 0) ? "#FFFFFF" : "#FFF3BB")); }
    else if (PropName=="X-endpoint.Mesh"  ) { GF.Points[1].MeshNr  =Event.GetValue().GetLong(); Event.GetProperty()->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsMeshNrOK  (GF, 1) ? "#FFFFFF" : "#FFF3BB")); }
    else if (PropName=="X-endpoint.Vertex") { GF.Points[1].VertexNr=Event.GetValue().GetLong(); Event.GetProperty()->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsVertexNrOK(GF, 1) ? "#FFFFFF" : "#FFF3BB")); }
    else if (PropName=="Y-endpoint.Mesh"  ) { GF.Points[2].MeshNr  =Event.GetValue().GetLong(); Event.GetProperty()->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsMeshNrOK  (GF, 2) ? "#FFFFFF" : "#FFF3BB")); }
    else if (PropName=="Y-endpoint.Vertex") { GF.Points[2].VertexNr=Event.GetValue().GetLong(); Event.GetProperty()->SetBackgroundColour(wxColour(m_ModelDoc->GetModel()->IsVertexNrOK(GF, 2) ? "#FFFFFF" : "#FFF3BB")); }
    else if (PropName=="Translation.x"    ) GF.Trans[0]          =Event.GetValue().GetDouble();
    else if (PropName=="Translation.y"    ) GF.Trans[1]          =Event.GetValue().GetDouble();
    else if (PropName=="Scale.x"          ) GF.Scale[0]          =Event.GetValue().GetDouble();
    else if (PropName=="Scale.y"          ) GF.Scale[1]          =Event.GetValue().GetDouble();
    else
    {
        // Changing child properties (e.g. "Pos.x" to "5") also generates events for the composite parent (e.g. "Pos" to "(5, 0, 0)")!
        // That is, if the following line is uncommented, it produces false warnings as well:
        // wxMessageBox("Unknown property label \""+Name+"\".", "Warning", wxOK | wxICON_ERROR);
    }

    m_IsRecursiveSelfNotify=true;
    wxASSERT(Event.CanVeto());    // EVT_PG_CHANGING events can be vetoed (as opposed to EVT_PG_CHANGED events).
    bool ok=m_Parent->SubmitCommand(new CommandUpdateGuiFixtureT(m_ModelDoc, GFixNr, GF));
    if (!ok) Event.Veto();
    m_IsRecursiveSelfNotify=false;
}
