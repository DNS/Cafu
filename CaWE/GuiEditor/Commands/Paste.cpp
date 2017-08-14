/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Paste.hpp"
#include "../GuiDocument.hpp"


using namespace GuiEditor;


CommandPasteT::CommandPasteT(GuiDocumentT* GuiDocument, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, IntrusivePtrT<cf::GuiSys::WindowT> NewParent)
    : m_GuiDocument(GuiDocument),
      m_NewParent(NewParent)
{
    // Have to clone the windows here, so the command is the owner of these windows.
    for (unsigned long WinNr=0; WinNr<Windows.Size(); WinNr++)
    {
        m_Windows.PushBack(new cf::GuiSys::WindowT(*Windows[WinNr], true));

        m_Windows[WinNr]->GetTransform()->SetPos(Vector2fT((WinNr + 1) * 20.0f, (WinNr + 1) * 10.0f));
    }
}


bool CommandPasteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        if (m_GuiDocument->GetGui() != &m_Windows[WinNr]->GetGui())
        {
            wxMessageBox("Sorry, cannot copy windows from one GUI document and paste them into another at this time.");
            return false;
        }

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        m_NewParent->AddChild(m_Windows[WinNr]);

    m_GuiDocument->UpdateAllObservers_Created(m_Windows);

    m_Done=true;
    return true;
}


void CommandPasteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    for (unsigned long WinNr=0; WinNr<m_Windows.Size(); WinNr++)
        m_NewParent->RemoveChild(m_Windows[WinNr]);

    m_GuiDocument->UpdateAllObservers_Deleted(m_Windows);

    m_Done=false;
}


wxString CommandPasteT::GetName() const
{
    if (m_Windows.Size()>1)
        return "Paste windows";
    else
        return "Paste window";
}
