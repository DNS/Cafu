/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DeleteComponent.hpp"
#include "../GuiDocument.hpp"

#include "GuiSys/CompBase.hpp"
#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandDeleteComponentT::CommandDeleteComponentT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window, unsigned long Index)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_Component(m_Window->GetComponents()[Index]),
      m_Index(Index)
{
}


bool CommandDeleteComponentT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    m_Window->DeleteComponent(m_Index);

    // TODO: Can we be more specific?
    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_GENERIC);

    m_Done=true;
    return true;
}


void CommandDeleteComponentT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // No checking the return type, it simply *must* work.
    m_Window->AddComponent(m_Component, m_Index);

    // TODO: Can we be more specific?
    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_GENERIC);

    m_Done=false;
}


wxString CommandDeleteComponentT::GetName() const
{
    return wxString("Delete component: ") + m_Component->GetName();
}
