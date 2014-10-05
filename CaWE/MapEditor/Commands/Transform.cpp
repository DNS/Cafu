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

#include "Transform.hpp"

#include "../MapDocument.hpp"
#include "../MapPrimitive.hpp"


CommandTransformT::CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, TransModeT Mode, const Vector3fT& RefPoint, const Vector3fT& Amount)
    : m_MapDoc(MapDoc),
      m_TransElems(TransElems),
      m_OldStates(),
      m_Mode(Mode),
      m_RefPoint(RefPoint),
      m_Amount(Amount),
      m_Matrix()
{
    wxASSERT(m_Mode!=MODE_MATRIX);
    wxASSERT(!(m_Mode==MODE_SCALE && m_Amount.x==0.0f));
    wxASSERT(!(m_Mode==MODE_SCALE && m_Amount.y==0.0f));
    wxASSERT(!(m_Mode==MODE_SCALE && m_Amount.z==0.0f));

    Init();
}


CommandTransformT::CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, const MatrixT& Matrix)
    : m_MapDoc(MapDoc),
      m_TransElems(TransElems),
      m_OldStates(),
      m_Mode(MODE_MATRIX),
      m_RefPoint(),
      m_Amount(),
      m_Matrix(Matrix)
{
    Init();
}


void CommandTransformT::Init()
{
    // The m_TransElems are directly modified, so keep a record of their old states.
    for (unsigned long ElemNr = 0; ElemNr < m_TransElems.Size(); ElemNr++)
        m_OldStates.PushBack(m_TransElems[ElemNr]->GetTrafoState());
}


CommandTransformT::~CommandTransformT()
{
    for (unsigned long i = 0; i < m_OldStates.Size(); i++)
        delete m_OldStates[i];

    m_OldStates.Clear();
}


bool CommandTransformT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    for (unsigned long ElemNr=0; ElemNr<m_TransElems.Size(); ElemNr++)
    {
        OldBounds.PushBack(m_TransElems[ElemNr]->GetBB());

        switch (m_Mode)
        {
            case MODE_TRANSLATE: m_TransElems[ElemNr]->TrafoMove(m_Amount);               break;
            case MODE_ROTATE:    m_TransElems[ElemNr]->TrafoRotate(m_RefPoint, m_Amount); break;
            case MODE_SCALE:     m_TransElems[ElemNr]->TrafoScale(m_RefPoint, m_Amount);  break;
            case MODE_MATRIX:    m_TransElems[ElemNr]->Transform(m_Matrix);               break;
        }
    }

    m_MapDoc.UpdateAllObservers_Modified(m_TransElems, MEMD_TRANSFORM, OldBounds);

    m_Done=true;
    return true;
}


void CommandTransformT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    for (unsigned long ElemNr=0; ElemNr<m_TransElems.Size(); ElemNr++)
    {
        OldBounds.PushBack(m_TransElems[ElemNr]->GetBB());
        m_TransElems[ElemNr]->RestoreTrafoState(m_OldStates[ElemNr]);
    }

    m_MapDoc.UpdateAllObservers_Modified(m_TransElems, MEMD_TRANSFORM, OldBounds);

    m_Done=false;
}


wxString CommandTransformT::GetName() const
{
    const wxString objects=m_TransElems.Size()==1 ? "object" : "objects";

    switch (m_Mode)
    {
        case MODE_TRANSLATE: return "move "+objects;
        case MODE_SCALE:     return "scale "+objects;
        case MODE_ROTATE:    return "rotate "+objects;
        case MODE_MATRIX:    return "shear "+objects;   // Rename to "transform "?
    }

    return "";
}
