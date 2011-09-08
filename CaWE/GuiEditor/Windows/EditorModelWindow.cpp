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

#include "EditorModelWindow.hpp"
#include "../ChildFrame.hpp"
#include "../GuiDocument.hpp"
#include "../Commands/ModifyWindow.hpp"

#include "GuiSys/WindowModel.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "wx/propgrid/manager.h"


using namespace GuiEditor;


EditorModelWindowT::EditorModelWindowT(cf::GuiSys::ModelWindowT* ModelWindow, GuiDocumentT* GuiDoc)
    : EditorWindowT(ModelWindow, GuiDoc),
      m_ModelWindow(ModelWindow)
{
}


void EditorModelWindowT::FillInPG(wxPropertyGridManager* PropMan)
{
    EditorWindowT::FillInPG(PropMan);

    PropMan->Append(new wxStringProperty("Model", wxPG_LABEL, m_ModelWindow->GetModel().GetFileName()));

    PropMan->Append(new wxIntProperty("ModelSequNr", wxPG_LABEL, m_ModelWindow->GetModelSequNr()));

    wxPGProperty* Vector=PropMan->Append(new wxStringProperty("ModelPos", wxPG_LABEL, "<composed>"));
    PropMan->AppendIn(Vector, new wxFloatProperty("x", wxPG_LABEL, m_ModelWindow->GetModelPos().x));
    PropMan->AppendIn(Vector, new wxFloatProperty("y", wxPG_LABEL, m_ModelWindow->GetModelPos().y));
    PropMan->AppendIn(Vector, new wxFloatProperty("z", wxPG_LABEL, m_ModelWindow->GetModelPos().z));

    PropMan->Append(new wxFloatProperty("ModelScale", wxPG_LABEL, m_ModelWindow->GetModelScale()));

    Vector=PropMan->Append(new wxStringProperty("ModelAngles", wxPG_LABEL, "<composed>"));
    PropMan->AppendIn(Vector, new wxFloatProperty("x", wxPG_LABEL, m_ModelWindow->GetModelAngles().x));
    PropMan->AppendIn(Vector, new wxFloatProperty("y", wxPG_LABEL, m_ModelWindow->GetModelAngles().y));
    PropMan->AppendIn(Vector, new wxFloatProperty("z", wxPG_LABEL, m_ModelWindow->GetModelAngles().z));

    Vector=PropMan->Append(new wxStringProperty("CameraPos", wxPG_LABEL, "<composed>"));
    PropMan->AppendIn(Vector, new wxFloatProperty("x", wxPG_LABEL, m_ModelWindow->GetCameraPos().x));
    PropMan->AppendIn(Vector, new wxFloatProperty("y", wxPG_LABEL, m_ModelWindow->GetCameraPos().y));
    PropMan->AppendIn(Vector, new wxFloatProperty("z", wxPG_LABEL, m_ModelWindow->GetCameraPos().z));
}


