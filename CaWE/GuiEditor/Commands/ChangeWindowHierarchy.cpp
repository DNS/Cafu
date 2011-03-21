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

#include "ChangeWindowHierarchy.hpp"
#include "../GuiDocument.hpp"

#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandChangeWindowHierarchyT::CommandChangeWindowHierarchyT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, cf::GuiSys::WindowT* NewParent, unsigned long NewPosition)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_NewParent(NewParent),
      m_NewPosition(NewPosition),
      m_OldParent(Window->Parent),
      m_OldPosition(Window->Parent ? Window->Parent->Children.Find(Window) : 0)
{
    wxASSERT(m_GuiDocument);
    wxASSERT(m_Window);     // m_NewParent==NULL or m_OldParent==NULL are handled below.
}


bool CommandChangeWindowHierarchyT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Make sure that m_NewParent is not in the subtree of m_Window
    // (or else the reparenting would create invalid cycles).
    // Note that we don't check for the NOP case m_NewParent==m_OldParent,
    // that's up to the caller (and not really a reason to return "false"/failure).
    {
        ArrayT<cf::GuiSys::WindowT*> SubTree;

        SubTree.PushBack(m_Window);
        m_Window->GetChildren(SubTree, true /*recurse*/);

        if (SubTree.Find(m_NewParent)>=0) return false;
    }

    if (m_OldParent) m_OldParent->Children.RemoveAtAndKeepOrder(m_OldPosition);
    m_Window->Parent=m_NewParent;
    if (m_NewParent) m_NewParent->Children.InsertAt(m_NewPosition, m_Window);

    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_HIERARCHY);
    m_Done=true;
    return true;
}


void CommandChangeWindowHierarchyT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (m_NewParent) m_NewParent->Children.RemoveAtAndKeepOrder(m_NewPosition);
    m_Window->Parent=m_OldParent;
    if (m_OldParent) m_OldParent->Children.InsertAt(m_OldPosition, m_Window);

    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_HIERARCHY);
    m_Done=false;
}


wxString CommandChangeWindowHierarchyT::GetName() const
{
    return "Change window hierarchy";
}
