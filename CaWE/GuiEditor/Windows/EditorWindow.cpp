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

#include "EditorWindow.hpp"
#include "../ChildFrame.hpp"
#include "../GuiDocument.hpp"
#include "../Commands/ModifyWindow.hpp"
#include "../../EditorMaterial.hpp"
#include "../../MaterialBrowser/DocAccess.hpp"
#include "../../MaterialBrowser/MaterialBrowserDialog.hpp"

#include "Fonts/FontTT.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/WindowCreateParams.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

#include "wx/propgrid/property.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"


using namespace GuiEditor;


EditorWindowT::EditorWindowT(cf::GuiSys::WindowT* Win, GuiDocumentT* GuiDoc)
    : m_Win(Win),
      m_GuiDoc(GuiDoc),
      m_IsSelected(false)
{
    if (m_Win->Name=="")
    {
        m_Win->Name=m_Win->GetType()->ClassName;

        const size_t len=m_Win->Name.length();

        if (len>1 && m_Win->Name[len-1]=='T')
        {
            // Remove the trailing "T" from our class name.
            m_Win->Name=std::string(m_Win->Name, 0, len-1);
        }
    }

    m_Win->Name=m_GuiDoc->CheckWindowName(m_Win->Name, this);
}


namespace
{
    const float SelectionColor[]={ 1.0, 0.0, 0.0, 1.0 };
    const float SelectionBorder =2;


    unsigned char* cast(float* f)
    {
        static unsigned char c[4];

        for (int i=0; i<4; i++)
        {
            float g=f[i];

            if (g<0.0f) g=0.0f;
            if (g>1.0f) g=1.0f;

            c[i]=(unsigned char)(g*255.0f + 0.5f);
        }

        return c;
    }


    /// Custom property to select materials for window backgrounds.
    class MaterialPropertyT : public wxLongStringProperty
    {
        public:

        MaterialPropertyT(const wxString& name,
                          const wxString& label,
                          const wxString& value,
                          GuiDocumentT* GuiDoc)
            : wxLongStringProperty(name, label, value),
              m_GuiDocument(GuiDoc)
        {
        }

