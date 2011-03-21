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

#include "SnapToGrid.hpp"

#include "../MapDocument.hpp"
#include "../MapElement.hpp"
#include "Math3D/Misc.hpp"


static void SnapToGrid(BoundingBox3fT& BB, int GridSize)
{
    // Does not alter the size of the box - just moves BB so that BB.Min is on the grid.
    const Vector3fT Size=BB.Max-BB.Min;

    for (int i=0; i<3; i++)
        BB.Min[i]=cf::math::round(BB.Min[i]/GridSize)*GridSize;

    BB.Max=BB.Min+Size;
}


CommandSnapToGridT::CommandSnapToGridT(MapDocumentT& MapDoc_, const ArrayT<MapElementT*>& Objects_)
    : SnapObjects(Objects_),
      MapDoc(MapDoc_)
{
    // Get bounding box of the map elements to snap to the grid.
    BoundingBox3fT ElemBB;

    for (unsigned long i=0; i<SnapObjects.Size(); i++)
        ElemBB.InsertValid(SnapObjects[i]->GetBB());

    BoundingBox3fT NewElemBB=ElemBB;
    SnapToGrid(NewElemBB, MapDoc.GetGridSpacing());

    MoveOffset=NewElemBB.Min-ElemBB.Min;

    // Create copies of all objects in their original state for Undo().
    for (unsigned long i=0; i<SnapObjects.Size(); i++)
        OldStates.PushBack(SnapObjects[i]->Clone());
}


CommandSnapToGridT::~CommandSnapToGridT()
{
    for (unsigned long i=0; i<OldStates.Size(); i++)
        delete OldStates[i];

    OldStates.Clear();
}


bool CommandSnapToGridT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    // Do move.
    for (unsigned long i=0; i<SnapObjects.Size(); i++)
    {
        OldBounds.PushBack(SnapObjects[i]->GetBB());
        SnapObjects[i]->TrafoMove(MoveOffset);
    }

    MapDoc.UpdateAllObservers_Modified(SnapObjects, MEMD_TRANSFORM, OldBounds);

    m_Done=true;
    return true;
}


void CommandSnapToGridT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Record the previous bounding-boxes for the observer message.
    ArrayT<BoundingBox3fT> OldBounds;

    // Do move.
    for (unsigned long i=0; i<SnapObjects.Size(); i++)
    {
        OldBounds.PushBack(SnapObjects[i]->GetBB());
        SnapObjects[i]->Assign(OldStates[i]);
    }

    MapDoc.UpdateAllObservers_Modified(SnapObjects, MEMD_TRANSFORM, OldBounds);

    m_Done=false;
}


wxString CommandSnapToGridT::GetName() const
{
    return "snap objects";
}
