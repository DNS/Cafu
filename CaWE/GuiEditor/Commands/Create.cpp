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

#include "Create.hpp"
#include "../GuiDocument.hpp"
#include "../../LuaAux.hpp"

#include "GuiSys/Window.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "Math3D/Vector3.hpp"


using namespace GuiEditor;


CommandCreateT::CommandCreateT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Parent)
    : m_GuiDocument(GuiDocument),
      m_Parent(Parent),
      m_NewWindow(NULL),
      m_OldSelection(m_GuiDocument->GetSelection())
{
    cf::GuiSys::WindowCreateParamsT CreateParams(*m_GuiDocument->GetGui());

    m_NewWindow = new cf::GuiSys::WindowT(CreateParams);

    std::string  WinName = m_NewWindow->GetType()->ClassName;
    const size_t len     = WinName.length();

    if (len > 1 && WinName[len-1] == 'T')
    {
        // Remove the trailing "T" from our class name.
        WinName = std::string(WinName, 0, len-1);
    }

    m_NewWindow->SetName(CheckLuaIdentifier(WinName).ToStdString());

    // Set a window default size and center it on its parent.
    // If the size is larger than parentsize/2, set it to parentsize/2.
    const Vector2fT Size(std::min(m_Parent->GetSize().x/2.0f, 100.0f),
                         std::min(m_Parent->GetSize().y/2.0f,  50.0f));

    m_NewWindow->SetPos((m_Parent->GetSize() - Size) / 2.0f);
    m_NewWindow->SetSize(Size);

    GuiDocumentT::CreateSibling(m_NewWindow, m_GuiDocument);
}


bool CommandCreateT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    m_Parent->AddChild(m_NewWindow);

    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > NewSelection;
    NewSelection.PushBack(m_NewWindow);

    m_GuiDocument->SetSelection(NewSelection);

    m_GuiDocument->UpdateAllObservers_Created(m_NewWindow);
    m_GuiDocument->UpdateAllObservers_SelectionChanged(m_OldSelection, NewSelection);

    m_Done=true;
    return true;
}


void CommandCreateT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Parent->RemoveChild(m_NewWindow);

    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > NewSelection;
    NewSelection.PushBack(m_NewWindow);

    m_GuiDocument->SetSelection(m_OldSelection);

    m_GuiDocument->UpdateAllObservers_Deleted(m_NewWindow);
    m_GuiDocument->UpdateAllObservers_SelectionChanged(NewSelection, m_OldSelection);

    m_Done=false;
}


wxString CommandCreateT::GetName() const
{
    return "Create new window";
}
