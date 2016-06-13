/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Select.hpp"


using namespace ModelEditor;


CommandSelectT* CommandSelectT::Clear(ModelDocumentT* ModelDoc, ModelElementTypeT Type)
{
    ArrayT<unsigned int> EmptySelection;

    return new CommandSelectT(ModelDoc, Type, ModelDoc->GetSelection(Type), EmptySelection);
}


CommandSelectT* CommandSelectT::Add(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Elems)
{
    ArrayT<unsigned int> OldSel(ModelDoc->GetSelection(Type));
    ArrayT<unsigned int> NewSel(ModelDoc->GetSelection(Type));

    // For each element, check if it is already part of the current selection.
    for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
    {
        unsigned long SelectionNr=0;

        for (SelectionNr=0; SelectionNr<OldSel.Size(); SelectionNr++)
            if (Elems[ElemNr]==OldSel[SelectionNr]) break;

        // Elem is not part of the current selection.
        if (SelectionNr==OldSel.Size()) NewSel.PushBack(Elems[ElemNr]);
    }

    return new CommandSelectT(ModelDoc, Type, OldSel, NewSel);
}


CommandSelectT* CommandSelectT::Add(ModelDocumentT* ModelDoc, ModelElementTypeT Type, unsigned int Elem)
{
    ArrayT<unsigned int> AddSelection;
    AddSelection.PushBack(Elem);

    return CommandSelectT::Add(ModelDoc, Type, AddSelection);
}


CommandSelectT* CommandSelectT::Remove(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Elems)
{
    ArrayT<unsigned int> NewSel(ModelDoc->GetSelection(Type));

    // For each element, check if it is already part of the current selection.
    for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
    {
        for (unsigned long SelectionNr=0; SelectionNr<NewSel.Size(); SelectionNr++)
        {
            // Elem is part of the current selection.
            if (Elems[ElemNr]==NewSel[SelectionNr])
            {
                NewSel.RemoveAtAndKeepOrder(SelectionNr);
                SelectionNr--; // The current position has to be checked again.
                break;
            }
        }
    }

    return new CommandSelectT(ModelDoc, Type, ModelDoc->GetSelection(Type), NewSel);
}


CommandSelectT* CommandSelectT::Remove(ModelDocumentT* ModelDoc, ModelElementTypeT Type, unsigned int Elem)
{
    ArrayT<unsigned int> RemoveSelection;
    RemoveSelection.PushBack(Elem);

    return CommandSelectT::Remove(ModelDoc, Type, RemoveSelection);
}


CommandSelectT* CommandSelectT::Set(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Elems)
{
    return new CommandSelectT(ModelDoc, Type, ModelDoc->GetSelection(Type), Elems);
}


CommandSelectT::CommandSelectT(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
    : CommandT(abs(int(OldSel.Size())-int(NewSel.Size()))>3, false), // Only show selection command in the undo/redo history if selection difference is greater 3.
      m_ModelDoc(ModelDoc),
      m_Type(Type),
      m_OldSel(OldSel),
      m_NewSel(NewSel)
{
}


bool CommandSelectT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Protect against "extra" EVT_TREE_SEL_CHANGED events, see
    // <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/72754> for details.
    if (m_OldSel.Size()==0 && m_NewSel.Size()==0) return false;
    if (m_OldSel.Size()==1 && m_NewSel.Size()==1 && m_OldSel[0]==m_NewSel[0]) return false;

    m_ModelDoc->SetSelection(m_Type, m_NewSel);

    m_ModelDoc->UpdateAllObservers_SelectionChanged(m_Type, m_OldSel, m_NewSel);
    m_Done=true;
    return true;
}


void CommandSelectT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_ModelDoc->SetSelection(m_Type, m_OldSel);

    m_ModelDoc->UpdateAllObservers_SelectionChanged(m_Type, m_NewSel, m_OldSel);
    m_Done=false;
}


wxString CommandSelectT::GetName() const
{
    return "Selection change";
}
