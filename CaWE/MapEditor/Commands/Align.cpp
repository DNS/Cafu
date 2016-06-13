/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Align.hpp"
#include "../ChildFrame.hpp"
#include "../MapElement.hpp"
#include "../MapDocument.hpp"


CommandAlignT::CommandAlignT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, const AxesInfoT& RefAxes, const BoundingBox3fT& Box, int Mode, bool LockTexCoords)
    : m_MapDoc(MapDoc),
      m_AlignElems(Elems),
      m_RefAxes(RefAxes),
      m_Box(Box),
      m_Mode(CorrectMode(Mode)),
      m_LockTexCoords(LockTexCoords)
{
    for (unsigned long i = 0; i < m_AlignElems.Size(); i++)
        m_OldStates.PushBack(m_AlignElems[i]->GetTrafoState());
}


CommandAlignT::~CommandAlignT()
{
    for (unsigned long i = 0; i < m_OldStates.Size(); i++)
        delete m_OldStates[i];

    m_OldStates.Clear();
}


bool CommandAlignT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    Vector3fT MoveOffset(0, 0, 0);

    for (unsigned long i=0; i<m_AlignElems.Size(); i++)
    {
        MapElementT* Elem=m_AlignElems[i];

        OldBounds.PushBack(Elem->GetBB());

        const Vector3fT Mins=OldBounds[OldBounds.Size()-1].Min;
        const Vector3fT Maxs=OldBounds[OldBounds.Size()-1].Max;

        const int HorzAxis=m_RefAxes.HorzAxis;
        const int VertAxis=m_RefAxes.VertAxis;

        switch (m_Mode)
        {
            case ChildFrameT::ID_MENU_TOOLS_ALIGN_LEFT:       MoveOffset[HorzAxis] = m_Box.Min[HorzAxis] - Mins[HorzAxis]; break;
            case ChildFrameT::ID_MENU_TOOLS_ALIGN_RIGHT:      MoveOffset[HorzAxis] = m_Box.Max[HorzAxis] - Maxs[HorzAxis]; break;
            case ChildFrameT::ID_MENU_TOOLS_ALIGN_HOR_CENTER: MoveOffset[HorzAxis] =(m_Box.Min[HorzAxis]+m_Box.Max[HorzAxis]-Mins[HorzAxis]-Maxs[HorzAxis])/2; break;
            case ChildFrameT::ID_MENU_TOOLS_ALIGN_TOP:        MoveOffset[VertAxis] = m_Box.Min[VertAxis] - Mins[VertAxis]; break;
            case ChildFrameT::ID_MENU_TOOLS_ALIGN_BOTTOM:     MoveOffset[VertAxis] = m_Box.Max[VertAxis] - Maxs[VertAxis]; break;
            default /* ID_MENU_TOOLS_ALIGN_VERT_CENTER */:    MoveOffset[VertAxis] =(m_Box.Min[VertAxis]+m_Box.Max[VertAxis]-Mins[VertAxis]-Maxs[VertAxis])/2; break;
        }

        Elem->TrafoMove(MoveOffset, m_LockTexCoords);
    }

    m_MapDoc.UpdateAllObservers_Modified(m_AlignElems, MEMD_TRANSFORM, OldBounds);

    m_Done=true;
    return true;
}


void CommandAlignT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    for (unsigned long i=0; i<m_AlignElems.Size(); i++)
    {
        OldBounds.PushBack(m_AlignElems[i]->GetBB());
        m_AlignElems[i]->RestoreTrafoState(m_OldStates[i]);
    }

    m_MapDoc.UpdateAllObservers_Modified(m_AlignElems, MEMD_TRANSFORM, OldBounds);

    m_Done=false;
}


wxString CommandAlignT::GetName() const
{
    switch (m_Mode)
    {
        case ChildFrameT::ID_MENU_TOOLS_ALIGN_LEFT:       return "Align left";
        case ChildFrameT::ID_MENU_TOOLS_ALIGN_RIGHT:      return "Align right";
        case ChildFrameT::ID_MENU_TOOLS_ALIGN_HOR_CENTER: return "Align horizontal center";
        case ChildFrameT::ID_MENU_TOOLS_ALIGN_TOP:        return "Align top";
        case ChildFrameT::ID_MENU_TOOLS_ALIGN_BOTTOM:     return "Align bottom";
    }

    // ChildFrameT::ID_MENU_TOOLS_ALIGN_VERT_CENTER
    return "Align vertical center";
}


int CommandAlignT::CorrectMode(int Mode) const
{
         if (Mode==ChildFrameT::ID_MENU_TOOLS_ALIGN_TOP    && m_RefAxes.MirrorVert) return ChildFrameT::ID_MENU_TOOLS_ALIGN_BOTTOM;
    else if (Mode==ChildFrameT::ID_MENU_TOOLS_ALIGN_BOTTOM && m_RefAxes.MirrorVert) return ChildFrameT::ID_MENU_TOOLS_ALIGN_TOP;
    else if (Mode==ChildFrameT::ID_MENU_TOOLS_ALIGN_LEFT   && m_RefAxes.MirrorHorz) return ChildFrameT::ID_MENU_TOOLS_ALIGN_RIGHT;
    else if (Mode==ChildFrameT::ID_MENU_TOOLS_ALIGN_RIGHT  && m_RefAxes.MirrorHorz) return ChildFrameT::ID_MENU_TOOLS_ALIGN_LEFT;

    return Mode;
}
