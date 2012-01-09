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

#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "Camera.hpp"
#include "DialogConsole.hpp"
#include "DialogCustomCompile.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "DialogInspector.hpp"
#include "DialogTerrainEdit.hpp"
#include "GameConfig.hpp"
#include "MapDocument.hpp"
#include "Options.hpp"
#include "ParentFrame.hpp"
#include "Tool.hpp"
#include "ToolbarMaterials.hpp"
#include "ToolbarGroups.hpp"
#include "ToolCamera.hpp"
#include "ToolTerrainEdit.hpp"
#include "ToolManager.hpp"

#include "wx/wx.h"
#include "wx/artprov.h"
#include "wx/aui/auibar.h"
#include "wx/confbase.h"
#include "wx/filename.h"
#include "wx/process.h"
#include "wx/stdpaths.h"


#ifdef __WXMSW__
static const wxString CompileScriptName="tempCompileScriptCaWE.bat";
#else
static const wxString CompileScriptName="./tempCompileScriptCaWE.bat";
#endif


// TODO:
// 1. Rename: Some toolbars are panels now, some dialogs are panels now...
// 2. Menu icons...
// 3. Update the pane captions properly, e.g. "2D view, top (XY), zoom: ...".
// 4. properly load/save the perspectives, i.e. with the list of open 2D and 3D views.
// 5. Toolbar toggles for: Texture locking und Options.general.NewUVsFaceAligned.


AutoSaveTimerT::AutoSaveTimerT(MapDocumentT* Doc, unsigned long ChildFrameNr)
    : m_Doc(Doc),
      m_AutoSaveName(wxStandardPaths::Get().GetUserDataDir()+"/"+wxString::Format("autosave%lu.cmap", ChildFrameNr))
{
}


void AutoSaveTimerT::Notify()
{
    // Auto-save the document under name m_AutoSaveName, overwriting any existing file.
    if (wxFileExists(m_AutoSaveName)) wxRemoveFile(m_AutoSaveName);
    m_Doc->OnSaveDocument(m_AutoSaveName, true);
}


AutoSaveTimerT::~AutoSaveTimerT()
{
    // Note that the document is already deleted when we get here!

    // On closing the child frame, delete the corresponding auto-saved file,
    // because the user is assumed to save it himself, as is usual when closing CaWE.
    // In fact, we only get here after a successful save or if the user explicitly denied the saving a modified file.
    if (wxFileExists(m_AutoSaveName)) wxRemoveFile(m_AutoSaveName);
}


/// A class that acts as an observer of the map document and the tools for the child frame.
/// It updates the associated child frames status bar, tool options bar, etc.
/// Note that the child frame could also derive from the observer classes and thus be the observer itself,
/// but having the functionality encapsulated in a separate class like this increases clarity.
class ChildFrameT::UpdaterT : public ObserverT, public ToolsObserverT
{
    public:

    UpdaterT(ChildFrameT* ChildFrame);
    ~UpdaterT();

    // Methods inherited from ObserverT.
    void NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail);
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
 // void NotifySubjectChanged_Created(const ArrayT<MapElementT*>& MapElements);
 // void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements);
 // void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
 // void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
 // void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const wxString& Key);
    void NotifySubjectDies(SubjectT* dyingSubject) { }  // The subjects always live longer than this updater (in contrast to the view windows etc.).

    // Methods inherited from ToolsObserverT.
    void NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority);


    private:

    ChildFrameT* m_ChildFrame;
    ToolT*       m_PrevTool;
};


ChildFrameT::UpdaterT::UpdaterT(ChildFrameT* ChildFrame)
    : m_ChildFrame(ChildFrame),
      m_PrevTool(NULL)
{
    m_ChildFrame->m_Doc->RegisterObserver(this);
    m_ChildFrame->m_ToolManager->RegisterObserver(this);

    // Issue the initial notifications.
    NotifySubjectChanged(m_ChildFrame->m_Doc, UPDATE_GRID);
    NotifySubjectChanged_Selection(m_ChildFrame->m_Doc, m_ChildFrame->m_Doc->GetSelection(), m_ChildFrame->m_Doc->GetSelection());
    NotifySubjectChanged(m_ChildFrame->m_ToolManager, m_ChildFrame->m_ToolManager->GetActiveTool(), UPDATE_NOW);
}


ChildFrameT::UpdaterT::~UpdaterT()
{
    m_ChildFrame->m_Doc->UnregisterObserver(this);
    m_ChildFrame->m_ToolManager->UnregisterObserver(this);
}


void ChildFrameT::UpdaterT::NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail)
{
    switch (OtherDetail)
    {
        case UPDATE_GRID:
            m_ChildFrame->SetStatusText(wxString::Format(" Snap: %s  Grid: %d ",
                m_ChildFrame->m_Doc->IsSnapEnabled() ? "On" : "Off", m_ChildFrame->m_Doc->GetGridSpacing()), SBP_GRID_SNAP);
            break;

        case UPDATE_POINTFILE:
            break;

        case UPDATE_GLOBALOPTIONS:
            break;
    }
}


void ChildFrameT::UpdaterT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    switch (NewSelection.Size())
    {
        case 0:
            m_ChildFrame->SetStatusText(" Nothing is selected.", SBP_SELECTION);
            break;

        case 1:
            m_ChildFrame->SetStatusText(wxString(" ")+NewSelection[0]->GetDescription(), SBP_SELECTION);
            break;

        default:
            m_ChildFrame->SetStatusText(wxString::Format(" %lu items are selected.", NewSelection.Size()), SBP_SELECTION);
            break;
    }
}


void ChildFrameT::UpdaterT::NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority)
{
    if (Tool->IsActiveTool())
    {
        if (Tool!=m_PrevTool)
        {
            const ArrayT<ToolT*>& Tools  =m_ChildFrame->m_ToolManager->GetTools();
            wxWindow*             Toolbar=NULL;

            // Show the options bar of the active tool, hide everything else.
            for (unsigned long ToolNr=0; ToolNr<Tools.Size(); ToolNr++)
            {
                wxWindow* OptionsBar=Tools[ToolNr]->GetOptionsBar();

                if (!OptionsBar) continue;

                OptionsBar->Show(Tools[ToolNr]->IsActiveTool());

                if (!Toolbar) Toolbar=OptionsBar->GetParent();
            }

            if (Toolbar) Toolbar->Layout();
            m_PrevTool=Tool;
        }

        // Update the status bar.
        Tool->UpdateStatusBar(m_ChildFrame);
    }
}


BEGIN_EVENT_TABLE(ChildFrameT, wxMDIChildFrame)
    EVT_CLOSE(ChildFrameT::OnClose)
    EVT_IDLE(ChildFrameT::OnIdle)
    EVT_END_PROCESS(wxID_ANY, ChildFrameT::OnProcessEnd)

    EVT_MENU_RANGE     (ID_MENU_FILE_CLOSE,            ID_MENU_FILE_SAVEAS,             ChildFrameT::OnMenuFile)
    EVT_UPDATE_UI_RANGE(ID_MENU_FILE_CLOSE,            ID_MENU_FILE_SAVEAS,             ChildFrameT::OnMenuFileUpdate)
    EVT_MENU_RANGE     (ID_MENU_EDIT_ENTITY_INSPECTOR, ID_MENU_EDIT_ENTITY_INSPECTOR,   ChildFrameT::OnMenuEdit)
    EVT_UPDATE_UI_RANGE(ID_MENU_EDIT_ENTITY_INSPECTOR, ID_MENU_EDIT_ENTITY_INSPECTOR,   ChildFrameT::OnMenuEditUpdate)
    EVT_MENU_RANGE     (ID_MENU_VIEW_TOOLBARS,         ID_MENU_VIEW_CENTER_3D_VIEWS,    ChildFrameT::OnMenuView)
    EVT_UPDATE_UI_RANGE(ID_MENU_VIEW_TOOLBARS,         ID_MENU_VIEW_CENTER_3D_VIEWS,    ChildFrameT::OnMenuViewUpdate)
    EVT_BUTTON         (ID_MENU_VIEW_CENTER_2D_VIEWS,                                   ChildFrameT::OnMenuView)
    EVT_MENU_RANGE     (ID_MENU_TOOLS_TOOL_SELECTION,  ID_MENU_TOOLS_TOOL_EDITVERTICES, ChildFrameT::OnMenuTools)
    EVT_UPDATE_UI_RANGE(ID_MENU_TOOLS_TOOL_SELECTION,  ID_MENU_TOOLS_TOOL_EDITVERTICES, ChildFrameT::OnMenuToolsUpdate)
    EVT_MENU_RANGE     (ID_MENU_COMPILE_FLAG_SAVE_MAP, ID_MENU_COMPILE_ABORT,           ChildFrameT::OnMenuCompile)

    EVT_ACTIVATE(ChildFrameT::OnWindowActivate)
    EVT_AUI_PANE_CLOSE(ChildFrameT::OnAuiPaneClose)
    // EVT_SIZE(ChildFrameT::OnSize)
