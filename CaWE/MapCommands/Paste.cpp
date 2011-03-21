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

#include "Paste.hpp"
#include "Select.hpp"
#include "Group_Assign.hpp"
#include "Group_New.hpp"

#include "../MapDocument.hpp"
#include "../MapEntity.hpp"
#include "../MapPrimitive.hpp"
#include "../Options.hpp"


CommandPasteT::CommandPasteT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Originals, const Vector3fT& OriginalsCenter,
    const Vector3fT& GoodPastePos, const Vector3fT& DeltaTranslation, const cf::math::AnglesfT& DeltaRotation,
    unsigned long NrOfCopies, bool PasteGrouped, bool CenterAtOriginals)
    : m_MapDoc(MapDoc),
      m_PastedElems(),
      m_CommandSelect(NULL),
      m_CommandCreateGroup(NULL),
      m_CommandAssignGroup(NULL)
{
    // Initialize the total translation and rotation for the first paste operation.
    // Note that a TotalTranslation of zero pastes each object in the exact same place as its original.
    Vector3fT          TotalTranslation=DeltaTranslation;
    cf::math::AnglesfT TotalRotation   =DeltaRotation;

    if (!CenterAtOriginals)
    {
        static Vector3fT     LastPastePoint(0, 0, 0);
        static unsigned long LastPasteCount=0;

        if (GoodPastePos!=LastPastePoint)
        {
            LastPastePoint=GoodPastePos;
            LastPasteCount=0;
        }

        float PasteOffset=m_MapDoc.GetGridSpacing();
        if (PasteOffset<1.0f) PasteOffset=1.0f;
        while (PasteOffset<8.0f) PasteOffset*=2.0f;     // Make PasteOffset some multiple of the grid spacing larger than 8.0.

        TotalTranslation=m_MapDoc.SnapToGrid(LastPastePoint+Vector3fT(((LastPasteCount % 8)+(LastPasteCount/8))*PasteOffset, (LastPasteCount % 8)*PasteOffset, 0.0f)-OriginalsCenter, false, -1 /*Snap all axes.*/);

        LastPasteCount++;
    }

    // FIXME: This should probably be a param to the Trafo*() methods, rather than having these methods query it from the global Options.general.LockingTextures.
    const bool PrevLockMats=Options.general.LockingTextures;
    Options.general.LockingTextures=true;

    for (unsigned long CopyNr=0; CopyNr<NrOfCopies; CopyNr++)
    {
        for (unsigned long OriginalNr=0; OriginalNr<Originals.Size(); OriginalNr++)
        {
            MapElementT* Clone=Originals[OriginalNr]->Clone();

            if (TotalTranslation!=Vector3fT())
                Clone->TrafoMove(TotalTranslation);

            if (TotalRotation!=cf::math::AnglesfT())
                Clone->TrafoRotate(Clone->GetBB().GetCenter(), TotalRotation);

            wxASSERT(Clone->GetGroup()==NULL);  // Our pasted elements are initially in no group at all.
            m_PastedElems.PushBack(Clone);
        }

        // Increment the total translation and rotation.
        TotalTranslation+=DeltaTranslation;
        TotalRotation   +=DeltaRotation;
    }

    Options.general.LockingTextures=PrevLockMats;
    m_CommandSelect=CommandSelectT::Set(&MapDoc, m_PastedElems);

    if (PasteGrouped && m_PastedElems.Size()>1)
    {
        // Note that creating commands but not immediately calling their Do() methods is somewhat dangerous - see CommandT documentation.
        // In this particular case, I've checked their ctors and found that this code is actually safe.
        // Generally, however, we should delay the creation of the commands just before their (first) call to Do().
        m_CommandCreateGroup=new CommandNewGroupT(m_MapDoc, wxString::Format("paste group (%lu element%s)", m_PastedElems.Size(), m_PastedElems.Size()==1 ? "" : "s"));
        m_CommandCreateGroup->GetGroup()->SelectAsGroup=true;

        m_CommandAssignGroup=new CommandAssignGroupT(m_MapDoc, m_PastedElems, m_CommandCreateGroup->GetGroup());
    }
}


CommandPasteT::~CommandPasteT()
{
    delete m_CommandAssignGroup;
    delete m_CommandCreateGroup;
    delete m_CommandSelect;

    if (!m_Done)
    {
        for (unsigned long ElemNr=0; ElemNr<m_PastedElems.Size(); ElemNr++)
            delete m_PastedElems[ElemNr];
    }
}


bool CommandPasteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Add the pasted group or individual elements into the world.
    for (unsigned long ElemNr=0; ElemNr<m_PastedElems.Size(); ElemNr++)
    {
        MapPrimitiveT* Prim=dynamic_cast<MapPrimitiveT*>(m_PastedElems[ElemNr]);
        if (Prim)
        {
            m_MapDoc.Insert(Prim, m_MapDoc.GetEntities()[0]);
            continue;
        }

        MapEntityT* Ent=dynamic_cast<MapEntityT*>(m_PastedElems[ElemNr]);
        if (Ent)
        {
            m_MapDoc.Insert(Ent);
            continue;
        }
    }

    m_MapDoc.UpdateAllObservers_Created(m_PastedElems);

    // Select the newly pasted elements.
    m_CommandSelect->Do();

    // Create a new group for the pasted elements.
    if (m_CommandCreateGroup) m_CommandCreateGroup->Do();

    // Put the m_PastedElems into the new group.
    if (m_CommandAssignGroup) m_CommandAssignGroup->Do();

    m_Done=true;
    return true;
}


void CommandPasteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (m_CommandAssignGroup) m_CommandAssignGroup->Undo();
    if (m_CommandCreateGroup) m_CommandCreateGroup->Undo();
    m_CommandSelect->Undo();

    for (unsigned long ElemNr=0; ElemNr<m_PastedElems.Size(); ElemNr++)
        m_MapDoc.Remove(m_PastedElems[ElemNr]);

    m_MapDoc.UpdateAllObservers_Deleted(m_PastedElems);
    m_Done=false;
}


wxString CommandPasteT::GetName() const
{
    if (m_PastedElems.Size()>1)
        return wxString::Format("paste %lu objects", m_PastedElems.Size());

    return "paste object";
}
