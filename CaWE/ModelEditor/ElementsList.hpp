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

#ifndef _MODELEDITOR_ELEMENTS_LIST_HPP_
#define _MODELEDITOR_ELEMENTS_LIST_HPP_

#include "ObserverPattern.hpp"
#include "wx/listctrl.h"
#include "wx/panel.h"


namespace ModelEditor
{
    class ChildFrameT;
    class ModelDocumentT;


    /// A control for displaying a list of the elements of the model.
    class ElementsListT : public wxListView, public ObserverT
    {
        public:

        /// The constructor.
        ElementsListT(ChildFrameT* MainFrame, wxWindow* Parent, const wxSize& Size, ModelElementTypeT Type);

        /// The destructor.
        ~ElementsListT();

        /// Returns whether one or more "default" elements are selected in the list.
        bool AreDefaultItemsSelected() const;

        // ObserverT implementation.
        void Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel);
        void Notify_Created(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices);
        void Notify_Deleted(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices);
        void Notify_MeshChanged(SubjectT* Subject, unsigned int MeshNr);
        void Notify_AnimChanged(SubjectT* Subject, unsigned int AnimNr);
        void Notify_SkinChanged(SubjectT* Subject, unsigned int SkinNr);
        void Notify_GuiFixtureChanged(SubjectT* Subject, unsigned int GuiFixtureNr);
        void Notify_SubjectDies(SubjectT* dyingSubject);


        private:

        void InitListItems();

        void OnFocus           (wxFocusEvent&       FE);
        void OnContextMenu     (wxContextMenuEvent& CE);
        void OnKeyDown         (wxListEvent&        LE);
        void OnItemActivated   (wxListEvent&        LE);    ///< A list item has been activated (ENTER or double click).
        void OnSelectionChanged(wxListEvent&        LE);
        void OnEndLabelEdit    (wxListEvent&        LE);

        DECLARE_EVENT_TABLE()

        const ModelElementTypeT m_TYPE;
        const int               m_NUM_DEFAULT_ITEMS;
        ModelDocumentT*         m_ModelDoc;
        ChildFrameT*            m_MainFrame;
        bool                    m_IsRecursiveSelfNotify;
    };


    class ElementsPanelT : public wxPanel
    {
        public:

        ElementsPanelT(ChildFrameT* MainFrame, const wxSize& Size, ModelElementTypeT Type);


        private:

        /// IDs for the controls whose events we are interested in.
        enum
        {
            ID_LISTVIEW=wxID_HIGHEST+1,
            ID_BUTTON_ADD,
            ID_BUTTON_UP,
            ID_BUTTON_DOWN,
            ID_BUTTON_DELETE
        };

        void OnButton(wxCommandEvent& Event);
        void OnButtonUpdate(wxUpdateUIEvent& UE);

        const ModelElementTypeT m_TYPE;
        ModelDocumentT*         m_ModelDoc;
        ChildFrameT*            m_MainFrame;
        ElementsListT*          m_List;

        DECLARE_EVENT_TABLE()
    };
}

#endif
