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

#include "Transform.hpp"
#include "Select.hpp"

#include "../MapDocument.hpp"
#include "../MapEntity.hpp"
#include "../MapPrimitive.hpp"


CommandTransformT::CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, TransModeT Mode, const Vector3fT& RefPoint, const Vector3fT& Amount, bool DoClone)
    : m_MapDoc(MapDoc),
      m_TransElems(TransElems),
      m_OldStates(),
      m_ClonedElems(),
      m_Mode(Mode),
      m_RefPoint(RefPoint),
      m_Amount(Amount),
      m_Matrix(),
      m_DoClone(DoClone),
      m_CommandSelect(NULL)
{
    wxASSERT(m_Mode!=MODE_MATRIX);
    wxASSERT(!(m_Mode==MODE_SCALE && m_Amount.x==0.0f));
    wxASSERT(!(m_Mode==MODE_SCALE && m_Amount.y==0.0f));
    wxASSERT(!(m_Mode==MODE_SCALE && m_Amount.z==0.0f));

    Init();
}


CommandTransformT::CommandTransformT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& TransElems, const MatrixT& Matrix, bool DoClone)
    : m_MapDoc(MapDoc),
      m_TransElems(TransElems),
      m_OldStates(),
      m_ClonedElems(),
      m_Mode(MODE_MATRIX),
      m_RefPoint(),
      m_Amount(),
      m_Matrix(Matrix),
      m_DoClone(DoClone),
      m_CommandSelect(NULL)
{
    Init();
}


void CommandTransformT::Init()
{
    if (m_DoClone)
    {
        // Note that for some uses of this command (namely as by the ToolSelectionT class),
        // it is important that the cloned elements are created directly here in the constructor,
        // rather than only in the first call to Do().
        for (unsigned long ElemNr=0; ElemNr<m_TransElems.Size(); ElemNr++)
        {
            MapElementT* Elem =m_TransElems[ElemNr];
            MapElementT* Clone=Elem->Clone();

            switch (m_Mode)
            {
                case MODE_TRANSLATE: Clone->TrafoMove(m_Amount);               break;
                case MODE_ROTATE:    Clone->TrafoRotate(m_RefPoint, m_Amount); break;
                case MODE_SCALE:     Clone->TrafoScale(m_RefPoint, m_Amount);  break;
                case MODE_MATRIX:    Clone->Transform(m_Matrix);               break;
            }

            m_ClonedElems.PushBack(Clone);
        }

        m_CommandSelect=CommandSelectT::Set(&m_MapDoc, m_ClonedElems);
    }
    else
    {
        // The m_TransElems are directly modified, so keep a record of their old states.
        for (unsigned long ElemNr=0; ElemNr<m_TransElems.Size(); ElemNr++)
            m_OldStates.PushBack(m_TransElems[ElemNr]->Clone());
    }
}


CommandTransformT::~CommandTransformT()
{
    if (!m_Done)
        for (unsigned long i=0; i<m_ClonedElems.Size(); i++)
            delete m_ClonedElems[i];

    delete m_CommandSelect;
    m_CommandSelect=NULL;

    for (unsigned long i=0; i<m_OldStates.Size(); i++)
        delete m_OldStates[i];

    m_OldStates.Clear();
}


bool CommandTransformT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    if (m_DoClone)
    {
        // Insert cloned objects into the document, attaching them to the same parents as the respective source element.
        for (unsigned long CloneNr=0; CloneNr<m_ClonedElems.Size(); CloneNr++)
        {
            MapEntityT* Ent=dynamic_cast<MapEntityT*>(m_ClonedElems[CloneNr]);
            if (Ent)
            {
                m_MapDoc.Insert(Ent);
                continue;
            }

            MapPrimitiveT* ClonedPrim=dynamic_cast<MapPrimitiveT*>(m_ClonedElems[CloneNr]);
            MapPrimitiveT* OrigPrim  =dynamic_cast<MapPrimitiveT*>(m_TransElems[CloneNr]);
            wxASSERT((ClonedPrim==NULL)==(OrigPrim==NULL));
            if (ClonedPrim && OrigPrim)
            {
                m_MapDoc.Insert(ClonedPrim, OrigPrim->GetParent());
                continue;
            }

            // TODO(?): Insert m_ClonedElems[CloneNr] into the same group as m_TransElems[CloneNr]?
        }

        m_MapDoc.UpdateAllObservers_Created(m_ClonedElems);
        m_CommandSelect->Do();
    }
    else
    {
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
    }

    m_Done=true;
    return true;
}


void CommandTransformT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (m_DoClone)
    {
        m_CommandSelect->Undo();

        // Remove cloned objects from world again.
        for (unsigned long CloneNr=0; CloneNr<m_ClonedElems.Size(); CloneNr++)
            m_MapDoc.Remove(m_ClonedElems[CloneNr]);

        m_MapDoc.UpdateAllObservers_Deleted(m_ClonedElems);
    }
    else
    {
        // Record the previous bounding-boxes for the observer message.
        ArrayT<BoundingBox3fT> OldBounds;

        for (unsigned long ElemNr=0; ElemNr<m_TransElems.Size(); ElemNr++)
        {
            OldBounds.PushBack(m_TransElems[ElemNr]->GetBB());
            m_TransElems[ElemNr]->Assign(m_OldStates[ElemNr]);
        }

        m_MapDoc.UpdateAllObservers_Modified(m_TransElems, MEMD_TRANSFORM, OldBounds);
    }

    m_Done=false;
}


wxString CommandTransformT::GetName() const
{
    const wxString objects=m_TransElems.Size()==1 ? "object" : "objects";

    if (m_DoClone) return "clone "+objects;

    switch (m_Mode)
    {
        case MODE_TRANSLATE: return "move "+objects;
        case MODE_SCALE:     return "scale "+objects;
        case MODE_ROTATE:    return "rotate "+objects;
        case MODE_MATRIX:    return "shear "+objects;   // Rename to "transform "?
    }

    return "";
}
