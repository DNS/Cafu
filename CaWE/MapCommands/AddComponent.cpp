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

#include "AddComponent.hpp"
#include "../MapDocument.hpp"

#include "GameSys/CompBase.hpp"
#include "GameSys/Entity.hpp"


using namespace MapEditor;


CommandAddComponentT::CommandAddComponentT(MapDocumentT* MapDocument, IntrusivePtrT<cf::GameSys::EntityT> Entity, IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp, unsigned long Index)
    : m_MapDocument(MapDocument),
      m_Entity(Entity),
      m_Component(Comp),
      m_Index(std::min(Index, m_Entity->GetComponents().Size()))
{
}


bool CommandAddComponentT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    if (!m_Entity->AddComponent(m_Component, m_Index)) return false;
    m_MapDocument->UpdateAllObservers_EntChanged(m_Entity, EMD_COMPONENTS);

    m_Done=true;
    return true;
}


void CommandAddComponentT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Entity->DeleteComponent(m_Index);
    m_MapDocument->UpdateAllObservers_EntChanged(m_Entity, EMD_COMPONENTS);

    m_Done=false;
}


wxString CommandAddComponentT::GetName() const
{
    return wxString("Add component: ") + m_Component->GetName();
}
