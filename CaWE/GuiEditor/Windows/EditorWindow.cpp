/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "EditorWindow.hpp"
#include "../ChildFrame.hpp"
#include "../GuiDocument.hpp"
#include "../Commands/ModifyWindow.hpp"
#include "../Commands/SetWinProp.hpp"
#include "../../EditorMaterial.hpp"
#include "../../MaterialBrowser/DocAccess.hpp"
#include "../../MaterialBrowser/MaterialBrowserDialog.hpp"

#include "Fonts/FontTT.hpp"
#include "GuiSys/CompBase.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/GuiResources.hpp"
#include "GuiSys/WindowCreateParams.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

#include "wx/propgrid/property.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"


using namespace GuiEditor;


EditorWindowT::EditorWindowT(IntrusivePtrT<cf::GuiSys::WindowT> Win, GuiDocumentT* GuiDoc)
    : m_Win(Win),
      m_GuiDoc(GuiDoc),
      m_IsSelected(false)
{
}


namespace
{
    const float SelectionColor[]={ 1.0, 0.0, 0.0, 1.0 };
    const float SelectionBorder =2;
}


void EditorWindowT::FillInPG(wxPropertyGridManager* PropMan)
{
    PropMan->Append(new wxStringProperty("Name", wxPG_LABEL, m_Win->GetName()));
    wxPGProperty* TypeInfo=PropMan->Append(new wxStringProperty("Type", wxPG_LABEL, m_Win->GetType()->ClassName));
    PropMan->DisableProperty(TypeInfo);

    wxPGProperty* Item=PropMan->Append(new wxBoolProperty("Visible", wxPG_LABEL, m_Win->IsShown()));

    PropMan->SetPropertyAttribute(Item, wxPG_BOOL_USE_CHECKBOX, (long)1, wxPG_RECURSE); // Use checkboxes instead of choice.

    wxPGProperty* Position=PropMan->Append(new wxStringProperty("Position", wxPG_LABEL, "<composed>"));

    PropMan->AppendIn(Position, new wxFloatProperty("X", wxPG_LABEL, m_Win->GetPos().x));
    PropMan->AppendIn(Position, new wxFloatProperty("Y", wxPG_LABEL, m_Win->GetPos().y));

    wxPGProperty* Size=PropMan->Append(new wxStringProperty("Size", wxPG_LABEL, "<composed>"));

    PropMan->AppendIn(Size, new wxFloatProperty("Width", wxPG_LABEL, m_Win->GetSize().x));
    PropMan->AppendIn(Size, new wxFloatProperty("Height", wxPG_LABEL, m_Win->GetSize().y));

    PropMan->Append(new wxFloatProperty("Rotation", wxPG_LABEL, m_Win->GetRotAngle()));
}


bool EditorWindowT::UpdateProperty(wxPGProperty* Property)
{
    wxString PropName=Property->GetName();

         if (PropName=="Name")             Property->SetValueFromString(m_Win->GetName());
    else if (PropName=="Visible")          Property->SetValueFromString(m_Win->IsShown() ? "true" : "false");
    else if (PropName=="Position.X")       Property->SetValue(m_Win->GetPos().x);
    else if (PropName=="Position.Y")       Property->SetValue(m_Win->GetPos().y);
    else if (PropName=="Size.Width")       Property->SetValue(m_Win->GetSize().x);
    else if (PropName=="Size.Height")      Property->SetValue(m_Win->GetSize().y);
    else if (PropName=="Rotation")         Property->SetValue(m_Win->GetRotAngle());
    else                                   return false;

    return true;
}


bool EditorWindowT::HandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    static cf::GuiSys::WindowT::MemberVarT DummyVar;

    const wxPGProperty* Prop      =Event.GetProperty();
    const wxString      PropName  =Prop->GetName();
    double              PropValueD=0.0;
    const float         PropValueF=Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;

    if (PropName=="Name")
    {
        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, DummyVar, Prop->GetValueAsString()));

        // The command may well have set a name different from Prop->GetValueAsString().
        wxASSERT(Event.GetEventType() == wxEVT_PG_CHANGED);
        Event.GetProperty()->SetValueFromString(m_Win->GetName());
    }
    else if (PropName=="Visible")
    {
        wxASSERT(m_Win->GetMemberVar("show").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("show"), Prop->GetValue().GetBool()));
    }
    else if (PropName=="Position.X")
    {
        wxASSERT(m_Win->GetMemberVar("pos.x").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("pos.x"), &PropValueF));
    }
    else if (PropName=="Position.Y")
    {
        wxASSERT(m_Win->GetMemberVar("pos.y").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("pos.y"), &PropValueF));
    }
    else if (PropName=="Size.Width")
    {
        wxASSERT(m_Win->GetMemberVar("size.x").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("size.x"), &PropValueF));
    }
    else if (PropName=="Size.Height")
    {
        wxASSERT(m_Win->GetMemberVar("size.y").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("size.y"), &PropValueF));
    }
    else if (PropName=="Rotation")
    {
        wxASSERT(m_Win->GetMemberVar("rotAngle").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("rotAngle"), &PropValueF));
    }
    else
    {
        return false;
    }

    return true;
}


