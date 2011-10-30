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

#include "SetWinProp.hpp"
#include "../GuiDocument.hpp"


using namespace GuiEditor;


template<class T>
CommandSetWinPropT<T>::CommandSetWinPropT(GuiDocumentT* GuiDoc, const EditorWindowT* Win, const wxString& PropName, T& Value, const T& NewValue)
    : m_GuiDoc(GuiDoc),
      m_Win(Win),
      m_PropName(PropName),
      m_Value(Value),
      m_NewValue(NewValue),
      m_OldValue(Value)
{
}


template<class T>
bool CommandSetWinPropT<T>::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the new value isn't different from the old, don't put this command into the command history.
    // if (m_NewValue==m_OldValue) return false;

    m_Value=m_NewValue;

    m_GuiDoc->UpdateAllObservers_Modified(m_Win, m_PropName);
    m_Done=true;
    return true;
}


template<class T>
void CommandSetWinPropT<T>::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Value=m_OldValue;

    m_GuiDoc->UpdateAllObservers_Modified(m_Win, m_PropName);
    m_Done=false;
}


template<class T>
wxString CommandSetWinPropT<T>::GetName() const
{
    return "Set " + m_PropName;
}


namespace cf { class TrueTypeFontT; }

template class CommandSetWinPropT< ArrayT<std::string> >;
template class CommandSetWinPropT< cf::TrueTypeFontT* >;
