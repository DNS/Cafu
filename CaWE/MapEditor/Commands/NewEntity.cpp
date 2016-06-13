/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "NewEntity.hpp"
#include "Select.hpp"

#include "../CompMapEntity.hpp"
#include "../MapDocument.hpp"
#include "../MapEntRepres.hpp"

#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"

using namespace MapEditor;


CommandNewEntityT::CommandNewEntityT(MapDocumentT& MapDoc, IntrusivePtrT<cf::GameSys::EntityT> Entity, IntrusivePtrT<cf::GameSys::EntityT> Parent, bool SetSel)
    : m_MapDoc(MapDoc),
      m_Entities(),
      m_Parent(Parent),
      m_SetSel(SetSel),
      m_CommandSelect(NULL)
{
    m_Entities.PushBack(Entity);
}


CommandNewEntityT::CommandNewEntityT(MapDocumentT& MapDoc, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities, IntrusivePtrT<cf::GameSys::EntityT> Parent, bool SetSel)
    : m_MapDoc(MapDoc),
      m_Entities(Entities),
      m_Parent(Parent),
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
        m_MapDoc.Insert(m_Entities[EntNr], m_Parent);

    m_MapDoc.UpdateAllObservers_Created(m_Entities);

    if (m_SetSel && !m_CommandSelect)
    {
        ArrayT<MapElementT*> MapElems;

        for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
            MapElems.PushBack(GetMapEnt(m_Entities[EntNr])->GetAllMapElements());

        m_CommandSelect = CommandSelectT::Set(&m_MapDoc, MapElems);
    }

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