bool EditorWindowT::WriteInitMethod(std::ostream& OutFile)
{
    // Create empty window to get default values.
    cf::GuiSys::WindowCreateParamsT Params(*m_GuiDoc->GetGui());
    cf::GuiSys::WindowT             Default(Params);

    if (m_Win->IsShown()!=Default.IsShown())
        OutFile << "    self:set(\"show\", " << (m_Win->IsShown() ? "true" : "false") << ");\n";

    if (m_Win->GetPos().x!=Default.GetPos().x || m_Win->GetPos().y!=Default.GetPos().y || m_Win->GetSize().x!=Default.GetSize().x || m_Win->GetSize().y!=Default.GetSize().y)
        OutFile << "    self:set(\"rect\", " << m_Win->GetPos().x << ", " << m_Win->GetPos().y << ", " << m_Win->GetSize().x << ", " << m_Win->GetSize().y << ");\n";

    if (m_Win->GetRotAngle()!=Default.GetRotAngle())
        OutFile << "    self:set(\"rotAngle\", " << m_Win->GetRotAngle() << ");\n";

    return true;
}


void EditorWindowT::Render() const
{
    // Render selection state of this window.
    if (m_IsSelected)
    {
        // Render selection border.
        float x1;
        float y1;

        m_Win->GetAbsolutePos(x1, y1);

        const float x2=x1+m_Win->GetSize().x;
        const float y2=y1+m_Win->GetSize().y;

        MatSys::Renderer->SetCurrentMaterial(m_Win->GetGui().GetDefaultRM());

        static MatSys::MeshT BorderMesh(MatSys::MeshT::Quads);
        BorderMesh.Vertices.Overwrite();
        BorderMesh.Vertices.PushBackEmpty(4*4);     // One rectangle for each side of the background.

        for (unsigned long VertexNr=0; VertexNr<BorderMesh.Vertices.Size(); VertexNr++)
        {
            for (unsigned long i=0; i<4; i++)
                BorderMesh.Vertices[VertexNr].Color[i]=SelectionColor[i];
        }

        // Left border rectangle.
        BorderMesh.Vertices[ 0].SetOrigin(x1,                 y1); BorderMesh.Vertices[ 0].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 1].SetOrigin(x1+SelectionBorder, y1); BorderMesh.Vertices[ 1].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 2].SetOrigin(x1+SelectionBorder, y2); BorderMesh.Vertices[ 2].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 3].SetOrigin(x1,                 y2); BorderMesh.Vertices[ 3].SetTextureCoord(0.0f, 1.0f);

        // Top border rectangle.
        BorderMesh.Vertices[ 4].SetOrigin(x1+SelectionBorder, y1                ); BorderMesh.Vertices[ 4].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 5].SetOrigin(x2-SelectionBorder, y1                ); BorderMesh.Vertices[ 5].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 6].SetOrigin(x2-SelectionBorder, y1+SelectionBorder); BorderMesh.Vertices[ 6].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 7].SetOrigin(x1+SelectionBorder, y1+SelectionBorder); BorderMesh.Vertices[ 7].SetTextureCoord(0.0f, 1.0f);

        // Right border rectangle.
        BorderMesh.Vertices[ 8].SetOrigin(x2-SelectionBorder, y1); BorderMesh.Vertices[ 8].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 9].SetOrigin(x2,                 y1); BorderMesh.Vertices[ 9].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[10].SetOrigin(x2,                 y2); BorderMesh.Vertices[10].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[11].SetOrigin(x2-SelectionBorder, y2); BorderMesh.Vertices[11].SetTextureCoord(0.0f, 1.0f);

        // Bottom border rectangle.
        BorderMesh.Vertices[12].SetOrigin(x1+SelectionBorder, y2-SelectionBorder); BorderMesh.Vertices[12].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[13].SetOrigin(x2-SelectionBorder, y2-SelectionBorder); BorderMesh.Vertices[13].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[14].SetOrigin(x2-SelectionBorder, y2                ); BorderMesh.Vertices[14].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[15].SetOrigin(x1+SelectionBorder, y2                ); BorderMesh.Vertices[15].SetTextureCoord(0.0f, 1.0f);

        MatSys::Renderer->RenderMesh(BorderMesh);
    }
}
