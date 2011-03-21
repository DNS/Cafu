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

#include "SetBPSubdivs.hpp"

#include "../MapBezierPatch.hpp"
#include "../MapDocument.hpp"


CommandSetBPSubdivsT::CommandSetBPSubdivsT(MapDocumentT* MapDoc, MapBezierPatchT* BezierPatch, int Amount, SubdivDirE Direction)
    : m_MapDoc(MapDoc),
      m_BezierPatch(BezierPatch),
      m_NewAmount(Amount),
      m_OldAmount(Direction==HORIZONTAL ? m_BezierPatch->GetSubdivsHorz() : m_BezierPatch->GetSubdivsVert()),
      m_Direction(Direction)
{
}


bool CommandSetBPSubdivsT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    if   (m_Direction==HORIZONTAL) m_BezierPatch->SetSubdivsHorz(m_NewAmount);
    else                           m_BezierPatch->SetSubdivsVert(m_NewAmount);

    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_BezierPatch);

    m_MapDoc->UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED);

    m_Done=true;

    return true;
}


void CommandSetBPSubdivsT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    if   (m_Direction==HORIZONTAL) m_BezierPatch->SetSubdivsHorz(m_OldAmount);
    else                           m_BezierPatch->SetSubdivsVert(m_OldAmount);

    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_BezierPatch);

    m_MapDoc->UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED);

    m_Done=false;
}


wxString CommandSetBPSubdivsT::GetName() const
{
    return "Change BezierPatch subdivisions";
}
