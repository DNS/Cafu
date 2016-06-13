/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CHILDFRAME_HPP_INCLUDED
#define CAFU_CHILDFRAME_HPP_INCLUDED

#include "../CommandHistory.hpp"

#include "Math3D/Angles.hpp"
#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"

#include "wx/mdi.h"
#include "wx/timer.h"
#include "wx/aui/framemanager.h"


namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace MapEditor { class EntityHierarchyPanelT; }

class ConsoleDialogT;
class EditSurfacePropsDialogT;
class InspectorDialogT;
class TerrainEditorDialogT;
class GroupsToolbarT;
class MapDocumentT;
class MaterialsToolbarT;
class ParentFrameT;
class ToolManagerT;
class ViewWindowT;
class wxProcessEvent;


class AutoSaveTimerT : public wxTimer
{
    public:

    AutoSaveTimerT(MapDocumentT* Doc, unsigned long ChildFrameNr);
    ~AutoSaveTimerT();

    void Notify();


    private:

    MapDocumentT* m_Doc;
    wxString      m_AutoSaveName;
};


/// This class represents a child frame.
class ChildFrameT : public wxMDIChildFrame
{
    public:

    /// IDs for the controls whose events we are interested in.
    /// Some of the IDs below are commented out. This is usually because there already exists a wx-defined
    /// ID for the same purpose, which works well with the wx model-view-controler framework.
    /// See the ParentFrameT class header and documentation for additional information.
    enum
    {
        ID_MENU_FILE_CLOSE=wxID_HIGHEST+1+2000,
        ID_MENU_FILE_SAVE,
        ID_MENU_FILE_SAVEAS,

     // ID_MENU_EDIT_UNDO,
     // ID_MENU_EDIT_REDO,
     // ID_MENU_EDIT_CUT,
     // ID_MENU_EDIT_COPY,
     // ID_MENU_EDIT_PASTE,
        ID_MENU_EDIT_PASTE_SPECIAL,
        ID_MENU_EDIT_DELETE,
        ID_MENU_EDIT_SELECT_NONE,
     // ID_MENU_EDIT_SELECT_ALL,
        ID_MENU_SELECTION_ASSIGN_TO_ENTITY,
        ID_MENU_SELECTION_GROUP,
        ID_MENU_SELECTION_HIDE,
        ID_MENU_SELECTION_HIDE_OTHER,

        ID_MENU_MAP_SNAP_TO_GRID,
        ID_MENU_MAP_SHOW_GRID_2D,
        ID_MENU_MAP_GRID_SETTINGS,
        ID_MENU_MAP_FINER_GRID,
        ID_MENU_MAP_COARSER_GRID,
        ID_MENU_MAP_AUTO_GROUP_ENTITIES,
        ID_MENU_MAP_GOTO_PRIMITIVE,
        ID_MENU_MAP_SHOW_INFO,
        ID_MENU_MAP_CHECK_FOR_PROBLEMS,
        ID_MENU_MAP_LOAD_POINTFILE,
        ID_MENU_MAP_UNLOAD_POINTFILE,

        ID_MENU_VIEW_TOOLBARS,
        ID_MENU_VIEW_TOOLBARS_FILE,
        ID_MENU_VIEW_TOOLBARS_TOOLS,
        ID_MENU_VIEW_TOOLBARS_TOOLOPTIONS,
        ID_MENU_VIEW_PANELS_ENTITY_HIERARCHY,
        ID_MENU_VIEW_PANELS_ENTITY_INSPECTOR,
        ID_MENU_VIEW_PANELS_MATERIALS,
        ID_MENU_VIEW_PANELS_GROUPS,
        ID_MENU_VIEW_PANELS_CONSOLE,
        ID_MENU_VIEW_NEW_2D_VIEW,
        ID_MENU_VIEW_NEW_3D_VIEW,
        ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE,
        ID_MENU_VIEW_LOAD_USER_PERSPECTIVE,
        ID_MENU_VIEW_SAVE_USER_PERSPECTIVE,
        ID_MENU_VIEW_CENTER_2D_VIEWS,
        ID_MENU_VIEW_CENTER_3D_VIEWS,
        ID_MENU_VIEW_SHOW_ENTITY_INFO,
        ID_MENU_VIEW_SHOW_ENTITY_TARGETS,

