/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "NewEntity.hpp"
#include "Select.hpp"

#include "../EntityClass.hpp"
#include "../MapDocument.hpp"
#include "../MapEntRepres.hpp"


CommandNewEntityT::CommandNewEntityT(MapDocumentT& MapDoc, IntrusivePtrT<cf::GameSys::EntityT> Entity, bool SetSel)
    : m_MapDoc(MapDoc),
      m_Entities(),
      m_SetSel(SetSel),
      m_CommandSelect(NULL)
{
    m_Entities.PushBack(Entity);
}


CommandNewEntityT::CommandNewEntityT(MapDocumentT& MapDoc, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities, bool SetSel)
    : m_MapDoc(MapDoc),
      m_Entities(Entities),
      m_SetSel(SetSel),
      m_CommandSelect(NULL)
{
}


CommandNewEntityT::~CommandNewEntityT()
{
    // if (!m_Done)
    //     for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
    //         delete m_Entities[EntNr];

    delete m_CommandSelect;
}


bool CommandNewEntityT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;
    if (m_Entities.Size() == 0) return false;

    // Insert the entities into the map.
    for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
        m_MapDoc.Insert(m_Entities[EntNr]);

    m_MapDoc.UpdateAllObservers_Created(m_Entities);

    if (m_SetSel && !m_CommandSelect)
        m_CommandSelect = CommandSelectT::Set(&m_MapDoc, m_Entities);

    if (m_CommandSelect)
        m_CommandSelect->Do();

    m_Done=true;
    return true;
}


void CommandNewEntityT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (m_CommandSelect)
        m_CommandSelect->Undo();

    // Remove the entities from the map again.
    for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
        m_MapDoc.Remove(m_Entities[EntNr]);

    m_MapDoc.UpdateAllObservers_Deleted(m_Entities);

    m_Done=false;
}


wxString CommandNewEntityT::GetName() const
{
    if (m_Entities.Size() == 1)
        return "add new entity";

    return wxString::Format("add %lu new entities", m_Entities.Size());
}
