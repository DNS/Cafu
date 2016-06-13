/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "AddComponent.hpp"
#include "../GuiDocument.hpp"

#include "GuiSys/CompBase.hpp"
#include "GuiSys/Window.hpp"

#include <algorithm>


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
