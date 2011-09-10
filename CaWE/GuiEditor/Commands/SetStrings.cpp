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

#include "SetStrings.hpp"
#include "../GuiDocument.hpp"


using namespace GuiEditor;


CommandSetStringsT::CommandSetStringsT(GuiDocumentT* GuiDoc, const EditorWindowT* Win, const wxString& PropertyName,
    ArrayT<std::string>& Strings, const ArrayT<std::string>& NewStrings)
    : m_GuiDoc(GuiDoc),
      m_Win(Win),
      m_PropertyName(PropertyName),
      m_Strings(Strings),
      m_NewStrings(NewStrings),
      m_OldStrings(Strings)
{
}


bool CommandSetStringsT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the next sequence didn't really change, don't put this command into the command history.
    // if (m_NewNext==m_OldNext) return false;

    m_Strings=m_NewStrings;

    m_GuiDoc->UpdateAllObservers_Modified(m_Win, m_PropertyName);
    m_Done=true;
    return true;
}


void CommandSetStringsT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Strings=m_OldStrings;

    m_GuiDoc->UpdateAllObservers_Modified(m_Win, m_PropertyName);
    m_Done=false;
}


wxString CommandSetStringsT::GetName() const
{
    return "Set strings for " + m_PropertyName;
}
