/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_CHILD_FRAME_HPP_INCLUDED
#define CAFU_GUIEDITOR_CHILD_FRAME_HPP_INCLUDED

#include "wx/docmdi.h"
#include "wx/aui/framemanager.h"

#include "ToolManager.hpp"
#include "../CommandHistory.hpp"

#include "Math3D/Vector2.hpp"


namespace cf { namespace TypeSys { class TypeInfoT; } }

class ParentFrameT;
class GameConfigT;
class wxAuiToolBar;


namespace GuiEditor
{
    class RenderWindowT;
    class WindowHierarchyT;
    class WindowInspectorT;
    class GuiInspectorT;
    class GuiDocumentT;


    class ChildFrameT : public wxMDIChildFrame
    {
        public:

        /// Constructor to create a child frame for GUI editing.
        /// @param Parent        The parent frame of this child frame.
        /// @param FileName      Filename for this frame.
        /// @param GuiDocument   This frames gui document. The frame becomes the owner of the document, i.e. it is responsible for destructing it.
        ///                      (\c GuiDocument is created externally so that this constructor doesn't fail on doc creation failure.)
        ChildFrameT(ParentFrameT* Parent, const wxString& FileName, GuiDocumentT* GuiDocument);
        ~ChildFrameT();

        GuiDocumentT* GetGuiDoc() { return m_GuiDocument; }

        ToolManagerT* GetToolManager() { return &m_ToolManager; }

        /// Snaps a single position coordinate to the current grid.
        float SnapToGrid(float Value) const;

        /// Snaps a position in GUI space to the grid.
        Vector2fT SnapToGrid(const Vector2fT& Position) const;

        /// [...]
        /// All(!) commands for modifying the document must be submitted via this method.
        bool SubmitCommand(CommandT* Command);

        bool Save(bool AskForFileName=false);


        private:

        /// Recursively builds the "Components" menu, traversing the given TypeInfoT hierarchy.
        static void BuildComponentsMenu(wxMenu* MenuParent, const cf::TypeSys::TypeInfoT* TypeParent);

        GuiDocumentT*     m_GuiDocument;
        wxString          m_FileName;
        GameConfigT*      m_GameConfig;
        CommandHistoryT   m_History;              ///< The command history.
        unsigned long     m_LastSavedAtCommandNr;

        bool              m_SnapToGrid;
        unsigned long     m_GridSpacing;

        ToolManagerT      m_ToolManager;

        wxAuiManager      m_AUIManager;

        ParentFrameT*     m_Parent;
        RenderWindowT*    m_RenderWindow;
        WindowHierarchyT* m_WindowHierarchy;
        WindowInspectorT* m_WindowInspector;
        GuiInspectorT*    m_GuiInspector;

        wxMenu*           m_FileMenu;
        wxMenu*           m_EditMenu;
        wxMenu*           m_CreateMenu;
        wxMenu*           m_ViewMenu;

        wxAuiToolBar*     m_ToolbarTools;


        enum
        {
            ID_MENU_FILE_CLOSE=wxID_HIGHEST+1+2000,
            ID_MENU_FILE_SAVE,
            ID_MENU_FILE_SAVEAS,

            ID_MENU_EDIT_DELETE,
            ID_MENU_EDIT_SNAP_TO_GRID,
            ID_MENU_EDIT_SET_GRID_SIZE,

            ID_MENU_CREATE_WINDOW,
            ID_MENU_CREATE_COMPONENT_FIRST,
            ID_MENU_CREATE_COMPONENT_MAX = ID_MENU_CREATE_COMPONENT_FIRST + 100,

            ID_MENU_VIEW_WINDOW_HIERARCHY,
            ID_MENU_VIEW_WINDOWINSPECTOR,
            ID_MENU_VIEW_GUIINSPECTOR,
            ID_MENU_VIEW_RESTORE_DEFAULT_LAYOUT,
            ID_MENU_VIEW_RESTORE_USER_LAYOUT,
            ID_MENU_VIEW_SAVE_USER_LAYOUT,

            ID_TOOLBAR_DOC_PREVIEW,

            ID_TOOLBAR_TOOL_SELECTION,
            ID_TOOLBAR_TOOL_NEW_WINDOW,

            ID_TOOLBAR_WINDOW_MOVE_UP,
            ID_TOOLBAR_WINDOW_MOVE_DOWN,
            ID_TOOLBAR_WINDOW_ROTATE_CW,
            ID_TOOLBAR_WINDOW_ROTATE_CCW,

            ID_TOOLBAR_TEXT_ALIGN_LEFT,
            ID_TOOLBAR_TEXT_ALIGN_CENTER,
            ID_TOOLBAR_TEXT_ALIGN_RIGHT,

            ID_TOOLBAR_ZOOM_IN,
            ID_TOOLBAR_ZOOM_OUT,
            ID_TOOLBAR_ZOOM_FIT,
            ID_TOOLBAR_ZOOM_100
        };

        void OnMenuFile(wxCommandEvent& CE);
        void OnMenuFileUpdate(wxUpdateUIEvent& UE);
        void OnMenuUndoRedo(wxCommandEvent& CE);
        void OnUpdateEditUndoRedo(wxUpdateUIEvent& UE);
        void OnMenuEditCut(wxCommandEvent& CE);
        void OnMenuEditCopy(wxCommandEvent& CE);
        void OnMenuEditPaste(wxCommandEvent& CE);
        void OnMenuEditDelete(wxCommandEvent& CE);
        void OnMenuEditGrid(wxCommandEvent& CE);
        void OnMenuEditUpdate(wxUpdateUIEvent& UE);
        void OnMenuCreate(wxCommandEvent& CE);
        void OnMenuView(wxCommandEvent& CE);
        void OnMenuViewUpdate(wxUpdateUIEvent& UE);
        void OnClose(wxCloseEvent& CE);
        void OnToolbar(wxCommandEvent& CE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
