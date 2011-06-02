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

#ifndef _MODELEDITOR_CHILD_FRAME_HPP_
#define _MODELEDITOR_CHILD_FRAME_HPP_

#include "../CommandHistory.hpp"
#include "wx/docmdi.h"
#include "wx/aui/framemanager.h"


class ParentFrameT;


namespace ModelEditor
{
    class AnimInspectorT;
    class ElementsListT;
    class GlobalsInspectorT;
    class JointInspectorT;
    class JointsHierarchyT;
    class MeshInspectorT;
    class ModelDocumentT;
    class ScenePropGridT;
    class SceneView3DT;


    class ChildFrameT : public wxMDIChildFrame
    {
        public:

        /// The constructor for creating a new model editor child frame.
        /// @param Parent     The applications parent frame.
        /// @param FileName   The file name of the document being edited.
        /// @param ModelDoc   This frames model document. The frame becomes the owner of the document, i.e. it is responsible for destructing it.
        ///                   (Document is created externally so that this constructor doesn't fail on doc creation failure.)
        ChildFrameT(ParentFrameT* Parent, const wxString& FileName, ModelDocumentT* ModelDoc);

        /// The destructor.
        ~ChildFrameT();

        /// [...]
        /// All(!) commands for modifying the document must be submitted via this method.
        bool SubmitCommand(CommandT* Command);

        /// Saves the model under the known or a new file name.
        /// @param AskForFileName   Whether the method should ask the user to enter a new file name, used for "Save as...".
        /// @returns whether the file was successfully saved.
        bool Save(bool AskForFileName=false);

        /// This method shows or hides the pane of the detail inspector that is related to the given list view.
        /// The method does nothing if the given list view is not the joints hierarchy, the meshes list or the anims list.
        void ShowRelatedInspector(wxWindow* List, bool DoShow=true);

        ModelDocumentT* GetModelDoc() const { return m_ModelDoc; }
        ScenePropGridT* GetScenePropGrid() const { return m_ScenePropGrid; }


        private:

        /// Saves all the materials that are not hand-crafted when the model is saved.
        void SaveMaterials(const wxString& OldBaseName, const wxString& BaseName);

        /// Shows or hides the given AUI pane.
        void PaneToggleShow(wxAuiPaneInfo& PaneInfo);

        wxString           m_FileName;
        ModelDocumentT*    m_ModelDoc;
        CommandHistoryT    m_History;               ///< The command history.
        unsigned long      m_LastSavedAtCommandNr;

        ParentFrameT*      m_Parent;
        wxAuiManager       m_AUIManager;
        SceneView3DT*      m_SceneView3D;
        GlobalsInspectorT* m_GlobalsInspector;
        JointsHierarchyT*  m_JointsHierarchy;
        JointInspectorT*   m_JointInspector;
        ElementsListT*     m_MeshesList;
        MeshInspectorT*    m_MeshInspector;
        ElementsListT*     m_AnimsList;
        AnimInspectorT*    m_AnimInspector;
        ScenePropGridT*    m_ScenePropGrid;

        wxMenu*            m_FileMenu;
        wxMenu*            m_EditMenu;


        enum
        {
            ID_MENU_FILE_CLOSE=wxID_HIGHEST+1+2000,
            ID_MENU_FILE_SAVE,
            ID_MENU_FILE_SAVEAS,

            ID_MENU_VIEW_AUIPANE_GLOBALS_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY,
            ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_MESHES_LIST,
            ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_ANIMS_LIST,
            ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_SCENE_SETUP,
            ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE,
            ID_MENU_VIEW_LOAD_USER_PERSPECTIVE,
            ID_MENU_VIEW_SAVE_USER_PERSPECTIVE
        };

        void OnMenuFile(wxCommandEvent& CE);
        void OnMenuUndoRedo(wxCommandEvent& CE);
        void OnUpdateEditUndoRedo(wxUpdateUIEvent& UE);
        void OnMenuView(wxCommandEvent& CE);
        void OnMenuViewUpdate(wxUpdateUIEvent& UE);
        void OnClose(wxCloseEvent& CE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
