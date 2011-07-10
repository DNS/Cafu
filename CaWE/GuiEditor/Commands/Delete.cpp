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

#include "Delete.hpp"
#include "Select.hpp"
#include "../GuiDocument.hpp"
#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandDeleteT::CommandDeleteT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window)
    : m_GuiDocument(GuiDocument),
      m_CommandSelect(NULL)
{
    // The root window cannot be deleted.
    if (Window!=Window->GetRoot())
    {
        wxASSERT(Window->Parent->Children.Find(Window)>=0);

        m_Windows.PushBack(Window);
        m_Indices.PushBack(-1);
    }

    m_CommandSelect=CommandSelectT::Remove(m_GuiDocument, m_Windows);
}


CommandDeleteT::CommandDeleteT(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& Windows)
    : m_GuiDocument(GuiDocument),
      m_CommandSelect(NULL)
{
    for (unsigned long WinNr=0; WinNr<Windows.Size(); WinNr++)
    {
        cf::GuiSys::WindowT* Window=Windows[WinNr];

        // The root window cannot be deleted.
        if (Window!=Window->GetRoot())
        {
            wxASSERT(Window->Parent->Children.Find(Window)>=0);

            m_Windows.PushBack(Window);
            m_Indices.PushBack(-1);
        }
    }

    m_CommandSelect=CommandSelectT::Remove(m_GuiDocument, m_Windows);
}


CommandDeleteT::~CommandDeleteT()
{
    delete m_CommandSelect;

    if (m_Done)
        for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
            delete m_Windows[WinNr];
}


bool CommandDeleteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Fail if there are no windows to delete.
    if (m_Windows.Size()==0) return false;

    // Deselect any affected windows that are selected.
    m_CommandSelect->Do();

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
    {
        cf::GuiSys::WindowT* Window=m_Windows[WinNr];

        m_Indices[WinNr]=Window->Parent->Children.Find(Window);
        Window->Parent->Children.RemoveAtAndKeepOrder(m_Indices[WinNr]);
    }

    m_GuiDocument->UpdateAllObservers_Deleted(m_Windows);

    m_Done=true;
    return true;
}


void CommandDeleteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    for (unsigned long RevNr=0; RevNr<m_Windows.Size(); RevNr++)
    {
        const unsigned long  WinNr =m_Windows.Size()-RevNr-1;
        cf::GuiSys::WindowT* Window=m_Windows[WinNr];

        Window->Parent->Children.InsertAt(m_Indices[WinNr], Window);
    }

    m_GuiDocument->UpdateAllObservers_Created(m_Windows);

    // Select the previously selected windows again (unless the command failed on Do(), which can happen e.g. on unchanged selection).
    if (m_CommandSelect->IsDone()) m_CommandSelect->Undo();

    m_Done=false;
}


wxString CommandDeleteT::GetName() const
{
    return (m_Windows.Size()==1) ? "Delete 1 window" : wxString::Format("Delete %lu windows", m_Windows.Size());
}
