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

#include "AddPrim.hpp"
#include "Select.hpp"
#include "../CompMapEntity.hpp"
#include "../MapDocument.hpp"
#include "../MapPrimitive.hpp"


using namespace MapEditor;


CommandAddPrimT::CommandAddPrimT(MapDocumentT& MapDoc, MapPrimitiveT* AddPrim, IntrusivePtrT<CompMapEntityT> Parent, wxString Name, bool SetSel)
    : m_MapDoc(MapDoc),
      m_AddPrims(),
      m_Parent(Parent),
      m_CommandSelect(NULL),
      m_Name(Name)
{
    m_AddPrims.PushBack(AddPrim);

    if (SetSel)
        m_CommandSelect = CommandSelectT::Set(&m_MapDoc, m_AddPrims);
}


CommandAddPrimT::CommandAddPrimT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& AddPrims, IntrusivePtrT<CompMapEntityT> Parent, wxString Name, bool SetSel)
    : m_MapDoc(MapDoc),
      m_AddPrims(AddPrims),
      m_Parent(Parent),
      m_CommandSelect(NULL),
      m_Name(Name)
{
    if (SetSel)
        m_CommandSelect = CommandSelectT::Set(&m_MapDoc, m_AddPrims);
}


CommandAddPrimT::~CommandAddPrimT()
{
    delete m_CommandSelect;

    if (!m_Done)
        for (unsigned long PrimNr=0; PrimNr<m_AddPrims.Size(); PrimNr++)
            delete m_AddPrims[PrimNr];
}


bool CommandAddPrimT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    for (unsigned long PrimNr=0; PrimNr<m_AddPrims.Size(); PrimNr++)
    {
        wxASSERT(m_AddPrims[PrimNr]->GetParent()==NULL);
        m_MapDoc.Insert(m_AddPrims[PrimNr], m_Parent);
    }

    m_MapDoc.UpdateAllObservers_Created(m_AddPrims);

    if (m_CommandSelect)
        m_CommandSelect->Do();

    m_Done=true;
    return true;
}


void CommandAddPrimT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (m_CommandSelect)
        m_CommandSelect->Undo();

    for (unsigned long PrimNr=0; PrimNr<m_AddPrims.Size(); PrimNr++)
    {
        m_MapDoc.Remove(m_AddPrims[PrimNr]);
        wxASSERT(m_AddPrims[PrimNr]->GetParent()==NULL);
    }

    m_MapDoc.UpdateAllObservers_Deleted(m_AddPrims);

    m_Done=false;
}


wxString CommandAddPrimT::GetName() const
{
    return m_Name;
}