END_EVENT_TABLE()


// About the wxMDIChildFrame constructor:
// Although the parent should actually be the parent frames MDI *client* (sub-)window, according to wx documentation
// it is fine to pass the MDI parent window itself (rather than its client window) here. See wxMDIChildFrame documentation.
// Contrary to wx docs, the wxDocMDIChildFrame even *requires* a pointer to wxMDIParentFrame here (not just to a wxFrame, as the docs say)!
ChildFrameT::ChildFrameT(ParentFrameT* Parent, const wxString& Title, MapDocumentT* Document)
    : wxMDIChildFrame(Parent, wxID_ANY, Title, wxDefaultPosition, wxSize(800, 600), wxDEFAULT_FRAME_STYLE | wxMAXIMIZE),
      m_Parent(Parent),      // Must use a fixed size in place of wxDefaultSize, see <http://trac.wxwidgets.org/ticket/12490> for details.
      m_AUIManager(this),
      m_AUIDefaultPerspective(""),
      m_Doc(Document),
      m_LastSavedAtCommandNr(0),
      m_AutoSaveTimer(m_Doc, Parent->m_ChildFrames.Size()),
      m_ToolManager(NULL),
      m_MaterialsToolbar(NULL),
      m_GroupsToolbar(NULL),
      m_ConsoleDialog(NULL),
      m_SurfacePropsDialog(NULL),
      m_TerrainEditorDialog(NULL),
      m_Updater(NULL),
      FileMenu(NULL),
      CompileMenu(NULL),
      CurrentProcess(NULL),
      CurrentProcessID(0),
      PendingCompileCommands(),
      m_ViewWindows()
{
    // Register us with the parents list of children.
    m_Parent->m_ChildFrames.PushBack(this);

    // Should really pass this in the MapDocumentT ctor (the mapdoc should be a member of us).
    // Correction: NO!  While the doc should be a member of us, it might be *created* before us so that its ctor may throw exceptions,
    // and then be given to us (we become the owner). In this case, we have to set us as the child frame here - no chance to do it
    // in the docs ctor. However, I wonder why the doc should know us in the first place...?
    m_Doc->SetChildFrame(this);

    // Register the document as our first event handler.
    PushEventHandler(m_Doc);


    // Create the menu.
    wxMenuBar *item0 = new wxMenuBar;

    FileMenu = new wxMenu;

    wxMenu* NewMenu = new wxMenu;

    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MAP,   wxT("New &Map\tCtrl+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_MODEL, wxT("New M&odel\tCtrl+Shift+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_GUI,   wxT("New &GUI\tCtrl+Alt+N"), wxT(""));
    NewMenu->Append(ParentFrameT::ID_MENU_FILE_NEW_FONT,  wxT("New &Font"), wxT(""));

    FileMenu->AppendSubMenu(NewMenu, wxT("&New"));

    FileMenu->Append(ParentFrameT::ID_MENU_FILE_OPEN, wxT("&Open...\tCtrl+O"), wxT("") );
    FileMenu->Append(ID_MENU_FILE_CLOSE, wxT("&Close\tCtrl+F4"), wxT("") );
    FileMenu->Append(ID_MENU_FILE_SAVE, wxT("&Save\tCtrl+S"), wxT("") );
    FileMenu->Append(ID_MENU_FILE_SAVEAS, wxT("Save &As..."), wxT("") );
    FileMenu->AppendSeparator();
    FileMenu->Append(ParentFrameT::ID_MENU_FILE_CONFIGURE, wxT("&Configure CaWE..."), wxT("") );
    FileMenu->Append(ParentFrameT::ID_MENU_FILE_EXIT, wxT("E&xit"), wxT("") );
    m_Parent->m_FileHistory.UseMenu(FileMenu);
    m_Parent->m_FileHistory.AddFilesToMenu(FileMenu);
    item0->Append( FileMenu, wxT("&File") );

    wxMenu* item2 = new wxMenu;
    item2->Append( wxID_UNDO, wxT("&Undo\tCtrl+Z"), wxT("") );
    item2->Append( wxID_REDO, wxT("&Redo\tCtrl+Y"), wxT("") );
    item2->AppendSeparator();
    item2->Append( wxID_CUT, wxT("Cu&t\tCtrl+X"), wxT("") );
    item2->Append( wxID_COPY, wxT("&Copy\tCtrl+C"), wxT("") );
    item2->Append( wxID_PASTE, wxT("&Paste\tCtrl+V"), wxT("") );
    item2->Append(ID_MENU_EDIT_PASTE_SPECIAL, wxT("Paste &Special..."), wxT("") );
    item2->Append(ID_MENU_EDIT_DELETE, wxT("&Delete\tShift+Del"), wxT("") );
    item2->AppendSeparator();
    item2->Append(ID_MENU_EDIT_SELECT_NONE, wxT("Select &None\tCtrl+Q"), wxT("") );
    item2->Append( wxID_SELECTALL, wxT("Select &All"), wxT("") );
    item2->AppendSeparator();
    item2->AppendCheckItem(ID_MENU_EDIT_ENTITY_INSPECTOR, wxT("Object &Properties\tAlt+Enter"), wxT("") );
    item0->Append( item2, wxT("&Edit") );

    wxMenu* item3 = new wxMenu;
    item3->AppendCheckItem(ID_MENU_MAP_SNAP_TO_GRID, wxT("&Snap to grid\tShift+W"), wxT("") ); item3->Check(ID_MENU_MAP_SNAP_TO_GRID, m_Doc->IsSnapEnabled());
    item3->AppendCheckItem(ID_MENU_MAP_SHOW_GRID_2D, wxT("Sho&w grid\tShift+R"), wxT("") );    item3->Check(ID_MENU_MAP_SHOW_GRID_2D, m_Doc->Is2DGridEnabled());
 // item3->AppendCheckItem(ID_MENU_MAP_SHOW_GRID_3D, wxT("Show 3D grid\tShift+R"), wxT("") );  item3->Check(ID_MENU_MAP_SHOW_GRID_3D, m_Doc->Is3DGridEnabled());  // Maybe later.

    wxMenu* item4 = new wxMenu;
    item4->Append(ID_MENU_MAP_FINER_GRID, wxT("&Finer Grid\t["), wxT("") );
    item4->Append(ID_MENU_MAP_COARSER_GRID, wxT("&Coarser Grid\t]"), wxT("") );
    item3->Append(ID_MENU_MAP_GRID_SETTINGS, wxT("&Grid settings"), item4 );

    item3->AppendSeparator();
    item3->Append(ID_MENU_MAP_GOTO_PRIMITIVE, wxT("G&o to Primitive...\tShift+Ctrl+G"), wxT("") );
    item3->Append(ID_MENU_MAP_SHOW_INFO, wxT("Show &Information..."), wxT("") );
    item3->Append(ID_MENU_MAP_CHECK_FOR_PROBLEMS, wxT("&Check for Problems...\tAlt+P"), wxT("") );
    item3->Append(ID_MENU_MAP_PROPERTIES, wxT("&Map Properties..."), wxT("") );
    item3->AppendSeparator();
    item3->Append(ID_MENU_MAP_LOAD_POINTFILE, wxT("&Load Pointfile..."), wxT("") );
    item3->Append(ID_MENU_MAP_UNLOAD_POINTFILE, wxT("&Unload Pointfile"), wxT("") );
    item0->Append( item3, wxT("&Map") );

    wxMenu* item6 = new wxMenu;

    wxMenu* item7 = new wxMenu;
    item7->AppendCheckItem(ID_MENU_VIEW_TOOLBARS_FILE, wxT("&General"), wxT("") );
    item7->AppendCheckItem(ID_MENU_VIEW_TOOLBARS_TOOLS, wxT("&Tools"), wxT("") );
    item6->Append(ID_MENU_VIEW_TOOLBARS, wxT("&Toolbars"), item7 );

    wxMenu* ViewPanelsMenu=new wxMenu;
    ViewPanelsMenu->AppendCheckItem(ID_MENU_VIEW_PANELS_TOOLOPTIONS, wxT("&Tool Options"), wxT("") );
    ViewPanelsMenu->AppendCheckItem(ID_MENU_VIEW_PANELS_MATERIALS,   wxT("&Materials"), wxT("") );
    ViewPanelsMenu->AppendCheckItem(ID_MENU_VIEW_PANELS_GROUPS,      wxT("&Groups"), wxT("") );
    ViewPanelsMenu->AppendCheckItem(ID_MENU_VIEW_PANELS_INSPECTOR,   wxT("Object &Properties"), wxT("") );
    ViewPanelsMenu->AppendCheckItem(ID_MENU_VIEW_PANELS_CONSOLE,     wxT("&Console"), wxT("") );
    item6->Append(ID_MENU_VIEW_PANELS, wxT("&Panels"), ViewPanelsMenu);

    item6->Append(ID_MENU_VIEW_NEW_2D_VIEW, "New &2D view", "Opens a new 2D view on the map");
    item6->Append(ID_MENU_VIEW_NEW_3D_VIEW, "New &3D view", "Opens a new 3D view on the map");
    item6->AppendSeparator();

    item6->Append(ID_MENU_VIEW_LOAD_USER_PERSPECTIVE, "&Load user window layout", "Loads the user defined window layout");
    item6->Append(ID_MENU_VIEW_SAVE_USER_PERSPECTIVE, "&Save user window layout", "Saves the current window layout");
    item6->Append(ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE, "Load &default window layout", "Restores the default window layout");
    item6->AppendSeparator();
    item6->Append(ID_MENU_VIEW_CENTER_2D_VIEWS, wxT("Center &2D Views on Selection\tCtrl+E"), wxT("") );
    item6->Append(ID_MENU_VIEW_CENTER_3D_VIEWS, wxT("Center &3D Views on Selection"), wxT("") );
    item6->AppendSeparator();
    item6->Append(ID_MENU_VIEW_SHOW_ENTITY_INFO, wxT("Show Entity &Info"), wxT(""), wxITEM_CHECK);
    item6->Append(ID_MENU_VIEW_SHOW_ENTITY_TARGETS, wxT("Show Entity &Targets"), wxT(""), wxITEM_CHECK );
    item6->AppendSeparator();
    item6->Append(ID_MENU_VIEW_HIDE_SELECTED_OBJECTS, wxT("H&ide Selected Objects"), wxT("") );
    item6->Append(ID_MENU_VIEW_SHOW_HIDDEN_OBJECTS, wxT("&Show Hidden Objects"), wxT("") );
    item0->Append( item6, wxT("&View") );

    wxMenu* item8 = new wxMenu;
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_SELECTION,             wxT("Selection\tShift+S"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_CAMERA,                wxT("Camera\tShift+C"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWBRUSH,              wxT("New Brush\tShift+B"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWENTITY,             wxT("New Entity\tShift+E"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWBEZIERPATCH,        wxT("New Bezier Patch\tShift+P"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWTERRAIN,            wxT("New Terrain\tShift+T"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWLIGHT,              wxT("New Light\tShift+L"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_NEWDECAL,              wxT("New Decal\tShift+D"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_EDITSURFACEPROPERTIES, wxT("Edit Surface Properties\tShift+A"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_TERRAINEDITOR,         wxT("Terrain Editor\tShift+F"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_CLIP,                  wxT("Clip Brushes\tShift+X"), wxT(""));
    item8->AppendRadioItem(ID_MENU_TOOLS_TOOL_EDITVERTICES,          wxT("Edit Brush Vertices\tShift+V"), wxT(""));
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_CARVE, wxT("&Carve\tShift+Ctrl+C"), wxT("") );
    item8->Append(ID_MENU_TOOLS_MAKE_HOLLOW, wxT("Make Hollow\tCtrl+H"), wxT("") );
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_GROUP, wxT("&Group\tCtrl+G"), wxT("") );
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_ASSIGN_PRIM_TO_ENTITY, wxT("&Tie to Entity\tCtrl+T"), wxT("") );
    item8->Append(ID_MENU_TOOLS_ASSIGN_PRIM_TO_WORLD, wxT("&Move to World\tShift+Ctrl+W"), wxT("") );
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_REPLACE_MATERIALS, wxT("R&eplace Materials"), wxT("") );
    item8->AppendCheckItem(ID_MENU_TOOLS_MATERIAL_LOCK, wxT("Material &Lock\tShift+L"), wxT("") );
    item8->AppendSeparator();
    item8->Append(ID_MENU_TOOLS_SNAP_SELECTION_TO_GRID, wxT("Snap Selection to Grid\tCtrl+B"), wxT("") );
    item8->Append(ID_MENU_TOOLS_TRANSFORM, wxT("Transform\tCtrl+M"), wxT("") );

    wxMenu* item9 = new wxMenu;
    item9->Append(ID_MENU_TOOLS_ALIGN_LEFT, wxT("to &Left"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_RIGHT, wxT("to &Right"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_HOR_CENTER, wxT("to &hor. Center"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_TOP, wxT("to &Top"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_BOTTOM, wxT("to &Bottom"), wxT("") );
    item9->Append(ID_MENU_TOOLS_ALIGN_VERT_CENTER, wxT("to &vert. Center"), wxT("") );
    item8->Append(ID_MENU_TOOLS_ALIGN, wxT("&Align Objects"), item9 );

    wxMenu* item10 = new wxMenu;
    item10->Append(ID_MENU_TOOLS_MIRROR_HOR, wxT("&Horizontally\tCtrl+L"), wxT("") );
    item10->Append(ID_MENU_TOOLS_MIRROR_VERT, wxT("&Vertically\tCtrl+I"), wxT("") );
    item8->Append(ID_MENU_TOOLS_MIRROR, wxT("M&irror Objects"), item10 );

    item0->Append( item8, wxT("&Tools") );

    CompileMenu=new wxMenu;
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_SAVE_MAP,   "1. &Save Map",     ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_SAVE_MAP,   true);
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_BSP,    "2. Run Ca&BSP",    ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_BSP,    true); CompileMenu->Enable(ID_MENU_COMPILE_FLAG_RUN_BSP, false);
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_PVS,    "3. Run Ca&PVS",    ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_PVS,    true);
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_LIGHT,  "4. Run Ca&Light",  ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_LIGHT,  false);  // Intentionally off per default, in order to make sure that first-time users are not confronted with a multi-hour CaLight session.
    CompileMenu->AppendCheckItem(ID_MENU_COMPILE_FLAG_RUN_ENGINE, "5. Start &Engine", ""); CompileMenu->Check(ID_MENU_COMPILE_FLAG_RUN_ENGINE, true);
    CompileMenu->AppendSeparator();
    CompileMenu->Append(ID_MENU_COMPILE_QUICK,   "Run &Quick...",        "");
    CompileMenu->Append(ID_MENU_COMPILE_NORMAL,  "Run &Normal...",       "");
    CompileMenu->Append(ID_MENU_COMPILE_QUALITY, "Run &High Quality...", "");
    CompileMenu->Append(ID_MENU_COMPILE_CUSTOM,  "Run Custom...",        "");
    CompileMenu->AppendSeparator();
    CompileMenu->Append(ID_MENU_COMPILE_ABORT, "Abort", ""); CompileMenu->Enable(ID_MENU_COMPILE_ABORT, false);
    item0->Append(CompileMenu, wxT("&Compile") );

    wxMenu* item12 = new wxMenu;
    item12->Append(ParentFrameT::ID_MENU_HELP_CONTENTS, wxT("&CaWE Help\tF1"), wxT("") );
    item12->AppendSeparator();
    item12->Append(ParentFrameT::ID_MENU_HELP_CAFU_WEBSITE, wxT("Cafu &Website"), wxT("") );
    item12->Append(ParentFrameT::ID_MENU_HELP_CAFU_FORUM, wxT("Cafu &Forum"), wxT("") );
    item12->AppendSeparator();
    item12->Append(ParentFrameT::ID_MENU_HELP_ABOUT, wxT("&About..."), wxT("") );
    item0->Append(item12, wxT("&Help") );

    SetMenuBar(item0);
    SetMinSize(wxSize(400,300));


    // Create the status bar at the bottom of the frame.
    int StatusBarWidths[6]={ -1, 200, 130, 130, 100, 120 };

    CreateStatusBar(6, wxST_SIZEGRIP);                  // Create the status bar with 6 panes.
    SetStatusWidths(6, StatusBarWidths);                // Set the widths of the individual panes.
    SetStatusBarPane(SBP_MENU_HELP);                    // Use the first pane (number 0) for displaying menu and toolbar help.
    SetStatusText(" Welcome to CaWE!", SBP_MENU_HELP);  // Set an initial text in the first pane.


    // Create the toolbars.
    wxAuiToolBar* ToolbarFile=new wxAuiToolBar(this, wxID_ANY);
    ToolbarFile->AddTool(ParentFrameT::ID_MENU_FILE_NEW_GUI, "New", wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), "Create a new file");
    ToolbarFile->AddTool(ParentFrameT::ID_MENU_FILE_OPEN,    "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), "Open an existing file");
    ToolbarFile->AddTool(ID_MENU_FILE_SAVE,                  "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), "Save the file");
    ToolbarFile->AddTool(ID_MENU_FILE_SAVEAS,                "Save as", wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR), "Save the file under a different name");
    ToolbarFile->AddSeparator();
    ToolbarFile->AddTool(wxID_UNDO,           "Undo", wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR), "Undo the last action");
    ToolbarFile->AddTool(wxID_REDO,           "Redo", wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR), "Redo the previously undone action");
    ToolbarFile->AddSeparator();
    ToolbarFile->AddTool(wxID_CUT,            "Cut", wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR), "Cut");
    ToolbarFile->AddTool(wxID_COPY,           "Copy", wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), "Copy");
    ToolbarFile->AddTool(wxID_PASTE,          "Paste", wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR), "Paste");
    ToolbarFile->AddTool(ID_MENU_EDIT_DELETE, "Delete", wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR), "Delete");
    ToolbarFile->Realize();

    // Note that we cannot have separators between these tools, because they must all be in the same radio group.
    wxAuiToolBar* ToolbarTools=new wxAuiToolBar(this, wxID_ANY);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_SELECTION,             "Selection",               wxArtProvider::GetBitmap("MapEditor/tool-selection",          wxART_TOOLBAR, wxSize(21, 18)), "Selection", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_CAMERA,                "Camera",                  wxArtProvider::GetBitmap("MapEditor/tool-camera",             wxART_TOOLBAR, wxSize(21, 18)), "Camera", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWBRUSH,              "New Brush",               wxArtProvider::GetBitmap("MapEditor/tool-new-brush",          wxART_TOOLBAR, wxSize(21, 18)), "New Brush", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWENTITY,             "New Entity",              wxArtProvider::GetBitmap("MapEditor/tool-new-entity",         wxART_TOOLBAR, wxSize(21, 18)), "New Entity", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWBEZIERPATCH,        "New Bezier Patch",        wxArtProvider::GetBitmap("MapEditor/tool-new-bezierpatch",    wxART_TOOLBAR, wxSize(21, 18)), "New Bezier Patch", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWTERRAIN,            "New Terrain",             wxArtProvider::GetBitmap("MapEditor/tool-new-terrain",        wxART_TOOLBAR, wxSize(21, 18)), "New Terrain", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWLIGHT,              "New Light",               wxArtProvider::GetBitmap("MapEditor/tool-new-light",          wxART_TOOLBAR, wxSize(21, 18)), "New Light", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_NEWDECAL,              "New Decal",               wxArtProvider::GetBitmap("MapEditor/tool-new-decal",          wxART_TOOLBAR, wxSize(21, 18)), "New Decal", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_EDITSURFACEPROPERTIES, "Edit Surface Properties", wxArtProvider::GetBitmap("MapEditor/tool-edit-surface-props", wxART_TOOLBAR, wxSize(21, 18)), "Edit Surface Properties", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_TERRAINEDITOR,         "Edit Terrain",            wxArtProvider::GetBitmap("MapEditor/tool-edit-terrain",       wxART_TOOLBAR, wxSize(21, 18)), "Edit Terrain", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_CLIP,                  "Clip Brushes",            wxArtProvider::GetBitmap("MapEditor/tool-clip",               wxART_TOOLBAR, wxSize(21, 18)), "Clip Brushes", wxITEM_RADIO);
    ToolbarTools->AddTool(ID_MENU_TOOLS_TOOL_EDITVERTICES,          "Edit Brush Vertices",     wxArtProvider::GetBitmap("MapEditor/tool-edit-vertices",      wxART_TOOLBAR, wxSize(21, 18)), "Edit Brush Vertices", wxITEM_RADIO);
    ToolbarTools->Realize();

    m_AUIManager.AddPane(ToolbarFile, wxAuiPaneInfo().ToolbarPane().
                         Name("File Toolbar").Caption("File Toolbar").
                         Top().Row(0).Position(0).LeftDockable(false).RightDockable(false));

    m_AUIManager.AddPane(ToolbarTools, wxAuiPaneInfo().ToolbarPane().
                         Name("Tools Toolbar").Caption("Tools Toolbar").
                         Top().Row(0).Position(1).LeftDockable(false).RightDockable(false));


    wxPanel* ToolbarToolOptions=new wxPanel(this, -1, wxDefaultPosition, wxSize(852, 30));
    m_ToolManager=new ToolManagerT(*m_Doc, ToolbarToolOptions);     // Allocate our tool manager.
    // This can NOT be called here - it will attempt to update the status bar and the tool options bars,
    // which do not yet exist at this point. Instead, the call is made below, near the end of this constructor.
    // m_ToolManager->SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));

    wxSizer* TTOSizer=new wxBoxSizer(wxVERTICAL);   // The sizer makes sure that the individual panels each extend to the full width and height of the toolbar.

    // Add all tool options bars to the sizer (initially hidden).
    for (unsigned long ToolNr=0; ToolNr<m_ToolManager->GetTools().Size(); ToolNr++)
    {
        ToolT*    Tool      =m_ToolManager->GetTools()[ToolNr];
        wxWindow* OptionsBar=Tool->GetOptionsBar();

        if (!OptionsBar) continue;

        OptionsBar->Hide();
        TTOSizer->Add(OptionsBar, 1, wxGROW);
    }

    ToolbarToolOptions->SetSizer(TTOSizer);
    // TTOSizer->SetSizeHints(ToolbarToolOptions);      // When this line is activated ("commented in"), the size of the pane gets wrong.

    m_AUIManager.AddPane(ToolbarToolOptions, wxAuiPaneInfo().
                         Name("Tool Options").Caption("Tool Options").
                         Top().Row(1));


    // Create the toolbars and non-modal dialogs.
    // Note that most if not all of these wxAUI panes have extra style wxWS_EX_BLOCK_EVENTS set,
    // so that they do not propagate their events to us, but behave as if they were derived from wxDialog.
    m_MaterialsToolbar=new MaterialsToolbarT(this, m_Doc);
    m_AUIManager.AddPane(m_MaterialsToolbar, wxAuiPaneInfo().
                         Name("Materials").Caption("Materials").
                         Left().Position(0));

    m_GroupsToolbar=new GroupsToolbarT(this, m_Doc);
    m_AUIManager.AddPane(m_GroupsToolbar, wxAuiPaneInfo().
                         Name("Groups").Caption("Groups").
                         Left().Position(1));

    m_InspectorDialog=new InspectorDialogT(this, m_Doc);
    m_AUIManager.AddPane(m_InspectorDialog, wxAuiPaneInfo().
                         Name("Properties").Caption("Object Properties").
                         Float().Hide());

    m_ConsoleDialog=new ConsoleDialogT(this);
    m_AUIManager.AddPane(m_ConsoleDialog, wxAuiPaneInfo().
                         Name("Console").Caption("Console").
                         BestSize(400, 300).
                         Bottom().Hide());

    m_SurfacePropsDialog=new EditSurfacePropsDialogT(this, m_Doc);
    m_AUIManager.AddPane(m_SurfacePropsDialog, wxAuiPaneInfo().
                         Name("Surface Properties").Caption("Surface Properties").
                         Float().Hide());

    m_TerrainEditorDialog=new TerrainEditorDialogT(this, *m_Doc->GetGameConfig(), static_cast<ToolTerrainEditorT*>(m_ToolManager->GetTool(ToolTerrainEditorT::TypeInfo)));
    m_AUIManager.AddPane(m_TerrainEditorDialog, wxAuiPaneInfo().
                         Name("Terrain Editor").Caption("Terrain Editor").
                         Float().Hide());


    ViewWindow3DT* CenterPaneView=new ViewWindow3DT(this, this, NULL, (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType00", ViewWindowT::VT_3D_FULL_MATS));
    m_AUIManager.AddPane(CenterPaneView, wxAuiPaneInfo().
                         Name("Main View").Caption(CenterPaneView->GetCaption()).
                         CenterPane().CaptionVisible().MaximizeButton().MinimizeButton());

    ViewWindow2DT* ViewTopRight=new ViewWindow2DT(this, this, (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType01", ViewWindowT::VT_2D_XY));
    m_AUIManager.AddPane(ViewTopRight, wxAuiPaneInfo().
                         // Name("xy").
                         Caption(ViewTopRight->GetCaption()).
                         DestroyOnClose().Right().Position(0).MaximizeButton().MinimizeButton());

    ViewWindow2DT* ViewBottomRight=new ViewWindow2DT(this, this, (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType11", ViewWindowT::VT_2D_XZ));
    m_AUIManager.AddPane(ViewBottomRight, wxAuiPaneInfo().
                         // Name("xy").
                         Caption(ViewBottomRight->GetCaption()).
                         DestroyOnClose().Right().Position(1).MaximizeButton().MinimizeButton());


    // Save the AUI perspective that we set up in this ctor code as the "default perspective".
    m_AUIDefaultPerspective=m_AUIManager.SavePerspective();

    // Load the AUI user perspective (implies a call to m_AUIManager.Update()).
    m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("MapEditor_AUIUserPerspective", m_AUIDefaultPerspective));


    // Activate the Selection tool as the default tool (having initially no tool active at all can be quite confusing for users).
    // This cannot be done near the creation of the tool manager above, because then the status bar and tool options bars do not yet exist.
    m_ToolManager->SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));

#ifndef __WXGTK__
    Centre(wxBOTH);
#endif
    if (!IsMaximized()) Maximize(true);     // Also have wxMAXIMIZE set as frame style.
    Show(true);

    m_Updater=new UpdaterT(this);

    // Start the autosave timer.
    int Minutes=0;
    wxConfigBase::Get()->Read("General/AutoSave Interval", &Minutes, 10);
    if (Minutes>0) m_AutoSaveTimer.Start(Minutes*60*1000);
              else m_AutoSaveTimer.Stop();
}


ChildFrameT::~ChildFrameT()
{
    m_AutoSaveTimer.Stop();

    if (CurrentProcess!=NULL)
    {
        CurrentProcess->Detach();
        CurrentProcess=NULL;
        CurrentProcessID=0;
    }

    m_Parent->m_FileHistory.RemoveMenu(FileMenu);

    delete m_Updater;
    m_Updater=NULL;

    delete m_ToolManager;
    m_ToolManager=NULL;

    // Unregister us from the parents list of children.
    const int Index=m_Parent->m_ChildFrames.Find(this);
    m_Parent->m_ChildFrames.RemoveAt(Index);

    // Remove the document as our first event handler.
    PopEventHandler();

    // Delete the document.
    delete m_Doc;
    m_Doc=NULL;

    // Uninit the AUI manager.
    m_AUIManager.UnInit();
}


Vector3fT ChildFrameT::GuessUserVisiblePoint() const
{
    Vector3fT     Point(0, 0, 0);   // TODO: Init along the view dir of the MRU camera instead? Or m_SelectionBB.GetCenter()?
    unsigned long HaveAxes=0;       // Bit i is set if we have a coordinate for the i-th axis.

    for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size() && HaveAxes<7; ViewWinNr++)
    {
        const ViewWindow2DT* ViewWin2D =dynamic_cast<ViewWindow2DT*>(m_ViewWindows[ViewWinNr]); if (!ViewWin2D) continue;
        const int            HorzAxis  =ViewWin2D->GetAxesInfo().HorzAxis;
        const int            VertAxis  =ViewWin2D->GetAxesInfo().VertAxis;
        const wxRect         ClientRect=wxRect(wxPoint(0, 0), ViewWin2D->GetClientSize());
        const Vector3fT      Center    =ViewWin2D->WindowToWorld(wxPoint(ClientRect.x+ClientRect.width/2, ClientRect.y+ClientRect.height/2), 0.0f);

        if ((HaveAxes & (1ul << HorzAxis))==0)
        {
            Point[HorzAxis]=Center[HorzAxis];
            HaveAxes|=(1ul << HorzAxis);
        }

        if ((HaveAxes & (1ul << VertAxis))==0)
        {
            Point[VertAxis]=Center[VertAxis];
            HaveAxes|=(1ul << VertAxis);
        }
    }

    return Point;
}


void ChildFrameT::All2DViews_Zoom(float ZoomFactor)
{
    for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
    {
        ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
        ViewWindow2DT* ViewWin2D=dynamic_cast<ViewWindow2DT*>(ViewWin);

        if (ViewWin2D)
        {
            ViewWin2D->SetZoom(ZoomFactor);
            ViewWin2D->Refresh();
        }
    }
}


void ChildFrameT::All2DViews_Center(const Vector3fT& CenterPoint)
{
    for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
    {
        ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
        ViewWindow2DT* ViewWin2D=dynamic_cast<ViewWindow2DT*>(ViewWin);

        if (ViewWin2D)
        {
            ViewWin2D->CenterView(CenterPoint);
         // ViewWin2D->Refresh();   // Already in CenterView().
        }
    }
}


bool ChildFrameT::IsPaneShown(wxWindow* Pane)
{
    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Pane);

    if (!PaneInfo.IsOk()) return false;

    return PaneInfo.IsShown();
}


