/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_CHILD_FRAME_HPP_INCLUDED
#define CAFU_MODELEDITOR_CHILD_FRAME_HPP_INCLUDED

#include "../CommandHistory.hpp"
#include "ElementTypes.hpp"
#include "wx/docmdi.h"
#include "wx/aui/framemanager.h"


class ParentFrameT;


namespace ModelEditor
{
    class AnimInspectorT;
    class ChannelInspectorT;
    class ElementsPanelT;
    class GuiFixInspectorT;
    class JointInspectorT;
    class JointsHierarchyT;
    class MeshInspectorT;
    class ModelDocumentT;
    class ScenePropGridT;
    class SceneView3DT;
    class SubmodelsPanelT;
    class TransformDialogT;


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

        /// Creates a new skin, calls SubmitCommand() accordingly and returns its result.
        bool SubmitNewSkin();

        /// Creates a new GUI fixture, calls SubmitCommand() accordingly and returns its result.
        bool SubmitNewGuiFixture();

        /// Imports animation sequences from a user selected file, calls SubmitCommand() accordingly and returns its result.
        bool SubmitImportAnims();

        /// Creates a new animation channel, calls SubmitCommand() accordingly and returns its result.
        bool SubmitNewChannel();

        /// Saves the model under the known or a new file name.
        /// @param AskForFileName   Whether the method should ask the user to enter a new file name, used for "Save as...".
        /// @returns whether the file was successfully saved.
        bool Save(bool AskForFileName=false);

        /// This method shows or hides the pane of the detail inspector that is related to the given list view.
        /// The method does nothing if the given list view is not the joints hierarchy, the meshes list, the anims list or the GUI fixtures list.
        void ShowRelatedInspector(wxWindow* List, bool DoShow=true);

        /// Returns the type of the most recently used (clicked) model element.
        const ModelElementTypeT GetLastUsedType() const { return m_LastUsedType; }

        /// Sets the type of the most recently used (clicked) model element.
        void SetLastUsedType(ModelElementTypeT Type) { wxASSERT(Type<6); m_LastUsedType=Type; }

        ModelDocumentT* GetModelDoc() const { return m_ModelDoc; }
        ScenePropGridT* GetScenePropGrid() const { return m_ScenePropGrid; }


        private:

        /// Saves all the materials that are not hand-crafted when the model is saved.
        void SaveMaterials(const wxString& OldBaseName, const wxString& BaseName);

        /// Shows or hides the given AUI pane.
        void PaneToggleShow(wxAuiPaneInfo& PaneInfo);

        /// Returns a string with all the LoD models loaded with the main model.
        wxString GetLodModelsString() const;

        wxString           m_FileName;
        ModelDocumentT*    m_ModelDoc;
        CommandHistoryT    m_History;               ///< The command history.
        unsigned long      m_LastSavedAtCommandNr;
        ModelElementTypeT  m_LastUsedType;          ///< The type of the most recently used (clicked) model element.

        ParentFrameT*      m_Parent;
        wxMenu*            m_FileMenu;
        wxMenu*            m_EditMenu;

        wxAuiManager       m_AUIManager;
        SceneView3DT*      m_SceneView3D;
        JointsHierarchyT*  m_JointsHierarchy;
        JointInspectorT*   m_JointInspector;
        ElementsPanelT*    m_MeshesList;
        MeshInspectorT*    m_MeshInspector;
        ElementsPanelT*    m_SkinsList;
        wxStaticText*      m_SkinInspector;
        ElementsPanelT*    m_GuiFixturesList;
        GuiFixInspectorT*  m_GuiFixtureInspector;
        ElementsPanelT*    m_AnimsList;
        AnimInspectorT*    m_AnimInspector;
        ElementsPanelT*    m_ChannelsList;
        ChannelInspectorT* m_ChannelInspector;
        ScenePropGridT*    m_ScenePropGrid;
        SubmodelsPanelT*   m_SubmodelsPanel;
        wxStaticText*      m_DlodModelsList;
        TransformDialogT*  m_TransformDialog;


        enum
        {
            ID_MENU_FILE_CLOSE=wxID_HIGHEST+1+2000,
            ID_MENU_FILE_SAVE,
            ID_MENU_FILE_SAVEAS,

            ID_MENU_VIEW_AUIPANE_JOINTS_HIERARCHY,
            ID_MENU_VIEW_AUIPANE_JOINT_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_MESHES_LIST,
            ID_MENU_VIEW_AUIPANE_MESH_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_SKINS_LIST,
            ID_MENU_VIEW_AUIPANE_SKIN_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_GUIFIXTURES_LIST,
            ID_MENU_VIEW_AUIPANE_GUIFIXTURE_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_ANIMS_LIST,
            ID_MENU_VIEW_AUIPANE_ANIM_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_CHANNELS_LIST,
            ID_MENU_VIEW_AUIPANE_CHANNEL_INSPECTOR,
            ID_MENU_VIEW_AUIPANE_SCENE_SETUP,
            ID_MENU_VIEW_AUIPANE_SUBMODELS_LIST,
            ID_MENU_VIEW_AUIPANE_DLOD_MODELS_LIST,
            ID_MENU_VIEW_AUIPANE_TRANSFORM_DIALOG,
            ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE,
            ID_MENU_VIEW_LOAD_USER_PERSPECTIVE,
            ID_MENU_VIEW_SAVE_USER_PERSPECTIVE,

            ID_MENU_MODEL_ANIM_SKIP_BACKWARD,
            ID_MENU_MODEL_ANIM_PLAY,
            ID_MENU_MODEL_ANIM_PAUSE,
            ID_MENU_MODEL_ANIM_SKIP_FORWARD,
            ID_MENU_MODEL_TRANSFORM,
            ID_MENU_MODEL_SKIN_ADD,
            ID_MENU_MODEL_GUIFIXTURE_ADD,
            ID_MENU_MODEL_ANIM_IMPORT,
            ID_MENU_MODEL_CHANNEL_ADD,
            ID_MENU_MODEL_LOAD_SUBMODEL,
            ID_MENU_MODEL_UNLOAD_SUBMODELS
        };

        void OnMenuFile(wxCommandEvent& CE);
        void OnMenuFileUpdate(wxUpdateUIEvent& UE);
        void OnMenuUndoRedo(wxCommandEvent& CE);
        void OnMenuUndoRedoUpdate(wxUpdateUIEvent& UE);
        void OnMenuEdit(wxCommandEvent& CE);
        void OnMenuEditUpdate(wxUpdateUIEvent& UE);
        void OnMenuView(wxCommandEvent& CE);
        void OnMenuViewUpdate(wxUpdateUIEvent& UE);
        void OnMenuModel(wxCommandEvent& CE);
        void OnMenuModelUpdate(wxUpdateUIEvent& UE);
        void OnClose(wxCloseEvent& CE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
