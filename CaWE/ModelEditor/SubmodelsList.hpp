/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_MODELEDITOR_SUBMODELS_LIST_HPP_INCLUDED
#define CAFU_MODELEDITOR_SUBMODELS_LIST_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "wx/listctrl.h"
#include "wx/panel.h"


namespace ModelEditor
{
    class ChildFrameT;
    class ModelDocumentT;


    /// A control for displaying a list of the submodels that are rendered with the model.
    class SubmodelsListT : public wxListView, public ObserverT
    {
        public:

        /// The constructor.
        SubmodelsListT(ChildFrameT* MainFrame, wxWindow* Parent, const wxSize& Size);

        /// The destructor.
        ~SubmodelsListT();

        // ObserverT implementation.
        void Notify_SubmodelsChanged(SubjectT* Subject);
        void Notify_SubjectDies(SubjectT* dyingSubject);


        private:

        friend class SubmodelsPanelT;

        void InitListItems();
        void LoadSubmodel();
        void UnloadSelectedSubmodels();

        void OnContextMenu  (wxContextMenuEvent& CE);
        void OnItemActivated(wxListEvent&        LE);   ///< The item has been activated (ENTER or double click).

        DECLARE_EVENT_TABLE()

        ModelDocumentT* m_ModelDoc;
        ChildFrameT*    m_MainFrame;
        bool            m_IsRecursiveSelfNotify;
    };


    class SubmodelsPanelT : public wxPanel
    {
        public:

        SubmodelsPanelT(ChildFrameT* MainFrame, const wxSize& Size);


        private:

        /// IDs for the controls whose events we are interested in.
        enum
        {
            ID_LISTVIEW=wxID_HIGHEST+1,
            ID_BUTTON_LOAD,
            ID_BUTTON_UP,
            ID_BUTTON_DOWN,
            ID_BUTTON_UNLOAD
        };

        void OnButton(wxCommandEvent& Event);
        void OnButtonUpdate(wxUpdateUIEvent& UE);

        ModelDocumentT* m_ModelDoc;
        ChildFrameT*    m_MainFrame;
        SubmodelsListT* m_List;

        DECLARE_EVENT_TABLE()
    };
}

#endif
