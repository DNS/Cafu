/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Create.hpp"
#include "../GuiDocument.hpp"

#include "GuiSys/Window.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include <algorithm>


using namespace GuiEditor;


CommandCreateT::CommandCreateT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Parent)
    : m_GuiDocument(GuiDocument),
      m_Parent(Parent),
      m_NewWindow(NULL),
      m_OldSelection(m_GuiDocument->GetSelection())
{
    cf::GuiSys::WindowCreateParamsT CreateParams(*m_GuiDocument->GetGui());

    m_NewWindow = new cf::GuiSys::WindowT(CreateParams);

    // Set a window default size and center it on its parent.
    // If the size is larger than parentsize/2, set it to parentsize/2.
    const Vector2fT Size(std::min(m_Parent->GetTransform()->GetSize().x/2.0f, 100.0f),
                         std::min(m_Parent->GetTransform()->GetSize().y/2.0f,  50.0f));

    m_NewWindow->GetTransform()->SetPos((m_Parent->GetTransform()->GetSize() - Size) / 2.0f);
    m_NewWindow->GetTransform()->SetSize(Size);
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
