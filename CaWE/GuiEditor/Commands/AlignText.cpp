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

#include "AlignText.hpp"

#include "../GuiDocument.hpp"

#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandAlignTextHorT::CommandAlignTextHorT(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, int Alignment)
    : m_GuiDocument(GuiDocument),
      m_Windows(Windows),
      m_NewAlignment(Alignment)
{
    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        m_OldAlignments.PushBack(m_Windows[WinNr]->TextAlignHor);
}


bool CommandAlignTextHorT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        m_Windows[WinNr]->TextAlignHor=(cf::GuiSys::WindowT::TextAlignHorT)m_NewAlignment;

    m_GuiDocument->UpdateAllObservers_Modified(m_Windows, WMD_HOR_TEXT_ALIGN);

    m_Done=true;

    return true;
}


void CommandAlignTextHorT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        m_Windows[WinNr]->TextAlignHor=(cf::GuiSys::WindowT::TextAlignHorT)m_OldAlignments[WinNr];

    m_GuiDocument->UpdateAllObservers_Modified(m_Windows, WMD_HOR_TEXT_ALIGN);

    m_Done=false;
}


wxString CommandAlignTextHorT::GetName() const
{
    wxString CommandName="Align text ";

    switch (m_NewAlignment)
    {
        case 0:  CommandName+="left";   break;
        case 1:  CommandName+="right";  break;
        case 2:  CommandName+="center"; break;
        default:                        break;
    }

    return CommandName;
}
