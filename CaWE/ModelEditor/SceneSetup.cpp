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

#include "SceneSetup.hpp"
#include "ChildFrame.hpp"
#include "../EditorMaterial.hpp"
#include "../GameConfig.hpp"

#include "wx/confbase.h"
#include "wx/propgrid/advprops.h"


BEGIN_EVENT_TABLE(ModelEditor::SceneSetupT, wxPropertyGridManager)
    EVT_PG_CHANGED(wxID_ANY, ModelEditor::SceneSetupT::OnPropertyGridChanged)
END_EVENT_TABLE()


ModelEditor::SceneSetupT::SceneSetupT(ChildFrameT* Parent, const wxSize& Size, GameConfigT* GameConfig)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_Camera(),
      m_BackgroundColor(wxColour(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/BackgroundColor", "rgb(0, 128, 255)"))),
      m_ShowOrigin(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/ShowOrigin", 1l)!=0),
      m_GroundPlane_Show(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_Show", 1l)!=0),
      m_GroundPlane_zPos(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_zPos", 0.0)),
      m_GroundPlane_Mat(GameConfig->GetMatMan().FindMaterial(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_Mat", "Textures/WilliH/rock01b"), true /*CreateDummy*/)),
      m_AmbientLightColor(wxColour(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/AmbientLightColor", "rgb(96, 96, 96)"))),
      m_Parent(Parent),
      m_GameConfig(GameConfig)
{
    m_Camera.Pos.y=-500.0f;

    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Scene Setup");
}


void ModelEditor::SceneSetupT::RefreshPropGrid()
{
    // if (m_GuiDocument==NULL) return;

    ClearPage(0);

    // "General" category.
    wxPGProperty* GeneralCat=Append(new wxPropertyCategory("General"));

    AppendIn(GeneralCat, new wxColourProperty("Background Color", wxPG_LABEL, m_BackgroundColor));
    AppendIn(GeneralCat, new wxBoolProperty("Show Origin", wxPG_LABEL, m_ShowOrigin));


    // "Camera" category.
    wxPGProperty* CameraCat=Append(new wxPropertyCategory("Camera"));

    wxPGProperty* CameraPos=AppendIn(CameraCat, new wxStringProperty("Pos", "Camera.Pos", "<composed>"));
    wxPGProperty* CameraPosX=AppendIn(CameraPos, new wxFloatProperty("x", wxPG_LABEL, m_Camera.Pos.x)); SetPropertyTextColour(CameraPosX, wxColour(200, 0, 0)); // With wx2.9, change this into: CameraPosX->SetTextColour(wxColour(255, 0, 0));
    wxPGProperty* CameraPosY=AppendIn(CameraPos, new wxFloatProperty("y", wxPG_LABEL, m_Camera.Pos.y)); SetPropertyTextColour(CameraPosY, wxColour(0, 200, 0));
    wxPGProperty* CameraPosZ=AppendIn(CameraPos, new wxFloatProperty("z", wxPG_LABEL, m_Camera.Pos.z)); SetPropertyTextColour(CameraPosZ, wxColour(0, 0, 200));

    wxPGProperty* CameraAngles=AppendIn(CameraCat, new wxStringProperty("Angles", "Camera.Angles", "<composed>"));
    AppendIn(CameraAngles, new wxFloatProperty("Pitch", wxPG_LABEL, m_Camera.Angles.pitch()));
 // AppendIn(CameraAngles, new wxFloatProperty("Roll", wxPG_LABEL, m_Camera.Angles.roll()));
    AppendIn(CameraAngles, new wxFloatProperty("Yaw", wxPG_LABEL, m_Camera.Angles.yaw()));

    wxPGProperty* CameraAdvanced=AppendIn(CameraCat, new wxStringProperty("Advanced", "Camera.Advanced", "<composed>"));
    AppendIn(CameraAdvanced, new wxFloatProperty("vertical FOV", wxPG_LABEL, m_Camera.VerticalFOV));
    AppendIn(CameraAdvanced, new wxFloatProperty("near plane dist", wxPG_LABEL, m_Camera.NearPlaneDist));
    AppendIn(CameraAdvanced, new wxFloatProperty("far plane dist", wxPG_LABEL, m_Camera.FarPlaneDist));
    Collapse(CameraAdvanced);


    // "Scene Elements" category.
    wxPGProperty* SceneElemsCat=Append(new wxPropertyCategory("Scene Elements"));

    wxPGProperty* GroundPlane=AppendIn(SceneElemsCat, new wxStringProperty("Ground Plane", wxPG_LABEL, "<composed>"));
    AppendIn(GroundPlane, new wxBoolProperty("Show", wxPG_LABEL, m_GroundPlane_Show));
    AppendIn(GroundPlane, new wxFloatProperty("Height (z-Pos)", wxPG_LABEL, m_GroundPlane_zPos));
    AppendIn(GroundPlane, new wxStringProperty("Material", wxPG_LABEL, m_GroundPlane_Mat->GetName()));      // TODO: MaterialProperty


    // "Light Sources" category.
    wxPGProperty* LightsCat=Append(new wxPropertyCategory("Light Sources"));
    AppendIn(LightsCat, new wxColourProperty("Ambient Light Color", wxPG_LABEL, m_AmbientLightColor));


    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true); // Use checkboxes instead of choice.

    RefreshGrid();
}


void ModelEditor::SceneSetupT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
 // ClearSelection();

    const wxPGProperty* Prop       =Event.GetProperty();
    const wxString      PropName   =Prop->GetName();
    double              PropValueD =0.0;
    const float         PropValueF =Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;

         if (PropName=="Background Color")                m_BackgroundColor=wxColour("rgb"+Prop->GetValueAsString());
    else if (PropName=="Show Origin")                     m_ShowOrigin=Prop->GetValue().GetBool();
    else if (PropName=="Camera.Pos.x")                    m_Camera.Pos.x=PropValueF;
    else if (PropName=="Camera.Pos.y")                    m_Camera.Pos.y=PropValueF;
    else if (PropName=="Camera.Pos.z")                    m_Camera.Pos.z=PropValueF;
    else if (PropName=="Camera.Angles.Pitch")             m_Camera.Angles.pitch()=PropValueF;
 // else if (PropName=="Camera.Angles.Roll")              m_Camera.Angles.roll()=PropValueF;
    else if (PropName=="Camera.Angles.Yaw")               m_Camera.Angles.yaw()=PropValueF;
    else if (PropName=="Camera.Advanced.vertical FOV")    m_Camera.VerticalFOV=PropValueF;
    else if (PropName=="Camera.Advanced.near plane dist") m_Camera.NearPlaneDist=PropValueF;
    else if (PropName=="Camera.Advanced.far plane dist")  m_Camera.FarPlaneDist=PropValueF;
    else if (PropName=="Ground Plane.Show")               m_GroundPlane_Show=Prop->GetValue().GetBool();
    else if (PropName=="Ground Plane.Height (z-Pos)")     m_GroundPlane_zPos=PropValueF;
    else if (PropName=="Ground Plane.Material")
    {
        EditorMaterialI* NewMat=m_GameConfig->GetMatMan().FindMaterial(Prop->GetValueAsString(), false /*CreateDummy*/);

        if (NewMat) m_GroundPlane_Mat=NewMat;
    }
    else if (PropName=="Ambient Light Color")             m_AmbientLightColor=wxColour("rgb"+Prop->GetValueAsString());
    else
    {
        // Don't uncomment this:
        // Changing child properties (e.g. "Pos.x" to "5") also generates events for the composite parent (e.g. "Pos" to "(5, 0, 0)")!
        // wxMessageBox("Unknown property label \""+Name+"\".", "Warning", wxICON_ERROR);
    }
}
