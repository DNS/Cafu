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

#include "Group_New.hpp"
#include "../Group.hpp"
#include "../MapDocument.hpp"


CommandNewGroupT::CommandNewGroupT(MapDocumentT& MapDoc, const wxString& Name)
    : m_MapDoc(MapDoc),
      m_Group(NULL)
{
    // Important detail: The new group is created *visible*, in order to minimize
    // potential side-effects on any map elements that are added to it later.
    m_Group=new GroupT(Name);

    wxASSERT(m_Group->IsVisible);
}


CommandNewGroupT::~CommandNewGroupT()
{
    if (!m_Done)
    {
        delete m_Group;
    }
}


bool CommandNewGroupT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Add m_Group to the groups array of the m_MapDoc
    // and notify all observers that our groups inventory changed.
    m_MapDoc.GetGroups().PushBack(m_Group);
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    m_Done=true;
    return true;
}


void CommandNewGroupT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Remove m_Group from the groups array of the m_MapDoc again
    // and notify all observers that our groups inventory changed.
    wxASSERT(m_MapDoc.GetGroups()[m_MapDoc.GetGroups().Size()-1]==m_Group);
    m_MapDoc.GetGroups().DeleteBack();
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    m_Done=false;
}


wxString CommandNewGroupT::GetName() const
{
    return "Add new group";
}