void ChildFrameT::ShowPane(wxWindow* Pane, bool DoShow)
{
    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Pane);

    if (!PaneInfo.IsOk()) return;

    PaneInfo.Show(DoShow);

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.floating_pos==wxDefaultPosition)
        PaneInfo.FloatingPosition(ClientToScreen(wxPoint(20, 20)));

    m_AUIManager.Update();
}


void ChildFrameT::SetCaption(wxWindow* Pane, const wxString& Caption)
{
    wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(Pane);

    if (!PaneInfo.IsOk()) return;

    PaneInfo.Caption(Caption);
    m_AUIManager.Update();
}


/**********************/
/*** Event Handlers ***/
/**********************/

void ChildFrameT::OnClose(wxCloseEvent& CE)
{
    if (!CE.CanVeto())
    {
        Destroy();
        return;
    }

    if (m_Doc->GetHistory().GetLastSaveSuggestedCommandID()==m_LastSavedAtCommandNr)
    {
        // Our document has not been modified since the last save - close this window.
        Destroy();
        return;
    }

    // This "Save Confirmation Alert" essentially follows the GNOME Human Interface Guidelines,
    // see http://developer.gnome.org/hig-book/ for details.
    wxMessageDialog Msg(NULL, "Save changes to map \"" + m_Doc->GetFileName() + "\" before closing?", "CaWE Map Editor", wxYES_NO | wxCANCEL);

    Msg.SetExtendedMessage("If you close without saving, your changes will be discarded.");
    Msg.SetYesNoLabels("Save", "Close without Saving");

    switch (Msg.ShowModal())
    {
        case wxID_YES:
            if (!m_Doc->Save())
            {
                // The document could not be saved - keep the window open.
                CE.Veto();
                return;
            }

            if (m_Doc->GetHistory().GetLastSaveSuggestedCommandID()!=m_LastSavedAtCommandNr)
            {
                // The save was successful, but maybe it was a map export rather than a native cmap save.
                // In this case, also keep the window open.
                wxMessageBox("The map was exported, not saved. Please save the map before leaving.", "Map not closed.", wxOK | wxICON_INFORMATION);
                CE.Veto();
                return;
            }

            // The map was successfully saved - close the window.
            Destroy();
            return;

        case wxID_NO:
            Destroy();
            return;

        default:    // wxID_CANCEL
            CE.Veto();
            return;
    }
}


