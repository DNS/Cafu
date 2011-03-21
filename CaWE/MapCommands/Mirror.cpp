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

#include "Mirror.hpp"
#include "../MapElement.hpp"
#include "../MapDocument.hpp"


CommandMirrorT::CommandMirrorT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, unsigned int NormalAxis, float Dist)
    : m_MapDoc(MapDoc),
      m_MirrorElems(Elems),
      m_OldStates(),
      m_NormalAxis(NormalAxis),
      m_Dist(Dist)
{
    for (unsigned long i=0; i<m_MirrorElems.Size(); i++)
        m_OldStates.PushBack(m_MirrorElems[i]->Clone());
}


CommandMirrorT::~CommandMirrorT()
{
    for (unsigned long i=0; i<m_OldStates.Size(); i++)
        delete m_OldStates[i];

    m_OldStates.Clear();
}


bool CommandMirrorT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    // Do mirror.
    for (unsigned long i=0; i<m_MirrorElems.Size(); i++)
    {
        OldBounds.PushBack(m_MirrorElems[i]->GetBB());
        m_MirrorElems[i]->TrafoMirror(m_NormalAxis, m_Dist);
    }

    m_MapDoc.UpdateAllObservers_Modified(m_MirrorElems, MEMD_TRANSFORM, OldBounds);

    m_Done=true;
    return true;
}


void CommandMirrorT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    for (unsigned long i=0; i<m_MirrorElems.Size(); i++)
    {
        OldBounds.PushBack(m_MirrorElems[i]->GetBB());
        m_MirrorElems[i]->Assign(m_OldStates[i]);
    }

    m_MapDoc.UpdateAllObservers_Modified(m_MirrorElems, MEMD_TRANSFORM, OldBounds);

    m_Done=false;
}


wxString CommandMirrorT::GetName() const
{
    const wxString AxesNames[]={ "x", "y", "z" };

    return "Mirror in "+AxesNames[m_NormalAxis % 3]+"-direction";
}
