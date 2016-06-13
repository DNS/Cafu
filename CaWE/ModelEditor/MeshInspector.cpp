/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MeshInspector.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Rename.hpp"
#include "Commands/SetMaterial.hpp"
#include "Commands/SetMeshShadows.hpp"
#include "Commands/SetMeshTSMethod.hpp"

#include "../DocumentAdapter.hpp"
#include "../EditorMaterial.hpp"
#include "../MaterialBrowser/MaterialBrowserDialog.hpp"

#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"


namespace
{
    /// Custom property to select materials for meshes.
    class MaterialPropertyT : public wxLongStringProperty
    {
        public:

        MaterialPropertyT(const wxString& name,
                          const wxString& label,
                          const wxString& value,
                          ModelEditor::ModelDocumentT* ModelDoc)
            : wxLongStringProperty(name, label, value),
              m_ModelDoc(ModelDoc)
        {
        }

        // Shows the file selection dialog and makes the choosen file path relative.
        virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
        {
            EditorMaterialI*                 InitMat=NULL;
            const ArrayT<EditorMaterialI*>&  EditorMaterials=m_ModelDoc->GetEditorMaterials();

            for (unsigned long EMNr=0; EMNr<EditorMaterials.Size(); EMNr++)
                if (EditorMaterials[EMNr]->GetName()==value)
                {
                    InitMat=EditorMaterials[EMNr];
                    break;
                }

            MaterialBrowser::DialogT MatBrowser(GetGrid(), ModelDocAdapterT(*m_ModelDoc), MaterialBrowser::ConfigT()
                .InitialMaterial(InitMat)
                .NoFilterEditorMatsOnly()
                .NoButtonMark()
                .NoButtonReplace());

            if (MatBrowser.ShowModal()!=wxID_OK) return false;

            EditorMaterialI* Mat=MatBrowser.GetCurrentMaterial();
            if (Mat==NULL) return false;

            value=Mat->GetName();
            return true;
        }


        private:

        ModelEditor::ModelDocumentT* m_ModelDoc;
    };
}


using namespace ModelEditor;


BEGIN_EVENT_TABLE(MeshInspectorT, wxPropertyGridManager)
    EVT_PG_CHANGING(wxID_ANY, MeshInspectorT::OnPropertyGridChanging)
END_EVENT_TABLE()


MeshInspectorT::MeshInspectorT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Mesh Properties");

    m_ModelDoc->RegisterObserver(this);
    RefreshPropGrid();
}


MeshInspectorT::~MeshInspectorT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void MeshInspectorT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;

    // Update the mesh also when a skin changed, as the material that
    // is used for this mesh depends on the currently selected skin.
    if (Type!=MESH && Type!=SKIN) return;

    RefreshPropGrid();
}


void MeshInspectorT::Notify_MeshChanged(SubjectT* Subject, unsigned int MeshNr)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshPropGrid();
}


void MeshInspectorT::Notify_SkinChanged(SubjectT* Subject, unsigned int SkinNr)
{
    if (m_IsRecursiveSelfNotify) return;

    // Update the mesh also when a skin changed, as the material that
    // is used for this mesh depends on the currently selected skin.
    RefreshPropGrid();
}


void MeshInspectorT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;
    ClearPage(0);
}