void ChildFrameT::OnIdle(wxIdleEvent& IE)
{
    // We have a compile process running - forward its output to the console.
    // This code is duplicated in ChildFrameT::OnProcessEnd(), to make sure we don't miss anything.
    if (CurrentProcess && CurrentProcess->IsInputAvailable())
    {
        wxInputStream* InStream=CurrentProcess->GetInputStream();
        char           Buffer[256];

        while (InStream->CanRead())
        {
            InStream->Read(Buffer, 255);

            size_t count=InStream->LastRead();

#ifdef __WXGTK__
            if (count==0) break;   // This apparently can happen under wxGTK; I don't know why.
#endif
            wxASSERT(count>0 && count<256);
            Buffer[count]=0;

            m_ConsoleDialog->Print(Buffer, wxLIGHT_GREY);
        }
    }

    // The following things are only performed if the child frame is the active childframe.
    ChildFrameT* ActiveChild=dynamic_cast<ChildFrameT*>(m_Parent->GetActiveChild());

    if (this==ActiveChild)
    {
        // Set the proper window title with or without a trailing "*", depending on the current save state.
        // The extra check (GetTitle()!=NewTitle) is necessary in order to prevent the title from flickering...
        // can we implement an entirely different solution here, e.g. as in the GUI editor?
        wxString NewTitle=m_Doc->GetFileName() + (m_Doc->GetHistory().GetLastSaveSuggestedCommandID()==m_LastSavedAtCommandNr ? "" : "*");
        if (GetTitle()!=NewTitle) SetTitle(NewTitle);

        // Cache materials for the childframes game configuration (one material per idle event).
        m_Doc->GetGameConfig()->GetMatMan().LazilyUpdateProxies();

        // Update the map documents BSP tree (regarding map elements that have moved or otherwise changed their bounding-box).
        const unsigned long ElemsUpdated=m_Doc->GetBspTree()->Update();

        if (ElemsUpdated>0)
        {
            wxLogDebug("OrthoBspTreeT::Update() updated %lu map elements.", ElemsUpdated);
        }

        // Update all 3D views.
        for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
        {
            ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
            ViewWindow3DT* ViewWin3D=dynamic_cast<ViewWindow3DT*>(ViewWin);

            if (ViewWin3D)
            {
                ViewWin3D->Refresh(false);
                ViewWin3D->Update();
            }
        }
    }

    IE.RequestMore();
}


