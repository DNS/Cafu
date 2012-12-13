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

#include "AddComponent.hpp"
#include "../GuiDocument.hpp"

#include "GuiSys/CompBase.hpp"
#include "GuiSys/Window.hpp"
#include "TypeSys.hpp"


using namespace GuiEditor;


CommandAddComponentT::CommandAddComponentT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window, const cf::TypeSys::TypeInfoT* TI)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_Component(NULL),
      m_Name("Add component")
{
    m_Component = static_cast<cf::GuiSys::ComponentBaseT*>(
        TI->CreateInstance(
            cf::TypeSys::CreateParamsT()));

    if (m_Component != NULL)
    {
        m_Name += ": ";
        m_Name += m_Component->GetName();
    }
}


bool CommandAddComponentT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    wxASSERT(m_Component != NULL);
    if (m_Component == NULL) return false;

    if (!m_Window->AddComponent(m_Component)) return false;
    m_Component = NULL;

    // TODO: Can we be more specific?
    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_GENERIC);

    m_Done=true;
    return true;
}


void CommandAddComponentT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    const unsigned long CompNr = m_Window->GetComponents().Size() - 1;

    m_Component = m_Window->GetComponents()[CompNr];
    m_Window->DeleteComponent(CompNr);

    // TODO: Can we be more specific?
    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_GENERIC);

    m_Done=false;
}


wxString CommandAddComponentT::GetName() const
{
    return m_Name;
}
