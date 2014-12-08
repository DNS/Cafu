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

#include "wx/panel.h"
#include "wx/dataview.h"
#include "wx/treectrl.h"


class ChildFrameT;
class MapDocumentT;


namespace MapEditor
{
    class EntityHierarchyDialogT : public wxTreeCtrl, public ObserverT
    {
        public:

        EntityHierarchyDialogT(ChildFrameT* Parent, wxWindow* WinParent);
        ~EntityHierarchyDialogT();

        // Implementation of the ObserverT interface.
        void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection) override;
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities) override;
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives) override;
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities) override;
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives) override;
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail) override;
        void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail) override;
        void Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var) override;
        void NotifySubjectDies(SubjectT* dyingSubject) override;

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


    class EntityHierarchyModelT : public wxDataViewModel
    {
        public:

        enum ColumnT
        {
            COLUMN_ENTITY_NAME = 0,
            COLUMN_NUM_PRIMITIVES,
            NR_OF_COLUMNS
        };

        EntityHierarchyModelT(IntrusivePtrT<cf::GameSys::EntityT> RootEntity);


        private:

        // Override / implement base class methods.
        unsigned int GetColumnCount() const override;
        wxString     GetColumnType(unsigned int col) const override;

        void GetValue(wxVariant& Variant, const wxDataViewItem& Item, unsigned int col) const override;
        bool SetValue(const wxVariant& Variant, const wxDataViewItem& Item, unsigned int col) override;

        wxDataViewItem GetParent(const wxDataViewItem& Item) const override;
        bool           IsContainer(const wxDataViewItem& Item) const override;
        bool           HasContainerColumns(const wxDataViewItem& Item) const override;
        unsigned int   GetChildren(const wxDataViewItem& Item, wxDataViewItemArray& Children) const override;

        IntrusivePtrT<cf::GameSys::EntityT> m_RootEntity;   ///< The root of the entity hierarchy.
    };


    class EntityHierarchyCtrlT : public wxDataViewCtrl, public ObserverT
    {
        public:

        EntityHierarchyCtrlT(ChildFrameT* MainFrame, wxWindow* Parent);
        ~EntityHierarchyCtrlT();

        // Implementation of the ObserverT interface.
        void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection) override;
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities) override;
        void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives) override;
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities) override;
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives) override;
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail) override;
        void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail) override;
        void Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var) override;
        void NotifySubjectDies(SubjectT* dyingSubject) override;


        private:

        ChildFrameT*                        m_MainFrame;
        MapDocumentT*                       m_MapDoc;
        bool                                m_IsRecursiveSelfNotify;
        IntrusivePtrT<cf::GameSys::EntityT> m_DraggedEntity;

        void OnSelectionChanged(wxDataViewEvent& Event);
        void OnEndItemEdit(wxDataViewEvent& Event);
        void OnTreeItemContextMenu(wxDataViewEvent& Event);
        void OnBeginDrag(wxDataViewEvent& Event);
        void OnCheckDrag(wxDataViewEvent& Event);
        void OnEndDrag(wxDataViewEvent& Event);

        DECLARE_EVENT_TABLE()
    };


    class EntityHierarchyPanelT : public wxPanel
    {
        public:

        EntityHierarchyPanelT(ChildFrameT* MainFrame, const wxSize& Size);


        private:

        /// IDs for the controls whose events we are interested in.
        enum
        {
            ID_TREECTRL = wxID_HIGHEST + 1,
            ID_BUTTON_ADD,
            ID_BUTTON_UP,
            ID_BUTTON_DOWN,
            ID_BUTTON_DELETE
        };

        // void OnButton(wxCommandEvent& Event);
        // void OnButtonUpdate(wxUpdateUIEvent& UE);

        ChildFrameT*            m_MainFrame;
        EntityHierarchyDialogT* m_OldTreeCtrl;
        EntityHierarchyCtrlT*   m_TreeCtrl;

        DECLARE_EVENT_TABLE()
    };
}

#endif
