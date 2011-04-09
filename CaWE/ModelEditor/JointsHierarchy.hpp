/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _MODELEDITOR_JOINTS_HIERARCHY_HPP_
#define _MODELEDITOR_JOINTS_HIERARCHY_HPP_

#include "ObserverPattern.hpp"
#include "Templates/Array.hpp"
#include "wx/treectrl.h"


namespace ModelEditor
{
    class ChildFrameT;
    class ModelDocumentT;


    class JointsHierarchyT : public wxTreeCtrl, public ObserverT
    {
        public:

        JointsHierarchyT(ChildFrameT* Parent, const wxSize& Size);
        ~JointsHierarchyT();

        // ObserverT implementation.
        // void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection);
        void Notify_JointChanged(SubjectT* Subject, unsigned int JointNr);
        void Notify_SubjectDies(SubjectT* dyingSubject);

        /// Redraws the whole tree.
        void RefreshTree();


        private:

        /// Recusively searches the tree for an item associated with a specified WindowT.
        /// @param StartingItem Item to start the recursive search at.
        /// @param JointNr      The number of the joint whose tree item we are interested in.
        const wxTreeItemId FindTreeItem(const wxTreeItemId& StartingItem, unsigned int JointNr) const;

        /// Recursively gets all tree items, beginning with the passed tree item.
        void GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items);

        ModelDocumentT* m_ModelDocument;
        ChildFrameT*    m_Parent;
        bool            m_IsRecursiveSelfNotify;

        void OnKeyDown           (wxKeyEvent&   KE);
     // void OnTreeLeftClick     (wxMouseEvent& ME);
     // void OnSelectionChanged  (wxTreeEvent&  TE);
        void OnLabelChanged      (wxTreeEvent&  TE);
        void OnTreeItemRightClick(wxTreeEvent&  TE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
