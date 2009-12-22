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

#include "Delete.hpp"

#include "../GuiDocument.hpp"

#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandDeleteT::CommandDeleteT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window)
    : m_GuiDocument(GuiDocument)
{
    if (Window!=Window->GetRoot()) // Root window can never be deleted.
    {
        m_DeleteWindows.PushBack(Window);
        m_Parents.PushBack(Window->Parent);
        m_Indexes.PushBack(Window->Parent->Children.Find(Window));
    }
}


CommandDeleteT::CommandDeleteT(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& Windows)
    : m_GuiDocument(GuiDocument),
      m_DeleteWindows(Windows)
{
    for (unsigned long WinNr=0; WinNr<m_DeleteWindows.Size(); WinNr++)
    {
        // Root window can never be deleted.
        if (m_DeleteWindows[WinNr]==m_DeleteWindows[WinNr]->GetRoot())
        {
            m_DeleteWindows.RemoveAtAndKeepOrder(WinNr);
            WinNr--;
            continue;
        }

        m_Parents.PushBack(m_DeleteWindows[WinNr]->Parent);
        m_Indexes.PushBack(m_DeleteWindows[WinNr]->Parent->Children.Find(m_DeleteWindows[WinNr]));
    }
}


CommandDeleteT::~CommandDeleteT()
{
    if (m_Done)
        for (unsigned long WinNr=0; WinNr<m_DeleteWindows.Size(); WinNr++)
            delete m_DeleteWindows[WinNr];
}


bool CommandDeleteT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    for (unsigned long WinNr=0; WinNr<m_DeleteWindows.Size(); WinNr++)
        m_Parents[WinNr]->Children.RemoveAtAndKeepOrder(m_Indexes[WinNr]);

    m_GuiDocument->UpdateAllObservers_Deleted(m_DeleteWindows);

    m_Done=true;

    return true;
}


void CommandDeleteT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    for (unsigned long WinNr=0; WinNr<m_DeleteWindows.Size(); WinNr++)
        m_Parents[WinNr]->Children.InsertAt(m_Indexes[WinNr], m_DeleteWindows[WinNr]);

    m_GuiDocument->UpdateAllObservers_Created(m_DeleteWindows);

    m_Done=false;
}


wxString CommandDeleteT::GetName() const
{
    return "Delete windows";
}
