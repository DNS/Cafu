/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
