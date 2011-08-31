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

#ifndef _GUIEDITOR_CHILD_FRAME_HPP_
#define _GUIEDITOR_CHILD_FRAME_HPP_

#include "wx/docmdi.h"
#include "wx/aui/framemanager.h"

#include "ToolManager.hpp"
#include "../CommandHistory.hpp"

#include "Math3D/Vector3.hpp"


class ParentFrameT;
class GameConfigT;
class wxAuiToolBar;


namespace GuiEditor
{
    class RenderWindowT;
    class WindowTreeT;
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
        Vector3fT SnapToGrid(const Vector3fT& Position) const;

        /// [...]
        /// All(!) commands for modifying the document must be submitted via this method.
        bool SubmitCommand(CommandT* Command);

        bool Save(bool AskForFileName=false);


        private:

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
        WindowTreeT*      m_WindowTree;
        WindowInspectorT* m_WindowInspector;
        GuiInspectorT*    m_GuiInspector;

        wxMenu*           m_FileMenu;
        wxMenu*           m_EditMenu;
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

            ID_MENU_VIEW_WINDOWTREE,
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
        void OnMenuView(wxCommandEvent& CE);
        void OnMenuViewUpdate(wxUpdateUIEvent& UE);
        void OnClose(wxCloseEvent& CE);
        void OnToolbar(wxCommandEvent& CE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
