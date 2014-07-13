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

#ifndef CAFU_MAPEDITOR_DIALOG_ENTITY_HIERARCHY_HPP_INCLUDED
#define CAFU_MAPEDITOR_DIALOG_ENTITY_HIERARCHY_HPP_INCLUDED

#include "ObserverPattern.hpp"

#include "wx/treectrl.h"


class ChildFrameT;
class MapDocumentT;


namespace MapEditor
{
    class EntityHierarchyDialogT : public wxTreeCtrl, public ObserverT
    {
        public:

        EntityHierarchyDialogT(ChildFrameT* Parent, const wxSize& Size);
        ~EntityHierarchyDialogT();

        // Implementation of the ObserverT interface.
        void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
        void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail);
        void Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var);
        void NotifySubjectDies(SubjectT* dyingSubject);

        /// Redraws the whole tree.
        void RefreshTree();


        private:

        void AddChildren(const wxTreeItemId& Item, bool Recursive);

        /// Recursively searches the tree for an item associated with a specified EntityT.
        /// @param StartingItem   Item to start the recursive search at.
        /// @param Entity         The entity whose tree item we are interested in.
        const wxTreeItemId FindTreeItem(const wxTreeItemId& StartingItem, IntrusivePtrT<cf::GameSys::EntityT> Entity) const;

        /// Recursively gets all tree items, beginning with the passed tree item.
        void GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items);

        /// Updates the labels of all items in the tree.
        void UpdateAllLabels();

        MapDocumentT*                       m_MapDoc;
        ChildFrameT*                        m_Parent;
        bool                                m_IsRecursiveSelfNotify;
        IntrusivePtrT<cf::GameSys::EntityT> m_DraggedEntity;

        void OnKeyDown           (wxKeyEvent&   KE);
        void OnTreeLeftClick     (wxMouseEvent& ME);
        void OnSelectionChanged  (wxTreeEvent&  TE);
        void OnBeginLabelEdit    (wxTreeEvent&  TE);
        void OnEndLabelEdit      (wxTreeEvent&  TE);
        void OnTreeItemRightClick(wxTreeEvent&  TE);
        void OnBeginDrag         (wxTreeEvent&  TE);
        void OnEndDrag           (wxTreeEvent&  TE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
