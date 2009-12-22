/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _MODELEDITOR_CHILD_FRAME_HPP_
#define _MODELEDITOR_CHILD_FRAME_HPP_

#include "wx/docmdi.h"
#include "wx/aui/framemanager.h"


class ParentFrameT;


namespace ModelEditor
{
    class ModelDocumentT;
    class SceneViewT;
    class ModelSetupT;
    class SceneSetupT;


    class ChildFrameT : public wxMDIChildFrame
    {
        public:

        /// The constructor.
        /// @param Parent     The parent frame.
        /// @param Title      The title of the new child frame (i.e. the name of the document).
        /// @param Document   This frames document. The frame becomes the owner of the document, i.e. it is responsible for destructing it.
        ///                   (Document is created externally so that this constructor doesn't fail on doc creation failure.)
        ChildFrameT(ParentFrameT* Parent, const wxString& Title, ModelDocumentT* ModelDoc);

        /// The destructor.
        ~ChildFrameT();

        ModelDocumentT* GetModelDoc() const { return m_ModelDoc; }
        SceneSetupT* GetSceneSetup() const { return m_SceneSetup; }


        private:

        ParentFrameT*   m_Parent;
        wxString        m_Title;
        ModelDocumentT* m_ModelDoc;

        wxMenu*         m_FileMenu;
        wxAuiManager    m_AUIManager;
        SceneViewT*     m_SceneView;
        ModelSetupT*    m_ModelSetup;
        SceneSetupT*    m_SceneSetup;


        enum
        {
            ID_MENU_FILE_CLOSE=wxID_HIGHEST+1+2000,
            ID_MENU_FILE_SAVE,
            ID_MENU_FILE_SAVEAS
        };

        void OnMenuFile(wxCommandEvent& CE);
        void OnClose(wxCloseEvent& CE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
