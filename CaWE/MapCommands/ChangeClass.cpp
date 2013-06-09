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

#include "ChangeClass.hpp"

#include "../CompMapEntity.hpp"
#include "../MapDocument.hpp"
#include "../MapEntRepres.hpp"


using namespace MapEditor;


CommandChangeClassT::CommandChangeClassT(MapDocumentT& MapDoc, IntrusivePtrT<CompMapEntityT> Entity, const EntityClassT* NewClass)
    : m_Entity(Entity),
      m_PrevProps(Entity->GetProperties()),
      m_MapDoc(MapDoc),
      m_NewClass(NewClass),
      m_PrevClass(Entity->GetClass())
{
}


bool CommandChangeClassT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    m_Entity->SetClass(m_NewClass);

    ArrayT< IntrusivePtrT<CompMapEntityT> > MapEnts;
    MapEnts.PushBack(m_Entity);

    // Treat changes in entity class like any other (old-style) property change.
    m_MapDoc.UpdateAllObservers_EntChanged(MapEnts, EMD_PROPERTIES);

    m_Done=true;
    return true;
}


void CommandChangeClassT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Entity->SetClass(m_PrevClass);

    // Reset previous properties.
    m_Entity->GetProperties()=m_PrevProps;

    ArrayT< IntrusivePtrT<CompMapEntityT> > MapEnts;
    MapEnts.PushBack(m_Entity);

    // Treat changes in entity class like any other (old-style) property change.
    m_MapDoc.UpdateAllObservers_EntChanged(MapEnts, EMD_PROPERTIES);

    m_Done=false;
}


wxString CommandChangeClassT::GetName() const
{
    return "Change entity class";
}