void ChildFrameT::OnProcessEnd(wxProcessEvent& PE)
{
    // Code duplicated from ChildFrameT::OnIdle(), to make sure we don't miss anything.
    if (CurrentProcess && CurrentProcess->IsInputAvailable())
    {
        wxInputStream* InStream=CurrentProcess->GetInputStream();
        char           Buffer[256];

        while (InStream->CanRead())
        {
            InStream->Read(Buffer, 255);

            size_t count=InStream->LastRead();

#ifdef __WXGTK__
            if (count==0) break;   // This apparently can happen under wxGTK; I don't know why.
#endif
            wxASSERT(count>0 && count<256);
            Buffer[count]=0;

            m_ConsoleDialog->Print(Buffer, wxLIGHT_GREY);
        }
    }

    if (PE.GetExitCode()!=0)
    {
        EndCompiling(wxString::Format("\nError: Map compilation process %i exited with code %i.\n", PE.GetPid(), PE.GetExitCode()), wxRED);
        return;
    }

    if (PendingCompileCommands.Size()==0)
    {
        EndCompiling("\nMap compilation ended successfully.");
        return;
    }

    m_ConsoleDialog->Print("\nMap compilation: Running command '"+PendingCompileCommands[0]+"'\n", wxBLUE);

    // Delete previous compile process.
    delete CurrentProcess;
    CurrentProcess=NULL;
    CurrentProcessID=0;

    // Start new compile process.
    CurrentProcess=new wxProcess(this);
    CurrentProcess->Redirect();

    if (PendingCompileCommands[0].StartsWith(Options.general.EngineExe))
        CurrentProcessID=wxExecute(PendingCompileCommands[0], wxEXEC_ASYNC | wxEXEC_NOHIDE, CurrentProcess);
    else
        CurrentProcessID=wxExecute(PendingCompileCommands[0], wxEXEC_ASYNC, CurrentProcess);

    PendingCompileCommands.RemoveAtAndKeepOrder(0);

    if (CurrentProcessID<=0)
        EndCompiling("\nCould not execute the map compilation process!\n", wxRED);
}


