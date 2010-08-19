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

#include "Paste.hpp"
#include "../GuiDocument.hpp"
#include "../EditorData/Window.hpp"
#include "GuiSys/Window.hpp"

#include <sstream>


using namespace GuiEditor;


CommandPasteT::CommandPasteT(GuiDocumentT* GuiDocument, const ArrayT<cf::GuiSys::WindowT*>& Windows, cf::GuiSys::WindowT* NewParent)
    : m_GuiDocument(GuiDocument),
      m_NewParent(NewParent)
{
    // Have to clone the windows here, so the command is the owner of these windows.
    for (unsigned long WinNr=0; WinNr<Windows.Size(); WinNr++)
    {
        m_Windows.PushBack(Windows[WinNr]->Clone(true));

        // Create editor data for the window itself and all of its children.
        new EditorDataWindowT(m_Windows[WinNr], m_GuiDocument);

        ArrayT<cf::GuiSys::WindowT*> Children;
        m_Windows[WinNr]->GetChildren(Children, true);

        for (unsigned long ChildNr=0; ChildNr<Children.Size(); ChildNr++)
            new EditorDataWindowT(Children[ChildNr], m_GuiDocument);
    }
}


CommandPasteT::~CommandPasteT()
{
    if (!m_Done)
    {
        for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        {
            ArrayT<cf::GuiSys::WindowT*> Children;
            m_Windows[WinNr]->GetChildren(Children, true);

            for (unsigned long ChildNr=0; ChildNr<Children.Size(); ChildNr++)
                delete Children[ChildNr];

            delete m_Windows[WinNr];
        }
    }
}


bool CommandPasteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
    {
        m_Windows[WinNr]->Parent=m_NewParent;
        m_NewParent->Children.PushBack(m_Windows[WinNr]);

        // If the name of the window is not unique among its siblings, find a new unique name.
        ((EditorDataWindowT*)m_Windows[WinNr]->EditorData)->RepairNameUniqueness();
    }

    m_GuiDocument->UpdateAllObservers_Created(m_Windows);

    m_Done=true;
    return true;
}


void CommandPasteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
    {
        m_Windows[WinNr]->Parent=NULL;

        m_NewParent->Children.RemoveAtAndKeepOrder(m_NewParent->Children.Find(m_Windows[WinNr]));
    }

    m_GuiDocument->UpdateAllObservers_Deleted(m_Windows);

    m_Done=false;
}


wxString CommandPasteT::GetName() const
{
    if (m_Windows.Size()>1)
        return "Paste windows";
    else
        return "Paste window";
}
