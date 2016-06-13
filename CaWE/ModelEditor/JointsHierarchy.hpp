/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_JOINTS_HIERARCHY_HPP_INCLUDED
#define CAFU_MODELEDITOR_JOINTS_HIERARCHY_HPP_INCLUDED

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
        void Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel);
        void Notify_Created(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices);
        void Notify_Deleted(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices);
        void Notify_JointChanged(SubjectT* Subject, unsigned int JointNr);
        void Notify_SubjectDies(SubjectT* dyingSubject);


        private:

        /// Redraws/re-inits the whole tree.
        void RefreshTree();

        /// Recursively searches the tree for an item associated with a specified WindowT.
        /// @param StartingItem Item to start the recursive search at.
        /// @param JointNr      The number of the joint whose tree item we are interested in.
        const wxTreeItemId FindTreeItem(const wxTreeItemId& StartingItem, unsigned int JointNr) const;

        /// Recursively gets all tree items, beginning with the passed tree item.
        void GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items);

        void OnFocus             (wxFocusEvent& FE);
        void OnKeyDown           (wxKeyEvent&   KE);
     // void OnTreeLeftClick     (wxMouseEvent& ME);
        void OnItemActivated     (wxTreeEvent&  TE);    ///< The item has been activated (ENTER or double click).
        void OnSelectionChanged  (wxTreeEvent&  TE);
        void OnLabelChanged      (wxTreeEvent&  TE);
        void OnTreeItemRightClick(wxTreeEvent&  TE);

        DECLARE_EVENT_TABLE()

        ModelDocumentT* m_ModelDoc;
        ChildFrameT*    m_Parent;
        bool            m_IsRecursiveSelfNotify;
    };
}

#endif
