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

#include "EditorChoiceWindow.hpp"
#include "../ChildFrame.hpp"
#include "../GuiDocument.hpp"
#include "../Commands/ModifyWindow.hpp"
#include "../Commands/SetWinProp.hpp"

#include "GuiSys/WindowChoice.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "wx/propgrid/manager.h"


using namespace GuiEditor;


EditorChoiceWindowT::EditorChoiceWindowT(IntrusivePtrT<cf::GuiSys::ChoiceT> Choice, GuiDocumentT* GuiDoc)
    : EditorWindowT(Choice, GuiDoc),
      m_Choice(Choice)
{
}


void EditorChoiceWindowT::FillInPG(wxPropertyGridManager* PropMan)
{
    EditorWindowT::FillInPG(PropMan);

    wxString Str;

    for (unsigned long ChoiceNr=0; ChoiceNr<m_Choice->GetChoices().Size(); ChoiceNr++)
    {
        if (ChoiceNr>0) Str+="\n";
        Str+=m_Choice->GetChoices()[ChoiceNr];
    }

    PropMan->Append(new wxLongStringProperty("Choices", wxPG_LABEL, Str));
    PropMan->Append(new wxIntProperty("DefaultChoice", wxPG_LABEL, m_Choice->GetSelectedChoice()));
}


bool EditorChoiceWindowT::UpdateProperty(wxPGProperty* Property)
{
    if (EditorWindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

    if (PropName=="Choices")
    {
        wxString Str;

        for (unsigned long ChoiceNr=0; ChoiceNr<m_Choice->GetChoices().Size(); ChoiceNr++)
        {
            if (ChoiceNr>0) Str+="\n";
            Str+=m_Choice->GetChoices()[ChoiceNr];
        }

        Property->SetValueFromString(Str);
        return true;
    }

    if (PropName=="DefaultChoice")
    {
        Property->SetValue(m_Choice->GetSelectedChoice());
        return true;
    }

    return false;
}


bool EditorChoiceWindowT::HandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (EditorWindowT::HandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty* Prop    =Event.GetProperty();
    const wxString      PropName=Prop->GetName();

    if (PropName=="Choices")
    {
        ArrayT<std::string> NewStrings;
        wxStringTokenizer   Tokenizer(Prop->GetValueAsString(), "\r\n");

        while (Tokenizer.HasMoreTokens())
            NewStrings.PushBack(std::string(Tokenizer.GetNextToken()));

        ChildFrame->SubmitCommand(new CommandSetWinPropT< ArrayT<std::string> >(m_GuiDoc, this, PropName, m_Choice->m_Choices, NewStrings));
        return true;
    }

    if (PropName=="DefaultChoice")
    {
        wxASSERT(m_Choice->GetMemberVar("selectedChoice").Member!=NULL);

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_Choice, PropName, m_Choice->GetMemberVar("selectedChoice"), Prop->GetValue().GetLong()));
        return true;
    }

    return false;
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
