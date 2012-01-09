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

#include "Select.hpp"

#include "../GuiDocument.hpp"


using namespace GuiEditor;


CommandSelectT* CommandSelectT::Clear(GuiDocumentT* GuiDocument)
{
    ArrayT<cf::GuiSys::WindowT*> EmptySelection;

    return new CommandSelectT(GuiDocument, GuiDocument->GetSelection(), EmptySelection);
}


CommandSelectT* CommandSelectT::Add(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    ArrayT<cf::GuiSys::WindowT*> OldSelection(GuiDocument->GetSelection());
    ArrayT<cf::GuiSys::WindowT*> NewSelection(GuiDocument->GetSelection());

    // For each window, check if it is already part of the current selection.
    for (unsigned long WindowNr=0; WindowNr<Windows.Size(); WindowNr++)
    {
        unsigned long SelectionNr=0;

        for (SelectionNr=0; SelectionNr<OldSelection.Size(); SelectionNr++)
            if (Windows[WindowNr]==OldSelection[SelectionNr]) break;

        // Window is not part of the current selection.
        if (SelectionNr==OldSelection.Size()) NewSelection.PushBack(Windows[WindowNr]);
    }

    return new CommandSelectT(GuiDocument, OldSelection, NewSelection);
}


CommandSelectT* CommandSelectT::Add(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window)
{
    ArrayT<cf::GuiSys::WindowT*> AddSelection;
    AddSelection.PushBack(Window);

    return CommandSelectT::Add(GuiDocument, AddSelection);
}


CommandSelectT* CommandSelectT::Remove(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    ArrayT<cf::GuiSys::WindowT*> NewSelection(GuiDocument->GetSelection());

    // For each window, check if it is already part of the current selection.
    for (unsigned long WindowNr=0; WindowNr<Windows.Size(); WindowNr++)
    {
        for (unsigned long SelectionNr=0; SelectionNr<NewSelection.Size(); SelectionNr++)
        {
            // Window is part of the current selection.
            if (Windows[WindowNr]==NewSelection[SelectionNr])
            {
                NewSelection.RemoveAtAndKeepOrder(SelectionNr);
                SelectionNr--; // The current position has to be checked again.
                break;
            }
        }
    }

    return new CommandSelectT(GuiDocument, GuiDocument->GetSelection(), NewSelection);
}


CommandSelectT* CommandSelectT::Remove(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window)
{
    ArrayT<cf::GuiSys::WindowT*> RemoveSelection;
    RemoveSelection.PushBack(Window);

    return CommandSelectT::Remove(GuiDocument, RemoveSelection);
}


CommandSelectT* CommandSelectT::Set(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    return new CommandSelectT(GuiDocument, GuiDocument->GetSelection(), Windows);
}


CommandSelectT::CommandSelectT(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection)
    : CommandT(abs(int(OldSelection.Size())-int(NewSelection.Size()))>3, false), // Only show selection command in the undo/redo history if selection difference is greater 3.
      m_GuiDocument(GuiDocument),
      m_OldSelection(OldSelection),
      m_NewSelection(NewSelection)
{
}


CommandSelectT::~CommandSelectT()
{
}


bool CommandSelectT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Protect against "extra" EVT_TREE_SEL_CHANGED events, see
    // <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/72754> for details.
    if (m_OldSelection.Size()==0 && m_NewSelection.Size()==0) return false;
    if (m_OldSelection.Size()==1 && m_NewSelection.Size()==1 && m_OldSelection[0]==m_NewSelection[0]) return false;

    m_GuiDocument->SetSelection(m_NewSelection);

    m_GuiDocument->UpdateAllObservers_SelectionChanged(m_OldSelection, m_NewSelection);
    m_Done=true;
    return true;
}


void CommandSelectT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_GuiDocument->SetSelection(m_OldSelection);

    m_GuiDocument->UpdateAllObservers_SelectionChanged(m_NewSelection, m_OldSelection);
    m_Done=false;
}


wxString CommandSelectT::GetName() const
{
    return "Selection change";
}
