/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
