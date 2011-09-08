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

#include "EditorEditWindow.hpp"
#include "../ChildFrame.hpp"
#include "../GuiDocument.hpp"
#include "../Commands/ModifyWindow.hpp"

#include "GuiSys/WindowEdit.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "wx/propgrid/property.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"


using namespace GuiEditor;


namespace
{
    unsigned char* cast(const float* f)
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
}


EditorEditWindowT::EditorEditWindowT(cf::GuiSys::EditWindowT* EditWindow, GuiDocumentT* GuiDoc)
    : EditorWindowT(EditWindow, GuiDoc),
      m_EditWindow(EditWindow)
{
}


void EditorEditWindowT::FillInPG(wxPropertyGridManager* PropMan)
{
    EditorWindowT::FillInPG(PropMan);

    wxPGChoices CursorTypes;
    CursorTypes.Add("|");
    CursorTypes.Add("_");

    PropMan->Append(new wxEnumProperty("TextCursorType", wxPG_LABEL, CursorTypes, (int)m_EditWindow->GetTextCursorType()));

    PropMan->Append(new wxFloatProperty("TextCursorRate", wxPG_LABEL, m_EditWindow->GetTextCursorRate()));

    unsigned char* Col=cast(m_EditWindow->GetTextCursorColor());
    PropMan->Append(new wxColourProperty("TextCursorColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("TextCursorColorAlpha", wxPG_LABEL, Col[3]));
}


bool EditorEditWindowT::UpdateProperty(wxPGProperty* Property)
{
    if (EditorWindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

    if (PropName=="TextCursorType")
    {
        int Selection=0;
        if (m_EditWindow->GetTextCursorType()=='_') Selection=1;

        Property->SetValue(Selection);
    }
    else if (PropName=="TextCursorRate")       { Property->SetValue(m_EditWindow->GetTextCursorRate()); }
    else if (PropName=="TextCursorColor")      { unsigned char* Col=cast(m_EditWindow->GetTextCursorColor()); Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="TextCursorColorAlhpa") { unsigned char* Col=cast(m_EditWindow->GetTextCursorColor()); Property->SetValue(Col[3]); }
    else                                       { return false; }

    return true;
}


bool EditorEditWindowT::HandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (EditorWindowT::HandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty* Prop      =Event.GetProperty();
    const wxString      PropName  =Prop->GetName();
    double              PropValueD=0.0;
    const float         PropValueF=Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;

    if (PropName=="TextCursorType")
    {
        wxASSERT(m_EditWindow->GetMemberVar("textCursorType").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_EditWindow, PropName, m_EditWindow->GetMemberVar("textCursorType"), Prop->GetValue().GetLong()));
    }
    else if (PropName=="TextCursorRate")
    {
        wxASSERT(m_EditWindow->GetMemberVar("textCursorRate").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_EditWindow, PropName, m_EditWindow->GetMemberVar("textCursorRate"), &PropValueF));
    }
    else if (PropName=="TextCursorColor")
    {
        wxASSERT(m_EditWindow->GetMemberVar("textCursorColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_EditWindow->GetTextCursorColor()[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_EditWindow, PropName, m_EditWindow->GetMemberVar("textCursorColor"), NewValue));
    }
    else if (PropName=="TextCursorColorAlpha")
    {
        wxASSERT(m_EditWindow->GetMemberVar("textCursorColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_EditWindow, PropName, m_EditWindow->GetMemberVar("textCursorColor.a"), &NewValue));
    }
    else
    {
        return false;
    }

    return true;
}


bool EditorEditWindowT::WriteInitMethod(std::ostream& OutFile)
{
    EditorWindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    cf::GuiSys::WindowCreateParamsT Params(*m_GuiDoc->GetGui());
    cf::GuiSys::EditWindowT         Default(Params);

    if (m_EditWindow->GetTextCursorType()!=Default.GetTextCursorType())
        OutFile << "    self:SetTextCursorType(\"" << m_EditWindow->GetTextCursorType() << "\");\n";

    if (m_EditWindow->GetTextCursorRate()!=Default.GetTextCursorRate())
        OutFile << "    self:SetTextCursorRate(" << m_EditWindow->GetTextCursorRate() << ");\n";

    if (m_EditWindow->GetTextCursorColor()[0]!=Default.GetTextCursorColor()[0] ||
        m_EditWindow->GetTextCursorColor()[1]!=Default.GetTextCursorColor()[1] ||
        m_EditWindow->GetTextCursorColor()[2]!=Default.GetTextCursorColor()[2] ||
        m_EditWindow->GetTextCursorColor()[3]!=Default.GetTextCursorColor()[3])
        OutFile << "    self:SetTextCursorColor("
                << m_EditWindow->GetTextCursorColor()[0] << ", "
                << m_EditWindow->GetTextCursorColor()[1] << ", "
                << m_EditWindow->GetTextCursorColor()[2] << ", "
                << m_EditWindow->GetTextCursorColor()[3] << ");\n";

    return true;
}
