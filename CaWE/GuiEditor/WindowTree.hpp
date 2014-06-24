/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_GUIEDITOR_WINDOW_TREE_HPP_INCLUDED
#define CAFU_GUIEDITOR_WINDOW_TREE_HPP_INCLUDED

#include "ObserverPattern.hpp"

#include "wx/treectrl.h"


namespace cf { namespace GuiSys { class GuiImplT; } }
namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class ChildFrameT;
    class GuiDocumentT;

    class WindowTreeT : public wxTreeCtrl, public ObserverT
    {
        public:

        WindowTreeT(ChildFrameT* Parent, const wxSize& Size);
        ~WindowTreeT();

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