        ID_MENU_TOOLS_TOOL_SELECTION,
        ID_MENU_TOOLS_TOOL_CAMERA,
        ID_MENU_TOOLS_TOOL_NEWBRUSH,
        ID_MENU_TOOLS_TOOL_NEWENTITY,
        ID_MENU_TOOLS_TOOL_NEWBEZIERPATCH,
        ID_MENU_TOOLS_TOOL_NEWTERRAIN,
        ID_MENU_TOOLS_TOOL_NEWDECAL,
        ID_MENU_TOOLS_TOOL_EDITSURFACEPROPERTIES,
        ID_MENU_TOOLS_TOOL_TERRAINEDITOR,
        ID_MENU_TOOLS_TOOL_CLIP,
        ID_MENU_TOOLS_TOOL_EDITVERTICES,
        ID_MENU_TOOLS_CARVE,
        ID_MENU_TOOLS_MAKE_HOLLOW,
        ID_MENU_TOOLS_APPLY_MATERIAL,
        ID_MENU_TOOLS_REPLACE_MATERIALS,
        ID_MENU_TOOLS_MATERIAL_LOCK,
        ID_MENU_TOOLS_TRANSFORM,
        ID_MENU_TOOLS_ALIGN,
        ID_MENU_TOOLS_ALIGN_LEFT,
        ID_MENU_TOOLS_ALIGN_RIGHT,
        ID_MENU_TOOLS_ALIGN_HOR_CENTER,
        ID_MENU_TOOLS_ALIGN_TOP,
        ID_MENU_TOOLS_ALIGN_BOTTOM,
        ID_MENU_TOOLS_ALIGN_VERT_CENTER,
        ID_MENU_TOOLS_MIRROR,
        ID_MENU_TOOLS_MIRROR_HOR,
        ID_MENU_TOOLS_MIRROR_VERT,

        ID_MENU_COMPONENTS_FIRST,
        ID_MENU_COMPONENTS_MAX = ID_MENU_COMPONENTS_FIRST + 100,

        ID_MENU_PREFABS_LOAD,
        ID_MENU_PREFABS_SAVE,
        ID_MENU_PREFABS_PATH_FIRST,
        ID_MENU_PREFABS_PATH_LAST = ID_MENU_PREFABS_PATH_FIRST + 31,

        ID_MENU_COMPILE_FLAG_SAVE_MAP,
        ID_MENU_COMPILE_FLAG_RUN_BSP,
        ID_MENU_COMPILE_FLAG_RUN_PVS,
        ID_MENU_COMPILE_FLAG_RUN_LIGHT,
        ID_MENU_COMPILE_FLAG_RUN_ENGINE,
        ID_MENU_COMPILE_QUICK,
        ID_MENU_COMPILE_NORMAL,
        ID_MENU_COMPILE_QUALITY,
        ID_MENU_COMPILE_CUSTOM,
        ID_MENU_COMPILE_ABORT
    };

    /// Constants for use with SetStatusText, to give the individual panes a name.
    enum StatusBarPaneIDs
    {
        SBP_MENU_HELP=0,
        SBP_SELECTION,
        SBP_MOUSE_POS,
        SBP_SELECTION_DIMS,
        SBP_GRID_ZOOM,
        SBP_GRID_SNAP
    };


    /// The constructor.
    /// @param Parent   The parent frame.
    /// @param MapDoc   This frame's document. The frame becomes the owner of the document,
    ///     i.e. it is responsible for destructing it. `MapDoc` is created externally so
    ///     that this constructor doesn't have to deal with problems constructing it.
    ChildFrameT(ParentFrameT* Parent, MapDocumentT* MapDoc);

    /// The destructor.
    /// Notifies the dependent ViewWindowTs that we're being destroyed before them.
    /// See documentation of ViewWindowT::NotifyChildFrameDies() for full details.
    ~ChildFrameT();

    /// Returns this child frames document.
    MapDocumentT* GetDoc() const { return m_Doc; }

    /// [...]
    /// All(!) commands for modifying the document must be submitted via this method.
    bool SubmitCommand(CommandT* Command);

    /// Returns the list of all (2D and 3D) view windows that are currently open in this frame.
    /// The returned list is always sorted in MRU (most-recently-used) order.
    const ArrayT<ViewWindowT*>& GetViewWindows() const { return m_ViewWindows; }

    /// Based on the most recently used views (2D or 3D), this method figures out a point in space that the user can see well.
    Vector3fT GuessUserVisiblePoint() const;

    void All2DViews_Zoom(float ZoomFactor);
    void All2DViews_Center(const Vector3fT& CenterPoint);

    // These two methods are needed because we cannot show or hide wxAUI panes from "outside" otherwise.
    bool IsPaneShown(wxWindow* Pane);                         ///< Tells if the given pane (one of our toolbars or dialogs that is managed by wxAUI) is currently shown.
    void ShowPane(wxWindow* Pane, bool DoShow=true);          ///< Shows or hides the given pane (one of our toolbars or dialogs that is managed by wxAUI).
    void SetCaption(wxWindow* Pane, const wxString& Caption); ///< Sets the caption of the AUI pane for the given window.

    ToolManagerT&            GetToolManager()         { return *m_ToolManager;        }  ///< Returns our tool manager.
    MaterialsToolbarT*       GetMaterialsToolbar()    { return m_MaterialsToolbar;    }  ///< Returns the Materials toolbar.
    GroupsToolbarT*          GetGroupsToolbar()       { return m_GroupsToolbar;       }  ///< Returns the Groups toolbar.
    EditSurfacePropsDialogT* GetSurfacePropsDialog()  { return m_SurfacePropsDialog;  }  ///< Returns the Surface Properties dialog.
    TerrainEditorDialogT*    GetTerrainEditorDialog() { return m_TerrainEditorDialog; }  ///< Returns the Terrain Editor dialog.
    InspectorDialogT*        GetInspectorDialog()     { return m_InspectorDialog;     }  ///< Returns the Entity Inspector dialog.


