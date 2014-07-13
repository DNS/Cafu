/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "ChangeEntityHierarchy.hpp"
#include "../MapDocument.hpp"

#include "GameSys/Entity.hpp"


using namespace MapEditor;


CommandChangeEntityHierarchyT::CommandChangeEntityHierarchyT(MapDocumentT* MapDoc, IntrusivePtrT<cf::GameSys::EntityT> Entity, IntrusivePtrT<cf::GameSys::EntityT> NewParent, unsigned long NewPosition)
    : m_MapDoc(MapDoc),
      m_Entity(Entity),
      m_NewParent(NewParent),
      m_NewPosition(NewPosition),
      m_OldParent(Entity->GetParent()),
      m_OldPosition(!Entity->GetParent().IsNull() ? Entity->GetParent()->GetChildren().Find(Entity) : 0),
      m_OldName(Entity->GetBasics()->GetEntityName())
{
    wxASSERT(m_MapDoc);
    wxASSERT(!m_Entity.IsNull());   // m_NewParent==NULL or m_OldParent==NULL are handled below.
}


bool CommandChangeEntityHierarchyT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Make sure that m_NewParent is not in the subtree of m_Entity
    // (or else the reparenting would create invalid cycles).
    // Note that we don't check for the NOP case m_NewParent == m_OldParent,
    // that's up to the caller (and not really a reason to return "false" / failure).
    {
        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > SubTree;

        SubTree.PushBack(m_Entity);
        m_Entity->GetChildren(SubTree, true /*recurse*/);

        if (SubTree.Find(m_NewParent) >= 0) return false;
    }

    if (!m_OldParent.IsNull()) m_OldParent->RemoveChild(m_Entity);
    if (!m_NewParent.IsNull()) m_NewParent->AddChild(m_Entity, m_NewPosition);

    m_MapDoc->UpdateAllObservers_EntChanged(m_Entity, EMD_HIERARCHY);
    m_Done = true;
    return true;
}


void CommandChangeEntityHierarchyT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (!m_NewParent.IsNull()) m_NewParent->RemoveChild(m_Entity);
    if (!m_OldParent.IsNull()) m_OldParent->AddChild(m_Entity, m_OldPosition);

    // The call to AddChild() in Do() might have modified the name of the m_Entity as a side-effect.
    // Now that we have restored the previous hierarchy, restore the previous name as well.
    m_Entity->GetBasics()->SetEntityName(m_OldName);

    m_MapDoc->UpdateAllObservers_EntChanged(m_Entity, EMD_HIERARCHY);
    m_Done = false;
}


wxString CommandChangeEntityHierarchyT::GetName() const
{
    return "Change entity hierarchy";
}
