/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "Scale.hpp"

#include "../GuiDocument.hpp"

#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandScaleT::CommandScaleT(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, const ArrayT<Vector2fT>& Positions, const ArrayT<Vector2fT>& Sizes, bool Done)
    : m_GuiDocument(GuiDocument),
      m_Windows(Windows)
{
    m_Done=Done; // Set command do/undo state.

    if (m_Done)
    {
        m_OldPositions=Positions;
        m_OldSizes    =Sizes;

        for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        {
            m_NewPositions.PushBack(m_Windows[WinNr]->GetPos());
            m_NewSizes    .PushBack(m_Windows[WinNr]->GetSize());
        }

        // If the command has already been completed we have to send an observer message so the observers can update themselves accordingly.
        m_GuiDocument->UpdateAllObservers_Modified(m_Windows, WMD_TRANSFORMED);
    }
    else
    {
        m_NewPositions=Positions;
        m_NewSizes    =Sizes;

        for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        {
            m_OldPositions.PushBack(m_Windows[WinNr]->GetPos());
            m_OldSizes    .PushBack(m_Windows[WinNr]->GetSize());
        }
    }

    wxASSERT(m_OldPositions.Size()==m_NewPositions.Size());
    wxASSERT(m_OldSizes.Size()==m_NewSizes.Size());
}


bool CommandScaleT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
    {
        m_Windows[WinNr]->SetPos(m_NewPositions[WinNr]);
        m_Windows[WinNr]->SetSize(m_NewSizes[WinNr]);
    }

    m_GuiDocument->UpdateAllObservers_Modified(m_Windows, WMD_TRANSFORMED);

    m_Done=true;

    return true;
}


void CommandScaleT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
    {
        m_Windows[WinNr]->SetPos(m_OldPositions[WinNr]);
        m_Windows[WinNr]->SetSize(m_OldSizes[WinNr]);
    }

    m_GuiDocument->UpdateAllObservers_Modified(m_Windows, WMD_TRANSFORMED);

    m_Done=false;
}


wxString CommandScaleT::GetName() const
{
    return "Scale windows";
}
