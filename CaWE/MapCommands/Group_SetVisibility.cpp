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

#include "Group_SetVisibility.hpp"
#include "Select.hpp"
#include "../Group.hpp"
#include "../MapDocument.hpp"
#include "../MapElement.hpp"


CommandGroupSetVisibilityT::CommandGroupSetVisibilityT(MapDocumentT& MapDoc, GroupT* Group, bool NewVis)
    : CommandT(true /*ShowInHistory*/, false /*SuggestsSave*/),
      m_MapDoc(MapDoc),
      m_Group(Group),
      m_NewVis(NewVis),
      m_CommandReduceSel(NULL)
{
    // Verify that m_Group is actually in the m_MapDoc.GetGroups().
    wxASSERT(m_MapDoc.GetGroups().Find(m_Group)>=0);

    // Determine ahead of time which currently selected map elements will stay visible and thus stay in the selection.
    // Thus, selected map elements that become insivisible are removed from the selection.
    // Should there be older invisible elements in the selection (as a result of buggy code elsewhere, should never happen),
    // they are unselected as well.
    const bool OldVis=m_Group->IsVisible;
    m_Group->IsVisible=NewVis;

    if (!NewVis)    // Assume that *showing* something cannot reduce the selection.
    {
        const ArrayT<MapElementT*>& OldSelection=m_MapDoc.GetSelection();
        ArrayT<MapElementT*>        NewSelection;

        for (unsigned long SelNr=0; SelNr<OldSelection.Size(); SelNr++)
        {
            MapElementT* Elem=OldSelection[SelNr];

            if (Elem->IsVisible())
                NewSelection.PushBack(Elem);
        }

        // If not all elements in the current selection stay visible, create a command that reduces the selection appropriately.
        if (NewSelection.Size()<OldSelection.Size())
            m_CommandReduceSel=CommandSelectT::Set(&m_MapDoc, NewSelection);
    }

    m_Group->IsVisible=OldVis;  // Restore the old state for now (until Do()).
}


CommandGroupSetVisibilityT::~CommandGroupSetVisibilityT()
{
    delete m_CommandReduceSel;
}


bool CommandGroupSetVisibilityT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Verify that m_Group->IsVisible!=m_NewVis.
    wxASSERT(m_Group->IsVisible!=m_NewVis);
    if (m_Group->IsVisible==m_NewVis) return false;

    // If the new visibility status hides some selected map elements, reduce the selection appropriately
    // (unselect those elements that become invisible, keep only those in the selection that stay visible).
    if (m_CommandReduceSel) m_CommandReduceSel->Do();

    // Set the new visibility
    // and notify all observers that our groups inventory changed.
    m_Group->IsVisible=m_NewVis;
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    // The elements in the group have changed visibility, so update all observers accordingly.
    const ArrayT<MapElementT*> GroupElems=m_Group->GetMembers(m_MapDoc);

    if (GroupElems.Size()>0)
        m_MapDoc.UpdateAllObservers_Modified(GroupElems, MEMD_VISIBILITY);

    m_Done=true;
    return true;
}


void CommandGroupSetVisibilityT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Verify that m_Group->IsVisible==m_NewVis.
    wxASSERT(m_Group->IsVisible==m_NewVis);

    // Restore the old visibility
    // and notify all observers that our groups inventory changed.
    m_Group->IsVisible=!m_NewVis;
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    // The elements in the group have changed visibility, so update all observers accordingly.
    const ArrayT<MapElementT*> GroupElems=m_Group->GetMembers(m_MapDoc);

    if (GroupElems.Size()>0)
        m_MapDoc.UpdateAllObservers_Modified(GroupElems, MEMD_VISIBILITY);

    // Restore the selection.
    if (m_CommandReduceSel) m_CommandReduceSel->Undo();

    m_Done=false;
}


wxString CommandGroupSetVisibilityT::GetName() const
{
    return m_NewVis ? "Show group" : "Hide group";
}