void ChildFrameT::OnMenuFile(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_FILE_CLOSE:
        {
            // Close() generates a EVT_CLOSE event which is handled by our OnClose() handler.
            // See wx Window Deletion Overview for more details.
            Close();
            break;
        }

        case ID_MENU_FILE_SAVE:
        {
            const bool SaveOK=m_Doc->Save();

            if (SaveOK && m_Doc->GetFileName().Right(5).MakeLower()==".cmap")
            {
                // Mark the document as "not modified" only if the save was successful AND in cmap file format.
                // All other file formats are exported formats, and may not be able to store all information that is stored in a cmap file.
                // Thus data loss may occur if the document is only saved in an exported, but not in cmap file format.
                // Considering file *exports* not as file *saves* (that mark the document as "not modified" when successful)
                // makes sure that on application exit, the user is warned about possible data loss in such cases.
                m_LastSavedAtCommandNr=m_Doc->GetHistory().GetLastSaveSuggestedCommandID();
                m_Parent->m_FileHistory.AddFileToHistory(m_Doc->GetFileName());
            }
            break;
        }

        case ID_MENU_FILE_SAVEAS:
        {
            const bool SaveOK=m_Doc->SaveAs();

            if (SaveOK && m_Doc->GetFileName().Right(5).MakeLower()==".cmap")
            {
                // Mark the document as "not modified" only if the save was successful AND in cmap file format.
                // All other file formats are exported formats, and may not be able to store all information that is stored in a cmap file.
                // Thus data loss may occur if the document is only saved in an exported, but not in cmap file format.
                // Considering file *exports* not as file *saves* (that mark the document as "not modified" when successful)
                // makes sure that on application exit, the user is warned about possible data loss in such cases.
                m_LastSavedAtCommandNr=m_Doc->GetHistory().GetLastSaveSuggestedCommandID();
                m_Parent->m_FileHistory.AddFileToHistory(m_Doc->GetFileName());
            }
            break;
        }
    }
}


void ChildFrameT::OnMenuFileUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_FILE_SAVE:
            UE.Enable(m_Doc->GetHistory().GetLastSaveSuggestedCommandID()!=m_LastSavedAtCommandNr);
            break;
    }
}


void ChildFrameT::OnMenuEdit(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_EDIT_ENTITY_INSPECTOR:
        {
            wxAuiPaneInfo& PaneInfo=m_AUIManager.GetPane(m_InspectorDialog);

            if (!PaneInfo.IsShown())
            {
                const int BestPage=m_InspectorDialog->GetBestPage(m_Doc->GetSelection());

                m_InspectorDialog->ChangePage(BestPage);
            }

            PaneToggleShow(PaneInfo);
            break;
        }
    }
}


