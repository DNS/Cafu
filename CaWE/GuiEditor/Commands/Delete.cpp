/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Delete.hpp"
#include "Select.hpp"
#include "../GuiDocument.hpp"
#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandDeleteT::CommandDeleteT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window)
    : m_GuiDocument(GuiDocument),
      m_CommandSelect(NULL)
{
    // The root window cannot be deleted.
    if (Window != Window->GetRoot())
    {
        m_Windows.PushBack(Window);
        m_Parents.PushBack(Window->GetParent());
        m_Indices.PushBack(-1);
    }

    m_CommandSelect=CommandSelectT::Remove(m_GuiDocument, m_Windows);
}


CommandDeleteT::CommandDeleteT(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
    : m_GuiDocument(GuiDocument),
      m_CommandSelect(NULL)
{
    for (unsigned long WinNr=0; WinNr<Windows.Size(); WinNr++)
    {
        IntrusivePtrT<cf::GuiSys::WindowT> Window = Windows[WinNr];

        // The root window cannot be deleted.
        if (Window != Window->GetRoot())
        {
            m_Windows.PushBack(Window);
            m_Parents.PushBack(Window->GetParent());
            m_Indices.PushBack(-1);
        }
    }

    m_CommandSelect=CommandSelectT::Remove(m_GuiDocument, m_Windows);
}


CommandDeleteT::~CommandDeleteT()
{
    delete m_CommandSelect;
}


bool CommandDeleteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Fail if there are no windows to delete.
    if (m_Windows.Size()==0) return false;

    // Deselect any affected windows that are selected.
    m_CommandSelect->Do();

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
    {
        IntrusivePtrT<cf::GuiSys::WindowT> Window = m_Windows[WinNr];

        wxASSERT(m_Parents[WinNr]->GetChildren().Find(Window) >= 0);

        // The proper index number can only be determined here, because removing a child
        // may change the index numbers of its siblings.
        m_Indices[WinNr] = m_Parents[WinNr]->GetChildren().Find(Window);

        m_Parents[WinNr]->RemoveChild(Window);
    }

    m_GuiDocument->UpdateAllObservers_Deleted(m_Windows);

    m_Done=true;
    return true;
}


void CommandDeleteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    for (unsigned long RevNr=0; RevNr<m_Windows.Size(); RevNr++)
    {
        const unsigned long WinNr = m_Windows.Size()-RevNr-1;

        // This call to AddChild() should never see a reason to modify the name of the m_Windows[WinNr]
        // to make it unique among its siblings -- it used to be there and was unique, after all.
        m_Parents[WinNr]->AddChild(m_Windows[WinNr], m_Indices[WinNr]);
    }

    m_GuiDocument->UpdateAllObservers_Created(m_Windows);

    // Select the previously selected windows again (unless the command failed on Do(), which can happen e.g. on unchanged selection).
    if (m_CommandSelect->IsDone()) m_CommandSelect->Undo();

    m_Done=false;
}


wxString CommandDeleteT::GetName() const
{
    return (m_Windows.Size()==1) ? "Delete 1 window" : wxString::Format("Delete %lu windows", m_Windows.Size());
}