bool EditorModelWindowT::UpdateProperty(wxPGProperty* Property)
{
    if (EditorWindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

         if (PropName=="Model")         Property->SetValueFromString(m_ModelWindow->GetModel().GetFileName());
    else if (PropName=="ModelSequNr")   Property->SetValue(m_ModelWindow->GetModelSequNr());
    else if (PropName=="ModelPos.x")    Property->SetValue(m_ModelWindow->GetModelPos().x);
    else if (PropName=="ModelPos.y")    Property->SetValue(m_ModelWindow->GetModelPos().y);
    else if (PropName=="ModelPos.z")    Property->SetValue(m_ModelWindow->GetModelPos().z);
    else if (PropName=="ModelScale")    Property->SetValue(m_ModelWindow->GetModelScale());
    else if (PropName=="ModelAngles.x") Property->SetValue(m_ModelWindow->GetModelAngles().x);
    else if (PropName=="ModelAngles.y") Property->SetValue(m_ModelWindow->GetModelAngles().y);
    else if (PropName=="ModelAngles.z") Property->SetValue(m_ModelWindow->GetModelAngles().z);
    else if (PropName=="CameraPos.x")   Property->SetValue(m_ModelWindow->GetCameraPos().x);
    else if (PropName=="CameraPos.y")   Property->SetValue(m_ModelWindow->GetCameraPos().y);
    else if (PropName=="CameraPos.z")   Property->SetValue(m_ModelWindow->GetCameraPos().z);
    else                                return false;

    return true;
}


bool EditorModelWindowT::HandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (EditorWindowT::HandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty* Prop      =Event.GetProperty();
    const wxString      PropName  =Prop->GetName();
    double              PropValueD=0.0;
    const float         PropValueF=Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;

    if (PropName=="Model")
    {
        static cf::GuiSys::WindowT::MemberVarT DummyVar;

        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, DummyVar, Prop->GetValueAsString()));
    }
    else if (PropName=="ModelSequNr")
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelSequNr").Member!=NULL);

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelSequNr"), Prop->GetValue().GetLong()));
    }
    else if (PropName==("ModelPos.x"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelPos.x").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelPos.x"), &PropValueF));
    }
    else if (PropName==("ModelPos.y"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelPos.y").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelPos.y"), &PropValueF));
    }
    else if (PropName==("ModelPos.z"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelPos.z").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelPos.z"), &PropValueF));
    }
    else if (PropName=="ModelScale")
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelScale").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelScale"), &PropValueF));
    }
    else if (PropName==("ModelAngles.x"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelAngles.x").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelAngles.x"), &PropValueF));
    }
    else if (PropName==("ModelAngles.y"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelAngles.y").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelAngles.y"), &PropValueF));
    }
    else if (PropName==("ModelAngles.z"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("modelAngles.z").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("modelAngles.z"), &PropValueF));
    }
    else if (PropName==("CameraPos.x"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("cameraPos.x").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("cameraPos.x"), &PropValueF));
    }
    else if (PropName==("CameraPos.y"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("cameraPos.y").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("cameraPos.y"), &PropValueF));
    }
    else if (PropName==("CameraPos.z"))
    {
        wxASSERT(m_ModelWindow->GetMemberVar("cameraPos.z").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ModelWindow, PropName, m_ModelWindow->GetMemberVar("cameraPos.z"), &PropValueF));
    }
    else
    {
        return false;
    }

    return true;
}


bool EditorModelWindowT::WriteInitMethod(std::ostream& OutFile)
{
    EditorWindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    cf::GuiSys::WindowCreateParamsT Params(*m_GuiDoc->GetGui());
    cf::GuiSys::ModelWindowT        Default(Params);

    if (m_ModelWindow->GetModel().GetFileName()!=Default.GetModel().GetFileName())
        OutFile << "    self:SetModel(\"" << m_ModelWindow->GetModel().GetFileName() << "\");\n";

    if (m_ModelWindow->GetModelSequNr()!=Default.GetModelSequNr())
        OutFile << "    self:SetModelSequNr(" << m_ModelWindow->GetModelSequNr() << ");\n";

    if (m_ModelWindow->GetModelPos()!=Default.GetModelPos())
        OutFile << "    self:SetModelPos(" << m_ModelWindow->GetModelPos().x << ", " << m_ModelWindow->GetModelPos().y << ", " << m_ModelWindow->GetModelPos().z << ");\n";

    if (m_ModelWindow->GetModelScale()!=Default.GetModelScale())
        OutFile << "    self:SetModelScale(" << m_ModelWindow->GetModelScale() << ");\n";

    if (m_ModelWindow->GetModelAngles()!=Default.GetModelAngles())
        OutFile << "    self:SetModelAngles(" << m_ModelWindow->GetModelAngles().x << ", " << m_ModelWindow->GetModelAngles().y << ", " << m_ModelWindow->GetModelAngles().z << ");\n";

    if (m_ModelWindow->GetCameraPos()!=Default.GetCameraPos())
        OutFile << "    self:SetCameraPos(" << m_ModelWindow->GetCameraPos().x << ", " << m_ModelWindow->GetCameraPos().y << ", " << m_ModelWindow->GetCameraPos().z << ");\n";

    return true;
}
