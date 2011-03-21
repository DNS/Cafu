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

#include "Align.hpp"
#include "../ChildFrame.hpp"
#include "../MapElement.hpp"
#include "../MapDocument.hpp"


CommandAlignT::CommandAlignT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Elems, const AxesInfoT& RefAxes, const BoundingBox3fT& Box, int Mode)
    : m_MapDoc(MapDoc),
      m_AlignElems(Elems),
      m_RefAxes(RefAxes),
      m_Box(Box),
      m_Mode(CorrectMode(Mode))
{
    for (unsigned long i=0; i<m_AlignElems.Size(); i++)
        m_OldStates.PushBack(m_AlignElems[i]->Clone());
}


CommandAlignT::~CommandAlignT()
{
    for (unsigned long i=0; i<m_OldStates.Size(); i++)
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

        Elem->TrafoMove(MoveOffset);
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
        m_AlignElems[i]->Assign(m_OldStates[i]);
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