void MeshInspectorT::RefreshPropGrid()
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<CafuModelT::MeshT>& Meshes   =m_ModelDoc->GetModel()->GetMeshes();
    const ArrayT<unsigned int>&      Selection=m_ModelDoc->GetSelection(MESH);

    // Currently (with wx(MSW)-2.9.1), we don't use Freeze() and Thaw(), because with them, a click
    // into the Joints Hierarchy tree (to select another joint) brings us here (via Notify_SelectionChanged()), where this
    // code (when Freeze() and Thaw() are used) steals the input focus from the Joints Hierarchy back to this property grid,
    // rendering the selected joint in the Joints Hierarchy grey (focus is elsewhere) instead of blue (tree has the focus).
    // This should be re-tested with wx(MSW)-2.9.2, which will contain fix <http://trac.wxwidgets.org/changeset/67142>.
    // Freeze();
    ClearPage(0);

    if (Selection.Size()==1)
    {
        const CafuModelT::MeshT& Mesh=Meshes[Selection[0]];
        const MaterialT*         Mat =m_ModelDoc->GetModel()->GetMaterial(Selection[0], m_ModelDoc->GetSelSkinNr());

        Append(new wxStringProperty("Name", wxPG_LABEL, Mesh.Name));

        Append(new MaterialPropertyT(wxString::Format("Material (%s)", m_ModelDoc->GetSelSkinString()),
            "Material", Mat ? Mat->Name : "<NULL>", m_ModelDoc));

        const wxChar* TSM_Strings[] = { wxT("HARD"), wxT("GLOBAL"), wxT("SM_GROUPS"), NULL };
        const long    TSM_Indices[] = { CafuModelT::MeshT::HARD, CafuModelT::MeshT::GLOBAL, CafuModelT::MeshT::SM_GROUPS };
        wxPGProperty* TSMethod=Append(new wxEnumProperty("Tangent-space method", wxPG_LABEL, TSM_Strings, TSM_Indices, Mesh.TSMethod));
        TSMethod->SetHelpString("The method that is used for generating the tangent-space axes at the vertices of this mesh. See the documentation for details.");

        wxPGProperty* CastShadows=Append(new wxBoolProperty("Cast Shadows", wxPG_LABEL, Mesh.CastShadows));
        CastShadows->SetHelpString("If checked, this mesh casts shadows when lit by dynamic light sources.");

        wxPGProperty* NumTriangles=Append(new wxIntProperty("Num Triangles", wxPG_LABEL, Mesh.Triangles.Size()));
        DisableProperty(NumTriangles);

        wxPGProperty* NumVertices=Append(new wxIntProperty("Num Vertices", wxPG_LABEL, Mesh.Vertices.Size()));
        DisableProperty(NumVertices);

        wxPGProperty* NumWeights=Append(new wxIntProperty("Num Weights", wxPG_LABEL, Mesh.Weights.Size()));
        DisableProperty(NumWeights);
    }
    else
    {
        const wxString InfoMessage=(Selection.Size()==0) ? "No mesh selected" : "Multiple selection";

        // Multiple selection and no selection are handled by showing an info message in the property grid.
        wxPGProperty* Info=Append(new wxStringProperty("Info", wxPG_LABEL, InfoMessage));
        DisableProperty(Info);
    }

    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true);  // Use checkboxes instead of choice.
    RefreshGrid();
    // Thaw();
}


void MeshInspectorT::OnPropertyGridChanging(wxPropertyGridEvent& Event)
{
    if (m_ModelDoc==NULL) return;

    const ArrayT<unsigned int>& Selection=m_ModelDoc->GetSelection(MESH);
    if (Selection.Size()!=1) return;

    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
    // ClearSelection();

    const unsigned int MeshNr  =Selection[0];
    const wxString     PropName=Event.GetPropertyName();

    m_IsRecursiveSelfNotify=true;
    bool ok=true;

         if (PropName=="Name"    )             ok=m_Parent->SubmitCommand(new CommandRenameT(m_ModelDoc, MESH, MeshNr, Event.GetValue().GetString()));
    else if (PropName=="Material")             ok=m_Parent->SubmitCommand(new CommandSetMaterialT(m_ModelDoc, MeshNr, m_ModelDoc->GetSelSkinNr(), Event.GetValue().GetString()));
    else if (PropName=="Tangent-space method") ok=m_Parent->SubmitCommand(new CommandSetMeshTSMethodT(m_ModelDoc, MeshNr, CafuModelT::MeshT::TangentSpaceMethodT(Event.GetValue().GetInteger())));
    else if (PropName=="Cast Shadows")         ok=m_Parent->SubmitCommand(new CommandSetMeshShadowsT(m_ModelDoc, MeshNr, Event.GetValue().GetBool()));
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