void ChildFrameT::OnMenuEditUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_EDIT_ENTITY_INSPECTOR:
            UE.Check(m_AUIManager.GetPane(m_InspectorDialog).IsShown());
            break;
    }
}


void ChildFrameT::PaneToggleShow(wxAuiPaneInfo& PaneInfo)
{
    if (!PaneInfo.IsOk()) return;

    const bool DoShow=!PaneInfo.IsShown();
    PaneInfo.Show(DoShow);

    if (DoShow && PaneInfo.IsFloating() && PaneInfo.floating_pos==wxDefaultPosition)
        PaneInfo.FloatingPosition(ClientToScreen(wxPoint(20, 20)));

    m_AUIManager.Update();
}


void ChildFrameT::OnMenuView(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_VIEW_TOOLBARS_FILE:
            PaneToggleShow(m_AUIManager.GetPane("File Toolbar"));
            break;

        case ID_MENU_VIEW_TOOLBARS_TOOLS:
            PaneToggleShow(m_AUIManager.GetPane("Tools Toolbar"));
            break;

        case ID_MENU_VIEW_PANELS_TOOLOPTIONS:
            PaneToggleShow(m_AUIManager.GetPane("Tool Options"));
            break;

        case ID_MENU_VIEW_PANELS_MATERIALS:
            PaneToggleShow(m_AUIManager.GetPane(m_MaterialsToolbar));
            break;

        case ID_MENU_VIEW_PANELS_GROUPS:
            PaneToggleShow(m_AUIManager.GetPane(m_GroupsToolbar));
            break;

        case ID_MENU_VIEW_PANELS_INSPECTOR:
            PaneToggleShow(m_AUIManager.GetPane(m_InspectorDialog));
            break;

        case ID_MENU_VIEW_PANELS_CONSOLE:
            PaneToggleShow(m_AUIManager.GetPane(m_ConsoleDialog));
            break;

        case ID_MENU_VIEW_NEW_2D_VIEW:
        {
            ViewWindow2DT* NewView=new ViewWindow2DT(this, this, ViewWindowT::VT_2D_XY);
            m_AUIManager.AddPane(NewView, wxAuiPaneInfo().
                                 // Name("xy").
                                 Caption(NewView->GetCaption()).
                                 DestroyOnClose().Float().MaximizeButton().MinimizeButton());

            m_AUIManager.Update();
            break;
        }

        case ID_MENU_VIEW_NEW_3D_VIEW:
        {
            ViewWindow3DT* NewView=new ViewWindow3DT(this, this, new CameraT(), (ViewWindowT::ViewTypeT)wxConfigBase::Get()->Read("Splitter/ViewType00", ViewWindowT::VT_3D_FULL_MATS));
            m_AUIManager.AddPane(NewView, wxAuiPaneInfo().
                                 // Name("xy").
                                 Caption(NewView->GetCaption()).
                                 DestroyOnClose().Float().MaximizeButton().MinimizeButton());

            m_AUIManager.Update();
            break;
        }

        case ID_MENU_VIEW_LOAD_DEFAULT_PERSPECTIVE:
            m_AUIManager.LoadPerspective(m_AUIDefaultPerspective);
            break;

        case ID_MENU_VIEW_LOAD_USER_PERSPECTIVE:
            m_AUIManager.LoadPerspective(wxConfigBase::Get()->Read("MapEditor_AUIUserPerspective", m_AUIManager.SavePerspective()));
            break;

        case ID_MENU_VIEW_SAVE_USER_PERSPECTIVE:
            wxConfigBase::Get()->Write("MapEditor_AUIUserPerspective", m_AUIManager.SavePerspective());
            break;

        case ID_MENU_VIEW_CENTER_2D_VIEWS:
            All2DViews_Center(m_Doc->GetMostRecentSelBB().GetCenter());
            break;

        case ID_MENU_VIEW_CENTER_3D_VIEWS:
        {
            const BoundingBox3fT& SelBB=m_Doc->GetMostRecentSelBB();
            const Vector3fT       Pos  =SelBB.GetCenter();

            for (unsigned long ViewWinNr=0; ViewWinNr<m_ViewWindows.Size(); ViewWinNr++)
            {
                ViewWindowT*   ViewWin  =m_ViewWindows[ViewWinNr];
                ViewWindow3DT* ViewWin3D=dynamic_cast<ViewWindow3DT*>(ViewWin);

                if (ViewWin3D)
                {
                    const CameraT& Camera=ViewWin3D->GetCamera();

                    ViewWin3D->MoveCamera(Pos-Camera.GetYAxis()*(std::max(Camera.ViewDirLength, length(SelBB.Max-SelBB.Min)/* /2.0f */)));
                    break;  // Only do this for the MRU 3D view - it really doesn't make sense to do it for all.
                }
            }

            break;
        }
    }
}


void ChildFrameT::OnMenuViewUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_MENU_VIEW_TOOLBARS_FILE:
            UE.Check(m_AUIManager.GetPane("File Toolbar").IsShown());
            break;

        case ID_MENU_VIEW_TOOLBARS_TOOLS:
            UE.Check(m_AUIManager.GetPane("Tools Toolbar").IsShown());
            break;

        case ID_MENU_VIEW_PANELS_TOOLOPTIONS:
            UE.Check(m_AUIManager.GetPane("Tool Options").IsShown());
            break;

        case ID_MENU_VIEW_PANELS_MATERIALS:
            UE.Check(m_AUIManager.GetPane(m_MaterialsToolbar).IsShown());
            break;

        case ID_MENU_VIEW_PANELS_GROUPS:
            UE.Check(m_AUIManager.GetPane(m_GroupsToolbar).IsShown());
            break;

        case ID_MENU_VIEW_PANELS_INSPECTOR:
            UE.Check(m_AUIManager.GetPane(m_InspectorDialog).IsShown());
            break;

        case ID_MENU_VIEW_PANELS_CONSOLE:
            UE.Check(m_AUIManager.GetPane(m_ConsoleDialog).IsShown());
            break;
    }
}


void ChildFrameT::OnMenuTools(wxCommandEvent& CE)
{
    // Find the tool whose wxWidgets event ID matches CE.GetId().
    const ArrayT<ToolT*>& Tools=m_ToolManager->GetTools();

    for (unsigned long ToolNr=0; ToolNr<Tools.Size(); ToolNr++)
    {
        ToolT* Tool=Tools[ToolNr];

        if (Tool->GetWxEventID()==CE.GetId())
        {
            m_ToolManager->SetActiveTool(Tool->GetType());
            break;
        }
    }
}


void ChildFrameT::OnMenuToolsUpdate(wxUpdateUIEvent& UE)
{
    const ToolT* ActiveTool=m_ToolManager->GetActiveTool();

    UE.Check(ActiveTool && ActiveTool->GetWxEventID()==UE.GetId());
}


