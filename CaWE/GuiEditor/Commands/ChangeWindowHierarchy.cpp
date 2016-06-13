/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChangeWindowHierarchy.hpp"
#include "../GuiDocument.hpp"

#include "GuiSys/Window.hpp"


using namespace GuiEditor;


CommandChangeWindowHierarchyT::CommandChangeWindowHierarchyT(GuiDocumentT* GuiDocument, IntrusivePtrT<cf::GuiSys::WindowT> Window, IntrusivePtrT<cf::GuiSys::WindowT> NewParent, unsigned long NewPosition)
    : m_GuiDocument(GuiDocument),
      m_Window(Window),
      m_NewParent(NewParent),
      m_NewPosition(NewPosition),
      m_OldParent(Window->GetParent()),
      m_OldPosition(!Window->GetParent().IsNull() ? Window->GetParent()->GetChildren().Find(Window) : 0),
      m_OldName(Window->GetBasics()->GetWindowName())
{
    wxASSERT(m_GuiDocument);
    wxASSERT(!m_Window.IsNull());   // m_NewParent==NULL or m_OldParent==NULL are handled below.
}


bool CommandChangeWindowHierarchyT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Make sure that m_NewParent is not in the subtree of m_Window
    // (or else the reparenting would create invalid cycles).
    // Note that we don't check for the NOP case m_NewParent==m_OldParent,
    // that's up to the caller (and not really a reason to return "false"/failure).
    {
        ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > SubTree;

        SubTree.PushBack(m_Window);
        m_Window->GetChildren(SubTree, true /*recurse*/);

        if (SubTree.Find(m_NewParent)>=0) return false;
    }

    if (!m_OldParent.IsNull()) m_OldParent->RemoveChild(m_Window);
    if (!m_NewParent.IsNull()) m_NewParent->AddChild(m_Window, m_NewPosition);

    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_HIERARCHY);
    m_Done=true;
    return true;
}


void CommandChangeWindowHierarchyT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (!m_NewParent.IsNull()) m_NewParent->RemoveChild(m_Window);
    if (!m_OldParent.IsNull()) m_OldParent->AddChild(m_Window, m_OldPosition);

    // The call to AddChild() in Do() might have modified the name of the m_Window as a side-effect.
    // Now that we have restored the previous hierarchy, restore the previous name as well.
    m_Window->GetBasics()->SetWindowName(m_OldName);

    m_GuiDocument->UpdateAllObservers_Modified(m_Window, WMD_HIERARCHY);
    m_Done=false;
}


wxString CommandChangeWindowHierarchyT::GetName() const
{
    return "Change window hierarchy";
}
