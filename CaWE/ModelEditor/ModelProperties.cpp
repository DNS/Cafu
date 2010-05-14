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
#include "ModelDocument.hpp"

#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"


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
    const CafuModelT* Model=m_Parent->GetModelDoc()->GetModel();

    const ArrayT<CafuModelT::JointT>& Joints=Model->GetJoints();
    const ArrayT<CafuModelT::MeshT>&  Meshes=Model->GetMeshes();
    const ArrayT<CafuModelT::AnimT>&  Anims =Model->GetAnims();

    ClearPage(0);


    // "General" category.
    wxPGProperty* GeneralCat=Append(new wxPropertyCategory("General"));

    AppendIn(GeneralCat, new wxStringProperty("File name", wxPG_LABEL, Model->GetFileName()));
    AppendIn(GeneralCat, new wxBoolProperty("Use given TS", wxPG_LABEL, Model->GetUseGivenTS()));

    // "Skeleton" category.
    wxPGProperty* SkeletonCat=Append(new wxPropertyCategory("Skeleton"));

    for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
    {
        const CafuModelT::JointT& Joint=Joints[JointNr];
        const wxString            NrStr=wxString::Format("%lu", JointNr);

        wxPGProperty* JointProp=AppendIn(SkeletonCat, new wxStringProperty("Joint "+NrStr, "Skeleton.Joint"+NrStr, "<composed>"));

        AppendIn(JointProp, new wxStringProperty("Name", wxPG_LABEL, Joint.Name));
        AppendIn(JointProp, new wxIntProperty("Parent", wxPG_LABEL, Joint.Parent));

        wxPGProperty* JointPos =AppendIn(JointProp, new wxStringProperty("Pos", "Joint.Pos", "<composed>"));
        wxPGProperty* JointPosX=AppendIn(JointPos, new wxFloatProperty("x", wxPG_LABEL, Joint.Pos.x)); JointPosX->SetTextColour(wxColour(200, 0, 0));
        wxPGProperty* JointPosY=AppendIn(JointPos, new wxFloatProperty("y", wxPG_LABEL, Joint.Pos.y)); JointPosY->SetTextColour(wxColour(0, 200, 0));
        wxPGProperty* JointPosZ=AppendIn(JointPos, new wxFloatProperty("z", wxPG_LABEL, Joint.Pos.z)); JointPosZ->SetTextColour(wxColour(0, 0, 200));
        Collapse(JointPos);

        wxPGProperty* JointQtr =AppendIn(JointProp, new wxStringProperty("Qtr", "Camera.Qtr", "<composed>"));
        AppendIn(JointQtr, new wxFloatProperty("x", wxPG_LABEL, Joint.Qtr.x));
        AppendIn(JointQtr, new wxFloatProperty("y", wxPG_LABEL, Joint.Qtr.y));
        AppendIn(JointQtr, new wxFloatProperty("z", wxPG_LABEL, Joint.Qtr.z));
        Collapse(JointQtr);
    }

    // "Meshes" category.
    wxPGProperty* MeshesCat=Append(new wxPropertyCategory("Meshes"));

    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh=Meshes[MeshNr];
        const wxString           NrStr=wxString::Format("%lu", MeshNr);

        wxPGProperty* MeshProp=AppendIn(MeshesCat, new wxStringProperty("Mesh "+NrStr, "Meshes.Mesh"+NrStr, "<composed>"));

        AppendIn(MeshProp, new wxStringProperty("Material", wxPG_LABEL, Mesh.Material->Name));
    }

    // "Animations" category.
    wxPGProperty* AnimationsCat=Append(new wxPropertyCategory("Animations"));

    for (unsigned long AnimNr=0; AnimNr<Anims.Size(); AnimNr++)
    {
        const CafuModelT::AnimT& Anim=Anims[AnimNr];
        const wxString           NrStr=wxString::Format("%lu", AnimNr);

        wxPGProperty* AnimProp=AppendIn(AnimationsCat, new wxStringProperty("Anim "+NrStr, "Anims.Anim"+NrStr, "<composed>"));

        AppendIn(AnimProp, new wxFloatProperty("FPS", wxPG_LABEL, Anim.FPS));
    }

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