void ChildFrameT::OnMenuCompile(wxCommandEvent& CE)
{
    switch (CE.GetId())
    {
        case ID_MENU_COMPILE_QUICK:
        case ID_MENU_COMPILE_NORMAL:
        case ID_MENU_COMPILE_QUALITY:
        case ID_MENU_COMPILE_CUSTOM:
        {
            if (CurrentProcess!=NULL)
            {
                wxMessageBox("Another process is already running.\nWait until it is finished, then try again.");
                return;
            }

            wxASSERT(PendingCompileCommands.Size()==0 && CurrentProcessID==0);

            if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_SAVE_MAP))
            {
                // Do a regular map save.
                m_Doc->Save();
            }

            CustomCompileDialogT CustomCompileDialog(this);

            // If compile mode is CUSTOM, show Custom Compile Options dialog.
            if (CE.GetId()==ID_MENU_COMPILE_CUSTOM)
            {
                if (CustomCompileDialog.ShowModal()!=wxID_OK) return;
            }

            // Console Fenster anzeigen.
            m_AUIManager.GetPane(m_ConsoleDialog).Show();
            m_AUIManager.Update();

            if (CE.GetId()==ID_MENU_COMPILE_QUICK  ) m_ConsoleDialog->Print(wxString("\n> compile (quick) "  )+m_Doc->GetFileName()+"\n");
            if (CE.GetId()==ID_MENU_COMPILE_NORMAL ) m_ConsoleDialog->Print(wxString("\n> compile (normal) " )+m_Doc->GetFileName()+"\n");
            if (CE.GetId()==ID_MENU_COMPILE_QUALITY) m_ConsoleDialog->Print(wxString("\n> compile (quality) ")+m_Doc->GetFileName()+"\n");
            if (CE.GetId()==ID_MENU_COMPILE_CUSTOM ) m_ConsoleDialog->Print(wxString("\n> compile (custom) " )+m_Doc->GetFileName()+"\n");


            // Test the document file name for validity, and construct the target .cw file name from it.
            wxFileName MapName(m_Doc->GetFileName());

            if (!MapName.IsOk())
            {
                m_ConsoleDialog->Print("Map file name \""+MapName.GetFullPath()+"\" is malformed.\n", wxRED);
                return;
            }

         // MapName.Normalize();
            MapName.MakeRelativeTo(".");   // Make it relative to the current working directory.

            if (MapName.GetExt()!="cmap")
            {
                m_ConsoleDialog->Print("Map file name \""+MapName.GetFullPath()+"\" doesn't end with \".cmap\".\n", wxRED);
                return;
            }

            wxFileName WorldName(m_Doc->GetGameConfig()->ModDir+"/Worlds/"+MapName.GetName()+".cw");
            WorldName.MakeRelativeTo("."); // Make it relative to the current working directory.


            // Create the compile commands.
            {
                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_BSP))
                {
                    if (!wxFileExists(Options.general.BSPExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.BSPExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxString Params="";

                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM) Params=" "+CustomCompileDialog.GetCaBSPOptions();

                    PendingCompileCommands.PushBack(Options.general.BSPExe+" \""+MapName.GetFullPath()+"\" \""+WorldName.GetFullPath()+"\""+Params);
                }

                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_PVS))
                {
                    if (!wxFileExists(Options.general.PVSExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.PVSExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxString Params="";

                    if (CE.GetId()==ID_MENU_COMPILE_QUICK)  Params=" -maxRecDepthSL 15";
                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM) Params=" "+CustomCompileDialog.GetCaPVSOptions();

                    PendingCompileCommands.PushBack(Options.general.PVSExe+" \""+WorldName.GetFullPath()+"\""+Params);
                }

                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_LIGHT))
                {
                    if (!wxFileExists(Options.general.LightExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.LightExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxFileName ModDirName=wxFileName::DirName(m_Doc->GetGameConfig()->ModDir);
                    wxString   Params    ="";

                    ModDirName.MakeRelativeTo(".");

                    if (CE.GetId()==ID_MENU_COMPILE_QUICK  ) Params=" -fast";
                    if (CE.GetId()==ID_MENU_COMPILE_QUALITY) Params=" -StopUE 0.1";
                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM ) Params=" "+CustomCompileDialog.GetCaLightOptions();

                    PendingCompileCommands.PushBack(Options.general.LightExe+" \""+WorldName.GetFullPath()+"\" -gd="+ModDirName.GetFullPath()+Params);
                }

                if (CompileMenu->IsChecked(ID_MENU_COMPILE_FLAG_RUN_ENGINE))
                {
                    if (!wxFileExists(Options.general.EngineExe))
                    {
                        EndCompiling("\nMap compilation error: '"+Options.general.EngineExe+"' File not found.\n", wxRED);
                        return;
                    }

                    wxString Params="";

                    if (CE.GetId()==ID_MENU_COMPILE_CUSTOM) Params=" "+CustomCompileDialog.GetEngineOptions();

                    PendingCompileCommands.PushBack(Options.general.EngineExe+" \""+WorldName.GetName()+"\""+Params);
                }
            }

            // Disable the compile buttons. These are re-enabled in ChildFrameT::EndCompiling().
            CompileMenu->Enable(ID_MENU_COMPILE_QUICK,   false);
            CompileMenu->Enable(ID_MENU_COMPILE_NORMAL,  false);
            CompileMenu->Enable(ID_MENU_COMPILE_QUALITY, false);
            CompileMenu->Enable(ID_MENU_COMPILE_CUSTOM,  false);
            CompileMenu->Enable(ID_MENU_COMPILE_ABORT,   true);

            // Create dummy process event and call end process to trigger compilation start.
            wxProcessEvent PE;  // Note: Is initialized with exit code 0.
            OnProcessEnd(PE);

            break;
        }

        case ID_MENU_COMPILE_ABORT:
        {
            if (CurrentProcessID==0)
            {
                wxMessageBox("There is currently no process running that could be aborted.");
                return;
            }

            m_AUIManager.GetPane(m_ConsoleDialog).Show();
            m_AUIManager.Update();

            wxKillError KillErr=wxProcess::Kill(CurrentProcessID, wxSIGKILL);
            m_ConsoleDialog->Print(wxString::Format("The compile process with ID %i has been terminated. Kill result: %i.\n", CurrentProcessID, KillErr), wxRED);

            // The TERM signal triggers a call to CurrentProcess->OnTerminate() and thus a call to ChildFrameT::OnProcessEnd().
            //     wxASSERT(CurrentProcess==NULL && CurrentProcessID==0);
            // doesn't work here though, because the event system must be running so that the event can actually be processed.
            break;
        }

        default:
            // We must also handle the ID_MENU_COMPILE_FLAG_* menu events here, locally in ChildFrameT.
            // Otherwise, the event is propagated to the parent window, where it's ID triggers an entirely unrelated action!
            break;
    }
}


void ChildFrameT::OnWindowActivate(wxActivateEvent& AE)
{
    AE.Skip();

    if (!AE.GetActive()) return;

    // To make sure the proper default material is always set, we reset it here according to current material toolbar MRU.
    ArrayT<EditorMaterialI*> Materials=m_MaterialsToolbar->GetMRUMaterials();

    wxASSERT(Materials.Size()>0);

    m_Doc->GetGameConfig()->GetMatMan().SetDefaultMaterial(Materials[0]);
}


void ChildFrameT::OnAuiPaneClose(wxAuiManagerEvent& AE)
{
    if (AE.GetPane()->window==m_SurfacePropsDialog || AE.GetPane()->window==m_TerrainEditorDialog)
    {
        // Reset the tool back to "Selection", this will close the specific panes.
        m_ToolManager->SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
    }
    else
    {
        // Let the default action occur (the pane is closed).
        AE.Skip();
    }
}


/* void ChildFrameT::OnSize(wxSizeEvent& SE)
{
    // wxLogDebug("ChildFrameT::OnSize");
    // HorSplitterRatio=float(SplitterWindowParent->GetSashPosition())/float(GetClientSize().y);
    // VerSplitterRatio=float(SplitterWindowTop   ->GetSashPosition())/float(GetClientSize().x);
    // wxLogDebug("%f %f     %i %i       %i %i", HorSplitterRatio, VerSplitterRatio, GetClientSize().x, SplitterWindowParent->GetClientSize().y, SplitterWindowParent->GetSashPosition(), SplitterWindowTop->GetSashPosition());

    SplitterWindowParent->SetSashPosition(HorSplitterRatio*SplitterWindowParent->GetClientSize().y);
    SplitterWindowTop   ->SetSashPosition(VerSplitterRatio*SplitterWindowTop   ->GetClientSize().x);
    SplitterWindowBottom->SetSashPosition(VerSplitterRatio*SplitterWindowBottom->GetClientSize().x);

    SE.Skip();
}*/


void ChildFrameT::EndCompiling(const wxString& ConsoleMessage, const wxColour* Colour)
{
    delete CurrentProcess;
    CurrentProcess=NULL;
    CurrentProcessID=0;

    PendingCompileCommands.Clear();

    CompileMenu->Enable(ID_MENU_COMPILE_QUICK,   true);
    CompileMenu->Enable(ID_MENU_COMPILE_NORMAL,  true);
    CompileMenu->Enable(ID_MENU_COMPILE_QUALITY, true);
    CompileMenu->Enable(ID_MENU_COMPILE_CUSTOM,  true);
    CompileMenu->Enable(ID_MENU_COMPILE_ABORT,   false);

    m_ConsoleDialog->Print(ConsoleMessage, Colour);
}
