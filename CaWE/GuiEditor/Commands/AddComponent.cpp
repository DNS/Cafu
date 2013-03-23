/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "AddComponent.hpp"
#include "../GuiDocument.hpp"

#include "GuiSys/CompBase.hpp"
#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandAddComponentT::CommandAddComponentT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window, IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp, unsigned long Index)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_Component(Comp),
      m_Index(std::min(Index, m_Window->GetComponents().Size()))
{
}


bool CommandAddComponentT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    if (!m_Window->AddComponent(m_Component, m_Index)) return false;

    // TODO: Can we be more specific?
    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_GENERIC);

    m_Done=true;
    return true;
}


void CommandAddComponentT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Window->DeleteComponent(m_Index);

    // TODO: Can we be more specific?
    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_GENERIC);

    m_Done=false;
}


wxString CommandAddComponentT::GetName() const
{
    return wxString("Add component: ") + m_Component->GetName();
}
