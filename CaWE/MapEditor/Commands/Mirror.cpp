/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Mirror.hpp"
#include "../MapElement.hpp"
#include "../MapDocument.hpp"


CommandMirrorT::CommandMirrorT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, unsigned int NormalAxis, float Dist, bool LockTexCoords)
    : m_MapDoc(MapDoc),
      m_MirrorElems(Elems),
      m_OldStates(),
      m_NormalAxis(NormalAxis),
      m_Dist(Dist),
      m_LockTexCoords(LockTexCoords)
{
    for (unsigned long i = 0; i < m_MirrorElems.Size(); i++)
        m_OldStates.PushBack(m_MirrorElems[i]->GetTrafoState());
}


CommandMirrorT::~CommandMirrorT()
{
    for (unsigned long i = 0; i < m_OldStates.Size(); i++)
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
        m_MirrorElems[i]->TrafoMirror(m_NormalAxis, m_Dist, m_LockTexCoords);
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
        m_MirrorElems[i]->RestoreTrafoState(m_OldStates[i]);
    }

    m_MapDoc.UpdateAllObservers_Modified(m_MirrorElems, MEMD_TRANSFORM, OldBounds);

    m_Done=false;
}


wxString CommandMirrorT::GetName() const
{
    const wxString AxesNames[]={ "x", "y", "z" };

    return "Mirror in "+AxesNames[m_NormalAxis % 3]+"-direction";
}