    private:

    class UpdaterT;             ///< An observer of the map document and the tools that updates this child frame (its status bar, tool options bar, etc.).
    friend class ViewWindowT;   ///< Friend class so that our ViewWindowT children can manage/maintain the m_ViewWindows list themselves.

    ParentFrameT*            m_Parent;
    wxAuiManager             m_AUIManager;
    wxString                 m_AUIDefaultPerspective;
    MapDocumentT*            m_Doc;
    CommandHistoryT*         m_History;                 ///< The command history.
    unsigned long            m_LastSavedAtCommandNr;    ///< The ID of the command after which the document was last saved. If the current command ID from the history differs from this, the document contains unsaved changes.
    AutoSaveTimerT           m_AutoSaveTimer;
    ToolManagerT*            m_ToolManager;
    MapEditor::EntityHierarchyPanelT* m_EntityHierarchyDialog;
    MaterialsToolbarT*       m_MaterialsToolbar;
    GroupsToolbarT*          m_GroupsToolbar;
    ConsoleDialogT*          m_ConsoleDialog;
    EditSurfacePropsDialogT* m_SurfacePropsDialog;
    TerrainEditorDialogT*    m_TerrainEditorDialog;
    InspectorDialogT*        m_InspectorDialog;
    UpdaterT*                m_Updater;
    wxMenu*                  FileMenu;
    wxMenu*                  m_ComponentsMenu;
    wxMenu*                  m_PrefabsMenu;
    ArrayT<wxString>         m_PrefabsMenuPaths;        ///< The paths for the quick-load items in the "Prefabs" menu.
    wxMenu*                  CompileMenu;

    wxProcess*               CurrentProcess;            ///< The currently running process started from the Compile menu. NULL when there is no process running.
    int                      CurrentProcessID;          ///< The PID of the currently running process, or 0 if no process is running.
    ArrayT<wxString>         PendingCompileCommands;    ///< Pending console commands for map compilation.
    ArrayT<ViewWindowT*>     m_ViewWindows;             ///< The list of all (2D and 3D) view windows that are currently open in this frame. Managed/maintained by the ViewWindowTs themselves.


    /// Recursively builds the "Components" menu, traversing the given TypeInfoT hierarchy.
    static void BuildComponentsMenu(wxMenu* MenuParent, const cf::TypeSys::TypeInfoT* TypeParent);

    /// Updates the quick-load items in the "Prefabs" menu.
    void UpdatePrefabsMenu();

    /// Loads the prefab with the given filename and adds it to the map.
    void LoadPrefab(const wxString& FileName);

    /// Helper method that resets compile process and pending commands and prints a message into the console.
    /// Used to prevent code duplication.
    void EndCompiling(const wxString& ConsoleMessage, const wxColour* Colour=wxGREEN);

    /// Shows or hides the given AUI pane.
    void PaneToggleShow(wxAuiPaneInfo& PaneInfo);

    // Event handlers.
    void OnClose          (wxCloseEvent&      CE);  ///< Event handler for close events, e.g. after a system close button or command or a call to Close() (also see ParentFrameT::OnClose()). See wx Window Deletion Overview for more details.
    void OnIdle           (wxIdleEvent&       IE);  ///< Idle event handler, for updating the console when an external compile process runs, update all 3D views and caching textures.
    void OnProcessEnd     (wxProcessEvent&    PE);
    void OnMenuFile       (wxCommandEvent&    CE);  ///< Event handler for File    menu events.
    void OnMenuFileUpdate (wxUpdateUIEvent&   UE);  ///< Event handler for File    menu update events.

    void OnMenuEditUndoRedo      (wxCommandEvent&  CE);
    void OnMenuEditCut           (wxCommandEvent&  CE);
    void OnMenuEditCopy          (wxCommandEvent&  CE);
    void OnMenuEditPaste         (wxCommandEvent&  CE);
    void OnMenuEditDelete        (wxCommandEvent&  CE);
    void OnMenuEditSelectNone    (wxCommandEvent&  CE);
    void OnMenuEditSelectAll     (wxCommandEvent&  CE);
    void OnMenuEditUpdate        (wxUpdateUIEvent& UE);

    void OnMenuView       (wxCommandEvent&    CE);  ///< Event handler for View    menu events.
    void OnMenuViewUpdate (wxUpdateUIEvent&   UE);  ///< Event handler for View    menu update events.
    void OnMenuTools      (wxCommandEvent&    CE);  ///< Event handler for Tools   menu events.
    void OnMenuToolsUpdate(wxUpdateUIEvent&   UE);  ///< Event handler for Tools   menu update events.
    void OnMenuComponents (wxCommandEvent&    CE);  ///< Event handler for Components menu events.
    void OnMenuPrefabs    (wxCommandEvent&    CE);  ///< Event handler for Prefabs menu events.
    void OnMenuCompile    (wxCommandEvent&    CE);  ///< Event handler for Compile menu events.
    void OnWindowActivate (wxActivateEvent&   AE);
    void OnAuiPaneClose   (wxAuiManagerEvent& AE);

    DECLARE_EVENT_TABLE()
};

#endif
