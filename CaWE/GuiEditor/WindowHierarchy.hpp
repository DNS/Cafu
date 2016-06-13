/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_WINDOW_HIERARCHY_HPP_INCLUDED
#define CAFU_GUIEDITOR_WINDOW_HIERARCHY_HPP_INCLUDED

#include "ObserverPattern.hpp"

#include "wx/treectrl.h"


namespace cf { namespace GuiSys { class GuiImplT; } }
namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class ChildFrameT;
    class GuiDocumentT;

    class WindowHierarchyT : public wxTreeCtrl, public ObserverT
    {
        public:

        WindowHierarchyT(ChildFrameT* Parent, const wxSize& Size);
        ~WindowHierarchyT();

        // ObserverT implementation.
        void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection);
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail);
        void Notify_Changed(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var);
        void NotifySubjectDies(SubjectT* dyingSubject);

        /// Redraws the whole tree.
        void RefreshTree();


        private:

        void AddChildren(const wxTreeItemId& Item, bool Recursive);

        /// Recusively searches the tree for an item associated with a specified WindowT.
        /// @param StartingItem   Item to start the recursive search at.
        /// @param Window         The window whose tree item we are interested in.
        const wxTreeItemId FindTreeItem(const wxTreeItemId& StartingItem, IntrusivePtrT<cf::GuiSys::WindowT> Window) const;

        /// Recursively gets all tree items, beginning with the passed tree item.
        void GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items);

        GuiDocumentT*                      m_GuiDocument;
        ChildFrameT*                       m_Parent;
        bool                               m_IsRecursiveSelfNotify;
        IntrusivePtrT<cf::GuiSys::WindowT> m_DraggedWindow;

        void OnKeyDown           (wxKeyEvent&   KE);
        void OnTreeLeftClick     (wxMouseEvent& ME);
        void OnSelectionChanged  (wxTreeEvent&  TE);
        void OnEndLabelEdit      (wxTreeEvent&  TE);
        void OnTreeItemRightClick(wxTreeEvent&  TE);
        void OnBeginDrag         (wxTreeEvent&  TE);
        void OnEndDrag           (wxTreeEvent&  TE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
