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

#include "EditorChoiceWindow.hpp"
#include "../ChildFrame.hpp"
#include "../GuiDocument.hpp"
#include "../Commands/ModifyWindow.hpp"

#include "GuiSys/WindowChoice.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "wx/propgrid/manager.h"


using namespace GuiEditor;


EditorChoiceWindowT::EditorChoiceWindowT(cf::GuiSys::ChoiceT* Choice, GuiDocumentT* GuiDoc)
    : EditorWindowT(Choice, GuiDoc),
      m_Choice(Choice)
{
}


void EditorChoiceWindowT::FillInPG(wxPropertyGridManager* PropMan)
{
    EditorWindowT::FillInPG(PropMan);

    wxString ChoicesString;

    for (unsigned long ChoiceNr=0; ChoiceNr<m_Choice->GetChoices().Size(); ChoiceNr++)
        ChoicesString+=m_Choice->GetChoices()[ChoiceNr]+";\n";

    PropMan->Append(new wxLongStringProperty("Choices", wxPG_LABEL, ChoicesString));

#ifndef __WXGTK__
    // This is only temporarily disabled, see http://trac.wxwidgets.org/ticket/13447 for details.
    // TODO: Re-enable also for wxGTK as soon as possible.
    PropMan->Append(new wxUIntProperty("DefaultChoice", wxPG_LABEL, m_Choice->GetSelectedChoice()));
#endif
}


bool EditorChoiceWindowT::UpdateProperty(wxPGProperty* Property)
{
    if (EditorWindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

    if (PropName=="Choices")
    {
        wxString ChoicesString;

        for (unsigned long ChoiceNr=0; ChoiceNr<m_Choice->GetChoices().Size(); ChoiceNr++)
            ChoicesString+=m_Choice->GetChoices()[ChoiceNr]+";\n";

        Property->SetValueFromString(ChoicesString);
    }
    else if (PropName=="DefaultChoice") Property->SetValue(int(m_Choice->GetSelectedChoice()));
    else                                return false;

    return true;
}


bool EditorChoiceWindowT::HandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (EditorWindowT::HandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty* Prop    =Event.GetProperty();
    const wxString      PropName=Prop->GetName();

    if (PropName=="Choices")
    {
        static cf::GuiSys::WindowT::MemberVarT DummyVar;

        // Remove all newline characters from the string since they break the choices display.
        wxString FormattedString=Prop->GetValueAsString();
        FormattedString.Replace("\\n", "");
        // Remove all trailing characters after the last delimiter (;).
        FormattedString=FormattedString.Mid(0, FormattedString.Find(';', true)+1);

        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Choice, PropName, DummyVar, FormattedString));
    }
    else if (PropName=="DefaultChoice")
    {
        wxASSERT(m_Choice->GetMemberVar("defaultChoice").Member!=NULL);

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Choice, PropName, m_Choice->GetMemberVar("defaultChoice"), Prop->GetValue().GetLong()));
    }
    else
    {
        return false;
    }

    return true;
}


bool EditorChoiceWindowT::WriteInitMethod(std::ostream& OutFile)
{
    EditorWindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    cf::GuiSys::WindowCreateParamsT Params(*m_GuiDoc->GetGui());
    cf::GuiSys::ChoiceT             Default(Params);

    for (unsigned long ChoiceNr=0; ChoiceNr<m_Choice->GetChoices().Size(); ChoiceNr++)
        OutFile << "    self:Append(\"" << m_Choice->GetChoices()[ChoiceNr] << "\");\n";

    if (m_Choice->GetSelectedChoice()!=Default.GetSelectedChoice())
        OutFile << "    self:SetSelection(" << m_Choice->GetSelectedChoice() << ");\n";

    return true;
}