        // Shows the file selection dialog and makes the choosen file path relative.
        virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
        {
            EditorMaterialI*                 InitMat=NULL;
            const ArrayT<EditorMaterialI*>&  EditorMaterials=m_GuiDocument->GetEditorMaterials();

            for (unsigned long EMNr=0; EMNr<EditorMaterials.Size(); EMNr++)
                if (EditorMaterials[EMNr]->GetName()==value)
                {
                    InitMat=EditorMaterials[EMNr];
                    break;
                }

            MaterialBrowser::DialogT MatBrowser(GetGrid(), MaterialBrowser::GuiDocAccessT(*m_GuiDocument), MaterialBrowser::ConfigT()
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

        GuiDocumentT* m_GuiDocument;
    };
}


void EditorWindowT::FillInPG(wxPropertyGridManager* PropMan)
{
    PropMan->Append(new wxStringProperty("Name", wxPG_LABEL, m_Win->Name));
    wxPGProperty* TypeInfo=PropMan->Append(new wxStringProperty("Type", wxPG_LABEL, m_Win->GetType()->ClassName));
    PropMan->DisableProperty(TypeInfo);

    wxPGProperty* Item=PropMan->Append(new wxBoolProperty("Visible", wxPG_LABEL, m_Win->ShowWindow));

    PropMan->SetPropertyAttribute(Item, wxPG_BOOL_USE_CHECKBOX, (long)1, wxPG_RECURSE); // Use checkboxes instead of choice.

    wxPGProperty* Position=PropMan->Append(new wxStringProperty("Position", wxPG_LABEL, "<composed>"));

    PropMan->AppendIn(Position, new wxFloatProperty("X", wxPG_LABEL, m_Win->Rect[0]));
    PropMan->AppendIn(Position, new wxFloatProperty("Y", wxPG_LABEL, m_Win->Rect[1]));

    wxPGProperty* Size=PropMan->Append(new wxStringProperty("Size", wxPG_LABEL, "<composed>"));

    PropMan->AppendIn(Size, new wxFloatProperty("Width", wxPG_LABEL, m_Win->Rect[2]));
    PropMan->AppendIn(Size, new wxFloatProperty("Height", wxPG_LABEL, m_Win->Rect[3]));

    PropMan->Append(new wxFloatProperty("Rotation", wxPG_LABEL, m_Win->RotAngle));

    PropMan->Append(new MaterialPropertyT("BackMatName", wxPG_LABEL, m_Win->BackRenderMatName, m_GuiDoc));

    unsigned char* Col=cast(m_Win->BackColor);
    PropMan->Append(new wxColourProperty("BackgroundColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("BackgroundAlpha", wxPG_LABEL, Col[3]));

    PropMan->Append(new wxFloatProperty("BorderWidth", wxPG_LABEL, m_Win->BorderWidth));

    Col=cast(m_Win->BorderColor);
    PropMan->Append(new wxColourProperty("BorderColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("BorderColorAlpha", wxPG_LABEL, Col[3]));

    PropMan->Append(new wxStringProperty("FontName", wxPG_LABEL, m_Win->Font->GetName()));

    PropMan->Append(new wxLongStringProperty("Text", wxPG_LABEL, m_Win->Text));

    PropMan->Append(new wxFloatProperty("TextScale", wxPG_LABEL, m_Win->TextScale));

    Col=cast(m_Win->TextColor);
    PropMan->Append(new wxColourProperty("TextColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("TextColorAlpha", wxPG_LABEL, Col[3]));

    wxPGChoices AlignHorChoices;
    wxPGChoices AlignVerChoices;

    AlignHorChoices.Add("Left");
    AlignHorChoices.Add("Right");
    AlignHorChoices.Add("Center");

    AlignVerChoices.Add("Top");
    AlignVerChoices.Add("Bottom");
    AlignVerChoices.Add("Middle");

    PropMan->Append(new wxEnumProperty("HorizontalAlign", wxPG_LABEL, AlignHorChoices, (int)m_Win->TextAlignHor));
    PropMan->Append(new wxEnumProperty("VerticalAlign",   wxPG_LABEL, AlignVerChoices, (int)m_Win->TextAlignVer));
}


bool EditorWindowT::UpdateProperty(wxPGProperty* Property)
{
    wxString PropName=Property->GetName();

         if (PropName=="Name")             Property->SetValueFromString(m_Win->Name);
    else if (PropName=="Visible")          Property->SetValueFromString(m_Win->ShowWindow ? "true" : "false");
    else if (PropName=="Position.X")       Property->SetValue(m_Win->Rect[0]);
    else if (PropName=="Position.Y")       Property->SetValue(m_Win->Rect[1]);
    else if (PropName=="Size.Width")       Property->SetValue(m_Win->Rect[2]);
    else if (PropName=="Size.Height")      Property->SetValue(m_Win->Rect[3]);
    else if (PropName=="Rotation")         Property->SetValue(m_Win->RotAngle);
    else if (PropName=="BackMatName")      Property->SetValueFromString(m_Win->BackRenderMatName);
    else if (PropName=="BackgroundColor")  { unsigned char* Col=cast(m_Win->BackColor); Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="BackgroundAlpha")  { unsigned char* Col=cast(m_Win->BackColor); Property->SetValue(Col[3]); }
    else if (PropName=="BorderWidth")      Property->SetValue(m_Win->BorderWidth);
    else if (PropName=="BorderColor")      { unsigned char* Col=cast(m_Win->BorderColor); Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="BorderColorAlpha") { unsigned char* Col=cast(m_Win->BorderColor); Property->SetValue(Col[3]); }
    else if (PropName=="FontName")         Property->SetValueFromString(m_Win->Font->GetName());
    else if (PropName=="Text")             Property->SetValueFromString(m_Win->Text);
    else if (PropName=="TextScale")        Property->SetValue(m_Win->TextScale);
    else if (PropName=="TextColor")        { unsigned char* Col=cast(m_Win->TextColor); Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="TextColorAlpha")   { unsigned char* Col=cast(m_Win->TextColor); Property->SetValue(Col[3]); }
    else if (PropName=="HorizontalAlign")  Property->SetValue((int)m_Win->TextAlignHor);
    else if (PropName=="VerticalAlign")    Property->SetValue((int)m_Win->TextAlignVer);
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
    else if (PropName=="BackMatName")
    {
        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, DummyVar, Prop->GetValueAsString()));
    }
    else if (PropName=="BackgroundColor")
    {
        wxASSERT(m_Win->GetMemberVar("backColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_Win->BackColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("backColor"), NewValue));
    }
    else if (PropName=="BackgroundAlpha")
    {
        wxASSERT(m_Win->GetMemberVar("backColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("backColor.a"), &NewValue));
    }
    else if (PropName=="BorderWidth")
    {
        wxASSERT(m_Win->GetMemberVar("borderWidth").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("borderWidth"), &PropValueF));
    }
    else if (PropName=="BorderColor")
    {
        wxASSERT(m_Win->GetMemberVar("borderColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_Win->BorderColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("borderColor"), NewValue));
    }
    else if (PropName=="BorderColorAlpha")
    {
        wxASSERT(m_Win->GetMemberVar("borderColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("borderColor.a"), &NewValue));
    }
    else if (PropName=="FontName")
    {
        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, DummyVar, Prop->GetValueAsString()));
    }
    else if (PropName=="Text")
    {
        wxASSERT(m_Win->GetMemberVar("text").Member!=NULL);

        wxString FormattedText=Prop->GetValueAsString();
        FormattedText.Replace("\\n", "\n");

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("text"), FormattedText));
    }
    else if (PropName=="TextScale")
    {
        wxASSERT(m_Win->GetMemberVar("textScale").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("textScale"), &PropValueF));
    }
    else if (PropName=="TextColor")
    {
        wxASSERT(m_Win->GetMemberVar("textColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_Win->TextColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("textColor"), NewValue));
    }
    else if (PropName=="TextColorAlpha")
    {
        wxASSERT(m_Win->GetMemberVar("textColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("textColor.a"), &NewValue));
    }
    else if (PropName=="HorizontalAlign")
    {
        wxASSERT(m_Win->GetMemberVar("textAlignHor").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("textAlignHor"), Prop->GetValue().GetLong()));
    }
    else if (PropName=="VerticalAlign")
    {
        wxASSERT(m_Win->GetMemberVar("textAlignVer").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Win, PropName, m_Win->GetMemberVar("textAlignVer"), Prop->GetValue().GetLong()));
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

    if (m_Win->ShowWindow!=Default.ShowWindow)
        OutFile << "    self:set(\"show\", " << (m_Win->ShowWindow ? "true" : "false") << ");\n";

    if (m_Win->Rect[0]!=Default.Rect[0] || m_Win->Rect[1]!=Default.Rect[1] || m_Win->Rect[2]!=Default.Rect[2] || m_Win->Rect[3]!=Default.Rect[3])
        OutFile << "    self:set(\"rect\", " << m_Win->Rect[0] << ", " << m_Win->Rect[1] << ", " << m_Win->Rect[2] << ", " << m_Win->Rect[3] << ");\n";

    if (m_Win->RotAngle!=Default.RotAngle)
        OutFile << "    self:set(\"rotAngle\", " << m_Win->RotAngle << ");\n";

    if (m_Win->BackRenderMatName!=Default.BackRenderMatName)
        OutFile << "    self:set(\"backMaterial\", \"" << m_Win->BackRenderMatName << "\");\n";

    if (m_Win->BackColor[0]!=Default.BackColor[0] || m_Win->BackColor[1]!=Default.BackColor[1] || m_Win->BackColor[2]!=Default.BackColor[2] || m_Win->BackColor[3]!=Default.BackColor[3])
        OutFile << "    self:set(\"backColor\", " << m_Win->BackColor[0] << ", " << m_Win->BackColor[1] << ", " << m_Win->BackColor[2] << ", " << m_Win->BackColor[3] << ");\n";

    if (m_Win->BorderWidth!=Default.BorderWidth)
        OutFile << "    self:set(\"borderWidth\", " << m_Win->BorderWidth << ");\n";

    if (m_Win->BorderColor[0]!=Default.BorderColor[0] || m_Win->BorderColor[1]!=Default.BorderColor[1] || m_Win->BorderColor[2]!=Default.BorderColor[2] || m_Win->BorderColor[3]!=Default.BorderColor[3])
        OutFile << "    self:set(\"borderColor\", " << m_Win->BorderColor[0] << ", " << m_Win->BorderColor[1] << ", " << m_Win->BorderColor[2] << ", " << m_Win->BorderColor[3] << ");\n";

    if (m_Win->Font->GetName()!=Default.Font->GetName())
        OutFile << "    self:set(\"font\", \"" << m_Win->Font->GetName() << "\");\n";

    if (m_Win->Text!=Default.Text)
    {
        // Replace line breaks with \n and " with \".
        wxString FormattedText=m_Win->Text;
        FormattedText.Replace("\n", "\\n");
        FormattedText.Replace("\"", "\\\"");

        OutFile << "    self:set(\"text\", \"" << FormattedText << "\");\n";
    }

    if (m_Win->TextScale!=Default.TextScale)
        OutFile << "    self:set(\"textScale\", " << m_Win->TextScale << ");\n";

    if (m_Win->TextColor[0]!=Default.TextColor[0] || m_Win->TextColor[1]!=Default.TextColor[1] || m_Win->TextColor[2]!=Default.TextColor[2] || m_Win->TextColor[3]!=Default.TextColor[3])
        OutFile << "    self:set(\"textColor\", " << m_Win->TextColor[0] << ", " << m_Win->TextColor[1] << ", " << m_Win->TextColor[2] << ", " << m_Win->TextColor[3] << ");\n";

    if (m_Win->TextAlignHor!=Default.TextAlignHor)
        OutFile << "    self:set(\"textAlignHor\", " << m_Win->TextAlignHor << ");\n";

    if (m_Win->TextAlignVer!=Default.TextAlignVer)
        OutFile << "    self:set(\"textAlignVer\", " << m_Win->TextAlignVer << ");\n";

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

        const float x2=x1+m_Win->Rect[2];
        const float y2=y1+m_Win->Rect[3];

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
