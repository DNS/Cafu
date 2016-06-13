/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DeleteComponent.hpp"
#include "../MapDocument.hpp"

#include "GameSys/CompBase.hpp"
#include "GameSys/Entity.hpp"


using namespace MapEditor;


CommandDeleteComponentT::CommandDeleteComponentT(MapDocumentT* MapDocument, IntrusivePtrT<cf::GameSys::EntityT> Entity, unsigned long Index)
    : m_MapDocument(MapDocument),
      m_Entity(Entity),
      m_Component(m_Entity->GetComponents()[Index]),
      m_Index(Index)
{
}


bool CommandDeleteComponentT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    m_Entity->DeleteComponent(m_Index);
    m_MapDocument->UpdateAllObservers_EntChanged(m_Entity, EMD_COMPONENTS);;

    m_Done=true;
    return true;
}


void CommandDeleteComponentT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // No checking of the return type, it simply *must* work.
    m_Entity->AddComponent(m_Component, m_Index);
    m_MapDocument->UpdateAllObservers_EntChanged(m_Entity, EMD_COMPONENTS);;

    m_Done=false;
}


wxString CommandDeleteComponentT::GetName() const
{
    return wxString("Delete component: ") + m_Component->GetName();
}
